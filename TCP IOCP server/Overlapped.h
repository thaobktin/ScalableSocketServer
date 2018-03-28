#pragma once

enum class operation_type { None=89, Accept=88, Send=87, Recv=86, Disconnect=85, Close=86 };

const size_t MAX_BUFFER_SIZE = 1248;
const DWORD overlapped__magic_number = 0x31415926;

class OVERLAPPED_plus : public WSAOVERLAPPED
{
public:
	OVERLAPPED_plus();

	operation_type  operation;
	char buffer[MAX_BUFFER_SIZE];
	char address_buffer[2*sizeof(SOCKADDR_STORAGE) + 16];  // must follow the read_buffer (see AcceptEx())
	class connection* connection;

	bool is_valid() const;
	bool is_valid_state() const;
	unsigned int magic_number;
};

class OVERLAPPED_factory
{
public:
	void allocate(size_t capacity);
	OVERLAPPED_plus* create(class connection*);
	void release(OVERLAPPED_plus*);
private:
	dedicated_heap<OVERLAPPED_plus> pool;
};

////////////// inline methods.

inline OVERLAPPED_plus::OVERLAPPED_plus()
	: operation(operation_type::None),
	  magic_number(overlapped__magic_number)
{
}



