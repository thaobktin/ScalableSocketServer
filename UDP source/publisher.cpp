#include "stdafx.h"
#include "../common/socket.h"
#include "../common/socket.inl"
#include "../common/dedicated_heap.h"
#include "../common/app.h"
#include "ticker_source.h"
#include "timings.h"

using std::vector;
using std::string;
namespace po = boost::program_options;

class UDP_source : public socket_app
{
public:
	string filename;
	ticker_source source;

	UDP_source()
		: socket_app("UDP Source")
	{
		port = port::from_host(9000);
	}

	bool process_options(int argc, char* argv[])
	{
		option_desc.add_options()
			("help", "produce help message")    
			("file,f", po::value<string>(&filename), "source file to UDP messages")
			;
	
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(option_desc).run(), vm);
		po::notify(vm); 

		if (vm.count("help")) 
		{
			std::cout << "Available options.\n" 
					  << option_desc << "\n";
			return false;
		}
		return true;
	}


	int init_source()
	{
		if(!source.load(filename.c_str()))
		{
			std::cerr << "Unable to load ticker file " << filename << "\n";
			return -1;
		}
		return 0;
	}

	void generate_messages(udp_socket_type& socket, socket_address& address, ticker_source& source)
	{
		static int msg_ID = 0;
		ticker* ticker = nullptr;
		while(!application_stopping())
		{
			source.get_random_simulated_price(ticker);
			ticker->msg_ID = msg_ID++;
			socket.send_to(address, (const char*)ticker, sizeof(struct ticker));
			if(msg_ID %10000 == 0)
				std::cout << msg_ID << " messages so far.\n";
		}
	}

	int run() override
	{
		int rc = init_source();
		if(rc != 0)
		{
			std::cerr << "Failed to load quotes from file.\n";
			return rc;
		}

		socket_address address;
		if(!get_bind_address("", port, address))
		{
			std::cerr << "Failed to resolve server bind address\n";	
			return -1;
		}
		std::cout << "Sending packets to " << address << "\n";

		udp_socket_type socket;
		if(!socket.create())
		{
			std::cerr << "Failed to create socket - " << socket_error_string(WSAGetLastError()) << "\n";
			return -2;
		}

		unsigned int max_msg_size;
		int len = sizeof(max_msg_size);
		getsockopt(socket, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&max_msg_size, &len);
		std::cout << "Max UDP message size is " << max_msg_size << " bytes\n";

		try
		{
			generate_messages(socket, address, source);
		}
		catch(std::runtime_error& error)
		{
			std::cerr << error.what();
			return -1;
		}

		notify_closed_down();

		return 0;
	}
};

int _tmain(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);

	UDP_source app;

	return app.main(argc, argv);
}

