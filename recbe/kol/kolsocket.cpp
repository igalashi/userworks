#include "kolsocket.h"
#ifndef WIN32
#include <errno.h>
extern int errno;
#endif
using namespace kol;

//
// Definitions for SocketLibrary class
//

SocketLibrary::SocketLibrary()
{
#ifdef WIN32
  WORD wVersion;
  WSADATA wsaData;
  wVersion = MAKEWORD(2,2);
  if(::WSAStartup( wVersion, &wsaData ))
    m_libloaded = false;
  else
    m_libloaded = true;
#else  /* !WIN32 */
  m_libloaded = true;
#endif /* WIN32 */
}

SocketLibrary::~SocketLibrary()
{
#ifdef WIN32
  if(m_libloaded)
    ::WSACleanup();
#else  /* WIN32 */
#endif /* WIN32 */
}

bool
SocketLibrary::isloaded() const throw()
{
  return m_libloaded;
}

//
// Definitions for SocketException class
//
SocketException::SocketException(const std::string& msg) : m_msg(msg)
{
  m_reason = 0;
}

SocketException::~SocketException() throw()
{
}

const char*
SocketException::what() const throw()
{
  return m_msg.c_str();
}

int
SocketException::reason() const throw()
{
  return m_reason;
}

//
// Definitions for SockAddrIn class
//
SockAddrIn::SockAddrIn(char* host, int port)
{
    struct sockaddr_in* resaddr;
    struct addrinfo hints;
    struct addrinfo* res;
    int err;

    res = 0;
    memset((char*)&m_saddr, 0, sizeof(m_saddr));
    if((host == 0) || host[0] == 0)
    {
      m_saddr.sin_family = AF_INET;
      m_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
      m_saddr.sin_port = htons((u_short)port);
    }
    else
    {
      memset((char*)&hints, 0, sizeof(hints));

      hints.ai_family = PF_INET;
      hints.ai_socktype = 0;
      hints.ai_protocol = 0;

      if((err = getaddrinfo(host, 0, &hints, &res)) == 0)
      {
        resaddr = (struct sockaddr_in*)res->ai_addr;
        m_saddr.sin_family = AF_INET;
        m_saddr.sin_port = htons((u_short)port);
        m_saddr.sin_addr = resaddr->sin_addr;
        freeaddrinfo(res);
      }
    }
}

SockAddrIn::~SockAddrIn()
{
}

const struct sockaddr_in*
SockAddrIn::Address()
{
  return &m_saddr;
}

//
// Helper functions for Socket class
//
int
Socket::IoctlSocket(socket_t s, int request, void* argp) throw()
{
  if(s == invalid)
    return sockerr;
#ifdef WIN32
  return ::ioctlsocket(s, request, (unsigned long*)argp);
#else  /* !WIN32 */
  return ::ioctl(s, request, argp);
#endif /* WIN32 */
}

socket_t
Socket::DuplicateSocket(socket_t s) throw()
{
  if(s == invalid)
    return s;
  socket_t t;
#ifdef WIN32
  HANDLE hProcess;
  hProcess = ::GetCurrentProcess();
  if(!::DuplicateHandle(hProcess, (HANDLE)s, hProcess, (HANDLE*)&t, 0, 0, DUPLICATE_SAME_ACCESS))
    t = invalid;
#else /* !WIN32 */
  t = ::dup(s);
#endif /* WIN32 */
  return t;
}

int
Socket::CloseSocket(socket_t s) throw()
{
  if(s == invalid)
    return sockerr;
#ifdef WIN32
  return ::closesocket(s);
#else  /* !WIN32 */
  return ::close(s);
#endif /* WIN32 */
}

//
// Socket class main functions
//
Socket::Socket()
{
  m_sd = invalid;
}

Socket::Socket(int domain, int type, int protocol)
{
  if((m_sd = ::socket(domain, type, protocol)) == invalid)
    throw SocketException("Socket::Socket error");
}

Socket::Socket(const Socket& from)
{
  m_sd = invalid;
  if(from.m_sd != invalid)
  {
    if((m_sd = DuplicateSocket(from.m_sd)) == invalid)
      throw SocketException("Socket::DuplicateSocket error");
  }
}

Socket::Socket(socket_t sd)
{
  m_sd = sd;
}

Socket::~Socket()
{
  close();
}

Socket& Socket::operator=(const Socket& from)
{
  if(&from == this)
    return *this;
  close();
  if(from.m_sd != invalid)
    m_sd = DuplicateSocket(from.m_sd);
  return *this;
}

int
Socket::close()
{
  int retval = sockerr;
  if(m_sd != invalid)
  {
    retval = CloseSocket(m_sd);
    m_sd = invalid;
  }
  return retval;
}

int
Socket::create(int domain, int type, int protocol)
{
  if(m_sd != invalid)
    CloseSocket(m_sd);
  if((m_sd = ::socket(domain, type, protocol)) == invalid)
    throw SocketException("Socket::create error");
  return 0;
}

int
Socket::connect(const struct sockaddr* addr, socklen_t len)
{
  if( ::connect(m_sd, addr, len) == sockerr)
    throw SocketException("Socket::connect error");
  return 0;
}
 
int
Socket::bind(const struct sockaddr* addr, socklen_t len)
{
  if( ::bind(m_sd, addr, len) == sockerr)
    throw SocketException("Socket::bind error");
  return 0;
}

int
Socket::listen(int backlog)
{
  if( ::listen(m_sd, backlog) == sockerr)
    throw SocketException("Socket::listen error");
  return 0;
}

Socket
Socket::accept(struct sockaddr* addr, socklen_t* addrlen)
{
  int sd;
  if((sd = ::accept(m_sd, addr, addrlen)) == sockerr)
    throw SocketException("Socket::accept error");
  return Socket(sd);
}

int
Socket::send(const void* buf, size_t len, int flags)
{
  int n;
  if((n = ::send(m_sd, (const sockmsg_t*)buf, len, flags)) == sockerr)
    throw SocketException("Socket::send error");
  return n;
}

int
Socket::sendto(const void* buf, size_t len, int flags, const struct sockaddr* to, socklen_t tolen)
{
  int n;
  if((n = ::sendto(m_sd, (const sockmsg_t*)buf, len, flags, to, tolen)) == sockerr)
    throw SocketException("Socket::sendto error");
  return n;
}

int
Socket::recv(void* buf, size_t len, int flags)
{
  int n;
  if((n = ::recv(m_sd, (sockmsg_t*)buf, len, flags)) == sockerr)
    throw SocketException("Socket::recv error");
  return n;
}

int
Socket::recvfrom(void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* fromlen)
{
  int n;
  if((n = ::recvfrom(m_sd, (sockmsg_t*)buf, len, flags, from, fromlen)) == sockerr)
    throw SocketException("Socket::recvfrom error");
  return n;
}

int
Socket::shutdown(int how)
{
  int n;
  if((n = ::shutdown(m_sd, how)) == sockerr)
    throw SocketException("Socket::shutdown error");
  return n;
}

int
Socket::getsockname(struct sockaddr* name, socklen_t* namelen) const
{
  return ::getsockname(m_sd, name, namelen);
}

int
Socket::getpeername(struct sockaddr* name, socklen_t* namelen) const
{
  return ::getpeername(m_sd, name, namelen);
}

int
Socket::getsockopt(int level, int optname, void* optval, socklen_t* optlen) const
{
  return ::getsockopt(m_sd, level, optname, (sockmsg_t*)optval, optlen);
}

int
Socket::setsockopt(int level, int optname, const void* optval, socklen_t optlen)
{
  return ::setsockopt(m_sd, level, optname, (const sockmsg_t*)optval, optlen);
}

int
Socket::ioctl(int request, void* argp)
{
  return IoctlSocket(m_sd, request, argp);
}
