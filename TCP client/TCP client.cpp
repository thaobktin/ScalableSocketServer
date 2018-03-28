#include "stdafx.h"
#include "../common/socket.h"
#include "../common/socket.inl"
#include "../common/dedicated_heap.h"
#include "../common/app.h"

using namespace std::chrono;

class TCP_client : public socket_app
{
public:
	TCP_client()
		: socket_app("TCP client")
	{
	}
	
	int run() override
	{
		tcp_socket_type socket;
		if (!socket.create())
		{
			std::cerr << "Failed to create socket.\n";
			return -1;
		}

		socket_address address;
		if (!get_bind_address("", port, address))
		{
			std::cerr << "Failed to resolve server address.\n";	
			return -2;
		}

		std::cout << "Connecting to server at " << address.ip_address() << ":" << address.port() << "\n";
		if(connect(socket, (SOCKADDR*)&address, sizeof(SOCKADDR)) == SOCKET_ERROR)
		{
			std::cerr << "Failed to connect socket - " << socket_error_string(WSAGetLastError()) << "\n";
			return -3; //Couldn't connect
		}

		//press_any_key_to_continue("Press any key to issue HTML request\n");

		std::stringstream request;
		request << "GET /default.html HTTP/1.1\n"
					"Accept: text/html, application/xhtml+xml, */*\n"
					"Accept-Language: en-GB,en;q=0.5\n"
					"Host: " << address.ip_address() << "\n\n";
		std::cout << "The following request will be sent to the server\n" << request.str() <<"\n";
		if(!socket.send(request.str().c_str(), request.str().length()))
		{
			std::cout << "Failed to send request to server - " << socket_error_string(WSAGetLastError()) << "\n";
			return -4;
		}
		
		std::this_thread::sleep_for(milliseconds(250));

		std::cout << "Listening for response from the server\n";
		char buf[10000];
		int bytes_received = 0;
		if(!socket.recv(buf, sizeof(buf), bytes_received))
		{
			std::cout << "Failed to read request from server - " << socket_error_string(WSAGetLastError()) << "\n";
			return -5;
		}
		std::cout << "The following text was recieved: \n" << std::string(buf, bytes_received) << "\n";

		press_any_key_to_continue("Press any key to continue.\n");

		notify_closed_down();

		return  0;
	}
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);
	TCP_client app;
	return app.main(argc, argv);
}
