#include "stdafx.h"
#include "../common/logging.h"
#include "../common/socket.h"
#include "../common/socket.inl"
#include "../common/allocator.h"
#include "../common/dedicated_heap.h"
#include "../common/stock_response_factory.h"
#include "Overlapped.h"
#include "request_response.h"
#include "connection.h"

stock_response_factory response_factory;

Virtual_allocator virtual_memory_allocator;

namespace 
{
	unsigned long long no_of_responses_sent_OK = 0;
	unsigned long long no_of_failed_sends = 0;
}
unsigned long long number_of_responses_sent_OK() {return no_of_responses_sent_OK; }
unsigned long long number_of_failed_sends() {return no_of_failed_sends; }

sample_app_state_machine::sample_app_state_machine()
	: state(state_type::Start)
{
}

void sample_app_state_machine::initialise()
{
	static bool initialised = false;
	if(initialised)
	{
		response_factory.initialise(virtual_memory_allocator);
		initialised = true;
	}
}

void sample_app_state_machine::reset()  
{ 
	state = state_type::Start; 
}

inline void sample_app_state_machine::on_acceptex_complete(class connection& connection, OVERLAPPED_plus& overlapped, char*, size_t)
{
	Log_info2(PROCESSING, "connection - on_accept_complete() called.");

	response_factory.increment_request_count();

	const char* response; size_t response_length;
	response_factory.get_stock_response(response, response_length);

	bool ok = connection.send(response, response_length, overlapped); assert(ok);
	if(ok)
		SafeIncrement(&no_of_responses_sent_OK);
	else
		SafeIncrement(&no_of_failed_sends);
	state = state_type::Accepted;	
}

inline void sample_app_state_machine::on_send_complete(class connection& connection, OVERLAPPED_plus& overlapped, char*, size_t)
{
	Log_info2(PROCESSING, "connection - on_send_complete() called.");
	connection.finished(overlapped);
	state = state_type::Finished;
}

// It is up to the state machine to track the state and decide when to close the connection.
// The server closes the connection if this function return false;
void sample_app_state_machine::on_IO_complete(class connection& connection, OVERLAPPED_plus& overlapped, char* buffer, size_t bytes)
{
	std::unique_lock<std::mutex> _lock(mtx); 
	switch (state)
	{
	case state_type::Start:
		on_acceptex_complete(connection, overlapped, buffer, bytes);
		break;

	case state_type::Accepted:
		on_send_complete(connection, overlapped, buffer, bytes);
		break;

	case state_type::Finished:
		break;

	default:
		assert(false);
	}
}
