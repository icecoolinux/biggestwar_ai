#ifndef _agent7_h_
#define _agent7_h_

#include <torch/script.h>

#include "../../ServerMessage.h"
#include "../../ClientMessage.h"
#include "../v6/inout.h"

#include "../../configRL.h"

#include "../../img.h"

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
		
		torch::jit::script::Module model;

		// Scalars, layers and minimap.
		torch::Tensor scalar_t;
		torch::Tensor layers_t;
		torch::Tensor minimap_t;
		std::vector<torch::jit::IValue> inputs;
		
		vec2 globalPos1, globalPos2;
		
		// Minimap.
		float minimapData[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4][3];
		
		// Pixels that mark how many time ago was showed
		float pixelsTimeUpdated[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4];
		
		// Auxiliar functions
		int getIndexBestAction(InOut::Action &action);
		int indexLocalFirst(InOut::Action &action);
		int indexLocalSecond(InOut::Action &action);
		int indexGlobalLayer(InOut::Action &action);
		
	public:
		Agent(char* username);
		~Agent();
		
		void init();
		
		void set(ServerMessage *msg);
		int step(ClientMessage *msg, int max);
};

#endif
