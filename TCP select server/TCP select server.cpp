#include "stdafx.h"
#include "../common/logging.h"
#include "../common/socket.h"
#include "../common/socket.inl"
#include "../common/allocator.h"
#include "../common/dedicated_heap.h"
#include "../common/app.h"

using namespace std;
using namespace std::chrono;
namespace po = boost::program_options;

class TCP_select_server : public socket_app
{
	tcp_socket_type listen_socket;

public:
	// statistics.
	size_t no_of_responses_sent_OK = 0;

	TCP_select_server()
		: socket_app("TCP select server")
	{
	}	

	int run() override
	{
		if(!listen_socket.create())
		{
			Log_error(INITIALISATION, "Unable to create socket" << socket_error_string(WSAGetLastError()));
			return -1;
		}			

		socket_address address;
		if(!get_bind_address("", port, address))
		{
			Log_error(INITIALISATION, "Failed to resolve server bind address");	
			return -1;
		}

		Log_display(INITIALISATION, "Binding to " << address.ip_address() << ":" << address.port());
		if(!listen_socket.bind(address))
		{
			Log_error(INITIALISATION, "Unable to bind to address " << address << socket_error_string(WSAGetLastError()));
			return -1;
		}

		if(!listen_socket.listen()) 
		{
			Log_error(INITIALISATION, "socket::listen() failed - " << socket_error_string(WSAGetLastError()));
			return -1;
		}

		Log_display(INITIALISATION, "Listening for connections on " << address);
		Log_info(INITIALISATION, "Listening for connections...");
		Log_error(INITIALISATION, "Checking error logging...");
	
		fd_set master;			// Master file descriptor list
		FD_ZERO(&master);
		FD_SET(listen_socket, &master);

		while(!application_stopping()) // Don't stop without responding to a request.
		{
			fd_set read_fds = master;
			int retcode = select(0, &read_fds, NULL, NULL, NULL);
			if(retcode == SOCKET_ERROR) // parameter 1 ignored in WSA
			{
				Log_error(PROCESSING, "select() error - " << socket_error_string(WSAGetLastError()));
				return -1;
			}
		
			for(size_t i=0; i<read_fds.fd_count; ++i)
			{
				tcp_socket_ref s = read_fds.fd_array[i]; // we do not own the socket.
				assert(s.is_valid());

				if(FD_ISSET(s, &read_fds))
				{
					if(s == listen_socket)
					{
						// New connection

						tcp_socket_ref client_socket;
						listen_socket.accept(client_socket);

						socket_app::increment_request_count();

						if(!client_socket.is_valid()) 
						{
							int err_code = WSAGetLastError();
							Log_error(PROCESSING, "socket::accept() failed - " << socket_error_string(err_code)  
										<< "\nLast error code: " << err_code);
						}
						else
							FD_SET(client_socket, &master); 
					}
					else
					{
						// socket I/O from existing connection.
						char buf[1024];
						int bytes_received = 0;
						if(!s.recv(buf, sizeof(buf), bytes_received))
						{
							Log_error(PROCESSING, socket_error_string(WSAGetLastError()));
							s.close();	
							FD_CLR(s, &master);
							continue;
						}

						// Send the stock response.
						const char* stock_response; size_t stock_response_length;
						get_stock_response(stock_response, stock_response_length);

						if(!s.send(stock_response, stock_response_length)) 
						{
							Log_error(PROCESSING, socket_error_string(WSAGetLastError()));
						}
						else 
						{
							++no_of_responses_sent_OK
							Log_info(PROCESSING, "No of responses sent: " << no_of_responses_sent_OK);
						}

						FD_CLR(s, &master);
						s.close(); 
					}
				}
			}
		}

		FD_CLR(listen_socket, &master);
		listen_socket.close();

		// close the socket in master - optional as these will be closed on exit anyway.
		// Highly non-portable.
		for(u_int i=0; i<master.fd_count; i++)
		{
			tcp_socket_ref s = master.fd_array[i];
			s.close();
		}

		notify_closed_down(); 

		return 0;
	}

	void on_closedown() override 
	{
		listen_socket.close();
	}

	int on_exit() override
	{
		Log_display(CLOSEDOWN, "No of requests processed: " << get_request_count());
		Log_display(CLOSEDOWN, "No of response sent OK: " << no_of_responses_sent_OK);
		return 0;
	}
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);

	TCP_select_server app;

	int ret_code = app.main(argc, argv);
	
	press_any_key_to_continue();

	return ret_code;
}



