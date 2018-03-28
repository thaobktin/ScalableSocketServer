#pragma once
// Copyright (c) Shaun O'Kane. 2012
#pragma comment(lib,"Ws2_32.lib")

#define SCK_VERSION1            0x0101
#define SCK_VERSION2            0x0202

enum class address_family : ADDRESS_FAMILY
{
	Unspecified = AF_UNSPEC,
	INET = AF_INET,
	IPX = AF_IPX,
	AppleTalk = AF_APPLETALK,
	NetBIOS = AF_NETBIOS,
	INET6 = AF_INET6,
	Infra_red_Association = AF_IRDA,
	Bluetooth = AF_BTH,
};

class port
{
public:
	port();

	// factory methods
	static class port from_native(unsigned short port_no);
	static class port from_host(unsigned short port_no);

	unsigned short native_format() const;
	unsigned short host_format() const;

	std::string to_string() const;
private:
	port(unsigned short _port_no);
	unsigned short port_no;  // native format
};

std::ostream& operator<<(std::ostream& os, port port);
std::istream& operator>>(std::istream& is, port& port);

// Only really interested in INET sockets.
class socket_address
{
	struct inet4_sockaddr  // used to intrepret sockaddr
	{
		address_family family;
		port port;
		IN_ADDR addr;
	};
public:
	socket_address();
	socket_address(const IN_ADDR& inet_address, port port_no);
	socket_address(const socket_address&);

	struct sockaddr sockaddr;  
	
	address_family family() const;
	port port() const;
	const IN_ADDR& ip_address() const;
};

void inet_resolve(const char* addr, const char* service, 
				address_family accept_family, 
				std::list<socket_address>& addresses);

const char* socket_error_string(int error_code);

std::ostream& to_stream(std::ostream& os, address_family af);
std::ostream& operator<<(std::ostream& os, const IN_ADDR& addr);
std::ostream& operator<<(std::ostream& os, socket_address& address);

#if defined(Use_WSASend)
typedef DWORD flags_t;
#elif defined(Use_send)
typedef int flags_t;
#else 
typedef int flags_t;
#endif

class our_socket 
{
public: 
	static void on_dtor(SOCKET&);
};

class not_our_socket
{
public: 
	static void on_dtor(SOCKET&);
};

class socketx
{
	socketx(const socketx& s) = delete;
	socketx& operator=(const socketx&) = delete;
public:
	socketx();
	bool is_valid() const;
	void close();
	bool bind(socket_address&);	
	operator SOCKET();
	SOCKET handle;
};

template<class socket_trait = our_socket>
class udp_socket : public socketx
{
public:
	// UDP
	udp_socket();
	bool create();
	~udp_socket();
	
	bool send_to(socket_address&, const char* buf, int buf_len);
	bool recv_from(socket_address&, char* buf, int buf_len, int& bytes_received);
};
typedef udp_socket<> udp_socket_type;

template<class socket_trait = our_socket>
class tcp_socket : public socketx
{
public:
	// TCP
	tcp_socket();
	bool create();
	~tcp_socket();

	bool connect(socket_address&);
	bool listen();
	void accept(socketx& s, struct sockaddr *addr = nullptr, int* addrlen = nullptr);
	bool send(const char* buf, int buf_len, flags_t flags=0, WSAOVERLAPPED* olp=nullptr);
	bool recv(char* buf, int buf_len, int& bytes_received, flags_t* flags=nullptr, WSAOVERLAPPED* olp=nullptr);
};
typedef tcp_socket<> tcp_socket_type;

// tcp_socket_ref does not own the socket HANDLE - only used by the TCP select server.
class tcp_socket_ref : public tcp_socket<not_our_socket> 
{
public:
	tcp_socket_ref(SOCKET h = INVALID_SOCKET);
	tcp_socket_ref& operator=(SOCKET h);
};

class socket_mgr
{
public:
	socket_mgr();
	~socket_mgr();
};

std::string gethostname();

