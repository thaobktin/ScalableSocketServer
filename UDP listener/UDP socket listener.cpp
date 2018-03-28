#include "stdafx.h"
#include "../common/socket.h"
#include "../common/socket.inl"
#include "../common/dedicated_heap.h"
#include "../common/app.h"
#include "../UDP source/ticker_source.h"

using std::vector;
using std::string;

class UDP_listener : public socket_app
{
	udp_socket_type socket;

public:
	UDP_listener()
		: socket_app("UDP listener")
	{
		port = port::from_native(9000);
	}

	int run() override
	{
		if(!socket.create())
		{
			std::cerr << "Failed to create socket for address\n";
			return -1;
		}			

		socket_address address;
		if(!get_bind_address("", port, address))
		{
			std::cerr << "Failed to resolve server bind address\n";	
			return -2;
		}

		std::cout << "Binding to " << address.ip_address() << ":" << address.port() << "\n";
		if(!socket.bind(address))
		{
			std::cerr << "Unable to bind to address " << address << "\n";
			std::cerr << socket_error_string(WSAGetLastError()) << "\n";
			return -3;
		}

		unsigned int max_msg_size;
		int len = sizeof(max_msg_size);
		getsockopt(socket, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&max_msg_size, &len);
		std::cout << "Max UDP message size is " << max_msg_size << " bytes\n";

		static char buf[1000];
		while(!application_stopping())
		{
			socket_address from; int bytes_recieved = 0;
			if(!socket.recv_from(from, buf, sizeof(buf), bytes_recieved))
			{
				std::cerr << socket_error_string(WSAGetLastError()) << "\n";
			}

			const struct ticker* ticker = reinterpret_cast<const struct ticker*>(buf);
			if(ticker->msg_ID %10000 == 0)
				std::cout << ticker->msg_ID << " messages so far.\n";
		}
		
		notify_closed_down();
	
		return 0;
	}

	void on_closedown() override 
	{
		socket.close(); 
	}
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);
	UDP_listener app;
	return app.main(argc, argv);
}
