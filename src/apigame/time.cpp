
#include "time.h"

Time::Time()
{
}

Time::~Time()
{
}

void Time::start()
{
	this->stopped = false;
	gettimeofday(&(this->t1), &(this->tz));
	this->ms = 0;
}

int Time::getMs()
{
	if(this->stopped)
		return this->ms;
	else 
	{
		struct timeval t2Temp;
		struct timezone tzTemp;
		gettimeofday(&t2Temp, &tzTemp);
		return (1000000*(t2Temp.tv_sec - this->t1.tv_sec) + t2Temp.tv_usec - this->t1.tv_usec)/1000;
	}
}

int Time::getMicro()
{
	if(this->stopped)
		return -1;
	else 
	{
		struct timeval t2Temp;
		struct timezone tzTemp;
		gettimeofday(&t2Temp, &tzTemp);
		return (1000000*(t2Temp.tv_sec - this->t1.tv_sec) + t2Temp.tv_usec - this->t1.tv_usec);
	}
}

int Time::stop()
{
	if(this->stopped)
		return this->ms;

	gettimeofday(&(t2), &(tz));
	ms = (1000000*(t2.tv_sec - t1.tv_sec) + t2.tv_usec - t1.tv_usec)/1000;
   
	this->stopped = true;

	return ms;
}

//Retorna los segundos actuales.
unsigned long int Time::currentSeconds()
{
	struct timeval t1;
	struct timezone tz;
	gettimeofday(&t1, &tz);
	return t1.tv_sec;
}

unsigned long long Time::currentMs()
{
	struct timeval t1;
	struct timezone tz;
	gettimeofday(&t1, &tz);
	return t1.tv_sec*1000 + t1.tv_usec/1000;
}

void Time::sleep(int ms, int seconds)
{
	if(ms < 0 || seconds < 0)
		return;
	
	//Duerme los segundos.
	for(int i=0; i<seconds; i++)
		sleep(1000, 0);

	struct timespec ts;
	struct timespec restante;

	//Seteo los segundos de espera.
	ts.tv_sec = ms / 1000;

	//Calculo los ms.
	ms %= 1000;

	//Seteo los ms.
	ts.tv_nsec = ms * 1E6;
	while(nanosleep(&ts, &restante) == -1){
		ts.tv_sec = restante.tv_sec;
		ts.tv_nsec = restante.tv_nsec;
	}
}
