/*
 *
 *
 */

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

class uStopWatch {
	public:
		#if 0
		uStopWatch();
		virtual ~uStopWatch();
		#endif
		int start();
		uint64_t period();
		uint64_t elapse();
		void restart();
	protected:
	private:
		struct timeval at_start;
		struct timeval now;
};

#if 0
uStopWatch::uStopWatch()
{
	return;
}

uStopWatch::~uStopWatch()
{
	std::cout << "# StopWatch destructed" << std::endl;
	return;
}
#endif

int uStopWatch::start()
{
	int ret = gettimeofday(&at_start, NULL);
	if (ret != 0) perror("StopWatch::start");
	
	return ret;
}

uint64_t uStopWatch::elapse()
{
	int ret = gettimeofday(&now, NULL);
	if (ret != 0) {
		perror("StopWatch::stop");
		return ret;
	}
	
	return period();
}

uint64_t uStopWatch::period()
{
	uint64_t dsec = now.tv_sec - at_start.tv_sec;
	uint64_t dusec = now.tv_usec - at_start.tv_usec;
	uint64_t diff = dsec * 1000 * 1000 + dusec;
	return diff;
}

void uStopWatch::restart()
{
	at_start.tv_sec = now.tv_sec;
	at_start.tv_usec = now.tv_usec;
	
	return;
}


#ifdef TEST_MAIN
int main(int argc, char *argv[])
{
	uStopWatch sw;
	for (int i = 0 ; i < 10 ; i++) {
		sw.start();
		usleep(i*10000);
		int elapse = sw.elapse();
		std::cout << "elapse : " << elapse << std::endl;;
	}

	return 0;
}
#endif

