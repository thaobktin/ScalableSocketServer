//
// boost tutorial - modified by Shaun O'Kane, 2015
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

using boost::asio::io_service;
using boost::asio::buffer;
using boost::asio::ip::tcp;
using namespace std; 

#include "../common/socket.h"	// not ideal...
#include "../common/app.h"

class tcp_connection : public enable_shared_from_this<tcp_connection>
{
public:
	typedef std::shared_ptr<tcp_connection> pointer;

	static pointer create(io_service& io_service) { return pointer(new tcp_connection(io_service)); }

	tcp::socket& socket() { return socket_; }

	void start()
	{
		socket_app::increment_request_count();

		const char* stock_response; size_t stock_response_length;
		socket_app::get_stock_response(stock_response, stock_response_length);

		boost::asio::async_write(socket_, 
								 buffer(stock_response, stock_response_length),
								 std::bind(&tcp_connection::handle_write, 
								           shared_from_this(),
										   std::placeholders::_1,
										   std::placeholders::_2));
	}

private:
	tcp_connection(io_service& io_service)
		: socket_(io_service)
	{
	}

	void handle_write(const boost::system::error_code& error, size_t /*bytes_transferred*/) 
	{ 
		boost::system::error_code ignored_error;
		socket_.shutdown(tcp::socket::shutdown_both, ignored_error);
	}

	tcp::socket socket_;
};

class tcp_server
{
public:
	tcp_server(io_service& io_service, unsigned short port_no)
		: acceptor_(io_service, tcp::endpoint(tcp::v4(), port_no))
	{
		start_accept();
	}

private:
	void start_accept()
	{
		tcp_connection::pointer new_connection = tcp_connection::create(acceptor_.get_io_service());
		acceptor_.async_accept(new_connection->socket(),
							   std::bind(&tcp_server::handle_accept, 
										   this, 
										   new_connection, 
										   std::placeholders::_1));
	}

	void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error)
	{
		if (!error)
			new_connection->start();
		start_accept();
	}

	tcp::acceptor acceptor_;
};

class TCP_select_server : public socket_app
{
public:
	TCP_select_server()
		: socket_app("TCP asio async server")
	{
	}	

	int run() override
	{
		try
		{
			io_service io_service;
			tcp_server server(io_service, this->port.host_format());
			io_service.run();
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << std::endl;
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
	TCP_select_server app;

	int ret_code = app.main(argc, argv);
	
	press_any_key_to_continue();

	return ret_code;
}



