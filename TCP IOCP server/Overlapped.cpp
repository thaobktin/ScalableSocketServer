#include "stdafx.h"
#include "../common/logging.h"
#include "../common/socket.h"
#include "../common/allocator.h"
#include "../common/dedicated_heap.h"
#include "Overlapped.h"
#include "request_response.h"
#include "connection.h"

inline bool OVERLAPPED_plus::is_valid_state() const
{
	return operation == operation_type::None
		|| operation == operation_type::Accept
		|| operation == operation_type::Send
		|| operation == operation_type::Recv
		|| operation == operation_type::Disconnect
		|| operation == operation_type::Close;
}

bool OVERLAPPED_plus::is_valid() const 
{
	return connection != nullptr && is_valid_state(); 
}

void OVERLAPPED_factory::allocate(size_t capacity)
{
	pool.allocate(capacity);
}

OVERLAPPED_plus* OVERLAPPED_factory::create(class connection* connection)
{
	OVERLAPPED_plus* overlapped = pool.get();
	assert(overlapped != nullptr);
	overlapped->connection = connection;	
	assert(overlapped->is_valid());
	return overlapped;
}

void OVERLAPPED_factory::release(OVERLAPPED_plus* p) { pool.release(p); }



