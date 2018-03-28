#include "stdafx.h"
#include "socket.h"
#include "socket.inl"
#include "allocator.h"
#include "dedicated_heap.h"
#include "app.h"

using namespace std;
namespace po = boost::program_options;

const port_t default_port = port_t::from_host(9000);	// used in program options.
static socket_app* app = nullptr;						// back pointer for Ctrl^C signal handler.
static bool socket_app_stop = false; 
static HANDLE evt = NULL;								// main thread waits on this before exiting.

int socket_app::main(int argc, char* argv[])
{	
	if(!process_options(argc, argv))
	{
		std::cerr << "Initialisation failed while processing options.\n";
		return -1;
	}

	if(!allocator)
		allocator = make_shared<Virtual_allocator>();

	if(!initialise(*allocator))
	{
		std::cerr << "Unable to load HTML response.\n";
		return -1;
	}

	socket_mgr socket_mgr; // initialise socket subsystem

	if(ip_address.empty())
		ip_address = gethostname();
	if(ip_address.empty())
	{
		std::cerr << "Unable to get host name\n"; // shouldn't happen.
		return -1;
	}

	int rc = 0;
	try
	{
		rc = run();
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception thrown: " << e.what() << "\n";
	}
	if(rc == 0)
		::WaitForSingleObject(evt, INFINITE);
	
	int rc2 = on_exit();
	return (rc != 0) ? rc : rc2;  // opportunity to return more complex return codes.
}

socket_app::socket_app(const char* _app_name)
	: option_desc("Options")
{
	assert(_app_name != nullptr);
	stock_response_factory::set_server_name(_app_name);
	app = this;
	evt = CreateEvent(NULL, FALSE, FALSE, NULL);
    ::SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
}

socket_app::~socket_app()
{
	::CloseHandle(evt);
	::SetConsoleCtrlHandler(NULL, FALSE); 
}

BOOL WINAPI socket_app::console_ctrl_handler(DWORD Ctrl)
{
    switch (Ctrl)
    {
    case CTRL_C_EVENT:      // Falls through..
    case CTRL_CLOSE_EVENT:
		{
			socket_app_stop = true; 
			app->on_closedown();
		}
		return TRUE;
    default:
        return FALSE;
    }
}

bool socket_app::process_options(int argc, char* argv[])
{
	option_desc.add_options()
		("help", "produce help message")    
		("ip,i", po::value<string>(&ip_address), "IP address (xxx.xxx.xxx.xxx)")
		("port,p", po::value<port_t>(&port)->default_value(default_port), "port")
		("file,f", po::value<string>(&response_file), "use the contents of the file as the HTML response")
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

bool socket_app::application_stopping() { return socket_app_stop; }

void socket_app::on_closedown() {}

void socket_app::notify_closed_down() {	::SetEvent(evt); } // signal the app to proceed to exit.

int socket_app::on_exit() { return 0; }  // print stats etc.

#include <conio.h>

void press_any_key_to_continue(const char* prompt)
{
	if(prompt == nullptr)
		std::cout << "Press any key to continue.";
	else
		std::cout << prompt;
	_getch();
}

