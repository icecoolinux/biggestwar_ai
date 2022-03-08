#ifndef _apigame_h_
#define _apigame_h_

#include "../../../server/src/config.h"
#include "net.h"

class ApiGame
{
	private:
		bool test;
		char urlServer[500];
		char user[LEN_NAME];
		char pass[LEN_PASS];
		Net* net;
		
	public:
		
		ApiGame();
		~ApiGame();
		
		void init(char* urlServer, bool test);
		int login_and_game(char* user, char* pass);
		ServerMessage* get();
		void send(ClientMessage* msg, int amount);
		bool isClosed();
		void close();
};


#endif
