
#include "semaphore.h"

bool Semaphore::freeFileNum[AMOUNT_FILES_SEM][AMOUNT_NUM_SEM];


union semun {
	int val;    /* Value for SETVAL */
	struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;  /* Array for GETALL, SETALL */
	struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
};

void Semaphore::init()
{
	for(int i=0; i<AMOUNT_FILES_SEM; i++)
		for(int j=0; j<AMOUNT_NUM_SEM; j++)
			freeFileNum[i][j] = true;
}

Semaphore::Semaphore()
{

}

Semaphore::Semaphore(int initValue)
{
	key_t key;
	char filename[100];
	
	for(int i=0; i<AMOUNT_FILES_SEM; i++)
	{
		sprintf(filename, "./semfiles/%d", i);
		for(int num=0; num<AMOUNT_NUM_SEM; num++)
		{
			key = ftok(filename, num);

			//Da -1 si ya esta creada o error
			id = semget(key, 1, 0666 | IPC_CREAT| IPC_EXCL);
			
			if(id >= 0)
			{
				this->fileKey = i;
				this->numKey = num;
				freeFileNum[i][num] = false;
				break;
			}
		}
		if(id >= 0)
			break;
	}

	operacionSemaforo(initValue);
}

Semaphore::~Semaphore()
{
	union semun temp;
	semctl(this->id, 0, IPC_RMID, temp);
	 
	freeFileNum[this->fileKey][this->numKey] = true;
}

void Semaphore::operacionSemaforo(int value)
{
	sembuf oper[1];
	oper[0].sem_num = 0;
	oper[0].sem_op = value;
	oper[0].sem_flg = 0;
	semop(this->id, oper, 1);
}

void Semaphore::P()
{
	operacionSemaforo(-1);
}

void Semaphore::V()
{
	operacionSemaforo(1);
}


