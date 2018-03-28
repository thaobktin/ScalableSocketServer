#include "stdafx.h"
#include "socket.h"
#include "socket.inl"

using std::vector;
using std::list;

socket_mgr::socket_mgr()
{
	static WSADATA wsdata;
	int rc = ::WSAStartup(SCK_VERSION2, &wsdata);
	if(rc != 0)
		throw std::runtime_error("Could not initialise socket subsystem.");
}

socket_mgr::~socket_mgr() { ::WSACleanup(); }

std::ostream& address_family_to_stream(std::ostream& os, const address_family& af)
{
	switch(af)
	{
	case address_family::Unspecified:
		return os << "Unspecified";
	case address_family::INET:
		return os << "INET";
	case address_family::IPX:
		return os << "IPX";
	case address_family::AppleTalk:
		return os << "AppleTalk";
	case address_family::NetBIOS:
		return os << "NetBIOS";
	case address_family::INET6:
		return os << "INET6";
	case address_family::Infra_red_Association:
		return os << "Infra_red_Association";
	case address_family::Bluetooth:
		return os << "Bluetooth";
	default:
		assert(false);
	}
	os << "*** UNRECOGNISE ADDRESS FAMILY ***";
	return os;
}

std::ostream& operator<<(std::ostream& os, const IN_ADDR& addr)
{
	os << (int)addr.S_un.S_un_b.s_b1 << "."
	   << (int)addr.S_un.S_un_b.s_b2 << "."
	   << (int)addr.S_un.S_un_b.s_b3 << "."
	   << (int)addr.S_un.S_un_b.s_b4;
	return os;
}

std::ostream& operator<<(std::ostream& os, socket_address& end_pt)
{
	os << "family:";
	address_family_to_stream(os, end_pt.family());
	if(end_pt.family() == address_family::INET)
	{
		os << " " << end_pt.ip_address() << ":" << end_pt.port();
	}
	return os;
}

socket_address::socket_address() { memset(&sockaddr, 0, sizeof(sockaddr)); }

socket_address::socket_address(const socket_address& other) { memcpy(this, &other, sizeof(socket_address)); }

const char* socket_error_string(int error_code)
{
	switch(error_code)
	{
	case 0:
		return "No error";

	case WSANOTINITIALISED:
		return "A successful WSAStartup call must occur before using this function.";
 
	case WSAENETDOWN: 
		return "The network subsystem has failed.";
 
	case WSAEACCES: 
		return "The requested address is a broadcast address, but the appropriate flag was not set. Call "
				"setsockopt with the SO_BROADCAST parameter to allow the use of the broadcast address.";
 
	case WSAEINVAL: 
		return "An unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled.";
 
	case WSAEINTR: 
		return "A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall.";
 
	case WSAEINPROGRESS: 
		return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
 
	case WSAEFAULT: 
		return "The buf or to parameters are not part of the user address space, or the tolen parameter is too small.";
 
	case WSAENETRESET: 
		return "The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.";
 
	case WSAENOBUFS: 
		return "No buffer space is available.";
 
	case WSAENOTCONN: 
		return "The socket is not connected (connection-oriented sockets only).";
 
	case WSAENOTSOCK: 
		return "The descriptor is not a socket.";
 
	case WSAEOPNOTSUPP: 
		return "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication "
				"domain associated with this socket, or the socket is unidirectional and supports only receive operations.";
 
	case WSAESHUTDOWN: 
		return "The socket has been shut down; it is not possible to sendto on a socket after shutdown has been invoked with how set to SD_SEND or SD_BOTH.";
 
	case WSAEWOULDBLOCK: 
		return "The socket is marked as nonblocking and the requested operation would block.";
 
	case WSAEMSGSIZE: 
		return "The socket is message oriented, and the message is larger than the maximum supported by the underlying transport.";
 
	case WSAEHOSTUNREACH: 
		return "The remote host cannot be reached from this host at this time.";
 
	case WSAECONNABORTED: 
		return "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.";
 
	case WSAECONNRESET: 
		return "The virtual circuit was reset by the remote side executing a hard or abortive close. For UPD sockets, the remote host was unable to deliver a"
				"previously sent UDP datagram and responded with a \"Port Unreachable\" ICMP packet. The application should close the socket as it is no longer usable.";
 
	case WSAEADDRNOTAVAIL: 
		return "The remote address is not a valid address, for example, ADDR_ANY.";
 
	case WSAEAFNOSUPPORT: 
		return "Addresses in the specified family cannot be used with this socket.";
 
	case WSAEDESTADDRREQ: 
		return "A destination address is required.";
 
	case WSAENETUNREACH: 
		return "The network cannot be reached from this host at this time.";
 
	case WSAETIMEDOUT: 
		return "The connection has been dropped, because of a network failure or because the system on the other end went down without notice.";

	default:
		;
	}
	return "Beats me why (not known error code)";
}

std::string gethostname()
{
	char buf[254];
	buf[0] = 0;
	buf[253] = 0;
	gethostname(buf, sizeof(buf)-1);
	return buf;
}

// Only works for INET and INET6.
void inet_resolve(const char* addr_in, const char* service, address_family family, list<socket_address>& addresses)
{
	ADDRINFOA hints;
	ADDRINFOA* address_info = nullptr;
	memset(&hints, 0, sizeof(hints));
	int rc = getaddrinfo(addr_in, service, &hints, &address_info);
	assert(rc == 0);
	ADDRINFOA*  p = address_info;
	while(p != nullptr)
	{
		socket_address* ip_addr = reinterpret_cast<socket_address*>(p->ai_addr);
		if(family == ip_addr->family() && family == address_family::INET)
				addresses.push_back(*ip_addr);
		p = p->ai_next;
	}
	freeaddrinfo(address_info);
}

// Kludgy but this is only test code.
bool get_bind_address(const std::string ip_address, const port port, socket_address& addr)
{
	std::list<socket_address> addresses;
	inet_resolve(ip_address.c_str(), port.to_string().c_str(), address_family::INET, addresses);
	for(auto& address : addresses)
	{
		if(address.ip_address().S_un.S_un_b.s_b1 == 192)  // you may want to change this depending on you network setup.
		{
			addr = address;
			return true;
		}
	}
	return false;
}


