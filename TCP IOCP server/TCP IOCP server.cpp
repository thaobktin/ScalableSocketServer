#include "stdafx.h"
#include "../common/logging.h"
#include "../common/socket.h"
#include "../common/socket.inl"
#include "../common/allocator.h"
#include "../common/dedicated_heap.h"
#include "../common/app.h"
#include "../common/thread_pool.h"
#include "Overlapped.h"
#include "request_response.h"
#include "connection.h"

using namespace std;
namespace po = boost::program_options;

enum class completion_key { none=90, IO=91, shutdown=92, check_connections=93 };

class TCP_server : public socket_app
{
public:
	tcp_socket_type listen_socket;
	HANDLE IO_completion_port = INVALID_HANDLE_VALUE;
    LPFN_ACCEPTEX fnAcceptEx;
	OVERLAPPED_factory Overlapped_factory;
	thread_pool pool;
	sample_app_state_machine app_state_machine;  // only 

	// configuration
	size_t no_of_threads = 3; 
	size_t no_of_processors = 0;
	size_t connections_per_thread = 250;
	size_t buffers_per_thread = 250;
	size_t no_of_OVERLAPPED_ENTRIES_per_thread = 25;

	// statistics
	size_t no_of_accepts = 0;
	size_t no_of_responses_sent_OK = 0;
	
	TCP_server()
		: socket_app("TCP IOCP server")
	{
		option_desc.add_options()
			("threads,t", po::value<size_t>(&no_of_threads)->default_value(3), "The number of threads created in the thread pool.")
			("connections,c", po::value<size_t>(&connections_per_thread)->default_value(250), "The maximum number of connections per thread.")
			("buffers,b", po::value<size_t>(&buffers_per_thread)->default_value(250), "The maximum number of buffers per thread.")
			("OVERLAPPED_ENTRIES,e", po::value<size_t>(&no_of_OVERLAPPED_ENTRIES_per_thread)->default_value(25), "The maximum number of OVERLAPPED_ENTRY(s) per thread.");

		no_of_threads = no_of_processors = retrieve_no_of_processors();
	}

	int retrieve_no_of_processors() const
	{
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		return system_info.dwNumberOfProcessors;
	}

	int get_TIME_WAIT()
	{
		DWORD time_wait  = (DWORD)-1;
		HKEY hkey;
		if(ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", 0, KEY_QUERY_VALUE, &hkey)) 
		{ 
			DWORD value_type = REG_DWORD;
			DWORD value_size = sizeof(DWORD);
			::RegQueryValueEx(hkey, "TcpTimedWaitDelay", 0, &value_type, (LPBYTE)&time_wait, &value_size);
			:: RegCloseKey(hkey); 
		} 
		return (int)time_wait;
	}

	int run() override
	{
		if(connections_per_thread > buffers_per_thread)
		{
			Log_display(INITIALISATION, "The number of connections per thread is greater than the number of buffers per thread.");
			Log_display(INITIALISATION, "This will cause an error as some connections will not be able to get buffers.");
			Log_display(INITIALISATION, "The number of buffers per thread will be changed so that it is the same as the number of connections per thread.");
			buffers_per_thread = connections_per_thread;
		}

		int TIME_WAIT = get_TIME_WAIT();

		// Need to know how big the stock response will be.
		const char* stock_response; size_t stock_response_length;
		get_stock_response(stock_response, stock_response_length);
		
		Log_display(INITIALISATION, "Number of processors: " << no_of_processors);
		Log_display(INITIALISATION, "Number of threads: " << no_of_threads);
		Log_display(INITIALISATION, "Number of connections per thread: " << connections_per_thread);
		Log_display(INITIALISATION, "Maximum number of simultaneous connections: " << no_of_threads*connections_per_thread);
		Log_display(INITIALISATION, "Response size: " << stock_response_length << " bytes");
		Log_display(INITIALISATION, "TIME_WAIT: " << TIME_WAIT << " seconds.");	// -1 = use default.

		pool.start_up(no_of_threads);
		
		size_t memory_required = 8000;
		memory_required += no_of_threads*connections_per_thread*sizeof(connection);						// memory for connections
		memory_required += no_of_threads*buffers_per_thread*sizeof(OVERLAPPED_plus);					// memory for OVERLAPPED_plus
		memory_required += no_of_threads*no_of_OVERLAPPED_ENTRIES_per_thread*sizeof(OVERLAPPED_ENTRY);	// memory for OVERLAPPED_ENTRY
		memory_required += stock_response_length + 1;

		Log_display(INITIALISATION, "Memory required for structures: " << memory_required << " bytes");

		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		Log_display(INITIALISATION, "Page size is " << system_info.dwAllocationGranularity);
		if (!Declare_alloctor_pool_size(memory_required))
		{
			Log_error(INITIALISATION, "Unable to modify working set size.");
			return -1;
		}

		if(!listen_socket.create())
		{
			Log_error(INITIALISATION, "Unable to create WSASocket - " << socket_error_string(WSAGetLastError()));
			return -1;
		}

		// Info only
		int send_buffer_len; int sbllen = sizeof(int);
		int recv_buffer_len; int rbllen = sizeof(int);
		getsockopt(listen_socket, SOL_SOCKET, SO_SNDBUF, (char*)&send_buffer_len, &sbllen);
		getsockopt(listen_socket, SOL_SOCKET, SO_RCVBUF, (char*)&recv_buffer_len, &rbllen);

		socket_address address;
		if(!get_bind_address("", port, address))
		{
			Log_error(INITIALISATION, "Failed to resolve server bind address");	
			return -2;
		}

		Log_display(INITIALISATION, "Server url is http://" << address.ip_address() << ":" << address.port() << "/");

		Log_info(INITIALISATION, "Binding to server address.")
		if(!listen_socket.bind(address))
		{
			Log_error(INITIALISATION, "Unable to bind to address " << address << socket_error_string(WSAGetLastError()));
			return -3;
		}

		Log_display(INITIALISATION, "Listening for connections on " << address);
	    if(!listen_socket.listen()) 
		{
			Log_error(INITIALISATION, "socket::listen() failed - " << socket_error_string(WSAGetLastError()));
			return -4;
		}
		
		IO_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, no_of_threads+1);
        if(IO_completion_port == NULL) 
		{
			Log_error(INITIALISATION, "Failed to create main completion port");
			return -5;
		}
	
		// New in-bound connections will be associated with a completion port as well as the accept socket.

		if(CreateIoCompletionPort((HANDLE)(SOCKET)listen_socket, IO_completion_port, (ULONG_PTR)completion_key::none, no_of_threads+1) == NULL)
		{
			Log_error(INITIALISATION, "Unable to associate listerner socket with completion port.");
			return -6;
		}

		if(!load_AcceptEx() || !load_DisconnectEx()) // provider error
			return -7;

		Log_info(INITIALISATION, "Allocating OVERLAPPED pool.");
		Overlapped_factory.allocate(no_of_threads*(buffers_per_thread+1));

		for(size_t i=0; i<no_of_threads; i++)
		{
			HANDLE IOCP = IO_completion_port;	// local copy for lambda.

			pool.add_task([this, IOCP]() 
			{
				// Connection objects contain the socket object as well as the "application" state.
				// There are multiple connection objects per thread.

				Log_info2(PROCESSING, "Allocating connection(s) for the current thread");
				connection_factory Connection_factory;
				Connection_factory.allocate(connections_per_thread);
				
				// Each socket (which is regarded as part of the connection) is setup to accept new network 
				// connections. Note that there will be multiple sockets waiting to accept at any given time.

				Log_info2(PROCESSING, "Setting up connections to AcceptEx");
				for(size_t i=0; i<connections_per_thread; i++)
				{
					class connection* connection = Connection_factory.create();
					assert(connection != nullptr);
					accept_connection(listen_socket, IOCP, connection);					
				}

				// Allocate space to hold retrived completion packets.

				Log_info(PROCESSING, "Allocating OVERLAPPED_ENTRY(s)");
				OVERLAPPED_ENTRY* overlapped_entries = (OVERLAPPED_ENTRY*)allocator->allocate(no_of_OVERLAPPED_ENTRIES_per_thread*sizeof(OVERLAPPED_ENTRY)); 

				while(!application_stopping())
				{
					// Retrieve completion packets.
					// GetQueuedCompletionStatusEx retrives possibly more than one completion packet.

					unsigned long no_entries_retrieved = 0;
					if(GetQueuedCompletionStatusEx(IOCP, overlapped_entries, no_of_OVERLAPPED_ENTRIES_per_thread, &no_entries_retrieved, INFINITE, FALSE) == FALSE)
					{
						Log_error(PROCESSING, "Unable to get I/O completion port status. " << socket_error_string(WSAGetLastError()));
						continue;
					}

					// Process each of the retrived completion packets.

					for(size_t i=0; i<no_entries_retrieved; i++)
					{
						OVERLAPPED_ENTRY& entry = overlapped_entries[i];
						if(entry.lpCompletionKey == (ULONG_PTR)completion_key::check_connections)
						{
							check_connections(); 
							break;
						}
						else if(   entry.lpCompletionKey == (ULONG_PTR)completion_key::IO	// read or write.
								|| entry.lpCompletionKey == (ULONG_PTR)completion_key::none // connection accepted.
							   ) 
						{ 
							OVERLAPPED_plus* overlapped = (OVERLAPPED_plus*)entry.lpOverlapped;
							assert(overlapped->is_valid());
							assert(overlapped->Internal == 0); // should always be true

							// Get the connection object originally associated with the returned completion packet.

							connection * connection = overlapped->connection;
							assert(connection->is_valid());

							// The connection object contains the "application" state so invoke on_IO_complete
							// to allow it to make sense of the new I/O.

							if(overlapped->operation == operation_type::Disconnect)
								connection->on_disconnect_complete();  // only invoked if using TF_REUSE_SOCKET. 
							else
								connection->on_IO_complete(*overlapped, overlapped->buffer, entry.dwNumberOfBytesTransferred);

							// If communication with the client is over, close and reuse the connection objects. 

							if(connection->is_finished())
							{
								Overlapped_factory.release(overlapped);
								Log_info2(PROCESSING, "Re-using connection");
								connection->state_machine->reset();
								accept_connection(listen_socket, IOCP, connection);
							}
						}
					}
				}
			});
		}

		return 0;
	}

	void on_closedown() override
	{
		PostQueuedCompletionStatus(IO_completion_port, 0, (ULONG_PTR)completion_key::shutdown, nullptr);
		notify_closed_down();
	}

	void check_connections()
	{
		Log_info2(PROCESSING, "Checking connections are still OK.");
		// Not implemented.
	}

	bool accept_connection(tcp_socket_type& listen_socket, HANDLE IO_completion_port, class connection* connection)
	{
		if(!connection->create_associated_socket())
		{
			Log_error(PROCESSING, "Unable to create WSASocket for new connection - " << socket_error_string(WSAGetLastError()));
			return false;
		}

		// Associate a connection with the new socket.
		HANDLE h = CreateIoCompletionPort(reinterpret_cast<HANDLE>((SOCKET)connection->accept_socket), IO_completion_port, (ULONG_PTR)completion_key::IO, no_of_threads+1);
		assert(h != NULL);

		OVERLAPPED_plus* p = Overlapped_factory.create(connection);

		p->operation = operation_type::Accept;
		DWORD bytes = 0;
		BOOL rc = fnAcceptEx(listen_socket, 
							connection->accept_socket,
							p->buffer,
							MAX_BUFFER_SIZE,
							sizeof(SOCKADDR_STORAGE) + 16, 
							sizeof(SOCKADDR_STORAGE) + 16,
							&bytes, 
							p);
		if(rc == FALSE && (ERROR_IO_PENDING != WSAGetLastError())) 
		{
			Log_error(PROCESSING, "AcceptEx() failed!! - " << socket_error_string(WSAGetLastError()));
			return false;
		}
		SafeIncrement(&no_of_accepts);

		::setsockopt(connection->accept_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, 
				(char*)&listen_socket, sizeof(SOCKET));

		return true;
	}

	int on_exit() override
	{
		Log_display(CLOSEDOWN, "Exiting the application.");
		pool.stop();

		Log_display(CLOSEDOWN, "No of accepts processed (not connections accepted): " << no_of_accepts);
		Log_display(CLOSEDOWN, "No of response sent OK: " << number_of_responses_sent_OK());
		Log_display(CLOSEDOWN, "No of failed sends: " << number_of_failed_sends());
		return 0;
	}

	bool load_AcceptEx()
	{
		// Load the AcceptEx function from the provider
		DWORD bytes = 0;
		GUID AcceptEx_guid = WSAID_ACCEPTEX;
		if(WSAIoctl(listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &AcceptEx_guid, sizeof(AcceptEx_guid),
					&fnAcceptEx, sizeof(fnAcceptEx), &bytes, NULL, NULL) == SOCKET_ERROR)
		{
			Log_error(INITIALISATION, "Failed to load AcceptEx - " << socket_error_string(WSAGetLastError()));
			return false;
		}
		return true;
	}

	bool load_DisconnectEx()
	{
		// Load the DisconnectEx function from the provider
		DWORD bytes = 0;
		GUID DisconnectEx_guid = WSAID_DISCONNECTEX;
		if(WSAIoctl(listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &DisconnectEx_guid, sizeof(DisconnectEx_guid),
					&connection::fnDisconnectEx, sizeof(connection::fnDisconnectEx), &bytes, NULL, NULL) == SOCKET_ERROR)
		{
			Log_error(INITIALISATION, "Failed to load DisconnectEx - " << socket_error_string(WSAGetLastError()));
			return false;
		}
		return true;
	}
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);
	TCP_server app;
	return app.main(argc, argv);
}

