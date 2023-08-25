#include <iostream>
#include <cstring>
#include "koltcp.h"


void putline(kol::TcpSocket &sock, const char *str)
{
	if (str != 0) sock.write(str, strlen(str));
	sock.write("\n", strlen("\n"));
	return;
}

int main()
{
	using namespace kol;

	try {
		SocketLibrary socklib;
		char linebuf[4096];
		TcpServer server(8880);
		while(1) {
			TcpSocket sock = server.accept();
			putline(sock, "HTTP/1.0 200 ");
			putline(sock, "Content-Type: text/plain");
			putline(sock, 0);
			while(sock.getline(linebuf, 4095)) {
				putline(sock, linebuf);
				if(linebuf[0] == 0) break;
			}
			sock.flush();
			sock.close();
		}
	} catch(...) {
		std::cout << "Error" << std::endl;
	}
	return 0;
}
