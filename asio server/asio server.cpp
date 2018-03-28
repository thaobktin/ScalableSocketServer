//
// boost tutorial - modified by Shaun O'Kane, 2015
// Simple modified asio tutorial. 
// Ctrl-C will cause the server to exit after the next request it processes.
// That could be fixed by using a strand but that would be too much of a 
// deviation from the tutorial.
//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com) 
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "stdafx.h"

#include "../common/socket.h"	// not ideal...
#include "../common/app.h"

using boost::asio::ip::tcp;
using namespace std; 

class TCP_select_server : public socket_app
{
public:
	TCP_select_server()
		: socket_app("TCP asio server")
	{
	}	

	int run() override
	{
		try
		{
			boost::asio::io_service ioservice;
			tcp::acceptor acceptor(ioservice, tcp::endpoint(tcp::v4(), this->port.host_format()));

			while(!application_stopping())
			{
				tcp::socket tcp_socket(ioservice);
				acceptor.accept(tcp_socket);
				
				increment_request_count();

				const char* stock_response; size_t stock_response_length;
				get_stock_response(stock_response, stock_response_length);

				boost::system::error_code ignored_error;
				boost::asio::write(tcp_socket, boost::asio::buffer((void*)stock_response, stock_response_length), ignored_error);
			}
		}
		catch (std::exception& e)
		{
			cerr << e.what() << endl;
		}
		return 0;
	}

	virtual void on_closedown()
	{
		notify_closed_down();
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



