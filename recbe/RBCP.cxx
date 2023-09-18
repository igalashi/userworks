/*
 *
 *
 */

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>


struct bcp_header{
        unsigned char type;
        unsigned char command;
        unsigned char id;
        unsigned char length;
        unsigned int address;
};

struct rbcp_header {
        unsigned char type;
        unsigned char command;
        unsigned char id;
        unsigned char length;
        unsigned int address;
};

#define RBCP_VER 0xFF
#define RBCP_CMD_WR 0x80
#define RBCP_CMD_RD 0xC0

const int gMaxContainerSize = 1472;
const int gBufSize = 2048;


class RBCP
{
public:
	RBCP() {};
	RBCP(const char *, int);
	virtual ~RBCP() {};
	int Open(const char *, int);
	int Close();
	int Send(char *, int);
	int Receive(char *);
	int Read(char *, unsigned int, int);
	int Write(char *, unsigned int, int);
	int GetSock() {return fSock;};
protected:
private:
	struct sockaddr_in fDestSockAddr;
	int fSequence = 0;
	int fSock = 0;
};


RBCP::RBCP(const char *hostname, int port)
{
	Open(hostname, port);
}


int RBCP::Open(const char *hostname, int port)
{
	struct addrinfo hints, *ai;
	int status;

	struct timeval timeout={3, 0};

	memset((char *)&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	status = getaddrinfo(hostname, NULL, &hints, &ai);
	if (status != 0) {
		fprintf(stderr, "Unknown host %s (%s)\n",
			hostname, gai_strerror(status));
		perror("RBCP::Open");
		return -1;
	}

	memset((char *)&fDestSockAddr, 0, sizeof(fDestSockAddr));
	memcpy(&fDestSockAddr, ai->ai_addr, ai->ai_addrlen);
	//fDestSockAddr.sin_addr.s_addr = ai.ai_addr.sin_addr.s_addr;
	//fDestSockAddr.sin_addr.s_addr = inet_addr(dest);
	fDestSockAddr.sin_port = htons(port);
	fDestSockAddr.sin_family = AF_INET;
	freeaddrinfo(ai);

	fSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (fSock < 0) {
		perror("RBCP::Open socket");
		return -1;
	}
	setsockopt(fSock, SOL_SOCKET, SO_RCVTIMEO,
		(char *)&timeout, sizeof(struct timeval));

	status = connect(fSock,
		(struct sockaddr *)&fDestSockAddr, sizeof(fDestSockAddr));
	if (status != 0) {
		perror("RBCP::Open connect");
		close(fSock);
		return -1;
	}

	return fSock;
}

int RBCP::Close()
{
	return close(fSock);
}

int RBCP::Send(char *buf, int len)
{
	int status;

	status = sendto(fSock, buf, len, 0,
		(struct sockaddr *)&fDestSockAddr, sizeof(fDestSockAddr));
	if (status != len) {
		perror("RBCP::Send sendto");
		return -1;
	}

	return 0;
}

int RBCP::Receive(char *buf)
{
	int status;
	//socklen_t fromlen;

	//status = *len = recvfrom(sock, buf, 1024, 0, NULL, &fromlen);
	status = recvfrom(fSock, buf, gBufSize, 0, NULL, NULL);
	
	if (status < 0) {
		perror("RBCP::Receive recvfrom (Timeout ?)");
		return -1;
	}
	return status;
}

#if 0
int updreg_wait_ans(int sock)
{
	return 0;
}
#endif

int RBCP::Read(char *buf, unsigned int addr, int len)
{
	struct bcp_header bcp;
	[[maybe_unused]] struct rbcp_header *rbcp;
	char *data;
	int status;
	int rlen = 0;

	bcp.type = RBCP_VER;
	bcp.command = RBCP_CMD_RD;
	bcp.address = htonl(addr);
	bcp.length = len;
	bcp.id = fSequence++;

	status = Send((char *)&bcp, sizeof(bcp));
	if (status < 0) printf("rbcp_read udp_send: error\n");
	rlen = Receive(buf);
	rbcp = (struct rbcp_header *)buf;
	data = buf + sizeof(struct rbcp_header);	

#ifdef DEBUG
	if (rlen > 0) {
		int i;
		printf("type: 0x%x, command: 0x%x, id: 0x%x, length: %d, address: %d",
			rbcp->type & 0xff, rbcp->command& 0xff,
			rbcp->id & 0xff, rbcp->length & 0xff,
			ntohl(rbcp->address & 0xffffffff));
	
		for (i = 0 ; i < (int)(rlen - sizeof(struct rbcp_header)) ; i++) {
			if ((i % 8) == 0) printf("\n%04x: ", addr + i);
			printf("%02x ", data[i] & 0xff);
		}
		printf("\n");
	}
#endif

	if (rlen > (int)(sizeof(struct rbcp_header))) {
		memmove(buf, data, rlen - sizeof(struct rbcp_header));
	} else {
		fprintf(stderr, "RBCP::Read: RBCP Read err.\n");
	}

	return rlen;
}

int RBCP::Write(char *data, unsigned int addr, int len)
{
	struct bcp_header *bcp;
	[[maybe_unused]] struct rbcp_header *rbcp;
	char *bcp_body;
	static char sbuf[gBufSize];
	static char rbuf[gBufSize];
	int status;
	int rlen = 0;

	bcp = (struct bcp_header *)sbuf;
	bcp_body = sbuf + sizeof(struct bcp_header);

	bcp->type = RBCP_VER;
	bcp->command = RBCP_CMD_WR;
	bcp->address = htonl(addr);
	//bcp->length = len + sizeof(struct bcp_header);
	bcp->length = len;
	bcp->id = fSequence++;

	if (len < gMaxContainerSize - static_cast<int>(sizeof(struct bcp_header))) {
		memcpy(bcp_body, data, len);
	} else {
		fprintf(stderr, "RBCP::Write: Buffer overflow %d\n", len);
		return -1;
	}

	status = Send((char *)&sbuf, sizeof(struct bcp_header) + len);
	if (status < 0) printf("RBCP::Write udp_send: error\n");
	rlen = Receive(rbuf);
	rbcp = (struct rbcp_header *)rbuf;
	data = rbuf + sizeof(struct rbcp_header);	

#ifdef DEBUG
	if (rlen > 0) {
		int i;
		printf("type: 0x%x, command: 0x%x, id: 0x%x, length: %d, address: %d",
			rbcp->type & 0xff, rbcp->command& 0xff,
			rbcp->id & 0xff, rbcp->length & 0xff,
			ntohl(rbcp->address & 0xffffffff));

		for (i = 0 ; i < (int)(rlen - sizeof(struct rbcp_header)) ; i++) {
			if ((i % 8) == 0) printf("\n%04x: ", addr + i);
			printf("%02x ", data[i] & 0xff);
		}
		printf("\n");
	}
#endif

	return rlen;
}


#ifdef TEST_MAIN

void helpline(char *cname)
{
	printf("%s <Commads>\n", cname);
	printf("Commands \n");
	printf("--host=<hotname>\n");
	printf("--port=<port nmber>\n");
	printf("--read=<address>:<number of reading>\n");
	printf("--write=<addres>:<data>\n");

	return;
}

int main(int argc, char* argv[])
{

	const char *default_host = "192.168.10.16";
	int default_port = 4660;
	int seq1 = 0;

	static char buf[1024];
	char hostname[256];
	int port;
	unsigned int raddress, rlen;
	unsigned int waddress;
	int wdata;
	int status;
	int i;

	strcpy(hostname, default_host);
	port = default_port;

	wdata = -1;
	rlen = 0;
	for (i = 1 ; i < argc ; i++) {
		if (sscanf(argv[i], "--host=%s", hostname) == 1) ;
		if (sscanf(argv[i], "--port=%d", &port) == 1) ;
		if (sscanf(argv[i], "--read=%x:%x", &raddress, &rlen) == 1);
		if (sscanf(argv[i], "--write=%x:%x", &waddress, &wdata) == 1);
		if (strncmp(argv[i], "--seq1", 6) == 0) seq1 = 1;
	}

	printf("host: %s\n", hostname);

	RBCP rbcp;

	rbcp.Open(hostname, port);

	
	if (rlen > 0) {
		printf("read: 0x%x : %d\n", raddress, rlen);
		rbcp.Read(buf, raddress, rlen);
	}
	if (wdata >= 0) {
		printf("write: 0x%x : %x\n", waddress, wdata);
		buf[0] = wdata & 0xff;
		buf[1] = 0;
		status = rbcp.Write(buf, waddress, 1);
		if (status <= 0) printf("Write Error %d\n", status);
	}


	if (seq1 == 1) {
		int j;
		for (j = 0 ; j < 128 ; j++) {
			for (i = 0 ; i < 128 ; i++) buf[i] = i;
			buf[0] = j;
			rbcp.Write(buf, 0 + 128*j, 128);
		}
	}


	rbcp.Close();

	return 0;
}

#endif
