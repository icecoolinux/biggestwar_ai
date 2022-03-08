
#include "agent_torch.h"

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
		
torch::Tensor foo = torch::rand({12, 12});
auto foo_a = foo.accessor<float,2>();
}

Agent::~Agent()
{
	delete this->inout;
	delete this->state;
	delete this->localState;
}

void Agent::init()
{
	// Load model.
	char dirModel[1000];
	strcpy(dirModel, "./models/v7/saves/final/model.pt");
	
	try {
		// Deserialize the ScriptModule from a file using torch::jit::load().
		this->model = torch::jit::load(dirModel);
	}
	catch (const c10::Error& e) {
		std::cerr << "error loading the model\n";
		return;
	}
  
	
	/// Make inputs.

	// Add scalars, layers and minimap input
	this->scalar_t = torch::zeros({1, 3});
	this->layers_t = torch::zeros({1, (CANT_INFO_LAYERS*3+CANT_INFO_LAYERS_MAP)*2, INFO_LAYER_SIDE_PIXELS, INFO_LAYER_SIDE_PIXELS});
	this->minimap_t = torch::zeros({1, CANT_LAYERS_GLOBAL, SIDE_MINIMAP_CONV_V4, SIDE_MINIMAP_CONV_V4});
	this->inputs.push_back(this->scalar_t);
	this->inputs.push_back(this->layers_t);
	this->inputs.push_back(this->minimap_t);
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
	
/*
char fff[1000];
unsigned long long aa = Time::currentMs();

sprintf(fff, "./temp/%llu_posta.png", aa);
FILE* f2 = fopen(fff, "wb");
fwrite (msg->pngData_MiniMap, 1, msg->lenPngData, f2);
fclose(f2);

sprintf(fff, "./models/tempC/%llu_01.txt", aa);
FILE* f2 = fopen(fff, "w");
for(int y=0; y<SIDE_MINIMAP_CONV_V3; y++)
{
	for(int x=0; x<SIDE_MINIMAP_CONV_V3; x++)
	{
		sprintf(fff, "%u,%u,%u ", this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3], this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3+1], this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3 +2]);
		fputs(fff, f2);
	}
	fputs("\n", f2);
}
fclose(f2);

sprintf(fff, "./temp/%llu_02.txt", aa);
f2 = fopen(fff, "w");
for(int y=0; y<SIDE_MINIMAP_CONV_V3; y++)
{
	for(int x=0; x<SIDE_MINIMAP_CONV_V3; x++)
	{
		sprintf(fff, "%u,%u,%u ", this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3], this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3+1], this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3 +2]);
		fputs(fff, f2);
	}
	fputs("\n", f2);
}
fclose(f2);


sprintf(fff, "./temp/%llu_posta.png", aa);
FILE* f2 = fopen(fff, "wb");
fwrite (msg->pngData_MiniMap, 1, msg->lenPngData, f2);
fclose(f2);

sprintf(fff, "./temp/%llu.png", aa);
array_to_file(this->minimapData, SIDE_MINIMAP_CONV_V3, SIDE_MINIMAP_CONV_V3, fff);

sprintf(fff, "./temp/%llu_original.png", aa);
array_to_file(array, width, height, fff);

for(int x=0; x<SIDE_MINIMAP_CONV_V3; x++)
{
	for(int y=0; y<SIDE_MINIMAP_CONV_V3; y++)
	{
		int pos = y*SIDE_MINIMAP_CONV_V3*3 + x*3;
		if(this->minimapData[pos] > 0) printf("R: %d %d\n", x, y);
		if(this->minimapData[pos+1] > 0) printf("G: %d %d\n", x, y);
		if(this->minimapData[pos+1] > 0) printf("B: %d %d\n", x, y);
	}
}
*/
	
/*
		// Set minimap.
		// Primer eje es rows (y), segundo es column (x)
		for(int y=0; y<SIDE_MINIMAP_CONV_V3; y++) // Primer fila y es arriba, la ultima es la de mas abajo.
		{
			for(int x=0; x<SIDE_MINIMAP_CONV_V3; x++)
			{
				minimap_tensor.tensor<float, 5>()(0,0,y,x,0) = ((float)this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3]) / 255.0;
				minimap_tensor.tensor<float, 5>()(0,0,y,x,1) = ((float)this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3 + 1]) / 255.0;
				minimap_tensor.tensor<float, 5>()(0,0,y,x,2) = ((float)this->minimapData[y*SIDE_MINIMAP_CONV_V3*3 + x*3 + 2]) / 255.0;
			}
		}
*/
	}
	
	// Init SelectAction.
	if(msg->newPlayerCurrent)
	{
		this->globalPos1.x = msg->xView_NewPlayerCurrent;
		this->globalPos1.y = msg->yView_NewPlayerCurrent;
		this->globalPos2.x = msg->xView_NewPlayerCurrent;
		this->globalPos2.y = msg->yView_NewPlayerCurrent;
		
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

int iiii=0;
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

	// Current global position.
	vec2 currentGlobal1 = this->globalPos1;
	vec2 currentGlobal2 = this->globalPos2;
	
	// Get Local State.
	inout->getLocalState(*state, this->minimapData, this->pixelsTimeUpdated, currentGlobal1, currentGlobal2, *localState);
	inout->standardize(*localState);
	
	/// Make input

	// Scalar - (1, 3)
	auto scalar_a = this->scalar_t.accessor<float,2>();

	scalar_a[0][0] = localState->timestamp;
	scalar_a[0][1] = localState->minerals;
	scalar_a[0][2] = localState->oils;

	// Layers - (1, 98, 128, 128)
	auto layers_a = this->layers_t.accessor<float,4>();
	for(int k=0; k<CANT_INFO_LAYERS; k++)
	{
		for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++) 
		{
			for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
			{
				// Current First.
				layers_a[0][k][i][j] = localState->layersCurrentFirst[k][i][j]; 
				// Aliades First.
				layers_a[0][CANT_INFO_LAYERS+k][i][j] = localState->layersAliadesFirst[k][i][j]; 
				// Enemies First.
				layers_a[0][2*CANT_INFO_LAYERS+k][i][j] = localState->layersEnemiesFirst[k][i][j];
				// Mineral First.
				if(k==0)
					layers_a[0][3*CANT_INFO_LAYERS][i][j] = localState->layersObjectMapFirst[0][i][j];
				
				// Current Second.
				layers_a[0][3*CANT_INFO_LAYERS +1 +k][i][j] = localState->layersCurrentSecond[k][i][j]; 
				// Aliades Second.
				layers_a[0][4*CANT_INFO_LAYERS +1 +k][i][j] = localState->layersAliadesSecond[k][i][j]; 
				// Enemies Second.
				layers_a[0][5*CANT_INFO_LAYERS +1 +k][i][j] = localState->layersEnemiesSecond[k][i][j];
				// Mineral Second.
				if(k==0)
					layers_a[0][6*CANT_INFO_LAYERS +1][i][j] = localState->layersObjectMapSecond[0][i][j];
			}
		}
	}

	// Minimap - (1, 9, 256, 256)
	auto minimap_a = this->minimap_t.accessor<float,4>();
	for(int k=0; k<CANT_LAYERS_GLOBAL; k++)
		for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
			for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
				minimap_a[0][k][i][j] = localState->layerGlobal[k][i][j];
	

	// Run model.
	auto outputs = model.forward(this->inputs).toTuple();
	
	// Get outpus
	torch::Tensor actions_output = outputs->elements()[0].toTensor();
	torch::Tensor units_output = outputs->elements()[1].toTensor();
	torch::Tensor local_output = outputs->elements()[2].toTensor();
	torch::Tensor global_output = outputs->elements()[3].toTensor();
	auto actions_output_a = actions_output.accessor<float,2>();
	auto units_output_a = units_output.accessor<float,2>();
	auto local_output_a = local_output.accessor<float,4>();
	auto global_output_a = global_output.accessor<float,4>();



/*
float array[512*512*3];
char buf[200];
for(int y=0; y<SIDE_MINIMAP_CONV_V4; y++)
	for(int x=0; x<SIDE_MINIMAP_CONV_V4; x++)
		array[(SIDE_MINIMAP_CONV_V4-1-y)*SIDE_MINIMAP_CONV_V4 + x] = this->selectGlobal.pos1Global[x][y] * 255.0f;
sprintf(buf, "./jaja1_%d.png", iiii);
array32FC1_to_file(&array[0], SIDE_MINIMAP_CONV_V4, SIDE_MINIMAP_CONV_V4, buf);

for(int y=0; y<SIDE_MINIMAP_CONV_V4; y++)
	for(int x=0; x<SIDE_MINIMAP_CONV_V4; x++)
		array[(SIDE_MINIMAP_CONV_V4-1-y)*SIDE_MINIMAP_CONV_V4 + x] = this->selectGlobal.pos2Global[x][y] * 255.0f;
sprintf(buf, "./jaja2_%d.png", iiii);
iiii++;
array32FC1_to_file(&array[0], SIDE_MINIMAP_CONV_V4, SIDE_MINIMAP_CONV_V4, buf);
*/
	
	



        
	
/*
printf("start,end: %f %f\n", target_seq.flat<float>()(0), target_seq.flat<float>()(1));
printf("noaction: %f\n", target_seq.flat<float>()(2));
printf("surrender: %f\n", target_seq.flat<float>()(3));
printf("update: %f %f,%f\n", target_seq.flat<float>()(4), target_seq.flat<float>()(5), target_seq.flat<float>()(6));
*/





	// Get action state.
	InOut::Action action;
	action.noAction = actions_output_a[0][0] > 0.5f;
	action.surrender = actions_output_a[0][1] > 0.5f;
	action.update = actions_output_a[0][2] > 0.5f;
	action.move = actions_output_a[0][3] > 0.5f;
	action.recollect = actions_output_a[0][4] > 0.5f;
	action.buildBuilding = actions_output_a[0][5] > 0.5f;
	action.buildUnit = actions_output_a[0][6] > 0.5f;
	action.attack = actions_output_a[0][7] > 0.5f;
	action.cancelAction = actions_output_a[0][8] > 0.5f;
printf("noAction: %f, action.surrender: %f, action.update: %f, action.move: %f, action.recollect: %f, action.buildBuilding: %f, action.buildUnit: %f, action.attack: %f, action.cancelAction: %f\n", action.noAction, action.surrender, action.update, action.move, action.recollect, action.buildBuilding, action.buildUnit, action.attack, action.cancelAction);

	for(int i=0; i<8; i++)
		action.units[i] = units_output_a[0][i];
printf("action.typeBase: %f, action.typeBarraca: %f, action.typeTorreta: %f, action.typeRecolector: %f, action.typeSoldadoRaso: %f, action.typeSoldadoEntrenado: %f, action.typeTanque: %f, action.typeTanquePesado: %f\n", action.units[0], action.units[1], action.units[2], action.units[3], action.units[4], action.units[5], action.units[6], action.units[7]);

	int indexLocal1 = indexLocalFirst(action);
	int indexLocal2 = indexLocalSecond(action);
	float maxValueLocal1 = -99999;
	float maxValueLocal2 = -99999;
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			if(indexLocal1 >= 0)
			{
				if(local_output_a[0][indexLocal1][i][j] > maxValueLocal1)
				{
					maxValueLocal1 = local_output_a[0][indexLocal1][i][j];
					action.local[0] = j;
					action.local[1] = i;
				}
			}
			
			if(indexLocal2 >= 0)
			{
				if(local_output_a[0][indexLocal2][i][j] > maxValueLocal2)
				{
					maxValueLocal2 = local_output_a[0][indexLocal2][i][j];
					action.local[2] = j;
					action.local[3] = i;
				}
			}
		}
	}
	for(int i=0; i<4; i++)
		action.local[i] = (action.local[i] / ((float)INFO_LAYER_SIDE_PIXELS)) * ((float)INFO_LAYER_SIDE_PIXELS*INFO_LAYER_METERS_BY_PIXELS);
	
	// Extract next global select.
	int indexGlobal = indexGlobalLayer(action);
	float maxGlobal1 = -9999;
	float maxGlobal2 = -9999;
	for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
	{
		for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
		{
			if(global_output_a[0][indexGlobal*2][i][j] > maxGlobal1)
			{
				maxGlobal1 = global_output_a[0][indexGlobal*2][i][j];
				this->globalPos1.x = j;
				this->globalPos1.y = i;
			}
			if(global_output_a[0][indexGlobal*2+1][i][j] > maxGlobal2)
			{
				maxGlobal2 = global_output_a[0][indexGlobal*2+1][i][j];
				this->globalPos2.x = j;
				this->globalPos2.y = i;
			}
		}
	}
	this->globalPos1.x = (this->globalPos1.x / ((float)SIDE_MINIMAP_CONV_V4)) * ((float)SIDE_MAP_METERS);
	this->globalPos1.y = (this->globalPos1.y / ((float)SIDE_MINIMAP_CONV_V4)) * ((float)SIDE_MAP_METERS);
	this->globalPos2.x = (this->globalPos2.x / ((float)SIDE_MINIMAP_CONV_V4)) * ((float)SIDE_MAP_METERS);
	this->globalPos2.y = (this->globalPos2.y / ((float)SIDE_MINIMAP_CONV_V4)) * ((float)SIDE_MAP_METERS);
	action.nextGlobal[0] = this->globalPos1.x;
	action.nextGlobal[1] = this->globalPos1.y;
	action.nextGlobal[2] = this->globalPos2.x;
	action.nextGlobal[3] = this->globalPos2.y;

if(DEBUG)
{
printf("Global select after: %f, %f : %f, %f\n", this->globalPos1.x, this->globalPos1.y, this->globalPos2.x, this->globalPos2.y);
}

	bool ret = inout->get(*state, currentGlobal1, currentGlobal2, action, &msg[0]);
	
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
