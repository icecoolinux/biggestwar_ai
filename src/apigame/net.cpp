
#include "net.h"

Net::Net(char* username_)
{
	strcpy(username, username_);
	this->sock = new Sock();
}

Net::~Net()
{
	if(!this->sock->isClosed())
		this->sock->close();
	delete this->sock;
}

bool Net::connectToServer(string uri)
{
	return this->sock->connectToServer(uri);
}

bool Net::isConnected()
{
	return this->sock->isConnected();
}

bool Net::isClosed()
{
	return this->sock->isClosed();
}

void Net::closeConnection()
{
	this->sock->close();
}



void Net::send(ClientMessage* clientMessage, int cant)
{
	std::string msg = "";
	for(int i=0; i<cant; i++)
	{
		std::string m = clientMessage[i].get();
		msg += m;
	}
printf("send %s\n", msg.c_str());
	this->sock->send(msg);
}

ServerMessage* Net::get(bool block, bool* exit)
{
	string msgS;
	
	// Wait until there be a message or exit if is not block.
	while(true)
	{
		bool thereis = sock->getMessage(msgS);
		
		if(!thereis && !block)
			return NULL;
		if(thereis)
			break;
		if(!thereis && block && *exit)
			return NULL;
		if(sock->isClosed())
			return NULL;
		
		Time::sleep(10, 0);
	}
	
	int len = msgS.size();
	
	if(msgS[len-1] == '\0')
		len--;
	
	char* msgC = new char[len];
	for(int i=0; i<len; i++)
		msgC[i] = msgS.c_str()[i];

	ServerMessage* msg = new ServerMessage();
	int pos = msg->set(msgC, len);
	
	// Put to list rest of message.
	if(pos < len)
	{
		msgS = string(&msgC[pos], len-pos);
		this->sock->setMessageFront(msgS);
	}

	delete[] msgC;
	
	return msg;
}




// Extract string until , or ;
bool Net::getString(const char* msg, char* str, int &pos)
{
	int pos_ = pos;
	int strPos = 0;
	
	while( msg[pos_] != ',' && msg[pos_] != ';' && msg[pos_] != '\0' )
	{
		str[strPos] = msg[pos_];
		pos_++;
		strPos++;
	}
	str[strPos] = '\0';
	
	if(msg[pos_] == '\0')
		return false;
	
	pos = pos_;
	pos++;
	
	return true;
}

// Extract float until , or ;
bool Net::getFloat(const char* msg, float &f, int &pos)
{
	int pos_ = pos;
	
	char str[100];
	int strPos = 0;
	
	while( msg[pos_] != ',' && msg[pos_] != ';' && msg[pos_] != '\0' )
	{
		str[strPos] = msg[pos_];
		pos_++;
		strPos++;
	}
	str[strPos] = '\0';
	
	if(msg[pos_] == '\0')
		return false;
	
	f = atof(str);
	
	pos = pos_;
	pos++;
	
	return true;
}

// Extract int until , or ;
bool Net::getInt(const char* msg, int &i, int &pos)
{
	int pos_ = pos;
	
	char str[100];
	int strPos = 0;
	
	while( msg[pos_] != ',' && msg[pos_] != ';' && msg[pos_] != '\0' )
	{
		str[strPos] = msg[pos_];
		pos_++;
		strPos++;
	}
	str[strPos] = '\0';
	
	if(msg[pos_] == '\0')
		return false;
	
	i = atoi(str);
	
	pos = pos_;
	pos++;
	
	return true;
}

// Extract long until , or ;
bool Net::getLong(const char* msg, unsigned long long &l, int &pos)
{
	int pos_ = pos;
	
	char str[100];
	int strPos = 0;
	
	while( msg[pos_] != ',' && msg[pos_] != ';' && msg[pos_] != '\0' )
	{
		str[strPos] = msg[pos_];
		pos_++;
		strPos++;
	}
	str[strPos] = '\0';
	
	if(msg[pos_] == '\0')
		return false;
	
	l = strtoull(str, NULL, 10);
	
	pos = pos_;
	pos++;
	
	return true;
}














