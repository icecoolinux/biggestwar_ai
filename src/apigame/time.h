#ifndef _time_h_
#define _time_h_

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
	
class Time
{
	private:
		struct timeval t1, t2;
		struct timezone tz;
		bool stopped;
		int ms;
		
	public:
		Time();
		~Time();

		void start();

		int getMs();
		int getMicro();

		int stop();

		static unsigned long int currentSeconds();
		static unsigned long long currentMs();

		static void sleep(int ms, int seconds);
};

#endif
