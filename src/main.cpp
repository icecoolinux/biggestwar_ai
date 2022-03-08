#include <stdio.h>
#include <signal.h>

#include "../../server/src/config.h"
#include "apigame/net.h"
#include "apigame/api.h"

#ifdef PROG
	#include "../models/prog/agent.h"
#endif

/*
#ifdef IA
	#include "models/v6/agent.h"
#endif
*/

#ifdef IATORCH
	#include "../models/ai/agent.h"
#endif

#include "configRL.h"

#define MAX_MSGS 5

#define TEST true

void test1();
void test2();


bool salir = false;

void handler(int s)
{
	printf("Exiting...\n");
	salir = true;
}

int main(int argc, char** argv)
{
	/*
	test1();
	test2();
	*/
	
	
	if(argc != 3)
	{
		printf("Use: agent <user> <pass>\n");
		return -1;
	}
	
	
	ClientMessage clientMessage[MAX_MSGS];
	
	// Get user and password.
	char user[LEN_NAME];
	char pass[LEN_PASS];
	strcpy(user, argv[1]);
	strcpy(pass, argv[2]);
	
	// Set Ctrl-C signal.
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
	sigaction(SIGTERM, &sigIntHandler, NULL);
	sigaction(SIGQUIT, &sigIntHandler, NULL);
	
	// Connect to the game
	ApiGame* api = new ApiGame();
	api->init("ws://127.0.0.1:8888", TEST);
	if(api->login_and_game(user, pass) < 0)
		return -1;
	
	// Make agent.
	Agent* agent = new Agent(user);
	agent->init();
	
	// Loop agent.
	unsigned long long timeStepAgent = 0;
	while(!salir)
	{
		// Receive all server package.
		while(!salir)
		{
			ServerMessage* serverMessage = api->get();

			if(serverMessage == NULL)
			{
				if(api->isClosed())
					salir = true;
				break;
			}
			else
			{
				agent->set(serverMessage);
				
				salir = serverMessage->endGame;
				delete serverMessage;
			}
		}

		if(salir)
			break;
		
		if( (Time::currentMs()-timeStepAgent) >= MS_AGENT_STEP)
		{
			int cant = agent->step(&clientMessage[0], MAX_MSGS);
			if(cant > 0)
				api->send(&clientMessage[0], cant);
			timeStepAgent = Time::currentMs();
		}
		
		Time::sleep(MS_SLEEP_LOOP, 0);
	}

	// Close.
	api->close();
	delete api;
	
	return 0;
}

/*
// Test agent's message to server
void test1()
{
	Agent* agent = new Agent();
	agent->init();
	
	printf("Set newplayercurrent,icecool,0,1409,8984;\n");
	ServerMessage* srv = new ServerMessage();
	srv->set("newplayercurrent,icecool,0,1409,8984;", strlen("newplayercurrent,icecool,0,1409,8984;"));
	agent->set(srv);
	printf("Done\n\n");
	
	printf("Set update,1259,1559,8808,9161;\n");
	srv->set("update,1259,1559,8808,9161;", strlen("update,1259,1559,8808,9161;"));
	agent->set(srv);
	printf("Done\n\n");
	
	printf("Set newplayer,icecool,0;resources,1500,0;\n");
	srv->set("newplayer,icecool,0;resources,1500,0;", strlen("newplayer,icecool,0;resources,1500,0;"));
	agent->set(srv);
	printf("Done\n\n");
	
	printf("Set new,27017,player,icecool,type,2,pos,1378.114,9014.333,life,500,fulllife,500,creada,500,construccioncreando,0,collected,0,amount,0,;\n");
	srv->set("new,27017,player,icecool,type,2,pos,1378.114,9014.333,life,500,fulllife,500,creada,500,construccioncreando,0,collected,0,amount,0,;", strlen("new,27017,player,icecool,type,2,pos,1378.114,9014.333,life,500,fulllife,500,creada,500,construccioncreando,0,collected,0,amount,0,;"));
	agent->set(srv);
	printf("Done\n\n");
	
	ClientMessage* cli = new ClientMessage();
	for(int i=0; i<5; i++)
	{
		if(agent->step(cli))
		{
			printf("yes\n");
			printf("%s\n", cli->get().c_str());
		}
	}
	
	delete agent;
}

// Test SOFM pos
void test2()
{
	InOut* inout = new InOut();
	
	float x = 800;
	float y = 700;
	float x2, y2;
	
	float sofm[NEURONS_SOFM_MAP];
	inout->posToSOFM(x, y, sofm);
	inout->SOFMToPos(sofm, x2, y2);
	printf("%f %f\n", x2, y2);
	
	delete inout;
}

*/















