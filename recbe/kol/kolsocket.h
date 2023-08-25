#ifndef KOLSOCKET_H_INCLUDED
#define KOLSOCKET_H_INCLUDED

#include <exception>
#include <string>
#include <cstring>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#endif /* WIN32 */

#ifndef SHUT_RD
#define SHUT_RD    0
#define SHUT_WR    1
#define SHUT_RDWR  2
#endif /* SHUT_RD */

namespace kol
{
#ifdef WIN32
  typedef SOCKET socket_t;
  typedef char sockmsg_t;
  typedef unsigned long in_addr_t;
#else
  typedef int socket_t;
  typedef void sockmsg_t;
#endif /* WIN32 */

  /// exception class for socket function.
  class SocketException : public std::exception
  {
  public:
    explicit SocketException(const std::string& msg);
    virtual ~SocketException() throw();
    virtual const char* what() const throw();
    int reason() const throw();
  
  private:
    int m_reason;
    std::string m_msg;
  };

  /// socket library (e.g., winsock) loader.
  class SocketLibrary
  {
  public:
    SocketLibrary();
    virtual ~SocketLibrary();
    bool isloaded() const throw();

  private:
    bool m_libloaded;
  };


  /// SockAddrIn : class for internet socket address class.
  class SockAddrIn
  {
  public:
    SockAddrIn(char* host, int port);
    virtual ~SockAddrIn();
    const struct sockaddr_in* Address();

  protected:
    struct sockaddr_in m_saddr;
  };

  /// BSD socket library wrapper.
  /**
   * This is a wrapper class for BSD socket library.
   * The main purpose of this class is to hide the socket descriptor.
   * Socket descriptor is automatically closed when desctructed.
   * The descritor is copied by using duplicate system call (i.e., dup()
   * for Linux, DuplicateHandle() for Windows) when necessary.
   *
   * No exception is thrown from this class. A member function bad()
   * can be used to check the validity of the socket descriptor.
   *
   * @author Hirofumi Fujii (KEK online-electronics group) (c)2006
   */
  class Socket
  {
  public:
#ifdef WIN32
    static const socket_t invalid = INVALID_SOCKET;
    static const int sockerr = SOCKET_ERROR;
#else  /* !WIN32 */
    static const socket_t invalid = -1;
    static const int sockerr = -1;
#endif /* WIN32 */

  private:
    static int CloseSocket(socket_t s) throw();
    static socket_t DuplicateSocket(socket_t s) throw();
    static int IoctlSocket(socket_t s, int request, void* argp) throw();

  public:
    Socket();
    Socket(const Socket& from);
    Socket(int domain, int type, int protocol=0);
    virtual ~Socket();
    Socket& operator=(const Socket& from);
    int close();
    int create(int domain, int type, int protocol=0);
    int connect(const struct sockaddr* addr, socklen_t len);
    int bind(const struct sockaddr* addr, socklen_t len);
    int listen(int backlog);
    Socket accept(struct sockaddr* addr=0, socklen_t* addrlen=0);
    int send(const void* buf, size_t len, int flags=0);
    int recv(void* buf, size_t len, int flags=0);
    int sendto(const void* buf, size_t len, int flags, const struct sockaddr* to, socklen_t tolen);
    int recvfrom(void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* fromlen);
    int shutdown(int how=SHUT_RDWR);
    int getsockname(struct sockaddr* name, socklen_t* namelen) const;
    int getpeername(struct sockaddr* name, socklen_t* namelen) const;
    int getsockopt(int level, int optname, void* optval, socklen_t* optlen) const;
    int setsockopt(int level, int optname, const void* optval, socklen_t optlen);
    int ioctl(int request, void* argp);

  protected:
    Socket(socket_t sd);

  protected:
    socket_t m_sd;
  };
}

#endif

