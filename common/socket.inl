#pragma once

// Copyright (c) Shaun O'Kane. 2012, 2015

inline port::port() : port_no(0) {}
inline port::port(unsigned short _port_no) : port_no(_port_no) {}

inline port port::from_native(unsigned short port_no) { return port(port_no); }
inline port port::from_host(unsigned short port_no) { return port(htons(port_no)); }

inline unsigned short port::native_format() const {return port_no;}
inline unsigned short port::host_format() const {return ntohs(port_no);}

inline std::string port::to_string() const { std::stringstream ss; ss << ntohs(port_no); return ss.str(); }

inline std::ostream& operator<<(std::ostream& os, port port) { return os << port.host_format(); }

inline std::istream& operator>>(std::istream& is, port& port)
{
	unsigned short p;
	is >> p;
	port = port::from_host(p);
	return is;
}

inline address_family socket_address::family() const { return reinterpret_cast<const inet4_sockaddr*>(&sockaddr)->family; }
inline port socket_address::port() const { return reinterpret_cast<const inet4_sockaddr*>(&sockaddr)->port; }
inline const IN_ADDR& socket_address::ip_address() const { return reinterpret_cast<const inet4_sockaddr*>(&sockaddr)->addr; }

/*******************************************************************************************************************************************
** socket 
********************************************************************************************************************************************/

inline void our_socket::on_dtor(SOCKET& h) { closesocket(h); h = INVALID_SOCKET; }

inline void not_our_socket::on_dtor(SOCKET&) { }

inline socketx::socketx() : handle(INVALID_SOCKET) {}

inline void socketx::close() { ::closesocket(handle); handle = INVALID_SOCKET; }

inline bool socketx::is_valid() const { return handle != INVALID_SOCKET; }

inline socketx::operator SOCKET() { return handle; }

inline bool socketx::bind(socket_address& address) { return(::bind(handle, &address.sockaddr, sizeof(sockaddr)) >= 0); }

/*******************************************************************************************************************************************
** tcp_socket 
********************************************************************************************************************************************/

template<typename socket_trait> tcp_socket<socket_trait>::tcp_socket() { }

template<typename socket_trait> tcp_socket<socket_trait>::~tcp_socket() { socket_trait::on_dtor(handle); }

template<typename socket_trait> bool tcp_socket<socket_trait>::create() { return((handle = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED)) != INVALID_SOCKET); }

template<typename socket_trait> bool tcp_socket<socket_trait>::connect(socket_address& addr) { return (::connect(handle, &addr.sockaddr, sizeof(addr.sockaddr)) == SOCKET_ERROR); }

template<typename socket_trait> bool tcp_socket<socket_trait>::listen() { return (::listen(handle, SOMAXCONN) != SOCKET_ERROR); }

template<typename socket_trait> void tcp_socket<socket_trait>::accept(socketx& s, struct sockaddr *addr, int* addrlen)
{ 
	s.handle = ::accept(handle, addr, addrlen); 
}

// Some ugly data conversions follow
#pragma warning(push)
#pragma warning (disable: 4189 4100 4512)

template<typename socket_trait> bool tcp_socket<socket_trait>::recv(char* buf, int buf_len, int& bytes_received, flags_t* flags, WSAOVERLAPPED* olp)
{
#if defined(Use_WSARecv)
	WSABUF wsabuf;
	wsabuf.buf = const_cast<char*>(buf);
	wsabuf.len = buf_len;
	DWORD bc = 0;
	int rc = ::WSARecv(handle, &wsabuf, 1, &bc, flags, olp, nullptr);
	bytes_received = bc;
	return (rc != SOCKET_ERROR); 

#elif defined(Use_ReadFile)
	DWORD bc = 0;
	BOOL retcode = ReadFile((HANDLE)handle, (void*)buf, buf_len, &bc, olp);
	bytes_received = (int)bc;
	return (retcode != FALSE);

#elif defined(Use_recv)
	bytes_received = 0;
	int iflags = (flags == nullptr) ? 0 : (int) *flags;
	int retcode = ::recv(handle, buf, buf_len, iflags);
	if(retcode >= 0) bytes_received = retcode;
	return (retcode != SOCKET_ERROR);

#else
//#pragma message("Please choose the socket::recv() implementation.")
	return false;
#endif
}

template<typename socket_trait> bool tcp_socket<socket_trait>::send(const char* buf, int buf_len, flags_t flags, WSAOVERLAPPED* olp) // not consistent with recv. TODO
{ 
#if defined(Use_WSASend)
	WSABUF wsabuf;
	wsabuf.buf = const_cast<char*>(buf);
	wsabuf.len = buf_len;
	int rc = ::WSASend(handle, &wsabuf, 1, flags, 0, NULL, NULL);
	return (rc != SOCKET_ERROR)

#elif defined(Use_WriteFile)
	DWORD bc = 0;
	return (WriteFile((HANDLE)handle, buf, buf_len, &bc, olp) != FALSE);

#elif defined(Use_send)
	return (::send(handle, buf, buf_len, flags) != SOCKET_ERROR); 

#else
//#pragma message("Please choose the socket::send() implementation.")
	return false;
#endif
}

#pragma warning(pop)

/*******************************************************************************************************************************************
** tcp_socket_cref 
********************************************************************************************************************************************/

inline tcp_socket_ref::tcp_socket_ref(SOCKET h) { handle = h; }

inline class tcp_socket_ref& tcp_socket_ref::operator=(SOCKET h) { handle = h; return *this; }

/*******************************************************************************************************************************************
** udp_socket 
********************************************************************************************************************************************/

template<typename socket_trait> udp_socket<socket_trait>::udp_socket() { }

template<typename socket_trait> udp_socket<socket_trait>::~udp_socket() { socket_trait::on_dtor(handle); }

template<typename socket_trait> bool udp_socket<socket_trait>::create() { return((handle = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED)) != INVALID_SOCKET); }

template<typename socket_trait> bool udp_socket<socket_trait>::send_to(socket_address& addr, const char* buf, int buf_len)
{ 
	return ::sendto(handle, buf, buf_len, 0, &addr.sockaddr, sizeof(sockaddr)) != SOCKET_ERROR; 
}

template<typename socket_trait> bool udp_socket<socket_trait>::recv_from(socket_address& address, char* buf, int buf_len, int& bytes_received)
{
	int sockaddr_len = sizeof(sockaddr);
	bytes_received = ::recvfrom(handle, buf, buf_len, 0, &address.sockaddr, &sockaddr_len);
	assert(sockaddr_len == sizeof(sockaddr));
	return (bytes_received >= 0);
}

