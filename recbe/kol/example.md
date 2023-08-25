---
title: KEK Online Class Library Examples
...

------------------------------------------------------------------------

<div align="center">

network データ収集用C++ class library 使用例
--------------------------------------------

(©2006 Hirofumi Fujii, KEK Online-electronics group)\
(Revised: 29-Mar-2013, 27-Mar-2013)\
(Revised: 24-Aug-2006)

</div>

------------------------------------------------------------------------
------------------------------------------------------------------------

### はじめに

この文書は、著者が製作した network データ収集用C++ class library
の使用例について 述べたものである。

------------------------------------------------------------------------

### 例1：簡単なウェブサーバ

"JAVAプログラム　クイック　リファレンス"の例題にあるHttpMirror.javaに
対応するものを作ってみる。ただし port は 8880 に固定してある。

------------------------------------------------------------------------

    #include <iostream>
    #include "koltcp.h"

    int main()
    {
      using namespace kekonline;

      try
      {
        SocketLibrary socklib;
        char linebuf[4096];
        TcpServer server( 8880 );
        while(1)
        {
          TcpSocket sock = server.accept();
          sock.putline("HTTP/1.0 200 ");
          sock.putline("Content-Type: text/plain");
          sock.putline(0);
          while(sock.getline(linebuf,4095))
          {
            sock.putline(linebuf);
            if(linebuf[0]==0)
              break;
          }
          sock.flush();
          sock.close();
        }
      }
      catch(...)
      {
        std::cout << "Error" << std::endl;
      }
      return 0;
    }

------------------------------------------------------------------------

### 例2:汎用クライアント

Thread と TcpClient を使った例を、前項と同じく "JAVA
プログラム　クイック　リファレンス"から
汎用クライアント（GenericClient.java）に対応するもの示す。\
この例は host, port を 指定してプログラムを起動すると TCP
接続で指定されたサーバに 接続し、行単位で入出力を行うプログラムで、

-   標準入力から入力された行をサーバへ送るthread
-   サーバから読み取った行を標準出力へ表示するthread

の二つのthreadを使う例である。終了は両方の端点がEOFを検出した
時点で終わる。通常TCP接続はサーバ側がcloseし、標準入力は Linux であれば
CTRL-D の入力で、Windows であれば CTRL-Z の 入力で終わる。

なお、Linux では **libpthread**
をリンクする（**-lpthread**を指定する）必要がある。

Windows XP 以降で g++ を使う場合

    g++ -Wall -DWIN32 -D_WIN32_WINNT=0x0501 gencli.cpp kolsocket.cpp koltcp.cpp kolthread.cpp -lws2_32 -o gencli.exe

とする（XP以降で support された getaddrinfo() を使う)。

------------------------------------------------------------------------

    // gencli.cpp

    #include <iostream>
    #include <sstream>
    #include <stdexcept>
    #include <cstring>
    #include "koltcp.h"
    #include "kolthread.h"

    using namespace kekonline;

    class RThread : public Thread
    {
    // Receive line from TCP stream
    // and display it on std::cout.
    public:
      RThread(TcpClient& tcp) {m_tcp = tcp;};
    protected:
      int run()
      {
        while(m_tcp.getline(buf,1023))
        {
          std::cout << buf << std::endl;
          std::cout.flush();
        }
        m_tcp.close();
        return 0;
      };
    protected:
      TcpClient m_tcp;
      char buf[1024];
    };

    class SThread : public Thread
    {
    // Send line from std::cin to TCP stream
    public:
      SThread(TcpClient& tcp) {m_tcp = tcp;};
    protected:
      int run()
      {
        while(std::cin.getline(buf,1021))
        {
          size_t lsize = strlen(buf);
          // Add net-newline (CR-LF pair)
          buf[lsize] = 0x0a; ++lsize;
          buf[lsize] = 0x0d; ++lsize;
          buf[lsize] = 0;  // Make buf[] as C-string just in case
          m_tcp.write(buf, lsize);
          m_tcp.flush();
        }
        m_tcp.close();
        return 0;
      };
    protected:
      TcpClient m_tcp;
      char buf[1024];
    };

    int main(int argc, char* argv[])
    {
      try
      {
        SocketLibrary socklib;
        // Check the arguments of the command line
        if(argc != 3)
          throw std::invalid_argument("See usage");

        // 3rd argument is port number
        // Convert it to integer using istringstream
        std::istringstream ss(argv[2]);
        int port;
        ss >> port;

        // generate TCP connection (2nd argument in the command line is the host name)
        TcpClient tcp(argv[1], port);

        // generate threads (passing TCP connection to each thread)
        RThread rthread(tcp);
        SThread sthread(tcp);

        // start threads 
        rthread.start();
        sthread.start();

        // Wait for completion of each thread.
        rthread.join();
        sthread.join();
      }
      catch(...)
      {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
      }
      return 0;
    }

------------------------------------------------------------------------
------------------------------------------------------------------------
