#ifndef _agent7_h_
#define _agent7_h_

#include "../../src/apigame/ServerMessage.h"
#include "../../src/apigame/ClientMessage.h"
#include "../../src/apigame/inout.h"

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
		
		bool startSteps;
		bool thereIsMiniMap;

		InOut* inout;
		InOut::State* state;
		InOut::LocalState* localState;
		
		vec2 globalPos1, globalPos2;
		
		// Minimap.
		float minimapData[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4][3];
		
		// Pixels that mark how many time ago was showed
		float pixelsTimeUpdated[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4];
		
		// For send state to the model
		int fd_write;
		char jsonScalarsOut[5000];
		float localLayersOut[CANT_LOCAL_LAYERS * SIDE_LAYERS * SIDE_LAYERS];
		float globalLayersOut[CANT_MINIMAPS * SIDE_MINIMAP * SIDE_MINIMAP];
		float cellState[2*1*64];
		float hiddenState[2*1*64];
		void sendStep(InOut::LocalState *local);
		
		// For recive the action
		int fd_read;
		float actionsOutput[9];
		float unitOutput[8];
		float localOutput[11 * SIDE_LAYERS * SIDE_LAYERS];
		float globalOutput[(CANT_ACTIONS-1)*2 * SIDE_MINIMAP * SIDE_MINIMAP];
		void getAction(InOut::Action &action, vec2 &nextGlobal1, vec2 &nextGlobal2);
		
		// Auxiliar functions
		int getIndexBestAction(InOut::Action &action);
		int indexLocalFirst(InOut::Action &action);
		int indexLocalSecond(InOut::Action &action);
		int indexGlobalLayer(InOut::Action &action); 
		void writeAll(char* data, int count);
		void readAll(char* data, int count);
		
	public:
		Agent(char* username);
		~Agent();
		
		void init();
		
		void set(ServerMessage *msg);
		int step(ClientMessage *msg, int max);
};

#endif
