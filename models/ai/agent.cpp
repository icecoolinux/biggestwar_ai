
#include "agent.h"

Agent::Agent(char* username_)
{
	strcpy(this->username, username_);
	this->thereIsCurrentPlayer = false;
	this->didFirstUpdate = false;
	
	this->startSteps = false;
	this->thereIsMiniMap = false;
	
	this->inout = new InOut();
	this->state = new InOut::State;
	this->localState = new InOut::LocalState;
	this->inout->initState(*state);
	
	// Init seen pixels to start game.
	for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
		for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
			this->pixelsTimeUpdated[i][j] = 0.0f;
}

Agent::~Agent()
{
	delete this->inout;
	delete this->state;
	delete this->localState;
}

void Agent::init()
{
	// Init neural network state
	for(int i=0; i<2*1*64; i++)
	{
		cellState[i] = 0.0f;
		hiddenState[i] = 0.0f;
	}
	
	// Open files to comunicate with the model
	char filenameRead[1000];
	char filenameWrite[1000];
	for(int i=0; i<1000; i++)
	{
		sprintf(filenameRead, "/tmp/fifo_bw_agent_tocpp_%d", i);
		sprintf(filenameWrite, "/tmp/fifo_bw_agent_topython_%d", i);
		if(mkfifo(filenameWrite, 0666) == 0)
		{
			printf("Using Fifo %d\n", i);
			mkfifo(filenameRead, 0666);
			fd_write = open(filenameWrite, O_WRONLY);
			fd_read = open(filenameRead, O_RDONLY);
			break;
		}
	}
	
	this->tsStart = Time::currentMs();
}

void Agent::set(ServerMessage *msg)
{
	// There's minimap, save it.
	if(msg->miniMap)
	{
		float array[512*512*3];
		int width, height;
		datapng_to_array32F(msg->pngData_MiniMap, msg->lenPngData, array, width, height);
		for(int y=0; y<SIDE_MINIMAP_CONV_V4; y++)
			for(int x=0; x<SIDE_MINIMAP_CONV_V4; x++)
				for(int k=0; k<3; k++)
					this->minimapData[y][x][k] = array[y*SIDE_MINIMAP_CONV_V3*3 + x*3 +k];
		//resize_img_array(array, width, height, this->minimapData, SIDE_MINIMAP_CONV_V4, SIDE_MINIMAP_CONV_V4);

		if(!thereIsMiniMap)
		{
			for(int y=0; y<SIDE_MINIMAP_CONV_V4; y++)
				for(int x=0; x<SIDE_MINIMAP_CONV_V4; x++)
					for(int k=0; k<3; k++)
						if(array[y*SIDE_MINIMAP_CONV_V3*3 + x*3 +k] > 0.01f)
							thereIsMiniMap = true;
		}
	}
	
	// Init SelectAction.
	if(msg->newPlayerCurrent)
	{
		this->globalPos1.x = msg->xView_NewPlayerCurrent;
		this->globalPos1.y = msg->yView_NewPlayerCurrent;
		this->globalPos2.x = msg->xView_NewPlayerCurrent;
		this->globalPos2.y = msg->yView_NewPlayerCurrent;
		
		this->state->view.x = msg->xView_NewPlayerCurrent;
		this->state->view.y = msg->yView_NewPlayerCurrent;
		
		thereIsCurrentPlayer = true;
		
if(DEBUG)
{
	printf("Init Global select: %f, %f : %f, %f\n", this->globalPos1.x, this->globalPos1.y, this->globalPos2.x, this->globalPos2.y);
}
	}
	
	// Update state.
	inout->put(Time::currentMs()-this->tsStart, msg, *state, this->pixelsTimeUpdated);
	
	if(thereIsMiniMap && thereIsCurrentPlayer)
		startSteps = true;
}

int Agent::step(ClientMessage *msg, int max)
{
	if(!startSteps)
		return false;
	
if(DEBUG)
	printf("STEP\n");

	/// Get Global Select.

if(DEBUG)
{
printf("Global select before: %f, %f : %f, %f\n", this->globalPos1.x, this->globalPos1.y, this->globalPos2.x, this->globalPos2.y);
}

	// Get Local State.
	inout->getLocalState(*state, this->minimapData, this->pixelsTimeUpdated, this->globalPos1, this->globalPos2, *localState);
	inout->standardize(*localState);
	
	/// Send state, get an action and get internal state.
	sendStep(localState);
	InOut::Action action;
	vec2 nextGlobal1, nextGlobal2;
	getAction(action, nextGlobal1, nextGlobal2);
	

if(DEBUG)
{
printf("Global select after: %f, %f : %f, %f\n", nextGlobal1.x, nextGlobal1.y, nextGlobal2.x, nextGlobal2.y);
}

	bool ret = inout->get(*state, action, &msg[0]);
	
	// Set pos global for the next action
	this->globalPos1 = nextGlobal1;
	this->globalPos2 = nextGlobal2;
	
printf("AA %s\n", msg);
	// When I do update I have to set state->view.
	if(ret && msg[0].update_)
	{
		this->state->view.x = (msg[0].xMin_Update + msg[0].xMax_Update)/2.0f;
		this->state->view.y = (msg[0].yMin_Update + msg[0].yMax_Update)/2.0f;
	}
	
if(DEBUG)
	printf("END STEP\n\n");

	if(ret)
		return 1;
	else
		return 0;
}


void Agent::sendStep(InOut::LocalState *local)
{
	char buf[5000];
	
	sprintf(jsonScalarsOut, "{\"timestamp\": %f, \"local\":{\"minerals\":%f, \"oils\":%f}, \"isClosingArea\": %f, \"msToStartToCloseArea\": %f}", 
						local->timestamp, local->minerals, local->oils, local->isClosingArea, local->msToStartToCloseArea);
	InOut::serializeLocalState(*local, localLayersOut, globalLayersOut);
	
	
	// jsonScalar
	sprintf(buf, "%s\n", jsonScalarsOut);
	writeAll( buf, strlen(buf)+1);
	
	// Local size
	sprintf(buf, "%d\n", sizeof(float)*CANT_LOCAL_LAYERS*SIDE_LAYERS*SIDE_LAYERS);
	writeAll( buf, strlen(buf)+1);
	
	// Global size
	sprintf(buf, "%d\n", sizeof(float)*CANT_MINIMAPS*SIDE_MINIMAP*SIDE_MINIMAP);
	writeAll( buf, strlen(buf)+1);
	
	// Cell state size
	sprintf(buf, "%d\n", sizeof(float)*2*1*64);
	writeAll( buf, strlen(buf)+1);
	
	// Hidden state size
	sprintf(buf, "%d\n", sizeof(float)*2*1*64);
	writeAll( buf, strlen(buf)+1);
	
	// Local 
	writeAll( (char*)localLayersOut, sizeof(float)*CANT_LOCAL_LAYERS*SIDE_LAYERS*SIDE_LAYERS);
	
	// Global
	writeAll( (char*)globalLayersOut, sizeof(float)*CANT_MINIMAPS*SIDE_MINIMAP*SIDE_MINIMAP);
	
	// Cell state
	writeAll( (char*)cellState, sizeof(float)*2*1*64);
	
	// Hidden state
	writeAll( (char*)hiddenState, sizeof(float)*2*1*64);
}


int aaaa=0;
void Agent::getAction(InOut::Action &action, vec2 &nextGlobal1, vec2 &nextGlobal2)
{
printf("a\n");
	// Receive
	readAll( (char*)actionsOutput, sizeof(float)*9);
printf("a1\n");
	readAll( (char*)unitOutput, sizeof(float)*8);
printf("a2\n");
	readAll( (char*)localOutput, sizeof(float)* 11 * SIDE_LAYERS * SIDE_LAYERS);
printf("a3\n");
	readAll( (char*)globalOutput, sizeof(float)* (CANT_ACTIONS-1)*2 * SIDE_MINIMAP * SIDE_MINIMAP);
printf("a4\n");
	// Read hidden state
	readAll( (char*)cellState, sizeof(float)*2*1*64);
printf("a5\n");
	readAll( (char*)hiddenState, sizeof(float)*2*1*64);
printf("a6\n");
/*
printf("\n");
for(int i=0; i<9; i++)
	printf("Action %d: %f\n", i, actionsOutput[i]);
for(int i=0; i<8; i++)
	printf("Unit %d: %f\n", i, unitOutput[i]);
*/
	action.noAction = false;
	action.surrender = false;
	action.update = false;
	action.move = false;
	action.recollect = false;
	action.buildBuilding = false;
	action.buildUnit = false;
	action.attack = false;
	action.cancelAction = false;
	
	float randAction = float(rand())/float((RAND_MAX));
	float accumAction = 0.0f;
	for(int i=0; i<9; i++)
	{
		if(randAction < (accumAction+actionsOutput[i]))
		{
			if(i==0)
				action.noAction = true;
			else if(i==1)
				action.surrender = true;
			else if(i==2)
				action.update = true;
			else if(i==3)
				action.move = true;
			else if(i==4)
				action.recollect = true;
			else if(i==5)
				action.buildBuilding = true;
			else if(i==6)
				action.buildUnit = true;
			else if(i==7)
				action.attack = true;
			else if(i==8)
				action.cancelAction = true;
			break;
		}
		else
			accumAction += actionsOutput[i];
	}
	
	/*
	// No action.
	if(actionsOutput[0] > actionsOutput[1] && actionsOutput[0] > actionsOutput[2] && actionsOutput[0] > actionsOutput[3] && 
		actionsOutput[0] > actionsOutput[4] && actionsOutput[0] > actionsOutput[5] && actionsOutput[0] > actionsOutput[6] && 
		actionsOutput[0] > actionsOutput[7] && actionsOutput[0] > actionsOutput[8])
		action.noAction = true;
	// Surrender
	else if(actionsOutput[1] > actionsOutput[0] && actionsOutput[1] > actionsOutput[2] && actionsOutput[1] > actionsOutput[3] && 
		actionsOutput[1] > actionsOutput[4] && actionsOutput[1] > actionsOutput[5] && actionsOutput[1] > actionsOutput[6] && 
		actionsOutput[1] > actionsOutput[7] && actionsOutput[1] > actionsOutput[8])
		action.surrender = true;
	// Update
	else if(actionsOutput[2] > actionsOutput[0] && actionsOutput[2] > actionsOutput[1] && actionsOutput[2] > actionsOutput[3] && 
		actionsOutput[2] > actionsOutput[4] && actionsOutput[2] > actionsOutput[5] && actionsOutput[2] > actionsOutput[6] && 
		actionsOutput[2] > actionsOutput[7] && actionsOutput[2] > actionsOutput[8])
		action.update = true;
	// Move
	else if(actionsOutput[3] > actionsOutput[0] && actionsOutput[3] > actionsOutput[1] && actionsOutput[3] > actionsOutput[2] && 
		actionsOutput[3] > actionsOutput[4] && actionsOutput[3] > actionsOutput[5] && actionsOutput[3] > actionsOutput[6] && 
		actionsOutput[3] > actionsOutput[7] && actionsOutput[3] > actionsOutput[8])
		action.move = true;
	// Recollect
	else if(actionsOutput[4] > actionsOutput[0] && actionsOutput[4] > actionsOutput[1] && actionsOutput[4] > actionsOutput[2] && 
		actionsOutput[4] > actionsOutput[3] && actionsOutput[4] > actionsOutput[5] && actionsOutput[4] > actionsOutput[6] && 
		actionsOutput[4] > actionsOutput[7] && actionsOutput[4] > actionsOutput[8])
		action.recollect = true;
	// BuildBuilding
	else if(actionsOutput[5] > actionsOutput[0] && actionsOutput[5] > actionsOutput[1] && actionsOutput[5] > actionsOutput[2] && 
		actionsOutput[5] > actionsOutput[3] && actionsOutput[5] > actionsOutput[4] && actionsOutput[5] > actionsOutput[6] && 
		actionsOutput[5] > actionsOutput[7] && actionsOutput[5] > actionsOutput[8])
		action.buildBuilding = true;
	// BuildUnit
	else if(actionsOutput[6] > actionsOutput[0] && actionsOutput[6] > actionsOutput[1] && actionsOutput[6] > actionsOutput[2] && 
		actionsOutput[6] > actionsOutput[3] && actionsOutput[6] > actionsOutput[4] && actionsOutput[6] > actionsOutput[5] && 
		actionsOutput[6] > actionsOutput[7] && actionsOutput[6] > actionsOutput[8])
		action.buildUnit = true;
	// Attack
	else if(actionsOutput[7] > actionsOutput[0] && actionsOutput[7] > actionsOutput[1] && actionsOutput[7] > actionsOutput[2] && 
		actionsOutput[7] > actionsOutput[3] && actionsOutput[7] > actionsOutput[4] && actionsOutput[7] > actionsOutput[5] && 
		actionsOutput[7] > actionsOutput[6] && actionsOutput[7] > actionsOutput[8])
		action.attack = true;
	// CancelAction
	else if(actionsOutput[8] > actionsOutput[0] && actionsOutput[8] > actionsOutput[1] && actionsOutput[8] > actionsOutput[2] && 
		actionsOutput[8] > actionsOutput[3] && actionsOutput[8] > actionsOutput[4] && actionsOutput[8] > actionsOutput[5] && 
		actionsOutput[8] > actionsOutput[6] && actionsOutput[8] > actionsOutput[7])
		action.cancelAction = true;
	else
		action.noAction = true;
	*/
	
	// Units
	for(int i=0; i<8; i++)
		action.units[i] = unitOutput[i];

	// Local
	int indexLocal1 = indexLocalFirst(action);
	int indexLocal2 = indexLocalSecond(action);
	float maxValueLocal1 = -99999;
	float maxValueLocal2 = -99999;
	for(int i=0; i<4; i++)
		action.local[i] = 0.0f;
	if(indexLocal1 >= 0 && indexLocal2 >= 0)
	{
		int pos1 = indexLocal1 * INFO_LAYER_SIDE_PIXELS * INFO_LAYER_SIDE_PIXELS;
		int pos2 = indexLocal2 * INFO_LAYER_SIDE_PIXELS * INFO_LAYER_SIDE_PIXELS;
		
		for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
		{
			for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
			{
				if(indexLocal1 >= 0)
				{
					if(localOutput[pos1] > maxValueLocal1)
					{
						maxValueLocal1 = localOutput[pos1];
						action.local[0] = j;
						action.local[1] = i;
					}
				}
				
				if(indexLocal2 >= 0)
				{
					if(localOutput[pos2] > maxValueLocal2)
					{
						maxValueLocal2 = localOutput[pos2];
						action.local[2] = j;
						action.local[3] = i;
					}
				}
				
				pos1++;
				pos2++;
			}
		}
		for(int i=0; i<4; i++)
			action.local[i] = (action.local[i] / ((float)INFO_LAYER_SIDE_PIXELS)) * ((float)INFO_LAYER_SIDE_PIXELS*INFO_LAYER_METERS_BY_PIXELS);
	}
/*
for(int i=0; i<4; i++)
	printf("Local %d: %f\n", i, action.local[i]);
*/








	// Extract next global select.
	int indexGlobal = indexGlobalLayer(action);
	float maxGlobal1 = -9999;
	float maxGlobal2 = -9999;
	int posGlobal1 = 2*indexGlobal * SIDE_MINIMAP_CONV_V4 * SIDE_MINIMAP_CONV_V4;
	int posGlobal2 = (2*indexGlobal +1) * SIDE_MINIMAP_CONV_V4 * SIDE_MINIMAP_CONV_V4;
	/*
	// Select next global with probability
	float amountGlobal1 = 0.0f;
	float amountGlobal2 = 0.0f;
	for(int i=0; i<SIDE_MINIMAP_CONV_V4 * SIDE_MINIMAP_CONV_V4; i++)
	{
		amountGlobal1 += globalOutput[posGlobal1 + i];
		amountGlobal2 += globalOutput[posGlobal2 + i];
	}
	float randGlobal1 = float(rand())/float((RAND_MAX));
	float randGlobal2 = float(rand())/float((RAND_MAX));
	randGlobal1 *= amountGlobal1;
	randGlobal2 *= amountGlobal2;
	float accumGlobal1 = 0.0f;
	float accumGlobal2 = 0.0f;
	bool finish = false;
	for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
	{
		for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
		{
			if(randGlobal1 < (accumGlobal1 + globalOutput[posGlobal1]) )
			{
				nextGlobal1.x = j;
				nextGlobal1.y = i;
				finish = true;
				break;
			}
			else
				accumGlobal1 += globalOutput[posGlobal1];
			posGlobal1++;
		}
		if(finish)
			break;
	}
	finish = false;
	for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
	{
		for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
		{
			if(randGlobal2 < (accumGlobal2 + globalOutput[posGlobal2]) )
			{
				nextGlobal2.x = j;
				nextGlobal2.y = i;
				finish = true;
				break;
			}
			else
				accumGlobal2 += globalOutput[posGlobal2];
			posGlobal2++;
		}
		if(finish)
			break;
	}
	*/
	// Select next global the best
/*
char buf2[1000];
sprintf(buf2, "%s_%d_1.txt", username, aaaa);
FILE* f1 = fopen(buf2, "w");
sprintf(buf2, "%s_%d_2.txt", username, aaaa);
FILE* f2 = fopen(buf2, "w");
aaaa++;
*/
	for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
	{
		for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
		{
/*
sprintf(buf2, "%d,%d,%f\n", i,j,globalOutput[posGlobal1]);
fputs(buf2, f1);
sprintf(buf2, "%d,%d,%f\n", i,j,globalOutput[posGlobal2]);
fputs(buf2, f2);
*/
			if(globalOutput[posGlobal1] > maxGlobal1)
			{
				maxGlobal1 = globalOutput[posGlobal1];
				nextGlobal1.x = j;
				nextGlobal1.y = i;
			}
			if(globalOutput[posGlobal2] > maxGlobal2)
			{
				maxGlobal2 = globalOutput[posGlobal2];
				nextGlobal2.x = j;
				nextGlobal2.y = i;
			}
			posGlobal1++;
			posGlobal2++;
		}
	}
/*
fclose(f1);
fclose(f2);
*/
	nextGlobal1.x = (nextGlobal1.x / ((float)SIDE_MINIMAP_CONV_V4)) * ((float)SIDE_MAP_METERS);
	nextGlobal1.y = (nextGlobal1.y / ((float)SIDE_MINIMAP_CONV_V4)) * ((float)SIDE_MAP_METERS);
	nextGlobal2.x = (nextGlobal2.x / ((float)SIDE_MINIMAP_CONV_V4)) * ((float)SIDE_MAP_METERS);
	nextGlobal2.y = (nextGlobal2.y / ((float)SIDE_MINIMAP_CONV_V4)) * ((float)SIDE_MAP_METERS);
	
	
//printf("AAA %f %f, %f %f\n", this->globalPos1.x, this->globalPos1.y, this->globalPos2.x, this->globalPos2.y);

	action.global[0] = this->globalPos1.x;
	action.global[1] = this->globalPos1.y;
	action.global[2] = this->globalPos2.x;
	action.global[3] = this->globalPos2.y;
/*
for(int i=0; i<4; i++)
	printf("Global %d: %f\n", i, action.nextGlobal[i]);
*/
}

int Agent::getIndexBestAction(InOut::Action &action)
{
	if(action.noAction)
		return 0;
	else if(action.surrender)
		return 1;
	if(action.update)
		return 2;
	if(action.move)
		return 3;
	if(action.recollect)
		return 4;
	if(action.buildBuilding)
		return 5;
	if(action.buildUnit)
		return 6;
	if(action.attack)
		return 7;
	if(action.cancelAction)
		return 8;
	else
		return -1;
}

int Agent::indexLocalFirst(InOut::Action &action)
{
    if(action.update)
        return 0;
    else if(action.move)
        return 1;
    else if(action.recollect)
        return 3;
    else if (action.buildUnit)
        return 5;
    else if(action.buildBuilding)
        return 6;
    else if(action.attack)
        return 8;
    else if(action.cancelAction)
        return 10;
    else
        return -1;
}

int Agent::indexLocalSecond(InOut::Action &action)
{
	if(action.move)
        return 2;
    else if(action.recollect)
        return 4;
    else if(action.buildBuilding)
        return 7;
    else if(action.attack)
        return 9;
    else
        return -1;
}

int Agent::indexGlobalLayer(InOut::Action &action)
{
	if(action.noAction)
        return 0;
    else if(action.update)
        return 1;
    else if(action.move)
        return 2;
    else if(action.recollect)
        return 3;
    else if (action.buildUnit)
        return 4;
    else if(action.buildBuilding)
        return 5;
    else if(action.attack)
        return 6;
    else if(action.cancelAction)
        return 7;
    else
        return -1;
}


void Agent::writeAll(char* data, int count)
{
	int pos = 0;
	while(pos < count)
		pos += write(fd_write, &(data[pos]), count-pos);
}

void Agent::readAll(char* data, int count)
{
	int pos = 0;
	while(pos < count)
		pos += read(fd_read, &(data[pos]), count-pos);
}

