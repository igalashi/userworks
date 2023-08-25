/*
 *
 *
 */

#include <iostream>
#include <string>
#include <sstream>
#include <csignal>

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>

#if 0
#include "zmq.hpp"
#endif

#include "koltcp.h"
#include "recbe.h"

const int default_buf_size = 16 * 1024 * 1024;
//const char rotbar[] = {'-', '/', '|', '\\'};
const char rotbar[] = {'_', 'o', 'O', 'o'};
const char scanner[][6] = {
	"o----", "-o---", "--o--", "---o-",
	"----o", "---o-", "--o--", "-o---" };

static bool g_lose_trig = false;
static bool g_ext_trig = false;
static double g_freq = 100.0;

static bool g_got_sigpipe = false;
void sigpipe_handler(int signum)
{
        std::cerr << "Got SIGPIPE! " << signum << std::endl;
	g_got_sigpipe = true;
        return;
}

int set_signal()
{
        struct sigaction act;

	g_got_sigpipe = false;

        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = sigpipe_handler;
        act.sa_flags |= SA_RESTART;

        if(sigaction(SIGPIPE, &act, NULL) != 0 ) {
		std::cerr << "sigaction(2) error!" << std::endl;
                return -1;
        }

        return 0;
}

#if 0
int get_trig(zmq::socket_t &subsocket)
{
	static int trig_num = 0;
	//struct timeval now;
	int retval;

	if (g_ext_trig) {
		zmq::message_t msg;
		subsocket.recv(&msg);
		retval = *(reinterpret_cast<int *>(msg.data()));
	} else {
		retval = trig_num++;
		usleep(static_cast<int>(1000000.0 / g_freq));
	}

	if (g_lose_trig) {
		#if 0
		gettimeofday(&now, NULL);
		if ((now.tv_usec % 100) == 0) {
			retval = false;
		}
		#else
		if ((rand() % 100000) < 10) {
			retval = -1 * retval;
		}
		#endif
	}

	return retval;
}
#else
int get_trig()
{
	static int trig_num = 0;
	int retval = trig_num++;
	usleep(static_cast<int>(1000000.0 / g_freq));
	return retval;
}
#endif

static unsigned short int g_nsent = 0;
int gen_dummy(int id, int trig_num, char *buf, int buf_size)
{
	const int nsamples = 1;

	int data_len = 48 * sizeof(unsigned short) * 2 * nsamples;
	if (buf_size < static_cast<int>(sizeof(recbe_header)) + data_len) {
		std::cerr << "Too small buffer!!" << std::endl;
		return 0;
	}

	struct recbe_header *header;
	header = reinterpret_cast<struct recbe_header *>(buf);
	header->type = T_RAW_OLD;
	header->id = static_cast<unsigned char>(id & 0xff);
	header->sent_num = htons(g_nsent++);
	header->time = htons(static_cast<unsigned short>(time(NULL) & 0xffff));
	header->trig_count = htonl(trig_num);

	unsigned short *body;
	body = reinterpret_cast<unsigned short*>(buf + sizeof(recbe_header));

	for (int j = 0 ; j < nsamples ; j++) {
		for (int i = 0 ; i < 48 ; i++) {
			*(body++) = htons(static_cast<unsigned short>(i & 0xffff) + 0xa0);
		}
		for (int i = 0 ; i < 48 ; i++) {
			 *(body++) = htons(0x1000 | static_cast<unsigned short>(i & 0xffff));
		}
	}

	header->len = ntohs(static_cast<unsigned short int>(data_len & 0xffff));

	return data_len + sizeof(recbe_header);
}

int send_data(int id, int port)
{

	#if 0
	const char *zport = "tcp://localhost:8888";
	zmq::context_t context(1);
	zmq::socket_t subsocket(context, ZMQ_SUB);
	subsocket.connect(zport);
	subsocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
	#endif

	try {
		kol::SocketLibrary socklib;

		char *buf = new char[default_buf_size];
		g_nsent = 0;
		kol::TcpServer server(port);
		while(true) {
			kol::TcpSocket sock = server.accept();

			while (true) {
				try {
					if (sock.good()) {
						#if 0
						int trig_num = get_trig(subsocket);
						#else
						int trig_num = get_trig();
						#endif
						if (trig_num >= 0) {
							int data_size = gen_dummy(
								id, trig_num, buf, default_buf_size);

							sock.write(buf, data_size);

							if ((g_nsent % 100) == 0) {
								#if 0
								static int count = 0;
								std::cout << "\r"
								<< rotbar[count++ % 4]
								<< std::flush;
								//std::cout << "\r"
								//<< scanner[count++ % 8]
								//<< std::flush;
								#endif
								std::cout << "\r" << trig_num << "  ";
							}

							#if 0
							struct recbe_header *header;
							header = reinterpret_cast
								<struct recbe_header *>(buf);
							std::cout << "#M Trig: "
								<< ntohl(header->trig_count)
								<< std::endl;
							#endif

						} else {
							//struct recbe_header *header;
							//header = reinterpret_cast
							//	<struct recbe_header *>(buf);
							//header->sent_num
							//header->time
							//header->trig_count
							std::cout << "#M lost Trig: "
								//<< ntohl(header->trig_count)
								<< (-1 * trig_num)
								<< std::endl;
						}
					} else {
						std::cout << "sock.good : false" << std::endl;
						sock.close();
						g_nsent = 0;
						break;
					}
				} catch (kol::SocketException &e) {
					std::cout << "sock.write : " << e.what() << std::endl;
					sock.close();
					g_nsent = 0;
					break;
				}
			}
		}
	} catch(kol::SocketException &e) {
		std::cout << "Error " << e.what() << std::endl;
	}

	std::cout << "end of send_data" << std::endl;

	return 0;
}

int main(int argc, char *argv[])
{
	int port = 8024;
	int id = 0;

	for (int i = 0 ; i < argc ; i++) {
		std::string sargv(argv[i]);
		if (((sargv == "-p") || (sargv == "--port"))
			&& (argc > i)) {
			std::string param(argv[i + 1]);
			std::istringstream iss(param);
			iss >> port;
		}
		if (((sargv == "-i") || (sargv == "--id"))
			&& (argc > i)) {
			std::string param(argv[i + 1]);
			std::istringstream iss(param);
			iss >> id;
		}
		if (((sargv == "-f") || (sargv == "--freq"))
			&& (argc > i)) {
			std::string param(argv[i + 1]);
			std::istringstream iss(param);
			iss >> g_freq;
		}
		if ((sargv == "-d") || (sargv == "--drop")) {
			g_lose_trig = true;
		}
		if ((sargv == "-t") || (sargv == "--trig")) {
			g_ext_trig = true;
		}
	}
	std::cout << "ID: " << id << "  Port: " << port;
	if (g_lose_trig) std::cout << ", Trigger Drop ";
	if (g_ext_trig) std::cout << ", External Trigger";
	std::cout << std::endl;


	set_signal();
	send_data(id, port);

	return 0;
}
