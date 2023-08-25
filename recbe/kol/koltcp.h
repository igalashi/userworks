#ifndef MYTCP_H_INCLUDED
#define MYTCP_H_INCLUDED

#include "kolsocket.h"

// 16-Nov-2006
//  - Constructors of TcpBuffer, TcpClient and TcpServer with Socket are added
//    for setting the socket options before bind or connect.
//  - The number of backlog is added as an argument of TcpServer.
// 28-July-2006
//  - 2nd argument of write() was changed to std::streamsize
// 11-July-2006
//  Changed several interfaces close to iostream.
//  - putline() was removed.
//  - return values of put(), write() and flush() were changed to TcpBuffer&
//  - put(const char*) was removed.
//  - added good()
//  - sync() was removed.

namespace kol
{
  class TcpBuffer
  {
  public:
    TcpBuffer();
    TcpBuffer(const Socket& s);
    TcpBuffer(int domain, int type, int protocol=0);
    virtual ~TcpBuffer();
    virtual int close();
    virtual int get();
    TcpBuffer& getline(char* buf, std::streamsize maxlen);
    TcpBuffer& ignore(std::streamsize len=1);
    TcpBuffer& read(char* buf, std::streamsize len);
    TcpBuffer& put(int c);
    TcpBuffer& write(const void* buf, std::streamsize len);
    TcpBuffer& flush();
    virtual int shutdown(int how=SHUT_RDWR);
    int getsockopt(int level, int optname, void* optval, socklen_t* optlen) const;
    int setsockopt(int level, int optname, const void* optval, socklen_t optlen);
    std::streamsize gcount() const { return m_gcount; }
    bool good() const { return (m_iostate == goodbit); }
    bool eof() const { return ((m_iostate & eofbit) != 0); }
    bool fail() const { return ((m_iostate & (failbit | badbit)) != 0); }
    bool bad() const { return ((m_iostate & badbit) != 0); }
    operator void*() const
    { if(fail()) return 0;return (void*)this; }
    bool operator!() const { return fail(); }
  protected:
    int sync();
    void initparams();
    int recv_all(unsigned char* buf, int nbytes);
    int send_all(const unsigned char* buf, int nbytes);
  private:
    enum { bufsize = 1024 };
    enum { goodbit = 0, eofbit = 1, failbit = 2, badbit = 4 };

  protected:
    Socket m_socket;
    std::streamsize m_gcount;
    int m_iostate;
    size_t m_rbufmax;
    size_t m_rbuflen;
    size_t m_rbufnxt;
    unsigned char m_rbuf[bufsize];
    size_t m_sbufmax;
    size_t m_sbuflen;
    size_t m_sbufnxt;
    unsigned char m_sbuf[bufsize];
  };

  class TcpSocket : public TcpBuffer
  {
  public:
    TcpSocket();
    TcpSocket(const Socket& s);
    virtual ~TcpSocket();
  };

  class TcpClient : public TcpBuffer
  {
  public:
    TcpClient();
    TcpClient(const char* host, int port);
    TcpClient(const Socket& s, const char* host, int port);
    virtual ~TcpClient();

  private:
    void Start(const char* host, int port);
  };

  class TcpServer : public TcpBuffer
  {
  public:
    TcpServer();
    TcpServer(int port, int backlog=5);
    TcpServer(const Socket& s, int port, int backlog=5);
    virtual ~TcpServer();
    virtual TcpSocket accept();

  private:
    void Start(int port, int backlog);
  };
}

#endif

