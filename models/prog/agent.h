#ifndef _agent5_h_
#define _agent5_h_

#include "../../src/apigame/ServerMessage.h"
#include "../../src/apigame/ClientMessage.h"
#include "inout.h"

#include "../../../server/src/defs.h"

#include "../../src/configRL.h"

#include "../../src/apigame/img.h"

#define DEBUG true

class Agent
{
	private:
		char username[LEN_NAME];
		unsigned long long tsStart;
		bool thereIsCurrentPlayer;
		bool didFirstUpdate;
		
		InOut* inout;
		InOut::State* state;
		
		// Minimap.
		float minimapData[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4][3];
		
		
		unsigned long long getNearUnUsedRecollector(list<InOut::InfoObject> &recollects, vec2 pos);
		vec2 getPositionToBuild(vec2 pos, float ratio);
	
	public:
		Agent(char* username);
		~Agent();
		
		void init();
		
		void set(ServerMessage *msg);
		int step(ClientMessage* msgs, int max);
};

#endif
