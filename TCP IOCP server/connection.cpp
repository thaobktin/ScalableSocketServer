#include "stdafx.h"
#include "../common/logging.h"
#include "../common/socket.h"
#include "../common/socket.inl"
#include "../common/allocator.h"
#include "../common/dedicated_heap.h"
#include "Overlapped.h"
#include "request_response.h"
#include "connection.h"

LPFN_DISCONNECTEX connection::fnDisconnectEx = nullptr;

inline bool connection::is_valid_state() const
{
	return state == connection_state_type::New
		|| state == connection_state_type::Connected
		|| state == connection_state_type::Disconnecting
		|| state == connection_state_type::Disconnected
		|| state == connection_state_type::Closed;
}

bool connection::is_valid() const { return state_machine != nullptr	&& is_valid_state(); }

void connection_factory::allocate(size_t capacity) { pool.allocate(capacity); }

connection* connection_factory::create()
{
	connection* connection = pool.get();
	assert(connection != nullptr);
	assert(connection->is_valid());
	return connection;
}

void connection_factory::release(connection* p) { pool.release(p); }