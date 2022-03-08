#include "api.h"

ApiGame::ApiGame()
{
	this->net = NULL;
}

ApiGame::~ApiGame()
{
}

void ApiGame::init(char* urlServer, bool test)
{
	strcpy(this->urlServer, urlServer);
	this->test = test;
	Semaphore::init();
	
	
}

int ApiGame::login_and_game(char* user, char* pass)
{
	bool salir = false;
	ServerMessage* serverMessage;
	ClientMessage clientMessage;
	
	strcpy(this->user, user);
	strcpy(this->pass, pass);
	
	this->net = new Net(user);
	if(!net->connectToServer(this->urlServer))
	{
		if(this->test)
			printf("Error 1 - %s\n", user);
		return -1;
	}
	
	// Login.
	// send: login,agent001,pass;
	clientMessage.login(user, pass);
	this->net->send(&clientMessage, 1);

	// Get if i'm in the server.
	// recive: login,0,0,false,,;
	serverMessage = this->net->get(true, &salir);

	if(serverMessage == NULL)
	{
		if(this->test)
			printf("Net error to login - %s\n", user);
		delete this->net;
		this->net = NULL;
		return -1;
	}
	
	if(!serverMessage->login)
	{
		// TODO if user not exists then register user.
		if(this->test)
			printf("There isn't register - %s\n", user);
		delete net;
		this->net = NULL;
		delete serverMessage;
		return -1;
	}
	
	// I'm not playing.
	if(!serverMessage->gameLogged_Login)
	{
		// Enter to play.
		// send play;
		clientMessage.play();
		
		this->net->send(&clientMessage, 1);

		// recive: newplayerworld,29,ws://127.0.0.1:8889,0,j91YyWEQJaCKT1OGKk6CB5vMKEWqY;
		delete serverMessage;
		serverMessage = this->net->get(true, &salir);

		// Error.
		if(!serverMessage->newPlayerWorld)
		{
			if(this->test)
				printf("Not game logged - %s\n", user);
			delete net;
			this->net = NULL;
			delete serverMessage;
			return -1;
		}
	}
	
	this->net->closeConnection();
	delete this->net;
	
	// Enter to world.
	this->net = new Net(user);
	bool ok = false;
	if(serverMessage->login)
	{
		printf("Connecting to (login): %s\n", serverMessage->uri_Login);
		ok = this->net->connectToServer(serverMessage->uri_Login);
	}
	else if(serverMessage->newPlayerWorld)
	{
		printf("%llu Connecting to (new player): %s\n", Time::currentMs(), serverMessage->uri_NewPlayer);
		ok = this->net->connectToServer(serverMessage->uri_NewPlayer);
	}
	if(!ok)
	{
		if(this->test)
			printf("Not enter to world %s\n", user);
		delete this->net;
		this->net = NULL;
		return -1;
	}

	// Login type.
	if(serverMessage->login)
		clientMessage.loginGame(user, serverMessage->token_Login);
	else if(serverMessage->newPlayerWorld)
		clientMessage.loginGame(user, serverMessage->token_NewPlayerWorld);
	this->net->send(&clientMessage, 1);
	
	delete serverMessage;
		
	return 0;
}

ServerMessage* ApiGame::get()
{
	return this->net->get(false, NULL);
}

void ApiGame::send(ClientMessage* msg, int amount)
{
	this->net->send(&msg[0], amount);
}

bool ApiGame::isClosed()
{
	return this->net->isClosed();
}

void ApiGame::close()
{
	this->net->closeConnection();
	
	delete this->net;
	this->net = NULL;

	if(this->test)
		printf("Goodbye - %s\n", this->user);
}






