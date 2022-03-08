#ifndef _net_h_
#define _net_h_

#include "time.h"
#include "vec2.h"
#include "semaphore.h"
#include "ServerMessage.h"
#include "ClientMessage.h"
#include "sock.h"
#include <list>

class Net
{
	private:
		char username[100];
		Sock* sock;
		
	public:
		Net(char* username_);
		~Net();
		
		bool connectToServer(string uri);
		bool isClosed();
		bool isConnected();
		
		void send(ClientMessage* clientMessage, int cant);
		ServerMessage* get(bool block, bool* exit);
		
		void closeConnection();
		
		
		static bool getString(const char* msg, char* str, int &pos);
		static bool getFloat(const char* msg, float &f, int &pos);
		static bool getInt(const char* msg, int &i, int &pos);
		static bool getLong(const char* msg, unsigned long long &l, int &pos);
};

#endif
