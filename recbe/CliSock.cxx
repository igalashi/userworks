/*
 *
 *
 */

#include <iostream>
#include <iomanip>
#include <string>

#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <error.h>


class CliSock
{
public:
	CliSock() {};
	virtual ~CliSock() {};
	int Connect(const char*, int);
	int Send(char*, unsigned int);
	int Receive(char*, unsigned int, int&);
	int Close();
	int SetTimeOut_ms(int);
protected:
private:
	struct sockaddr_in fSockAddr;
	int fSocket;
	struct timeval fTimeOut = {0,  0};
};


int CliSock::Connect(const char* ip, int port)
{
	memset(reinterpret_cast<char *>(&fSockAddr), 0, sizeof(fSockAddr));
	fSockAddr.sin_family      = AF_INET;
	fSockAddr.sin_port        = htons(static_cast<unsigned short int>(port & 0xffff));
	fSockAddr.sin_addr.s_addr = inet_addr(ip);

	fSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fSocket < 0) {
		perror("CliSock::Connect ");
		return -1;
	}

	if ((fTimeOut.tv_sec != 0) || (fTimeOut.tv_usec != 0)) {
		if (setsockopt(fSocket,
			SOL_SOCKET, SO_RCVTIMEO, (char*)&fTimeOut, sizeof(fTimeOut)) < 0) {
			perror("CliSock::Connect setsockopt timeout ");
			return -1;
		}
	}

	#if 0
	int flag = 1;
	setsockopt(fSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
	#endif

	if(connect(fSocket, reinterpret_cast<struct sockaddr *>(&fSockAddr), sizeof(fSockAddr))
		!= 0) {
		perror("CliSock::Connect connect");
		return -1;
	}

	return fSocket;
}


int CliSock::Close()
{
	return close(fSocket);
}


int CliSock::Send( [[maybe_unused]] char* buf,  [[maybe_unused]] unsigned int len)
{
	return 0;
}


int CliSock::Receive(char* data_buf, unsigned int length, int& flag)
{
	unsigned int revd_size = 0;
	flag = 0;

	while(revd_size < length) {
		int nread = recv(fSocket, data_buf + revd_size, length - revd_size, 0);

		//if(nread == 0) break;
		if(nread <= 0) {
			flag = errno;
			perror("CliSock::Receive");
			break;
		}

		revd_size += nread;
	}

	return revd_size;
}


int CliSock::SetTimeOut_ms(int t_ms)
{
	fTimeOut.tv_sec  = t_ms / 1000;
	fTimeOut.tv_usec = (t_ms - ((t_ms / 1000) * 1000)) * 1000;

	std::cout << "#D " << t_ms << " " << fTimeOut.tv_sec << " " << fTimeOut.tv_usec << std::endl;

	return t_ms;
}

#if 0
HulStrTdcSampler::Event_Cycle(uint8_t* buffer)
{
  static const unsigned int sizeData = fnByte*fnWordPerCycle*sizeof(uint8_t);
  int ret = receive(fHulSocket, (char*)buffer, sizeData);
  if(ret < 0) return ret;

  return fnWordPerCycle;
}
#endif

#ifdef TEST_MAIN_CLISOCK
int main(int argc, char *argv[])
{
	int port = 24;
	std::string host("192.168.10.16");

	for (int i = 1 ; i < argc ; i++) {
		int val[4];
		if (sscanf(argv[i], "%d.%d.%d.%d", &(val[0]), &(val[1]), &(val[2]), &(val[3])) == 4) {
			host = std::string(argv[i]);
		}
		if (sscanf(argv[i], "%d", &(val[0])) == 1) {
			port =  val[0];
		}
	}

	std::cout << "host: " << host << " port: " << port << std::endl;


	CliSock s;

	s.SetTimeOut_ms(1000);
	s.SetTimeOut_ms(0);
	s.SetTimeOut_ms(1001);
	s.SetTimeOut_ms(100);

	int sock = s.Connect(host.c_str(), port);
	if (sock < 0) return 1;

	char *buf = new char[1024];
	int n = 100;
	int flag;
	for (int i = 0 ; i < 10 ; i++) {
		int nread = s.Receive(buf, n, flag);
		std::cout << "# " << nread << " : ";
		for (int j = 0 ; j < nread ; j++) {
			if ((j % 16) == 0) std::cout << std::endl;
			std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
				<< (static_cast<unsigned int>(buf[j]) & 0xff);
		}
		std::cout << std::endl;
	}

	return 0;
}
#endif
