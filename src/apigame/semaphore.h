#ifndef _semaphore_h_
#define _semaphore_h_

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/sem.h>

#define AMOUNT_FILES_SEM 124
#define AMOUNT_NUM_SEM 256

class Semaphore
{
	private:
		static bool freeFileNum[AMOUNT_FILES_SEM][AMOUNT_NUM_SEM];
		
		int id;
		int fileKey;
		int numKey;
		
		Semaphore();
		void operacionSemaforo(int value);
		
	public:

		static void init();
		
		Semaphore(int initValue);
		~Semaphore();
		void P();
		void V();
};

#endif
