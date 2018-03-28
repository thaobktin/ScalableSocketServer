#include "stdafx.h"
#include "../common/logging.h"
#include "../common/socket.h"
#include "../common/socket.inl"
#include "../common/allocator.h"
#include "../common/dedicated_heap.h"
#include "../common/app.h"
#include "../common/thread_pool.h"
#include "../common/dedicated_heap.h"

using namespace std;
using namespace std::chrono;
namespace po = boost::program_options;

class simple_buffer 
{ 
public:
	char* buf() { return data; }
	int len() const { return sizeof(data); }
private:
	char data[1024];
};

class TCP_server : public socket_app
{
	tcp_socket_type listen_socket;

public:
	thread_pool pool;
	dedicated_heap<simple_buffer, std_allocator> buffer_heap;

	// configuration
	size_t no_of_threads = 400;
	size_t max_connections = 1000;
	size_t task_length = 0; // ms.

	// statistics.
	size_t no_of_requests = 0;
	LONG no_of_connections_accepted = 0;
	LONG no_of_responses_sent_OK = 0;

	TCP_server()
		: socket_app("Simple TCP server")
	{
		option_desc.add_options()
			("threads,t", po::value<size_t>(&no_of_threads)->default_value(4), "The number of threads created in the thread pool.")
			("connections,c", po::value<size_t>(&max_connections)->default_value(1000), "The maximum number of connections that can be simultaneously handled.")
			("tasklength,l", po::value<size_t>(&task_length)->default_value(0), "The length of time between receiving a request and sending a response, in milliseconds.");
	}	
	
	int run() override
	{
		buffer_heap.allocate(max_connections);
		pool.start_up(no_of_threads);
		Log_info(INITIALISATION, "Free space: " << buffer_heap.free_space());

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

		while(!application_stopping())
		{
			// accept rew requests
			shared_ptr<tcp_socket_type> client_socket = make_shared<tcp_socket_type>();
			listen_socket.accept(*client_socket);
			SafeIncrement(&no_of_requests); 
			if(!client_socket->is_valid()) 
			{
				int err_code = WSAGetLastError();
				Log_error(PROCESSING, "socket::accept() failed - " << socket_error_string(err_code)  
							<< "\nLast error code: " << err_code);
				continue;
			}

			pool.add_task([client_socket, this]() mutable
			{
				assert(!!client_socket);

				long no_of_conns = InterlockedIncrement(&no_of_connections_accepted);
				Log_info2(PROCESSING, "No of connections: " << &no_of_conns);

				simple_buffer* buffer = buffer_heap.get();
				if(buffer == nullptr)
				{
					Log_error(PROCESSING, "******* Run out of buffers!!");
				}
				else
				{
					try
					{
						assert(client_socket->is_valid());

						DWORD sock_timeout = 10000;
						setsockopt(*client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&sock_timeout, sizeof(sock_timeout));

						// Only one recv() accepted. ** This is not an attempt to write a browser! **
						int bytes_received = 0;
						if(!client_socket->recv(buffer->buf(), buffer->len(), bytes_received))
						{
							Log_error(PROCESSING, socket_error_string(WSAGetLastError()));
						}
						else
						{
							Log_info2(PROCESSING, "Received " << bytes_received << " bytes from client.\nReceived: " << std::string(buffer->buf(), bytes_received));

							// Simulate a delay if so configured.
							if(task_length > 0)
								this_thread::sleep_for(milliseconds(task_length));
							
							increment_request_count();

							// Send the stock response.
							const char* stock_response; size_t stock_response_length;
							get_stock_response(stock_response, stock_response_length);

							if(!client_socket->send(stock_response, stock_response_length))
							{
								Log_error(PROCESSING, socket_error_string(WSAGetLastError()));
							}
							else
							{
								long no_of_OK_responses = InterlockedIncrement(&no_of_responses_sent_OK);
								Log_info2(PROCESSING, "No of responses sent: " << no_of_OK_responses);
							}							
						}
					}
					catch(...)
					{
						Log_error(PROCESSING, "Unexpected exception thrown inside thread");
					}
				}
				buffer_heap.release(buffer);	// buffer is no longer needed.
			});

			//client_socket.reset();	// explicit release of shared_ptr.
		}

		pool.stop();
		notify_closed_down(); 

		return 0;
	}

	void on_closedown() override 
	{
		listen_socket.close();
	}

	int on_exit() override
	{
		Log_display(CLOSEDOWN, "Free space: " << buffer_heap.free_space());
		Log_display(CLOSEDOWN, "No of requests processed: " << no_of_requests);
		Log_display(CLOSEDOWN, "No of connections accepted: " << no_of_connections_accepted);
		Log_display(CLOSEDOWN, "No of response sent OK: " << no_of_responses_sent_OK);
		return 0;
	}
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);
	TCP_server app;
	return app.main(argc, argv);
}


