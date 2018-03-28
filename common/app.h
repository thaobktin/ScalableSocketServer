#pragma once
// Copyright (c) Shaun O'Kane. 2012

// potpourri class - takes care of 
// 1. Parsing of command line arguments (standard args: help, verson, ip-address, port)
// 2. Ctrl-C handler - calls closedown() on each instance of socket_app.
// 3. closedown() - bool stopping() returns true after call to closedown().
//
// closedown() should close down ALL instances of socket_apps.

#include "allocator.h"
#include "stock_response_factory.h"

// Intended to be a singleton.
class socket_app : public stock_response_factory
{
public:
	socket_app(const char* app_name);
	~socket_app();

	int main(int argc, char* argv[]);

	virtual int run() = 0;
	virtual void on_closedown();
	virtual void notify_closed_down();
	virtual int on_exit();

	static bool application_stopping();
	
protected:
	socket_app& operator=(const socket_app&) = delete; // no copying!

	// Command-line options.
	boost::program_options::options_description option_desc;  
	std::string ip_address;					// fefault bind address. e.g. 127.0.0.1 resolved into an IN_ADDR.
	port port;
	std::string response_file;				// The content of the specified file is used as the HTML response if not empty.

	std::shared_ptr<allocator_type> allocator;

	static BOOL WINAPI console_ctrl_handler(DWORD Ctrl);
	virtual bool process_options(int argc, char* argv[]);
};

typedef class port port_t;
bool get_bind_address(const std::string ip_address, const port_t port, socket_address& addr);
void press_any_key_to_continue(const char* prompt=nullptr);

