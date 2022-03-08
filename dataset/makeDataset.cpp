
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <list>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

#include "../src/apigame/ServerMessage.h"
#include "../src/apigame/ClientMessage.h"
#include "../src/apigame/inout.h"

#include "../src/apigame/img.h"
#include "../src/configRL.h"

#define REPLAYS_DIR_DEFAULT "../../server/data/replays"
#define EPISODES_DIR_DEFAULT "./episodes"

#define SEND_TO_NET_DEFAULT false
#define USE_NOACTION_DEFAULT false
#define USE_ONLY_ICECOOL_DEFAULT false
#define AT_THAT_MILISECONDS (22*60*1000 +60*1000) 

char replaysDir[2000];
char episodesDir[2000];
bool sendToNet = SEND_TO_NET_DEFAULT;
bool useNoAction = USE_NOACTION_DEFAULT;
bool useOnlyIcecool = USE_ONLY_ICECOOL_DEFAULT;

int amountEpisodes;

int indexEpisode = -1;
int epochs = 1;

#define MAX_MESSAGES 30000
char* serverMessages[MAX_MESSAGES][300];
char* clientMessages[MAX_MESSAGES][300];
int cantServerMessages[MAX_MESSAGES];
int cantClientMessages[MAX_MESSAGES];

int fd_read, fd_write;


void init_messages()
{
	for(int i=0; i<MAX_MESSAGES; i++)
	{
		for(int j=0; j<300; j++)
		{
			serverMessages[i][j] = NULL;
			clientMessages[i][j] = NULL;
		}
		cantServerMessages[i] = 0;
		cantClientMessages[i] = 0;
	}
}
void addServerMessage(char* m, int indexAction)
{
	int len = strlen(m);
	serverMessages[indexAction][cantServerMessages[indexAction]] = new char[len+2];
	strcpy(serverMessages[indexAction][cantServerMessages[indexAction]], m);
	strcat(serverMessages[indexAction][cantServerMessages[indexAction]], "\n");
	cantServerMessages[indexAction]++;
}
void addClientMessage(char* m, int indexAction)
{
	int len = strlen(m);
	clientMessages[indexAction][cantClientMessages[indexAction]] = new char[len+2];
	strcpy(clientMessages[indexAction][cantClientMessages[indexAction]], m);
	strcat(clientMessages[indexAction][cantClientMessages[indexAction]], "\n");
	cantClientMessages[indexAction]++;
}
void save_messages(int indexAction)
{
	char buf[300];
	sprintf(buf, "%s/%d/messages.txt", episodesDir, indexEpisode);
	FILE* f = fopen(buf, "a");
	sprintf(buf, "\n\nAction %d\n", indexAction);
	fputs(buf, f);
	fputs("Server:\n", f);
	for(int i=0; i<cantServerMessages[indexAction]; i++)
		fputs(serverMessages[indexAction][i], f);
	fputs("Client:\n", f);
	for(int i=0; i<cantClientMessages[indexAction]; i++)
		fputs(clientMessages[indexAction][i], f);
	fclose(f);
}
void clear_messages()
{
	for(int i=0; i<MAX_MESSAGES; i++)
	{
		for(int j=0; j<300; j++)
		{
			if(serverMessages[i][j] != NULL)
			{
				delete[] serverMessages[i][j];
				serverMessages[i][j] = NULL;
			}
			if(clientMessages[i][j] != NULL)
			{
				delete[] clientMessages[i][j];
				clientMessages[i][j] = NULL;
			}
		}
		cantServerMessages[i] = 0;
		cantClientMessages[i] = 0;
	}
}

void generate_state_action(InOut::LocalState &local, 
							InOut::Action &action, 
							int indexAction, 
							int enemies_deads, 
							int aliades_deads, 
							int aliades_alive, 
							int win, 
							int lose,
							char* jsonScalarsOut,
							float* localLayersOut, // CANT_LOCAL_LAYERS x SIDE_LAYERS x SIDE_LAYERS
							float* globalLayersOut) // CANT_MINIMAPS x SIDE_MINIMAP x SIDE_MINIMAP
{
	char buf[20000];
	char buf2[20000];
	
	strcpy(jsonScalarsOut, "");
	
	sprintf(buf, "{\"timestamp\": %f, \"local\":{\"minerals\":%f, \"oils\":%f}, \"isClosingArea\": %f, \"msToStartToCloseArea\": %f, ", 
						local.timestamp, local.minerals, local.oils, local.isClosingArea, local.msToStartToCloseArea);
	strcat(jsonScalarsOut, buf);
	
	// Surrender
	if(action.surrender)
	{
		sprintf(buf, "\"action\":{\"name\":\"surrender\" ");
		strcat(jsonScalarsOut, buf);
	}
	// Update
	else if(action.update)
	{
		sprintf(buf, "\"action\":{\"name\":\"update\" ");
		strcat(jsonScalarsOut, buf);
	}
	// Intent move
	else if(action.move)
	{
		sprintf(buf, "\"action\":{\"name\":\"move\" ");
		strcat(jsonScalarsOut, buf);
	}
	// Intent recollect
	else if(action.recollect)
	{
		sprintf(buf, "\"action\":{\"name\":\"recollect\" ");
		strcat(jsonScalarsOut, buf);
	}
	// Intent build building
	else if(action.buildBuilding)
	{
		sprintf(buf, "\"action\":{\"name\":\"buildBuilding\"");
		strcat(jsonScalarsOut, buf);
	}
	// Intent build unit
	else if(action.buildUnit)
	{
		sprintf(buf, "\"action\":{\"name\":\"buildUnit\" ");
		strcat(jsonScalarsOut, buf);
	}
	// Intent attack
	else if(action.attack)
	{
		sprintf(buf, "\"action\":{\"name\":\"attack\" ");
		strcat(jsonScalarsOut, buf);
	}
	// CancelAction
	else if(action.cancelAction)
	{
		sprintf(buf, "\"action\":{\"name\":\"cancelAction\" ");
		strcat(jsonScalarsOut, buf);
	}
	// NoAction
	else
	{
		sprintf(buf, "\"action\":{\"name\":\"noAction\" ");
		strcat(jsonScalarsOut, buf);
	}
	
	// Units
	if(action.theresUnits[LAYER_BASE])
	{
		sprintf(buf, ", \"base\":%f ", action.units[LAYER_BASE]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresUnits[LAYER_BARRACA])
	{
		sprintf(buf, ", \"barraca\":%f ", action.units[LAYER_BARRACA]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresUnits[LAYER_TORRETA])
	{
		sprintf(buf, ", \"torreta\":%f ", action.units[LAYER_TORRETA]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresUnits[LAYER_RECOLECTOR])
	{
		sprintf(buf, ", \"recolector\":%f ", action.units[LAYER_RECOLECTOR]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresUnits[LAYER_SOLDADORASO])
	{
		sprintf(buf, ", \"soldadoRaso\":%f ", action.units[LAYER_SOLDADORASO]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresUnits[LAYER_SOLDADOENTRENADO])
	{
		sprintf(buf, ", \"soldadoEntrenado\":%f ", action.units[LAYER_SOLDADOENTRENADO]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresUnits[LAYER_TANQUE])
	{
		sprintf(buf, ", \"tanque\":%f ", action.units[LAYER_TANQUE]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresUnits[LAYER_TANQUEPESADO])
	{
		sprintf(buf, ", \"tanquePesado\":%f ", action.units[LAYER_TANQUEPESADO]);
		strcat(jsonScalarsOut, buf);
	}
	//Local
	if(action.theresLocal[0])
	{
		sprintf(buf, ", \"x1Local\":%f ", action.local[0]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresLocal[1])
	{
		sprintf(buf, ", \"y1Local\":%f ", action.local[1]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresLocal[2])
	{
		sprintf(buf, ", \"x2Local\":%f ", action.local[2]);
		strcat(jsonScalarsOut, buf);
	}
	if(action.theresLocal[3])
	{
		sprintf(buf, ", \"y2Local\":%f ", action.local[3]);
		strcat(jsonScalarsOut, buf);
	}
	
	strcat(jsonScalarsOut, "}, ");

	sprintf(buf, " \"x1NextGlobal\":%f, \"y1NextGlobal\":%f, \"x2NextGlobal\":%f, \"y2NextGlobal\":%f, ", action.global[0], action.global[1], action.global[2], action.global[3]);
	strcat(jsonScalarsOut, buf);
	
	sprintf(buf, " \"enemies_deads\":%d, \"aliades_deads\":%d, \"aliades_alive\":%d ", enemies_deads, aliades_deads, aliades_alive);
	strcat(jsonScalarsOut, buf);
	
	sprintf(buf, ", \"endgame\": {\"win\":%d, \"lose\":%d} ", win, lose);
	strcat(jsonScalarsOut, buf);
	
	strcat(jsonScalarsOut, "}");
	
	
	
	InOut::serializeLocalState(local, localLayersOut, globalLayersOut);
}

void save_state_action(int indexAction, 
						char* jsonScalarsOut,
						float* localLayersOut, // CANT_LOCAL_LAYERS x SIDE_LAYERS x SIDE_LAYERS
						float* globalLayersOut) // CANT_MINIMAPS x SIDE_MINIMAP x SIDE_MINIMAP
{
	char buf[2000];
	
	sprintf(buf, "%s/%d/%d", episodesDir, indexEpisode, indexAction);
	mkdir(buf, 0755);
	
	sprintf(buf, "%s/%d/%d/scalars.json", episodesDir, indexEpisode, indexAction);
	FILE* f = fopen(buf, "w");
	fputs(jsonScalarsOut, f);
	fclose(f);
	
	
	float array[512*512*3];
	int posLayer = 0;
	int posArray = 0;
	
	/// Local
	// Current first
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				array[row*SIDE_LAYERS +col] =  localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
				posArray++;
			}
		sprintf(buf, "%s/%d/%d/state_current_first_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS[i]);
		array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
		posLayer++;
	}
	// Aliades first
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				array[row*SIDE_LAYERS +col] =  localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
				posArray++;
			}
		sprintf(buf, "%s/%d/%d/state_aliades_first_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS[i]);
		array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
		posLayer++;
	}
	// Enemies first
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				array[row*SIDE_LAYERS +col] =  localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
				posArray++;
			}
		sprintf(buf, "%s/%d/%d/state_enemies_first_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS[i]);
		array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
		posLayer++;
	}
	// Object map first: Minerals
	for(int row=0; row<SIDE_LAYERS; row++)
		for(int col=0; col<SIDE_LAYERS; col++)
		{
			array[row*SIDE_LAYERS +col] =  localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
			posArray++;
		}
	sprintf(buf, "%s/%d/%d/state_objectmap_first_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS_OBJECTMAP[0]);
	array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
	posLayer++;
	// Object map first: Bases zones
	for(int row=0; row<SIDE_LAYERS; row++)
		for(int col=0; col<SIDE_LAYERS; col++)
		{
			array[row*SIDE_LAYERS +col] =  localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
			posArray++;
		}
	sprintf(buf, "%s/%d/%d/state_objectmap_first_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS_OBJECTMAP[1]);
	array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
	posLayer++;
	
	// Current second
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				array[row*SIDE_LAYERS +col] =  localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
				posArray++;
			}
		sprintf(buf, "%s/%d/%d/state_current_second_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS[i]);
		array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
		posLayer++;
	}
	// Aliades second
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				array[row*SIDE_LAYERS +col] =  localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
				posArray++;
			}
		sprintf(buf, "%s/%d/%d/state_aliades_second_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS[i]);
		array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
		posLayer++;
	}
	// Enemies second
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				array[row*SIDE_LAYERS +col] = localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
				posArray++;
			}
		sprintf(buf, "%s/%d/%d/state_enemies_second_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS[i]);
		array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
		posLayer++;
	}
	// Object map second: Minerals
	for(int row=0; row<SIDE_LAYERS; row++)
		for(int col=0; col<SIDE_LAYERS; col++)
		{
			array[row*SIDE_LAYERS +col] = localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
			posArray++;
		}
	sprintf(buf, "%s/%d/%d/state_objectmap_second_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS_OBJECTMAP[0]);
	array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
	posLayer++;
	// Object map second: Bases zones
	for(int row=0; row<SIDE_LAYERS; row++)
		for(int col=0; col<SIDE_LAYERS; col++)
		{
			array[row*SIDE_LAYERS +col] = localLayersOut[posArray] * 255.0f; // [posLayer][row][col]
			posArray++;
		}
	sprintf(buf, "%s/%d/%d/state_objectmap_second_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS_OBJECTMAP[1]);
	array32FC1_to_file(&array[0], SIDE_LAYERS, SIDE_LAYERS, buf);
	posLayer++;

	/// Global
	posArray = 0;
	for(int i=0; i<CANT_LAYERS_GLOBAL; i++)
	{
		for(int row=0; row<SIDE_MINIMAP; row++)
			for(int col=0; col<SIDE_MINIMAP; col++)
			{
				array[row*SIDE_MINIMAP +col] = globalLayersOut[posArray] * 255.0f; // [i][row][col]
				posArray++;
			}
		sprintf(buf, "%s/%d/%d/state_global_%s.png", episodesDir, indexEpisode, indexAction, NAMES_LAYERS_GLOBALS[i]);
		array32FC1_to_file(&array[0], SIDE_MINIMAP, SIDE_MINIMAP, buf);
	}
}

int cantMinimaps = 0;
unsigned long long minimapsTs[10000];
void calcularMinimaps(char* play, char* player)
{
	cantMinimaps = 0;
	
	// Busco el minimap anterior a ts.
	char file[300];
	char timestamp[50];
	sprintf(file, "%s/%s/%s/", replaysDir, play, player);
	DIR *dr;
	struct dirent *en;
	dr = opendir(file);
	while ((en = readdir(dr)) != NULL)
	{
		if( strcmp(en->d_name, ".")==0 || strcmp(en->d_name, "..")==0 || strcmp(en->d_name, "messages.txt")==0)
			continue;
		
		sscanf(en->d_name, "minimap_%s.png", &timestamp[0]);
		minimapsTs[cantMinimaps] = atoll(timestamp);
		cantMinimaps++;
	}
	closedir(dr);
}
void getMinimap(char* play, char* player, unsigned long long ts, float (&minimap)[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4][3])
{
	char file[300];
	
	char data[1000000];
	int lenData;
	float array[512*512*3];
	int width, height;
	
	// Busco el minimap anterior a ts.
	unsigned long long maxTs = 0;
	for(int i=0; i<cantMinimaps; i++)
	{
		if(minimapsTs[i] < ts && minimapsTs[i] > maxTs)
			maxTs = minimapsTs[i];
	}
	
	if(maxTs == 0)
	{
		for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
		{
			for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
			{
				minimap[i][j][0] = 0;
				minimap[i][j][1] = 0;
				minimap[i][j][2] = 0;
			}
		}
		return;
	}
	
	sprintf(file, "%s/%s/%s/minimap_%llu.png", replaysDir, play, player, maxTs);
	file_to_datapng(file, data, 1000000, lenData);
	datapng_to_array32F(data, lenData, array, width, height);
	
	for(int i=0; i<height; i++)
	{
		for(int j=0; j<width; j++)
		{
			minimap[i][j][0] = array[i*width*3 + j*3];
			minimap[i][j][1] = array[i*width*3 + j*3 +1];
			minimap[i][j][2] = array[i*width*3 + j*3 +2];
		}
	}
}

// Check noaction moments.
list<unsigned long long> getNoActions(char* filename)
{
	list<unsigned long long> ret;
	unsigned long long start;
	list<unsigned long long> tsServer;
	
	char buf[20000];
	FILE* f = fopen(filename, "r");
	while(fgets(buf, 20000, f) != NULL)
	{
		// remove end '\n'
		int p_ = strlen(buf)-1;
		while(buf[p_] == '\n')
		{
			buf[p_] = '\0';
			p_--;
		}

		char timestamp[20];
		strncpy(timestamp, buf, 13);
		timestamp[13] = '\0';
		unsigned long long ts = atoll(timestamp);
		
		if(strncmp("START", &(buf[15]), strlen("START")) == 0)
		{
			start = ts;
		}
		else if(strncmp("END", &(buf[15]), strlen("END")) == 0)
		{
		}
		else if(strncmp("fromServer: config", &(buf[15]), strlen("fromServer: config")) == 0)
		{
			tsServer.push_back(ts);
		}
		else if(strncmp("fromServer: endgame", &(buf[15]), strlen("fromServer: endgame")) == 0)
		{
		}
		else
		{
			if(strncmp("fromServer", &(buf[15]), strlen("fromServer")) == 0)
			{
				tsServer.push_back(ts);
			}
			else if(strncmp("fromClient", &(buf[15]), strlen("fromClient")) == 0)
			{
				//Check time without client message.
				int duration = ts-start;
				int ticks = duration/MS_AGENT_STEP;
				if(ticks >= 2)
				{
					// For each tick.
					int tickDuration = duration / ticks;
					for(int i=1; i<ticks; i++)
					{
						unsigned long long tsTick = start+tickDuration*i;
						
						// Find the last server message for join the noaction.
						list<unsigned long long>::iterator it = tsServer.begin();
						unsigned long long lastServerTs = 0;
						while(it != tsServer.end())
						{
							if( *it > tsTick && lastServerTs > 0)
							{
								ret.push_back(lastServerTs);
								break;
							}
							lastServerTs = *it;
							it++;
						}
					}
				}
				
				tsServer.clear();
				start = ts;
			}
		}
	}
	fclose(f);
	
	return ret;
}

void writeAll(int fd, char* data, int count)
{
	int pos = 0;
	while(pos < count)
		pos += write(fd, &(data[pos]), count-pos);
}
int sendStepNet(int index, bool isLast, 
					char* jsonScalars,
					float* localLayers, // CANT_LOCAL_LAYERS x SIDE_LAYERS x SIDE_LAYERS
					float* globalLayers) // CANT_MINIMAPS x SIDE_MINIMAP x SIDE_MINIMAP)
{
	char buf[10000];
	
	// Index
	sprintf(buf, "%d\n", index);
	writeAll(fd_write, buf, strlen(buf)+1);
	
	// isLast
	if(isLast)
		writeAll(fd_write, "1\n", 3);
	else
		writeAll(fd_write, "0\n", 3);
	
	// jsonScalar
	sprintf(buf, "%s\n", jsonScalars);
	writeAll(fd_write, buf, strlen(buf)+1);
	
	// Local size
	sprintf(buf, "%d\n", sizeof(float)*CANT_LOCAL_LAYERS*SIDE_LAYERS*SIDE_LAYERS);
	writeAll(fd_write, buf, strlen(buf)+1);
	
	// Global size
	sprintf(buf, "%d\n", sizeof(float)*CANT_MINIMAPS*SIDE_MINIMAP*SIDE_MINIMAP);
	writeAll(fd_write, buf, strlen(buf)+1);
	
	// Local 
	writeAll(fd_write, (char*)localLayers, sizeof(float)*CANT_LOCAL_LAYERS*SIDE_LAYERS*SIDE_LAYERS);
	
	// Global
	writeAll(fd_write, (char*)globalLayers, sizeof(float)*CANT_MINIMAPS*SIDE_MINIMAP*SIDE_MINIMAP);
}

void commandNet(int amountEpisodes)
{
	char buf[1000];
	char bufTmp[1000];
	
	while(true)
	{
		// Read commands byte by byte
		int bytes = 0;
		while(bytes < 1000)
		{
			bytes += read(fd_read, &(buf[bytes]), 1);
			if(buf[bytes-1] == '\n')
				break;
		}
		buf[bytes-1] = '\0';
		bytes--;

		// Execute commands		
		if(strcmp(buf, "episodes")==0)
		{
			sprintf(bufTmp, "%d\n", amountEpisodes);
			write(fd_write, bufTmp, strlen(bufTmp)+1 );
		}
		else if(buf[0] == 'e' && buf[1] == 'p' && buf[2] == 'o' && buf[3] == 'c' && buf[4] == 'h' && buf[5] == 's')
		{
			epochs = atoi(&(buf[7]));
			printf("Epochs used: %d\n", epochs);
		}
		else if(strcmp(buf, "start_steps")==0)
			return;
		else
			printf("Commando not found: %s\n", buf);
	}
}

int get_replays_location(char** worldDir, char** usernameDir)
{
	int amount = 0;
	
	DIR *dr;
	struct dirent *en;
	dr = opendir(replaysDir);
	if (dr)
	{
		// For each replay world.
		while ((en = readdir(dr)) != NULL)
		{
			if( strcmp(en->d_name, ".")==0 || strcmp(en->d_name, "..")==0)
				continue;
			
			char buf[20000];
			sprintf(buf, "%s/%s", replaysDir, en->d_name);
			
			// Read usernames
			DIR *dr2;
			struct dirent *en2;
			dr2 = opendir(buf);
			if (dr2) 
			{
				// For each username
				while ((en2 = readdir(dr2)) != NULL)
				{
					if( strcmp(en2->d_name, ".")==0 || strcmp(en2->d_name, "..")==0)
						continue;
				
					worldDir[amount] = new char[strlen(en->d_name)+1];
					strcpy(worldDir[amount], en->d_name);
					
					usernameDir[amount] = new char[strlen(en2->d_name)+1];
					strcpy(usernameDir[amount], en2->d_name);
					
					amount++;
				}
			}
			closedir(dr2);
		}
	}
	closedir(dr);

	return amount;
}

void readParams(int argc, char** argv)
{
	strcpy(replaysDir, REPLAYS_DIR_DEFAULT);
	strcpy(episodesDir, EPISODES_DIR_DEFAULT);
	sendToNet = SEND_TO_NET_DEFAULT;
	useNoAction = USE_NOACTION_DEFAULT;
	useOnlyIcecool = USE_ONLY_ICECOOL_DEFAULT;
	
	bool showHelp = false;
	bool notSave = false;
	
	int i=0;
	while(i<argc)
	{
		if(strcmp(argv[i], "--dir") == 0)
		{
			i++;
			strcpy(episodesDir, argv[i]);
		}
		else if(strcmp(argv[i], "--replays") == 0)
		{
			i++;
			strcpy(replaysDir, argv[i]);
		}
		else if(strcmp(argv[i], "--net") == 0)
			sendToNet = true;
		else if(strcmp(argv[i], "--use-noaction") == 0)
			useNoAction = true;
		else if(strcmp(argv[i], "--use-only-icecool") == 0)
			useOnlyIcecool = true;
		else if(strcmp(argv[i], "--no-save") == 0)
			notSave = true;
		else if(strcmp(argv[i], "--help") == 0)
			showHelp = true;
		i++;
	}
	
	if(notSave)
		strcpy(episodesDir, "");
			
	if(showHelp)
	{
		printf("makeDataset --net --dir <dir to save> --no-save --replays <dir> --use-noaction --use-only-icecool\n");
		printf("\n");
		printf("--dir <dir to save>: Default %s\n", EPISODES_DIR_DEFAULT);
		printf("--replays <dir>: Default %s\n", REPLAYS_DIR_DEFAULT);
		if(SEND_TO_NET_DEFAULT)
			printf("--net: Default true\n");
		else
			printf("--net: Default false\n");
		if(USE_NOACTION_DEFAULT)
			printf("--use-noaction: Default true\n");
		else
			printf("--use-noaction: Default false\n");
		if(USE_ONLY_ICECOOL_DEFAULT)
			printf("--use-only-icecool: Default true\n");
		else
			printf("--use-only-icecool: Default false\n");
		printf("--no-save: Not save to disk\n");
	}
	else
		printf("--help for options\n\n");
	printf("\n");
}


// Save two actions, noAction with the global position and another with the action.
void pushAction(InOut* inout, InOut::LocalState* localNoAction,
				int &index, bool isLast,
				InOut::Action* action, InOut::LocalState* localAction,
				int &enemies_deads, int &aliades_deads, int &aliades_alive, int &win, int &lose, 
				vec2 currentGlobal1, vec2 currentGlobal2,
				char* jsonScalars, float* localLayers, float* globalLayers)
{
	// Make and send/save no action
	InOut::Action* noAction = new InOut::Action;
	inout->setNoAction(currentGlobal1, currentGlobal2, *noAction);
	generate_state_action(*localNoAction, *noAction, index, enemies_deads, aliades_deads, aliades_alive, win, lose, jsonScalars, localLayers, globalLayers);
	if(strcmp(episodesDir, "") != 0)
	{
		save_state_action(index, jsonScalars, localLayers, globalLayers);
		save_messages(index);
	}
	if(sendToNet)
	{
		bool isLast_ = isLast && action == NULL;
		sendStepNet(index, isLast_, jsonScalars, localLayers, globalLayers);
	}
	delete noAction;
	index++;
	enemies_deads = 0; aliades_deads = 0;
	
	// Send/save the action (if there's)
	if(action != NULL)
	{
		generate_state_action(*localAction, *action, index, enemies_deads, aliades_deads, aliades_alive, win, lose, jsonScalars, localLayers, globalLayers);
		if(strcmp(episodesDir, "") != 0)
		{
			save_state_action(index, jsonScalars, localLayers, globalLayers);
			save_messages(index);
		}
		if(sendToNet)
			sendStepNet(index, isLast, jsonScalars, localLayers, globalLayers);
		index++;
	}
}

int main(int argc, char** argv)
{
	srand(time(NULL));

	readParams(argc, argv);
	
	InOut::State* state = new InOut::State;
	InOut::Action* action = new InOut::Action;
	InOut::LocalState* localAction = new InOut::LocalState;
	InOut::LocalState* localNoAction = new InOut::LocalState;
	vec2 lastGlobal1, lastGlobal2;
										
	char jsonScalars[20000];
	float minimap[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4][3];
	float pixelsTimeUpdated[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4];
	float* localLayers = new float[CANT_LOCAL_LAYERS*SIDE_LAYERS*SIDE_LAYERS];
	float* globalLayers = new float[CANT_MINIMAPS*SIDE_MINIMAP*SIDE_MINIMAP];
	
	char* worldDir[10000];
	char* usernameDir[10000];
	amountEpisodes = get_replays_location(worldDir, usernameDir);
	
	init_messages();

	// Make fifo to send espides
	if(sendToNet)
	{
printf("A\n");
		mkfifo("/tmp/fifo_bw_topython", 0666);
		mkfifo("/tmp/fifo_bw_tocpp", 0666);
printf("B\n");
		fd_write = open("/tmp/fifo_bw_topython", O_WRONLY);
printf("C\n");
		fd_read = open("/tmp/fifo_bw_tocpp", O_RDONLY);
printf("D\n");
	}
		
	char buf[20000];
	char buf2[20000];
	
	int epoch = 0;
	while(epoch < epochs)
	{
		// Shuffle episodes
		if(sendToNet)
		{
			for(int i=0; i<amountEpisodes-1; i++)
			{
				int r = rand() % (amountEpisodes-i-1);
				r+=i;
				r++;
				
				char* swap = worldDir[r];
				worldDir[r] = worldDir[i];
				worldDir[i] = swap;
				
				swap = usernameDir[r];
				usernameDir[r] = usernameDir[i];
				usernameDir[i] = swap;
			}
		}

		for(int r=0; r<amountEpisodes; r++)
		{
			if(useOnlyIcecool && strcmp(usernameDir[r], "icecool") != 0)
				continue;
			
			// First handle the Net
			if(sendToNet)
				commandNet(amountEpisodes);
			
			InOut* inout = new InOut();
			
			sprintf(buf, "%s/%s/%s/messages.txt", replaysDir, worldDir[r], usernameDir[r]);
			
			bool saveLastAction = false;
			int indexAction = 0;
			
			int enemies_deads = 0;
			int aliades_deads = 0;
			int aliades_alive = 0;
			int win = 0;
			int lose = 0;
			
	printf("Archivo %s\n", buf);
			FILE* f = fopen(buf, "r");
			if(f != NULL)
			{
				// Check noAction moments.
				list<unsigned long long>tsNoActions;
				if(useNoAction)
					tsNoActions = getNoActions(buf);
				
				calcularMinimaps(worldDir[r], usernameDir[r]);
				unsigned long long tsStart = 0;
				unsigned long long tsCurrent = 0;
				
				int linea = 0;
				while(fgets(buf, 20000, f) != NULL)
				{
					linea++;
	printf("Linea: %d\n", linea);
					
					// remove end '\n'
					int p_ = strlen(buf)-1;
					while(buf[p_] == '\n')
					{
						buf[p_] = '\0';
						p_--;
					}

					char timestamp[20];
					strncpy(timestamp, buf, 13);
					timestamp[13] = '\0';
					
					tsCurrent = atoll(timestamp);
					if(tsStart == 0)
						tsStart = tsCurrent;
							
					if(strncmp("START", &(buf[15]), strlen("START")) == 0)
					{
						// Make episode directory.
						indexEpisode ++;
						sprintf(buf2, "%s/%d", episodesDir, indexEpisode);
						mkdir(buf2, 0755);
						
						inout->initState(*state);

						saveLastAction = false;
						indexAction = 0;
						lastGlobal1 = vec2(SIDE_MINIMAP/2, SIDE_MINIMAP/2);
						lastGlobal2 = vec2(SIDE_MINIMAP/2, SIDE_MINIMAP/2);

						clear_messages();
						
						// Init pixel time updated
						for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
							for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
								pixelsTimeUpdated[i][j] = 0.0f;
					}
					else if(strncmp("END", &(buf[15]), strlen("END")) == 0)
					{
						// Get local state for noAction
						getMinimap(worldDir[r], usernameDir[r], tsCurrent, minimap);
						inout->getLocalState(*state, minimap, pixelsTimeUpdated, lastGlobal1, lastGlobal2, *localNoAction);
						inout->standardize(*localNoAction);

						// Save action.
						pushAction(inout, localNoAction, 
									indexAction, true,
									NULL, NULL,
									enemies_deads, aliades_deads, aliades_alive, win, lose, 
									lastGlobal1, lastGlobal2, 
									jsonScalars, localLayers, globalLayers);
						enemies_deads = 0; aliades_deads = 0; aliades_alive = 0; win = 0; lose = 0;
						
						saveLastAction = true;
					}
					// Ignore some commands.
					else if(strncmp("fromServer: config", &(buf[15]), strlen("fromServer: config")) == 0)
					{
					}
					else if(strncmp("fromServer: endgame", &(buf[15]), strlen("fromServer: endgame")) == 0)
					{
						// Extract if win or lose
						ServerMessage sm;
						sm.set(&buf[27], strlen(buf)-27);
						
						if(sm.win_EndGame)
							win = 1;
						if(sm.lose_EndGame)
							lose = 1;
					}
					else
					{
						if(strncmp("fromServer", &(buf[15]), strlen("fromServer")) == 0)
						{
							addServerMessage(buf, indexAction);
							
							unsigned long long ts = tsCurrent - tsStart;
							
							int pos = 27;
							int len = strlen(buf);
							ServerMessage sm;
							while(pos < len)
							{
								pos += sm.set(&buf[pos], len-pos);
								
								// Add dead aliades or dead enemies
								if(sm.removePlayer)
								{
									if(inout->isEnemy(sm.playerName_RemovePlayer))
										enemies_deads++;
									else
									{
										aliades_deads++;
										aliades_alive--;
									}
								}
								// New aliade player, new alive
								else if(sm.newPlayer && !inout->isEnemy(sm.playerName_NewPlayer))
									aliades_alive++;
								// Init position.
								else if(sm.newPlayerCurrent)
								{
									lastGlobal1 = vec2(sm.xView_NewPlayerCurrent, sm.yView_NewPlayerCurrent);
									lastGlobal2 = vec2(sm.xView_NewPlayerCurrent, sm.yView_NewPlayerCurrent);
								}
								
								inout->put(ts, &sm, *state, pixelsTimeUpdated);
							}
							
							if(useNoAction)
							{
								// I have to put a noAction.
								unsigned long long globalTS = tsCurrent;
								list<unsigned long long>::iterator it = tsNoActions.begin();
								while(it != tsNoActions.end())
								{
									if(*it == globalTS)
									{
										sprintf(buf, "NoAction %f %f, %f %f", lastGlobal1.x, lastGlobal1.y, lastGlobal2.x, lastGlobal2.y);
										addClientMessage(buf, indexAction);
										
										// Get local state for noAction
										getMinimap(worldDir[r], usernameDir[r], globalTS, minimap);
										inout->getLocalState(*state, minimap, pixelsTimeUpdated, lastGlobal1, lastGlobal2, *localNoAction);
										inout->standardize(*localNoAction);

										// Save action.
										pushAction(inout, localNoAction, 
													indexAction, false,
													NULL, NULL,
													enemies_deads, aliades_deads, aliades_alive, win, lose, 
													lastGlobal1, lastGlobal2, 
													jsonScalars, localLayers, globalLayers);
										
										break;
									}
									it++;
								}
							}
						}
						else if(strncmp("fromClient", &(buf[15]), strlen("fromClient")) == 0)
						{
							addClientMessage(buf, indexAction);

							int pos = 27;
							int len = strlen(buf);
							ClientMessage cm;
							while(pos < len)
							{
								pos += cm.set(&buf[pos], len-pos);

								if(!cm.miniMap_)
								{
									// Get action.
									bool ok = inout->inverse_get(*state, cm, *action);
									
									if(ok)
									{
										// Save only if it's not noAction
										if(!action->noAction)
										{
											// If it is update then set view
											if(cm.update_)
											{
												state->view.x = (cm.xMin_Update + cm.xMax_Update)/2;
												state->view.y = (cm.yMin_Update + cm.yMax_Update)/2;
											}
											
											// Get local state for noAction.
											getMinimap(worldDir[r], usernameDir[r], tsCurrent, minimap);
											inout->getLocalState(*state, minimap, pixelsTimeUpdated, lastGlobal1, lastGlobal2, *localNoAction);
											inout->standardize(*localNoAction);
											
											// Get local state for action.
											lastGlobal1.x = action->global[0];
											lastGlobal1.y = action->global[1];
											lastGlobal2.x = action->global[2];
											lastGlobal2.y = action->global[3];
											getMinimap(worldDir[r], usernameDir[r], tsCurrent, minimap);
											inout->getLocalState(*state, minimap, pixelsTimeUpdated, lastGlobal1, lastGlobal2, *localAction);
											inout->standardize(*localAction);

											// Save action.
											pushAction(inout, localNoAction, 
														indexAction, false,
														action, localAction,
														enemies_deads, aliades_deads, aliades_alive, win, lose, 
														lastGlobal1, lastGlobal2, 
														jsonScalars, localLayers, globalLayers);
										}
									}
									// Print error.
									else
									{
										printf("Error in file %s/%s/%s/messages.txt, line: %s", replaysDir, worldDir[r], usernameDir[r], buf);
									}
								}
							}
						}
					}
					
					// End by time limit.
					if(tsStart > 0 && (tsCurrent-tsStart) > AT_THAT_MILISECONDS)
						break;
				}
				
				if(!saveLastAction)
				{
					// Get local state for noAction, it will be the input for this action.
					getMinimap(worldDir[r], usernameDir[r], tsCurrent, minimap);
					inout->getLocalState(*state, minimap, pixelsTimeUpdated, lastGlobal1, lastGlobal2, *localNoAction);
					inout->standardize(*localNoAction);
												
					// Save action.
					pushAction(inout, localNoAction, 
								indexAction, true,
								NULL, NULL,
								enemies_deads, aliades_deads, aliades_alive, win, lose, 
								lastGlobal1, lastGlobal2, 
								jsonScalars, localLayers, globalLayers);
				}

				fclose(f);
				delete inout;
			}
		}
		
		epoch++;
	}
	
	if(sendToNet)
	{
		close(fd_write);
		close(fd_read);
	}
}

