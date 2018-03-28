#pragma once

// An connection is assocaited with each socket. It contains pointers to all the required
// buffers and WSAOVERLAPPED structures.

enum class connection_state_type { New=47, Connected=48, Disconnecting=47, Disconnected=46, Closed=45 };

class connection
{
public:
	connection();
	~connection();
	bool is_valid() const;

	// This needs to be called so that the socket function DisconnectEx is
	// available to allow re-use of "disconnected" sockets.
	static LPFN_DISCONNECTEX fnDisconnectEx;

	bool create_associated_socket();
	bool recv(char* buf, int& length, OVERLAPPED_plus& overlapped);
	bool send(const char* buf, int length, OVERLAPPED_plus& overlapped);
	void finished(OVERLAPPED_plus& overlapped);		// may not be the same as close() - sockets can be reused.
	bool is_finished() const;
	void close();
	bool is_closed();

    tcp_socket_type accept_socket;

	// These connection (key) methods are called when the completion packets are dequeued. 
	// The associated buffer(s) are assembled and the appropriate method in the application state machine called.
	// The state machine decides what should be done (send/recv/close).
	void on_IO_complete(OVERLAPPED_plus& overlapped, char* buffer, size_t bytes);

	void on_disconnect_complete();  // Only used with TF_REUSE_SOCKET

	// This is the application state machine NOT not the connection state.
	interface app_state_machine* state_machine;

	// This is the connection state.
	connection_state_type state;
	bool is_valid_state() const;

	sample_app_state_machine sample_state_machine;  // convenient place to put this.
};

class connection_factory
{
public:
	void allocate(size_t capacity);
	connection* create();
	void release(connection*);
private:
	dedicated_heap<connection> pool;
};

////////////// inline methods.

inline connection::connection()
	: state(connection_state_type::New)
{
	state_machine = &sample_state_machine;
}

inline bool connection::create_associated_socket()
{
	if(!accept_socket.create())
	{
		Log_error(PROCESSING, "Unable to create WSASocket for new connection - " << socket_error_string(WSAGetLastError()));
		return false;
	}
	state = connection_state_type::Connected;
	return true;
}

inline connection::~connection() { if(!is_closed()) close(); } 

inline void connection::close() { state = connection_state_type::Closed; accept_socket.close(); }

inline bool connection::is_closed() { return (state == connection_state_type::Closed); }

/// Using TF_REUSE_SOCKET does not seem to have a noticable effect on performance. (Windows version specific?)
/// Easier just to close the socket when finished.
inline void connection::finished(OVERLAPPED_plus& overlapped) 
{
#ifdef USE_TF_REUSE_SOCKET
	overlapped.operation = operation_type::Disconnect;
	if(state == connection_state_type::Connected)
	{
		if(fnDisconnectEx(accept_socket, &overlapped, TF_REUSE_SOCKET, 0) == TRUE)
			state = connection_state_type::Disconnected;
		else
			state = connection_state_type::Disconnecting;
	}
#else
	overlapped.operation = operation_type::Close;
	accept_socket.close();
	state = connection_state_type::Closed;
#endif
}

inline bool connection::is_finished() const 
{
#ifdef USE_TF_REUSE_SOCKET
	return state == connection_state_type::Disconnected; 
#else
	return state == connection_state_type::Closed; 
#endif
}

inline void connection::on_disconnect_complete() { state = connection_state_type::Disconnected; }

inline bool connection::recv(char* buf, int& length, OVERLAPPED_plus& overlapped)
{
	overlapped.operation = operation_type::Recv;
	return accept_socket.recv(buf, MAX_BUFFER_SIZE, length, 0, &overlapped);
}

inline bool connection::send(const char* buf, int buf_len, OVERLAPPED_plus& overlapped)
{
	overlapped.operation = operation_type::Send;
	return accept_socket.send(buf, buf_len, 0, &overlapped);
}

inline void connection::on_IO_complete(OVERLAPPED_plus& overlapped, char* buffer, size_t bytes)
{
	assert(state_machine != nullptr);
	state_machine->on_IO_complete(*this, overlapped, buffer, bytes);
}

