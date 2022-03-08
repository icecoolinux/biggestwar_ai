
#include "inout.h"

char* NAMES_LAYERS[] = {"base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"};
char* NAMES_LAYERS_OBJECTMAP[] = {"mineral", "baseszones"};
char* NAMES_LAYERS_GLOBALS[] = {"minimapR", "minimapG", "minimapB", "update", "first", "second", "currentclosed", "futureclosed", "pixelstimeupdated"};

InOut::InOut()
{
	myEquip = -1;
	strcpy(myPlayerName, "");
	
	for(int i=0; i<EQUIPS; i++)
	{
		for(int j=0; j<MAX_PLAYERS_BY_EQUIP; j++)
		{
			playerIndexOccuped[i][j] = NULL;
			for(int k=0; k<MAX_UNITS_PLAYER; k++)
				unitsIndexOccuped[i][j][k] = NULL;
			for(int k=0; k<MAX_BUILDINGS_PLAYER; k++)
				buildingsIndexOccuped[i][j][k] = NULL;
		}
	}
	for(int i=0; i<MAX_MINERALS_MAP; i++)
		mineralsIndexOccuped[i] = NULL;
}

InOut::~InOut()
{/*
	unordered_map<string, PlayerInfo_*> playersInfo;
		unordered_map<unsigned long long, ObjectInfo_*> objectsInfo;*/
}

bool InOut::isEnemy(char* playerName)
{
	PlayerInfo_* info = this->getInfoPlayer(playerName);
	if(info == NULL || info->indexEquip == 0)
		return false;
	else
		return true;
}

void InOut::initState(State &state)
{
	state.areaIsClosing = false;
	state.areaStartToCloseTs = 0;
	state.areaFutureCenterX = 0;
	state.areaFutureCenterY = 0;
	state.areaCurrentBottom = 0;
	state.areaCurrentTop = 0;
	state.areaCurrentLeft = 0;
	state.areaCurrentRight = 0;
	state.areaFutureBottom = 0;
	state.areaFutureTop = 0;
	state.areaFutureLeft = 0;
	state.areaFutureRight = 0;
	state.areaSpeedCloseSecBottom = 0.0f;
	state.areaSpeedCloseSecTop = 0.0f;
	state.areaSpeedCloseSecLeft = 0.0f;
	state.areaSpeedCloseSecRight = 0.0f;
	
	state.timestamp = 0;
	state.view.x = 0;
	state.view.y = 0;
	state.minerals = 0;
	state.oils = 0;

	for(int e=0; e<EQUIPS; e++)
		state.amountBasesZones[e] = 0;
	
	for(int e=0; e<EQUIPS; e++)
		for(int p=0; p<MAX_PLAYERS_BY_EQUIP; p++)
			initPlayer(state.players[e][p]);
	
	for(int i=0; i<MAX_MINERALS_MAP; i++)
		initObjectMap(state.objectMap[i]);
}

void InOut::copyState(State &src, State &dst)
{
	memcpy(&dst, &src, sizeof(State));
}

bool InOut::put(unsigned long long tsIn, ServerMessage* msgIn, State &stateOut, float (&pixelsTimeUpdatedOut)[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4])
{
	stateOut.timestamp = tsIn;
	
	if(msgIn->newPlayerCurrent)
	{
		stateOut.players[0][0].active = true;
		stateOut.view.x = msgIn->xView_NewPlayerCurrent;
		stateOut.view.y = msgIn->yView_NewPlayerCurrent;
		
		myEquip = msgIn->equip_NewPlayerCurrent;
		strcpy(myPlayerName, msgIn->playerName_NewPlayerCurrent);
		
		PlayerInfo_* info = insertPlayer(msgIn->playerName_NewPlayerCurrent, msgIn->equip_NewPlayerCurrent);
	}
	else if(msgIn->newPlayer)
	{
		// Only if i'm not.
		if(strcmp(myPlayerName, msgIn->playerName_NewPlayer) != 0)
		{
			PlayerInfo_* info = insertPlayer(msgIn->playerName_NewPlayer, msgIn->equip_NewPlayer);
		
			stateOut.players[info->indexEquip][info->indexPlayer].active = true;
		}
	}
	else if(msgIn->removePlayer)
	{
		PlayerInfo_* info = getInfoPlayer(msgIn->playerName_RemovePlayer);
		if(info == NULL)
			return false;
		
		initPlayer(stateOut.players[info->indexEquip][info->indexPlayer]);
		
		removePlayer(msgIn->playerName_RemovePlayer);
	}
	else if(msgIn->delete_)
	{
		ObjectInfo_* info = getInfoObject(msgIn->id_Delete);
		if(info == NULL)
			return false;
		
		if(msgIn->destroyed_Delete)
		{
			if(info->indexMineral >= 0)
				initObjectMap(stateOut.objectMap[info->indexMineral]);
			else
			{
				if(info->indexUnit >= 0)
					initUnit(stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit]);
				if(info->indexBuilding >= 0)
					initBuilding(stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding]);
			}
		
			removeObject(msgIn->id_Delete);
		}
		else
		{
			if(info->indexMineral >= 0)
				stateOut.objectMap[info->indexMineral].visible = false;
			else
			{
				if(info->indexUnit >= 0)
					stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].visible = false;
				if(info->indexBuilding >= 0)
					stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].visible = false;
			}
		}
	}
	else if(msgIn->resources)
	{
		stateOut.minerals = msgIn->mineral_Resources;
		stateOut.oils = msgIn->oil_Resources;
	}
	else if(msgIn->new_)
	{
		ObjectInfo_* info = getInfoObject(msgIn->id_New);
		// Is really new object
		if(info == NULL)
			info = insertObject(msgIn->id_New, msgIn->player_New, msgIn->type_New);

		if(info->indexMineral >= 0)
		{
			stateOut.objectMap[info->indexMineral].active = true;
			stateOut.objectMap[info->indexMineral].visible = true;
			stateOut.objectMap[info->indexMineral].updated = Time::currentMs();
			stateOut.objectMap[info->indexMineral].id = msgIn->id_New;
			stateOut.objectMap[info->indexMineral].pos.x = msgIn->pos_New.x;
			stateOut.objectMap[info->indexMineral].pos.y = msgIn->pos_New.y;
			stateOut.objectMap[info->indexMineral].amount = msgIn->amount_New;
		}
		else
		{
			if(info->indexUnit >= 0)
			{
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].active = true;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].visible = true;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].updated = Time::currentMs();
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].id = msgIn->id_New;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].type = msgIn->type_New;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.x = msgIn->pos_New.x;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.y = msgIn->pos_New.y;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].life = msgIn->life_New;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].fullLife = msgIn->fullLife_New;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].creada = msgIn->creada_New;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].collected = msgIn->collected_New;
			}
			else if(info->indexBuilding >= 0)
			{
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].active = true;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].visible = true;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].updated = Time::currentMs();
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].id = msgIn->id_New;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].type = msgIn->type_New;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.x = msgIn->pos_New.x;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.y = msgIn->pos_New.y;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].life = msgIn->life_New;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].fullLife = msgIn->fullLife_New;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].creada = msgIn->creada_New;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].construccionCreando = msgIn->construccionCreando_New;
			}
		}
	}
	else if(msgIn->change)
	{
		ObjectInfo_* info = getInfoObject(msgIn->id_Change);
		if(info == NULL)
			return false;
		
		if(info->indexMineral >= 0)
		{
			stateOut.objectMap[info->indexMineral].updated = Time::currentMs();
			if(msgIn->change_pos)
			{
				stateOut.objectMap[info->indexMineral].pos.x = msgIn->pos_Change.x;
				stateOut.objectMap[info->indexMineral].pos.y = msgIn->pos_Change.y;
			}
			if(msgIn->change_amount)
				stateOut.objectMap[info->indexMineral].amount = msgIn->amount_Change;
		}
		else
		{
			if(info->indexUnit >= 0)
			{
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].updated = Time::currentMs();
				if(msgIn->change_pos)
				{
					stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.x = msgIn->pos_Change.x;
					stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.y = msgIn->pos_Change.y;
				}
				if(msgIn->change_life)
					stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].life = msgIn->life_Change;
				if(msgIn->change_fullLife)
					stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].fullLife = msgIn->fullLife_Change;
				if(msgIn->change_creada)
					stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].creada = msgIn->creada_Change;
				if(msgIn->change_collected)
					stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].collected = msgIn->collected_Change;
			}
			else if(info->indexBuilding >= 0)
			{
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].updated = Time::currentMs();
				if(msgIn->change_pos)
				{
					stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.x = msgIn->pos_Change.x;
					stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.y = msgIn->pos_Change.y;
				}
				if(msgIn->change_life)
					stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].life = msgIn->life_Change;
				if(msgIn->change_fullLife)
					stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].fullLife = msgIn->fullLife_Change;
				if(msgIn->change_creada)
					stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].creada = msgIn->creada_Change;
				if(msgIn->change_construccionCreando)
					stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].construccionCreando = msgIn->construccionCreando_Change;
			}
		}

		if(msgIn->deleteAction_Change || msgIn->newAction_Change)
		{
			if(info->indexUnit >= 0)
			{
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].updated = Time::currentMs();
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].action = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].subAction = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionPos.x = 0.0f;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionPos.y = 0.0f;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionMake = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionUnit2ID = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionBuild2ID = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionObjectMap2ID = 0;
			}
			else if(info->indexBuilding >= 0)
			{
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].updated = Time::currentMs();
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].action = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].subAction = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionMake = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionUnit2ID = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionBuild2ID = 0;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionObjectMap2ID = 0;
			}
		}
		
		if(msgIn->newAction_Change)
		{
			if(info->indexUnit >= 0)
			{
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].updated = Time::currentMs();
				
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].action = msgIn->actionType_NewAction;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].subAction = msgIn->actionSubType_NewAction;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionPos.x = msgIn->actionPos_NewAction.x;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionPos.y = msgIn->actionPos_NewAction.y;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionMake = msgIn->actionMake_NewAction;
				
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionUnit2ID = msgIn->actionUnit2ID_NewAction;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionBuild2ID = msgIn->actionBuild2ID_NewAction;
				stateOut.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionObjectMap2ID = msgIn->actionObjectMapID_NewAction;
			}
			else if(info->indexBuilding >= 0)
			{
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].updated = Time::currentMs();
				
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].action = msgIn->actionType_NewAction;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].subAction = msgIn->actionSubType_NewAction;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionMake = msgIn->actionMake_NewAction;
				
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionUnit2ID = msgIn->actionUnit2ID_NewAction;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionBuild2ID = msgIn->actionBuild2ID_NewAction;
				stateOut.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionObjectMap2ID = msgIn->actionObjectMapID_NewAction;
			}
		}
	}
	else if(msgIn->basesZones)
	{
		for(int i=0; i<msgIn->amountBasesZones; i++)
		{
			// Add
			if(msgIn->isAddBasesZones[i])
			{
				int e = msgIn->equipBasesZones[i];
				stateOut.basesZones[e][ stateOut.amountBasesZones[e] ].pos = msgIn->posBasesZones[i];
				stateOut.basesZones[e][ stateOut.amountBasesZones[e] ].radio = msgIn->radioBasesZones[i];
				stateOut.amountBasesZones[e]++;
			}
			// Del
			else
			{
				// Find the zone.
				bool foundIt = false;
				int e2=0, i2=0;
				while(e2<EQUIPS)
				{
					while(i2<stateOut.amountBasesZones[e2])
					{
						if(stateOut.basesZones[e2][i2].pos.dist(msgIn->posBasesZones[i]) < stateOut.basesZones[e2][i2].radio)
						{
							foundIt = true;
							break;
						}
						i2++;
					}
					if(foundIt)
						break;
					e2++;
				}
				// Remove it.
				if(foundIt)
				{
					for(int i3=i2+1; i3<stateOut.amountBasesZones[e2]; i3++)
						stateOut.basesZones[e2][i3-1] = stateOut.basesZones[e2][i3];
					stateOut.amountBasesZones[e2]--;
				}
			}
		}
	}
	else if(msgIn->area)
	{
		stateOut.areaIsClosing = msgIn->isClosing_Area;
		stateOut.areaStartToCloseTs = tsIn + msgIn->msToClose_Area;
		stateOut.areaFutureCenterX = msgIn->futureCenter_Area.x;
		stateOut.areaFutureCenterY = msgIn->futureCenter_Area.y;
		stateOut.areaCurrentBottom = msgIn->currentBottom_Area;
		stateOut.areaCurrentTop = msgIn->currentTop_Area;
		stateOut.areaCurrentLeft = msgIn->currentLeft_Area;
		stateOut.areaCurrentRight = msgIn->currentRight_Area;
		stateOut.areaFutureBottom = msgIn->futureBottom_Area;
		stateOut.areaFutureTop = msgIn->futureTop_Area;
		stateOut.areaFutureLeft = msgIn->futureLeft_Area;
		stateOut.areaFutureRight = msgIn->futureRight_Area;
		stateOut.areaSpeedCloseSecBottom = msgIn->speedCloseSecBottom_Area;
		stateOut.areaSpeedCloseSecTop = msgIn->speedCloseSecTop_Area;
		stateOut.areaSpeedCloseSecLeft = msgIn->speedCloseSecLeft_Area;
		stateOut.areaSpeedCloseSecRight = msgIn->speedCloseSecRight_Area;
	}
	
	// Set pixel time updated.
	int metersByPixelsMinimap = SIDE_MAP_METERS / SIDE_MINIMAP_CONV_V4;
	for(int i=(stateOut.view.x -ANCHO_UPDATE/2)/metersByPixelsMinimap; i<(stateOut.view.x +ANCHO_UPDATE/2)/metersByPixelsMinimap; i++)
		for(int j=(stateOut.view.y -ALTO_UPDATE/2)/metersByPixelsMinimap; j<(stateOut.view.y +ALTO_UPDATE/2)/metersByPixelsMinimap; j++)
			if(i>=0 && i<SIDE_MINIMAP_CONV_V4 && j>=0 && j<SIDE_MINIMAP_CONV_V4)
				pixelsTimeUpdatedOut[i][j] = tsIn;
			
	return true;
}


void InOut::getLocalState(State &globalIn, 
							float (&minimapIn)[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4][3], 
							float (&pixelsTimeUpdatedIn)[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4], 
							vec2 global1In, vec2 global2In, 
							LocalState &localOut)
{
	initLocalState(localOut);
	
	float xView1 = global1In.x;
	float yView1 = global1In.y;
	float xView2 = global2In.x;
	float yView2 = global2In.y;
	
	int side = INFO_LAYER_SIDE_PIXELS * INFO_LAYER_METERS_BY_PIXELS;
	
	localOut.timestamp = globalIn.timestamp;
	localOut.minerals = globalIn.minerals;
	localOut.oils = globalIn.oils;
	localOut.isClosingArea = globalIn.areaIsClosing;
	localOut.msToStartToCloseArea = globalIn.areaStartToCloseTs - globalIn.timestamp;

	// Current player info.
	generateLayers(xView1 -side/2, yView1 -side/2, globalIn.players[0][0], localOut.layersCurrentFirst);
	generateLayers(xView2 -side/2, yView2 -side/2, globalIn.players[0][0], localOut.layersCurrentSecond);
	
	// Aliades.
	for(int i=1; i<MAX_PLAYERS_BY_EQUIP; i++)
	{
		generateLayers(xView1 -side/2, yView1 -side/2, globalIn.players[0][i], localOut.layersAliadesFirst);
		generateLayers(xView2 -side/2, yView2 -side/2, globalIn.players[0][i], localOut.layersAliadesSecond);
	}
	
	// Enemies.
	for(int e=1; e<EQUIPS; e++)
	{
		for(int i=0; i<MAX_PLAYERS_BY_EQUIP; i++)
		{
			generateLayers(xView1 -side/2, yView1 -side/2, globalIn.players[e][i], localOut.layersEnemiesFirst);
			generateLayers(xView2 -side/2, yView2 -side/2, globalIn.players[e][i], localOut.layersEnemiesSecond);
		}
	}

	/// Object Map.
	// Minerals
	for(int i=0; i<MAX_MINERALS_MAP; i++)
	{
		if(globalIn.objectMap[i].active)
		{
			int x_ = xView1 -side/2;
			int y_ = yView1 -side/2;
			int xObj = globalIn.objectMap[i].pos.x;
			int yObj = globalIn.objectMap[i].pos.y;
			if( x_ <= xObj && xObj <= (x_+side) && y_ <= yObj && yObj <= (y_+side))
				putObjectToLayer(xObj-x_, yObj-y_, RADIO_MINERAL, localOut.layersObjectMapFirst[LAYER_MINERAL], globalIn.objectMap[i].amount);
			
			x_ = xView2 -side/2;
			y_ = yView2 -side/2;
			if( x_ <= xObj && xObj <= (x_+side) && y_ <= yObj && yObj <= (y_+side))
				putObjectToLayer(xObj-x_, yObj-y_, RADIO_MINERAL, localOut.layersObjectMapSecond[LAYER_MINERAL], globalIn.objectMap[i].amount);
		}
	}
	// Bases zones.
	for(int e=0; e<EQUIPS; e++)
	{
		for(int i=0; i<globalIn.amountBasesZones[e]; i++)
		{
			int x_ = xView1 -side/2;
			int y_ = yView1 -side/2;
			int xObj = globalIn.basesZones[e][i].pos.x;
			int yObj = globalIn.basesZones[e][i].pos.y;
			int radio = globalIn.basesZones[e][i].radio;
			if( x_ <= (xObj+radio) && (xObj-radio) <= (x_+side) && y_ <= (yObj+radio) && (yObj-radio) <= (y_+side))
				putObjectToLayer(xObj-x_, yObj-y_, radio, localOut.layersObjectMapFirst[LAYER_BASES_ZONES], 1.0f);
			
			x_ = xView2 -side/2;
			y_ = yView2 -side/2;
			if( x_ <= (xObj+radio) && (xObj-radio) <= (x_+side) && y_ <= (yObj+radio) && (yObj-radio) <= (y_+side))
				putObjectToLayer(xObj-x_, yObj-y_, radio, localOut.layersObjectMapSecond[LAYER_BASES_ZONES], 1.0f);
		}
	}
	
	// Minimap.
	for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
	{
		for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
		{
			localOut.layerGlobal[GLOBAL_LAYER_MINIMAP_R][i][j] = minimapIn[i][j][0];
			localOut.layerGlobal[GLOBAL_LAYER_MINIMAP_G][i][j] = minimapIn[i][j][1];
			localOut.layerGlobal[GLOBAL_LAYER_MINIMAP_B][i][j] = minimapIn[i][j][2];
		}
	}
	
	// Local view update.
	int metersByPixelsMinimap = SIDE_MAP_METERS / SIDE_MINIMAP_CONV_V4;
	for(int i=(globalIn.view.x -side/2)/metersByPixelsMinimap; i<(globalIn.view.x +side/2)/metersByPixelsMinimap; i++)
		for(int j=(globalIn.view.y -side/2)/metersByPixelsMinimap; j<(globalIn.view.y +side/2)/metersByPixelsMinimap; j++)
			if(i>=0 && i<SIDE_MINIMAP_CONV_V4 && j>=0 && j<SIDE_MINIMAP_CONV_V4)
				localOut.layerGlobal[GLOBAL_LAYER_VIEWLOCAL_UPDATE][i][j] = 1.0f;
		
	// First local view.
	for(int i=(xView1 -side/2)/metersByPixelsMinimap; i<(xView1 +side/2)/metersByPixelsMinimap; i++)
		for(int j=(yView1 -side/2)/metersByPixelsMinimap; j<(yView1 +side/2)/metersByPixelsMinimap; j++)
			if(i>=0 && i<SIDE_MINIMAP_CONV_V4 && j>=0 && j<SIDE_MINIMAP_CONV_V4)
				localOut.layerGlobal[GLOBAL_LAYER_VIEWLOCAL_FIRST][i][j] = 1.0f;
			
	// Second local view.
	for(int i=(xView2 -side/2)/metersByPixelsMinimap; i<(xView2 +side/2)/metersByPixelsMinimap; i++)
		for(int j=(yView2 -side/2)/metersByPixelsMinimap; j<(yView2 +side/2)/metersByPixelsMinimap; j++)
			if(i>=0 && i<SIDE_MINIMAP_CONV_V4 && j>=0 && j<SIDE_MINIMAP_CONV_V4)
				localOut.layerGlobal[GLOBAL_LAYER_VIEWLOCAL_SECOND][i][j] = 1.0f;
			
	// Current closed.
	for(int i=globalIn.areaCurrentLeft/metersByPixelsMinimap; i<globalIn.areaCurrentRight/metersByPixelsMinimap; i++)
		for(int j=globalIn.areaCurrentBottom/metersByPixelsMinimap; j<globalIn.areaCurrentTop/metersByPixelsMinimap; j++)
			if(i>=0 && i<SIDE_MINIMAP_CONV_V4 && j>=0 && j<SIDE_MINIMAP_CONV_V4)
				localOut.layerGlobal[GLOBAL_LAYER_CURRENTCLOSED][i][j] = 1.0f;
	
	// Future closed.
	for(int i=globalIn.areaFutureLeft/metersByPixelsMinimap; i<globalIn.areaFutureRight/metersByPixelsMinimap; i++)
		for(int j=globalIn.areaFutureBottom/metersByPixelsMinimap; j<globalIn.areaFutureTop/metersByPixelsMinimap; j++)
			if(i>=0 && i<SIDE_MINIMAP_CONV_V4 && j>=0 && j<SIDE_MINIMAP_CONV_V4)
				localOut.layerGlobal[GLOBAL_LAYER_FUTURECLOSED][i][j] = 1.0f;
			
	// Pixels time updated.
	for(int i=0; i<SIDE_MINIMAP_CONV_V4; i++)
		for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
			localOut.layerGlobal[GLOBAL_LAYER_PIXELSTIMEUPDATED][i][j] = (globalIn.timestamp - pixelsTimeUpdatedIn[i][j]) / ((float)MAX_SECONDS_GAME*1000);
	
	// Swap Y, transform game coords to image coords.
//TODO
}

void InOut::standardize(LocalState &state)
{
	state.timestamp /= ((float)MAX_SECONDS_GAME*1000);
	state.minerals /= MAX_MINERALS_RL;
	state.oils /= MAX_OILS_RL;
	state.msToStartToCloseArea /= ((float)MAX_SECONDS_GAME*1000);
	
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			state.layersCurrentFirst[LAYER_LIFE][i][j] /= MAX_LIFE_RL;
			state.layersCurrentFirst[LAYER_CREADA][i][j] /= MAX_LIFE_RL;
			state.layersCurrentSecond[LAYER_LIFE][i][j] /= MAX_LIFE_RL;
			state.layersCurrentSecond[LAYER_CREADA][i][j] /= MAX_LIFE_RL;
		}
	}
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			state.layersAliadesFirst[LAYER_LIFE][i][j] /= MAX_LIFE_RL;
			state.layersAliadesFirst[LAYER_CREADA][i][j] /= MAX_LIFE_RL;
			state.layersAliadesSecond[LAYER_LIFE][i][j] /= MAX_LIFE_RL;
			state.layersAliadesSecond[LAYER_CREADA][i][j] /= MAX_LIFE_RL;
		}
	}
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			state.layersEnemiesFirst[LAYER_LIFE][i][j] /= MAX_LIFE_RL;
			state.layersEnemiesFirst[LAYER_CREADA][i][j] /= MAX_LIFE_RL;
			state.layersEnemiesSecond[LAYER_LIFE][i][j] /= MAX_LIFE_RL;
			state.layersEnemiesSecond[LAYER_CREADA][i][j] /= MAX_LIFE_RL;
		}
	}
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			state.layersObjectMapFirst[LAYER_MINERAL][i][j] /= ((float)MAX_AMOUNT_MINERAL_BY_ONE);
			state.layersObjectMapSecond[LAYER_MINERAL][i][j] /= ((float)MAX_AMOUNT_MINERAL_BY_ONE);
		}
	}
}

void InOut::destandardize(LocalState &state)
{
	state.timestamp *= ((float)MAX_SECONDS_GAME*1000);
	state.minerals *= MAX_MINERALS_RL;
	state.oils *= MAX_OILS_RL;
	state.msToStartToCloseArea *= ((float)MAX_SECONDS_GAME*1000);
	
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			state.layersCurrentFirst[LAYER_LIFE][i][j] *= MAX_LIFE_RL;
			state.layersCurrentFirst[LAYER_CREADA][i][j] *= MAX_LIFE_RL;
			state.layersCurrentSecond[LAYER_LIFE][i][j] *= MAX_LIFE_RL;
			state.layersCurrentSecond[LAYER_CREADA][i][j] *= MAX_LIFE_RL;
		}
	}
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			state.layersAliadesFirst[LAYER_LIFE][i][j] *= MAX_LIFE_RL;
			state.layersAliadesFirst[LAYER_CREADA][i][j] *= MAX_LIFE_RL;
			state.layersAliadesSecond[LAYER_LIFE][i][j] *= MAX_LIFE_RL;
			state.layersAliadesSecond[LAYER_CREADA][i][j] *= MAX_LIFE_RL;
		}
	}
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			state.layersEnemiesFirst[LAYER_LIFE][i][j] *= MAX_LIFE_RL;
			state.layersEnemiesFirst[LAYER_CREADA][i][j] *= MAX_LIFE_RL;
			state.layersEnemiesSecond[LAYER_LIFE][i][j] *= MAX_LIFE_RL;
			state.layersEnemiesSecond[LAYER_CREADA][i][j] *= MAX_LIFE_RL;
		}
	}
	for(int i=0; i<INFO_LAYER_SIDE_PIXELS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			state.layersObjectMapFirst[LAYER_MINERAL][i][j] *= ((float)MAX_AMOUNT_MINERAL_BY_ONE);
			state.layersObjectMapSecond[LAYER_MINERAL][i][j] *= ((float)MAX_AMOUNT_MINERAL_BY_ONE);
		}
	}
}

void InOut::serializeLocalState(LocalState &local, 
								float* localLayersOut,  // CANT_LOCAL_LAYERS x SIDE_LAYERS x SIDE_LAYERS
								float* globalLayersOut) // CANT_MINIMAPS x SIDE_MINIMAP x SIDE_MINIMAP
{
	int posLayer = 0;
	int posArray = 0;
	
	/// Local
	// Current first
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
		{
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				// [posLayer][row][col]
				localLayersOut[posArray] = local.layersCurrentFirst[i][col][SIDE_LAYERS-1-row];
				posArray++;
			}
		}
		posLayer++;
	}
	// Aliades first
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
		{
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				// [posLayer][row][col]
				localLayersOut[posArray] = local.layersAliadesFirst[i][col][SIDE_LAYERS-1-row];
				posArray++;
			}
		}
		posLayer++;
	}
	// Enemies first
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
		{
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				// [posLayer][row][col]
				localLayersOut[posArray] = local.layersEnemiesFirst[i][col][SIDE_LAYERS-1-row];
				posArray++;
			}
		}
		posLayer++;
	}
	// Object map first.
	for(int i=0; i<CANT_INFO_LAYERS_MAP; i++)
	{
		for(int row=0; row<SIDE_LAYERS; row++)
		{
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				// [posLayer][row][col]
				localLayersOut[posArray] = local.layersObjectMapFirst[i][col][SIDE_LAYERS-1-row];
				posArray++;
			}
		}
		posLayer++;
	}
	
	// Current second
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
		{
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				// [posLayer][row][col]
				localLayersOut[posArray] = local.layersCurrentSecond[i][col][SIDE_LAYERS-1-row];
				posArray++;
			}
		}
		posLayer++;
	}
	// Aliades second
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
		{
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				//[posLayer][row][col]
				localLayersOut[posArray] = local.layersAliadesSecond[i][col][SIDE_LAYERS-1-row];
				posArray++;
			}
		}
		posLayer++;
	}
	// Enemies second
	for(int i=0; i<CANT_INFO_LAYERS; i++)  // "base", "barraca", "torreta", "recolector", "soldadoraso", "soldadoentrenado", "tanque", "tanquepesado", "life", "creada", "buildunit", "buildbuilding", "moving", "recollecting", "attacking", "gotoattack"
	{
		for(int row=0; row<SIDE_LAYERS; row++)
		{
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				// [posLayer][row][col]
				localLayersOut[posArray] = local.layersEnemiesSecond[i][col][SIDE_LAYERS-1-row];
				posArray++;
			}
		}
		posLayer++;
	}
	// Object map second.
	for(int i=0; i<CANT_INFO_LAYERS_MAP; i++)
	{
		for(int row=0; row<SIDE_LAYERS; row++)
		{
			for(int col=0; col<SIDE_LAYERS; col++)
			{
				// [posLayer][row][col]
				localLayersOut[posArray] = local.layersObjectMapSecond[i][col][SIDE_LAYERS-1-row];
				posArray++;
			}
		}
		posLayer++;
	}
	
	/// Global
	posArray = 0;
	for(int i=0; i<CANT_LAYERS_GLOBAL; i++) // "minimapR", "minimapG", "minimapB", "update", "first", "second", "currentclosed", "futureclosed", "pixelstimeupdated"
	{
		for(int row=0; row<SIDE_MINIMAP; row++)
			for(int col=0; col<SIDE_MINIMAP; col++)
			{
				//[i][row][col]
				if( i == GLOBAL_LAYER_MINIMAP_R || i == GLOBAL_LAYER_MINIMAP_G || i == GLOBAL_LAYER_MINIMAP_B)
					globalLayersOut[posArray] = local.layerGlobal[i][row][col] / 255.0f;
				else
					globalLayersOut[posArray] = local.layerGlobal[i][col][SIDE_MINIMAP-1-row];
				posArray++;
			}
	}
}



bool InOut::inverse_get(State &stateIn, ClientMessage &msgIn, Action &actionOut)
{
	actionOut.noAction = false;
	actionOut.surrender = false;
	actionOut.update = false;
	actionOut.move = false;
	actionOut.recollect = false;
	actionOut.buildBuilding = false;
	actionOut.buildUnit = false;
	actionOut.attack = false;
	actionOut.cancelAction = false;
	for(int i=0; i<8; i++)
	{
		actionOut.units[i] = 0.0f;
		actionOut.theresUnits[i] = false;
	}
	for(int i=0; i<4; i++)
	{
		actionOut.local[i] = 0.0f;
		actionOut.theresLocal[i] = false;
	}

	
	if(msgIn.surrender_)
	{
		actionOut.surrender = true;
	}
	else if(msgIn.update_)
	{
		actionOut.update = true;
		for(int i=0; i<4; i++)
			actionOut.theresLocal[i] = true;
		posToMapLocal((msgIn.xMin_Update + msgIn.xMax_Update)/2, (msgIn.yMin_Update + msgIn.yMax_Update)/2, 
						actionOut.local[0], actionOut.local[1], actionOut.global[0], actionOut.global[1]);
		posToMapLocal((msgIn.xMin_Update + msgIn.xMax_Update)/2, (msgIn.yMin_Update + msgIn.yMax_Update)/2, 
						actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
	}
	else if(msgIn.intent_)
	{
		vec2 posUnit1, posUnit2, posBuild1, posBuild2, posObjectMap, pos;
		bool hayUnit1=false, hayUnit2=false, hayBuild1=false, hayBuild2=false, hayObjectMap=false, hayPos=false;
		
		ObjectInfo_* info = getInfoObject(msgIn.unitId_Intent);
		if(info != NULL)
		{
			hayUnit1 = true;
			posUnit1 = stateIn.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos;
		}
		info = getInfoObject(msgIn.unit2Id_Intent);
		if(info != NULL)
		{
			hayUnit2 = true;
			posUnit2 = stateIn.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos;
		}
		info = getInfoObject(msgIn.buildId_Intent);
		if(info != NULL)
		{
			hayBuild1 = true;
			posBuild1 = stateIn.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos;
		}
		info = getInfoObject(msgIn.build2Id_Intent);
		if(info != NULL)
		{
			hayBuild2 = true;
			posBuild2 = stateIn.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos;
		}
		info = getInfoObject(msgIn.objectMapId_Intent);
		if(info != NULL)
		{
			hayObjectMap = true;
			posObjectMap = stateIn.objectMap[info->indexMineral].pos;
		}
		
		if(msgIn.pos_Intent.x > 0.0001f && msgIn.pos_Intent.y > 0.0001f)
		{
			hayPos = true;
			pos.x = msgIn.pos_Intent.x;
			pos.y = msgIn.pos_Intent.y;
		}

			
		if(msgIn.type_Intent == AT_MOVE)
		{
			if(!hayUnit1)
				return false;
			
			actionOut.move = true;
			actionOut.theresLocal[0] = true;
			actionOut.theresLocal[1] = true;
			
			posToMapLocal(posUnit1.x, posUnit1.y,  
							actionOut.local[0], actionOut.local[1], actionOut.global[0], actionOut.global[1]);
			
			if(hayUnit2)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(posUnit2.x, posUnit2.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else if(hayBuild2)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(posBuild2.x, posBuild2.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else if(hayObjectMap)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(posObjectMap.x, posObjectMap.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else if(hayPos)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(pos.x, pos.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else
				return false;
		}
		else if(msgIn.type_Intent == AT_RECOLLECT)
		{
			if(!hayUnit1 || !hayObjectMap)
				return false;
			
			actionOut.recollect = true;
			for(int i=0; i<4; i++)
				actionOut.theresLocal[i] = true;
			
			posToMapLocal(posUnit1.x, posUnit1.y, 
							actionOut.local[0], actionOut.local[1], actionOut.global[0], actionOut.global[1]);
			posToMapLocal(posObjectMap.x, posObjectMap.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
		}
		else if(msgIn.type_Intent == AT_BUILD)
		{
			if(!hayUnit1)
				return false;
			
			actionOut.buildBuilding = true;
			actionOut.theresLocal[0] = true;
			actionOut.theresLocal[1] = true;
			
			posToMapLocal(posUnit1.x, posUnit1.y, 
							actionOut.local[0], actionOut.local[1], actionOut.global[0], actionOut.global[1]);

			if(hayBuild2)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(posBuild2.x, posBuild2.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else if(hayPos)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(pos.x, pos.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else
				return false;

			if(msgIn.make_Intent == OT_BASE)
				actionOut.units[LAYER_BASE] = 1.0f;
			else if(msgIn.make_Intent == OT_BARRACA)
				actionOut.units[LAYER_BARRACA] = 1.0f;
			else if(msgIn.make_Intent == OT_TORRETA)
				actionOut.units[LAYER_TORRETA] = 1.0f;
			
			for(int i=0; i<8; i++)
				actionOut.theresUnits[i] = true;
		}
		else if(msgIn.type_Intent == AT_ATACK)
		{
			if(!hayUnit1)
				return false;
			
			actionOut.attack = true;
			actionOut.theresLocal[0] = true;
			actionOut.theresLocal[1] = true;
			
			posToMapLocal(posUnit1.x, posUnit1.y, 
							actionOut.local[0], actionOut.local[1], actionOut.global[0], actionOut.global[1]);
			
			if(hayUnit2)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(posUnit2.x, posUnit2.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else if(hayBuild2)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(posBuild2.x, posBuild2.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else if(hayObjectMap)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(posObjectMap.x, posObjectMap.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else if(hayPos)
			{
				actionOut.theresLocal[2] = true;
				actionOut.theresLocal[3] = true;
				posToMapLocal(pos.x, pos.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			}
			else
				return false;
		}
		else if(msgIn.type_Intent == AT_NEWUNIT)
		{
			if(!hayBuild1)
				return false;
			
			actionOut.buildUnit = true;
			for(int i=0; i<4; i++)
				actionOut.theresLocal[i] = true;
			
			posToMapLocal(posBuild1.x, posBuild1.y, 
							actionOut.local[0], actionOut.local[1], actionOut.global[0], actionOut.global[1]);
			posToMapLocal(posBuild1.x, posBuild1.y, 
							actionOut.local[2], actionOut.local[3], actionOut.global[2], actionOut.global[3]);
			
			if(msgIn.make_Intent == OT_RECOLECTOR)
				actionOut.units[LAYER_RECOLECTOR] = 1.0f;
			else if(msgIn.make_Intent == OT_SOLDADO_RASO)
				actionOut.units[LAYER_SOLDADORASO] = 1.0f;
			else if(msgIn.make_Intent == OT_SOLDADO_ENTRENADO)
				actionOut.units[LAYER_SOLDADOENTRENADO] = 1.0f;
			else if(msgIn.make_Intent == OT_TANQUE)
				actionOut.units[LAYER_TANQUE] = 1.0f;
			else if(msgIn.make_Intent == OT_TANQUE_PESADO)
				actionOut.units[LAYER_TANQUEPESADO] = 1.0f;
			
			for(int i=0; i<8; i++)
				actionOut.theresUnits[i] = true;
		}
	}
	else if(msgIn.cancelAction_)
	{
		actionOut.cancelAction = true;
		
		ObjectInfo_* info = getInfoObject(msgIn.objectId_CancelAction);
		if(info == NULL)
			return false;

		float x, y;
		if(info->indexUnit >= 0)
		{
			x = stateIn.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.x;
			y = stateIn.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.y;
		}
		else if(info->indexBuilding >= 0)
		{
			x = stateIn.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.x;
			y = stateIn.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.y;
		}
		actionOut.theresLocal[0] = true;
		actionOut.theresLocal[1] = true;
		posToMapLocal(x, y, actionOut.local[0], actionOut.local[1], actionOut.global[0], actionOut.global[1]);
	}
	else
		actionOut.noAction = true;
	
	return true;
}

bool InOut::get(State &stateIn, Action &actionIn, ClientMessage* msgOut)
{
	float x1, y1, x2, y2;
	
	vec2 pos;
	pos.x = 0;
	pos.y = 0;
	
	int makeBuild = 0;
	int makeUnit = 0;
	if(actionIn.units[LAYER_BASE] >= actionIn.units[LAYER_BARRACA] && actionIn.units[LAYER_BASE] >= actionIn.units[LAYER_TORRETA])
		makeBuild = OT_BASE;
	else if(actionIn.units[LAYER_BARRACA] >= actionIn.units[LAYER_BASE] && actionIn.units[LAYER_BARRACA] >= actionIn.units[LAYER_TORRETA])
		makeBuild = OT_BARRACA;
	else if(actionIn.units[LAYER_TORRETA] >= actionIn.units[LAYER_BASE] && actionIn.units[LAYER_TORRETA] >= actionIn.units[LAYER_BARRACA])
		makeBuild = OT_TORRETA;
	if(actionIn.units[LAYER_RECOLECTOR] >= actionIn.units[LAYER_SOLDADORASO] && actionIn.units[LAYER_RECOLECTOR] >= actionIn.units[LAYER_SOLDADOENTRENADO] && actionIn.units[LAYER_RECOLECTOR] >= actionIn.units[LAYER_TANQUE] && actionIn.units[LAYER_RECOLECTOR] >= actionIn.units[LAYER_TANQUEPESADO])
		makeUnit = OT_RECOLECTOR;
	else if(actionIn.units[LAYER_SOLDADORASO] >= actionIn.units[LAYER_RECOLECTOR] && actionIn.units[LAYER_SOLDADORASO] >= actionIn.units[LAYER_SOLDADOENTRENADO] && actionIn.units[LAYER_SOLDADORASO] >= actionIn.units[LAYER_TANQUE] && actionIn.units[LAYER_SOLDADORASO] >= actionIn.units[LAYER_TANQUEPESADO])
		makeUnit = OT_SOLDADO_RASO;
	else if(actionIn.units[LAYER_SOLDADOENTRENADO] >= actionIn.units[LAYER_RECOLECTOR] && actionIn.units[LAYER_SOLDADOENTRENADO] >= actionIn.units[LAYER_SOLDADORASO] && actionIn.units[LAYER_SOLDADOENTRENADO] >= actionIn.units[LAYER_TANQUE] && actionIn.units[LAYER_SOLDADOENTRENADO] >= actionIn.units[LAYER_TANQUEPESADO])
		makeUnit = OT_SOLDADO_ENTRENADO;
	else if(actionIn.units[LAYER_TANQUE] >= actionIn.units[LAYER_RECOLECTOR] && actionIn.units[LAYER_TANQUE] >= actionIn.units[LAYER_SOLDADORASO] && actionIn.units[LAYER_TANQUE] >= actionIn.units[LAYER_SOLDADOENTRENADO] && actionIn.units[LAYER_TANQUE] >= actionIn.units[LAYER_TANQUEPESADO])
		makeUnit = OT_TANQUE;
	else if(actionIn.units[LAYER_TANQUEPESADO] >= actionIn.units[LAYER_RECOLECTOR] && actionIn.units[LAYER_TANQUEPESADO] >= actionIn.units[LAYER_SOLDADORASO] && actionIn.units[LAYER_TANQUEPESADO] >= actionIn.units[LAYER_SOLDADOENTRENADO] && actionIn.units[LAYER_TANQUEPESADO] >= actionIn.units[LAYER_TANQUE])
		makeUnit = OT_TANQUE_PESADO;

		
	// Surrender
	if(actionIn.surrender)
	{
printf("ACTION: Surrender");
		msgOut->surrender();
	}
	// Update
	else if(actionIn.update)
	{
		mapLocalToPos(actionIn.local[0], actionIn.local[1],
						actionIn.global[0], actionIn.global[1], 
						x1, y1);
		msgOut->update(x1-ANCHO_UPDATE/2, x1+ANCHO_UPDATE/2, y1-ALTO_UPDATE/2, y1+ALTO_UPDATE/2);
printf("ACTION: update %f %f\n", x1, y1);
	}
	// Intent move
	else if(actionIn.move)
	{
		mapLocalToPos(actionIn.local[0], actionIn.local[1],
						actionIn.global[0], actionIn.global[1], 
						x1, y1);
		mapLocalToPos(actionIn.local[2], actionIn.local[3],
						actionIn.global[2], actionIn.global[3], 
						x2, y2);
		
		unsigned long long unit1 = getMyUnitFromPos(stateIn, x1, y1);
		unsigned long long unit2 = getUnitFromPos(stateIn, x2, y2);
		unsigned long long build2 = getBuildFromPos(stateIn, x2, y2);
		pos.x = x2;
		pos.y = y2;
printf("ACTION: move %llu %llu %llu\n", unit1, unit2, build2);
		if(unit1 > 0)
			msgOut->intent(AT_MOVE, AST_MOVE_MOVE, unit1, unit2, 0, build2, 0, pos, 0);
		else
			return false;
	}
	// Intent recollect
	else if(actionIn.recollect)
	{
		mapLocalToPos(actionIn.local[0], actionIn.local[1],
						actionIn.global[0], actionIn.global[1], 
						x1, y1);
		mapLocalToPos(actionIn.local[2], actionIn.local[3],
						actionIn.global[2], actionIn.global[3], 
						x2, y2);
		
		unsigned long long unit1 = getMyUnitFromPos(stateIn, x1, y1);
		unsigned long long objectMap = getObjectMapFromPos(stateIn, x2, y2);
printf("ACTION: recollect %llu %llu\n", unit1, objectMap);
		if(unit1 > 0)
			msgOut->intent(AT_RECOLLECT, AST_RECOLLECT_GORESOURCE, unit1, 0, 0, 0, objectMap, pos, 0);
		else
			return false;
	}
	// Intent build building
	else if(actionIn.buildBuilding)
	{
		mapLocalToPos(actionIn.local[0], actionIn.local[1],
						actionIn.global[0], actionIn.global[1], 
						x1, y1);
		mapLocalToPos(actionIn.local[2], actionIn.local[3],
						actionIn.global[2], actionIn.global[3], 
						x2, y2);
		
		unsigned long long unit1 = getMyUnitFromPos(stateIn, x1, y1);
		unsigned long long build2 = getBuildFromPos(stateIn, x2, y2);
		pos.x = x2;
		pos.y = y2;
printf("ACTION: buildBuilding %llu %llu\n", unit1, build2);
		if(unit1 > 0)
			msgOut->intent(AT_BUILD, AST_BUILD_GOTOBUILD, unit1, 0, 0, build2, 0, pos, makeBuild);
		else
			return false;
	}
	// Intent build unit
	else if(actionIn.buildUnit)
	{
		mapLocalToPos(actionIn.local[0], actionIn.local[1],
						actionIn.global[0], actionIn.global[1], 
						x1, y1);
		
		unsigned long long build1 = getMyBuildFromPos(stateIn, x1, y1);
printf("ACTION: buildUnit %llu\n", build1);
		if(build1 > 0)
			msgOut->intent(AT_NEWUNIT, AST_NEWUNIT_MAKEUNIT, 0, 0, build1, 0, 0, pos, makeUnit);
		else
			return false;
	}
	// Intent attack
	else if(actionIn.attack)
	{
		mapLocalToPos(actionIn.local[0], actionIn.local[1],
						actionIn.global[0], actionIn.global[1], 
						x1, y1);
		mapLocalToPos(actionIn.local[2], actionIn.local[3],
						actionIn.global[2], actionIn.global[3], 
						x2, y2);
		
		unsigned long long unit1 = getMyUnitFromPos(stateIn, x1, y1);
		unsigned long long unit2 = getUnitFromPos(stateIn, x2, y2);
		unsigned long long build2 = getBuildFromPos(stateIn, x2, y2);
		pos.x = x2;
		pos.y = y2;
printf("ACTION: attack %llu -> %llu %llu\n", unit1, unit2, build2);
		if(unit1 > 0)
			msgOut->intent(AT_ATACK, AST_ATACK_GOTOATACK, unit1, unit2, 0, build2, 0, pos, 0);
		else
			return false;
	}
	
	// CancelAction
	else if(actionIn.cancelAction)
	{
		mapLocalToPos(actionIn.local[0], actionIn.local[1],
						actionIn.global[0], actionIn.global[1], 
						x1, y1);
		
		unsigned long long unit1 = getMyUnitFromPos(stateIn, x1, y1);
		unsigned long long build1 = getMyBuildFromPos(stateIn, x1, y1);
printf("ACTION: cancelAction %llu %llu\n", unit1, build1);
		if(unit1 > 0)
			msgOut->cancelAction(unit1);
		else if(build1 > 0)
			msgOut->cancelAction(build1);
		else
			return false;
	}
	
	// NoAction
	else
	{
printf("ACTION: No Action");
		return false;
	}

	return true;
}

void InOut::setNoAction(vec2 global1In, vec2 global2In, Action &actionOut)
{
	actionOut.noAction = true;
	actionOut.surrender = false;
	actionOut.update = false;
	actionOut.move = false;
	actionOut.recollect = false;
	actionOut.buildBuilding = false;
	actionOut.buildUnit = false;
	actionOut.attack = false;
	actionOut.cancelAction = false;
	for(int i=0; i<8; i++)
	{
		actionOut.units[i] = 0.0f;
		actionOut.theresUnits[i] = false;
	}
	for(int i=0; i<4; i++)
	{
		actionOut.local[i] = 0.0f;
		actionOut.theresLocal[i] = false;
	}

	actionOut.global[0] = global1In.x;
	actionOut.global[1] = global1In.y;
	actionOut.global[2] = global2In.x;
	actionOut.global[3] = global2In.y;
}






void InOut::initPlayer(Player &player)
{
	player.active = false;
	for(int i=0; i<MAX_BUILDINGS_PLAYER; i++)
		initBuilding(player.buildings[i]);
	for(int i=0; i<MAX_UNITS_PLAYER; i++)
		initUnit(player.units[i]);
}
void InOut::initObjectMap(ObjectMap &object)
{
	object.active = false;
	object.visible = false;
	object.updated = 0;
	object.id = 0;
	object.pos.x = 0.0f;
	object.pos.y = 0.0f;
	object.amount = 0;
}
void InOut::initBuilding(Building &building)
{
	building.active = false;
	building.visible = false;
	building.updated = 0;
	building.id = 0;
	building.type = 0;
	building.pos.x = 0.0f;
	building.pos.y = 0.0f;
	building.life = 0;
	building.fullLife = 0;
	building.creada = 0;
	building.construccionCreando = 0;
	building.action = 0;
	building.subAction = 0;
	building.actionMake = 0;
	building.actionUnit2ID = 0;
	building.actionBuild2ID = 0;
	building.actionObjectMap2ID = 0;
}
void InOut::initUnit(Unit &unit)
{
	unit.active = false;
	unit.visible = false;
	unit.updated = 0;
	unit.id = 0;
	unit.type = 0;
	unit.pos.x = 0.0f;
	unit.pos.y = 0.0f;
	unit.life = 0;
	unit.fullLife = 0;
	unit.creada = 0;
	unit.collected = 0;
	
	unit.action = 0;
	unit.subAction = 0;
	unit.actionPos.x = 0.0f;
	unit.actionPos.y = 0.0f;
	unit.actionMake = 0;
	unit.actionUnit2ID = 0;
	unit.actionBuild2ID = 0;
	unit.actionObjectMap2ID = 0;
}




void InOut::initLocalState(LocalState &state)
{
	state.minerals = 0.0f;
	state.oils = 0.0f;
	
	for(int i=0; i<CANT_INFO_LAYERS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			for(int k=0; k<INFO_LAYER_SIDE_PIXELS; k++)
			{
				state.layersCurrentFirst[i][j][k] = 0.0f;
				state.layersAliadesFirst[i][j][k] = 0.0f;
				state.layersEnemiesFirst[i][j][k] = 0.0f;
			}
		}
	}
	for(int i=0; i<CANT_INFO_LAYERS_MAP; i++)
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
			for(int k=0; k<INFO_LAYER_SIDE_PIXELS; k++)
				state.layersObjectMapFirst[i][j][k] = 0.0f;
	
	for(int i=0; i<CANT_INFO_LAYERS; i++)
	{
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
		{
			for(int k=0; k<INFO_LAYER_SIDE_PIXELS; k++)
			{
				state.layersCurrentSecond[i][j][k] = 0.0f;
				state.layersAliadesSecond[i][j][k] = 0.0f;
				state.layersEnemiesSecond[i][j][k] = 0.0f;
			}
		}
	}
	for(int i=0; i<CANT_INFO_LAYERS_MAP; i++)
		for(int j=0; j<INFO_LAYER_SIDE_PIXELS; j++)
			for(int k=0; k<INFO_LAYER_SIDE_PIXELS; k++)
				state.layersObjectMapSecond[i][j][k] = 0.0f;
			
	for(int i=0; i<CANT_LAYERS_GLOBAL; i++)
		for(int j=0; j<SIDE_MINIMAP_CONV_V4; j++)
			for(int k=0; k<SIDE_MINIMAP_CONV_V4; k++)
				state.layerGlobal[i][j][k] = 0.0f;
}

void InOut::putObjectToLayer(int x, int y, int ratio, float (&layers)[INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS], float value)
{
	int x_ = x / INFO_LAYER_METERS_BY_PIXELS;
	int y_ = y / INFO_LAYER_METERS_BY_PIXELS;
	int ratio_ = ratio / INFO_LAYER_METERS_BY_PIXELS;
	if(ratio_ == 0)
		ratio_ = 1;
	
	for(int i=x_-ratio_+1; i<x_+ratio_; i++)
	{
		for(int j=y_-ratio_+1; j<y_+ratio_; j++)
		{
			int diffX = (x_-i);
			int diffY = (y_-j);
			int distpow = diffX*diffX + diffY*diffY;
			if( distpow <= ratio_*ratio_)
			{
				if(i>=0 && i<INFO_LAYER_SIDE_PIXELS && j>=0 && j<INFO_LAYER_SIDE_PIXELS)
					layers[i][j] = value;
			}
		}
	}
}
void InOut::generateLayers(float xView, float yView, Player &player, float (&layers)[CANT_INFO_LAYERS][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS])
{
	int x = xView;
	int y = yView;
	int side = INFO_LAYER_SIDE_PIXELS * INFO_LAYER_METERS_BY_PIXELS;
	
	for(int i=0; i<MAX_BUILDINGS_PLAYER; i++)
	{
		if(player.buildings[i].active)
		{
			int xObj = player.buildings[i].pos.x;
			int yObj = player.buildings[i].pos.y;
			if( x <= xObj && xObj <= (x+side) && y <= yObj && yObj <= (y+side))
			{
				int radio = 0;
				if(player.buildings[i].type == OT_BASE)
				{
					radio = RADIO_BASE;
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_BASE], 1.0f);
				}
				else if(player.buildings[i].type == OT_BARRACA)
				{
					radio = RADIO_BARRACA;
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_BARRACA], 1.0f);
				}
				else if(player.buildings[i].type == OT_TORRETA)
				{
					radio = RADIO_TORRETA;
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_TORRETA], 1.0f);
				}
				
				putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_LIFE], player.buildings[i].life);
				putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_CREADA], player.buildings[i].creada);

				if(player.buildings[i].action == AT_NEWUNIT)
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_CONSTRUYENDO_UNIDAD], 1.0f);
			}
		}
	}
	for(int i=0; i<MAX_UNITS_PLAYER; i++)
	{
		if(player.units[i].active)
		{
			int xObj = player.units[i].pos.x;
			int yObj = player.units[i].pos.y;
			if( x <= xObj && xObj <= (x+side) && y <= yObj && yObj <= (y+side))
			{
				int radio = 0;
				if(player.units[i].type == OT_RECOLECTOR)
				{
					radio = RADIO_RECOLECTOR;
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_RECOLECTOR], 1.0f);
				}
				else if(player.units[i].type == OT_SOLDADO_RASO)
				{
					radio = RADIO_SOLDADORASO;
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_SOLDADORASO], 1.0f);
				}
				else if(player.units[i].type == OT_SOLDADO_ENTRENADO)
				{
					radio = RADIO_SOLDADOENTRENADO;
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_SOLDADOENTRENADO], 1.0f);
				}
				else if(player.units[i].type == OT_TANQUE)
				{
					radio = RADIO_TANQUE;
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_TANQUE], 1.0f);
				}
				else if(player.units[i].type == OT_TANQUE_PESADO)
				{
					radio = RADIO_TANQUEPESADO;
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_TANQUEPESADO], 1.0f);
				}
				
				putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_LIFE], player.units[i].life);
				putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_CREADA], player.units[i].creada);
				
				if(player.units[i].action == AT_BUILD)
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_CONSTRUYENDO_BUILDING], 1.0f);
				else if(player.units[i].action == AT_MOVE)
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_MOVIENDOSE], 1.0f);
				else if(player.units[i].action == AT_RECOLLECT)
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_RECOLECTANDO], 1.0f);
				else if(player.units[i].action == AT_ATACK && player.units[i].subAction == AST_ATACK_ATACKING)
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_ATACANDO], 1.0f);
				else if(player.units[i].action == AT_ATACK && player.units[i].subAction == AST_ATACK_GOTOATACK)
					putObjectToLayer(xObj-x, yObj-y, radio, layers[LAYER_YENDO_A_ATACAR], 1.0f);
			}
		}
	}
}







void InOut::mapLocalToPos(float xLocal, float yLocal, float xGlobal, float yGlobal, float &x, float &y)
{
	x = xGlobal + (xLocal - (INFO_LAYER_SIDE_PIXELS/2)*INFO_LAYER_METERS_BY_PIXELS);
	y = yGlobal + (yLocal - (INFO_LAYER_SIDE_PIXELS/2)*INFO_LAYER_METERS_BY_PIXELS);
}

void InOut::posToMapLocal(float x, float y, float &xLocal, float &yLocal, float &xGlobal, float &yGlobal)
{
	xGlobal = x;
	yGlobal = y;
	xLocal = INFO_LAYER_SIDE_PIXELS/2;
	yLocal = INFO_LAYER_SIDE_PIXELS/2;
}

		
		





InOut::PlayerInfo_* InOut::getInfoPlayer(char* name)
{
	string nameS = string(name);
	if(this->playersInfo.find(nameS) != this->playersInfo.end())
		return this->playersInfo[name];
	else
		return NULL;
}

InOut::PlayerInfo_* InOut::insertPlayer(char* name, int equip)
{
	string nameS = string(name);
	
	PlayerInfo_* info = new PlayerInfo_;
	
	if(myEquip == 0)
		info->indexEquip = equip;
	else if(myEquip == 1)
	{
		if(equip == 0)
			info->indexEquip = 1;
		else if(equip == 1)
			info->indexEquip = 0;
		else if(equip == 2)
			info->indexEquip = 2;
	}
	else if(myEquip == 2)
	{
		if(equip == 0)
			info->indexEquip = 1;
		else if(equip == 1)
			info->indexEquip = 2;
		else if(equip == 2)
			info->indexEquip = 0;
	}
	
	info->indexPlayer = 0;
	while(playerIndexOccuped[info->indexEquip][info->indexPlayer] != NULL)
		info->indexPlayer++;
	playerIndexOccuped[info->indexEquip][info->indexPlayer] = info;
	
	this->playersInfo[nameS] = info;
	
	return info;
}

void InOut::removePlayer(char* name)
{
	string nameS = string(name);
	PlayerInfo_* info = playersInfo[nameS];
	playerIndexOccuped[info->indexEquip][info->indexPlayer] = NULL;
	this->playersInfo.erase(nameS);
	delete info;
}

InOut::ObjectInfo_* InOut::insertObject(unsigned long long id, char* player, int type)
{
	PlayerInfo_* pinfo = NULL;
	
	if(strcmp(player, "")!=0)
	{
		string nameS = string(player);
		pinfo = playersInfo[nameS];
		if(pinfo == NULL)
			return NULL;
	}
		
	ObjectInfo_* info = new ObjectInfo_;
	info->id = id;
	
	if(pinfo == NULL)
	{
		info->indexEquip = -1;
		info->indexPlayer = -1;
	}
	else
	{
		info->indexEquip = pinfo->indexEquip;
		info->indexPlayer = pinfo->indexPlayer;
	}
	info->indexUnit = -1;
	info->indexBuilding = -1;
	info->indexMineral = -1;

	if(type == OT_RECOLECTOR || type == OT_SOLDADO_RASO || type == OT_SOLDADO_ENTRENADO || type == OT_TANQUE || type == OT_TANQUE_PESADO)
	{
		info->indexUnit = 0;
		while(unitsIndexOccuped[info->indexEquip][info->indexPlayer][info->indexUnit] != NULL)
			info->indexUnit++;
		unitsIndexOccuped[info->indexEquip][info->indexPlayer][info->indexUnit] = info;
	}
	else if(type == OT_BASE || type == OT_BARRACA || type == OT_TORRETA)
	{
		info->indexBuilding = 0;
		while(buildingsIndexOccuped[info->indexEquip][info->indexPlayer][info->indexBuilding] != NULL)
			info->indexBuilding++;
		buildingsIndexOccuped[info->indexEquip][info->indexPlayer][info->indexBuilding] = info;
	}
	else if(type == OT_MINERAL)
	{
		info->indexMineral = 0;
		while(mineralsIndexOccuped[info->indexMineral] != NULL)
			info->indexMineral++;
		mineralsIndexOccuped[info->indexMineral] = info;
	}
	else
	{
		delete info;
		return NULL;
	}
		
	
	this->objectsInfo[id] = info;
	
	return info;
}

InOut::ObjectInfo_* InOut::getInfoObject(unsigned long long id)
{
	if(this->objectsInfo.find(id) != this->objectsInfo.end())
		return this->objectsInfo[id];
	else
		return NULL;
}

void InOut::removeObject(unsigned long long id)
{
	ObjectInfo_* info = objectsInfo[id];
	
	if(info->indexUnit >= 0)
		unitsIndexOccuped[info->indexEquip][info->indexPlayer][info->indexUnit] = NULL;
	if(info->indexBuilding >= 0)
		buildingsIndexOccuped[info->indexEquip][info->indexPlayer][info->indexBuilding] = NULL;
	if(info->indexMineral >= 0)
		mineralsIndexOccuped[info->indexMineral] = NULL;
	
	this->objectsInfo.erase(id);
	delete info;
}




unsigned long long InOut::getMyUnitFromPos(State &state, float x, float y)
{
	float nearDist = 99999999;
	int indexUnit = -1;
	for(int i=0; i<MAX_UNITS_PLAYER; i++)
	{
		if(state.players[0][0].units[i].active)
		{
			float deltaX = state.players[0][0].units[i].pos.x - x;
			float deltaY = state.players[0][0].units[i].pos.y - y;
			float dist = deltaX*deltaX + deltaY*deltaY;
			if( dist < nearDist && dist < DIST_MAX_TO_CONSIDER_NEAR_PICK_OBJECT)
			{
				nearDist = dist;
				indexUnit = i;
			}
		}
	}
	
	if(indexUnit < 0)
		return 0;
	else
		return unitsIndexOccuped[0][0][indexUnit]->id;
}
unsigned long long InOut::getMyBuildFromPos(State &state, float x, float y)
{
	float nearDist = 99999999;
	int indexBuilding = -1;
	for(int i=0; i<MAX_BUILDINGS_PLAYER; i++)
	{
		if(state.players[0][0].buildings[i].active)
		{
			float deltaX = state.players[0][0].buildings[i].pos.x - x;
			float deltaY = state.players[0][0].buildings[i].pos.y - y;
			float dist = deltaX*deltaX + deltaY*deltaY;
			if( dist < nearDist && dist < DIST_MAX_TO_CONSIDER_NEAR_PICK_OBJECT)
			{
				nearDist = dist;
				indexBuilding = i;
			}
		}
	}
	
	if(indexBuilding < 0)
		return 0;
	else
		return buildingsIndexOccuped[0][0][indexBuilding]->id;
}
unsigned long long InOut::getUnitFromPos(State &state, float x, float y)
{
	float nearDist = 99999999;
	int indexEquip = -1;
	int indexPlayer = -1;
	int indexUnit = -1;
	for(int i=0; i<EQUIPS; i++)
	{
		for(int j=0; j<MAX_PLAYERS_BY_EQUIP; j++)
		{
			if(state.players[i][j].active)
			{
				for(int k=0; k<MAX_UNITS_PLAYER; k++)
				{
					if(state.players[i][j].units[k].active)
					{
						float deltaX = state.players[i][j].units[k].pos.x - x;
						float deltaY = state.players[i][j].units[k].pos.y - y;
						float dist = deltaX*deltaX + deltaY*deltaY;
						if( dist < nearDist && dist < DIST_MAX_TO_CONSIDER_NEAR_PICK_OBJECT)
						{
							nearDist = dist;
							indexEquip = i;
							indexPlayer = j;
							indexUnit = k;
						}
					}
				}
			}
		}
	}
	
	if(indexUnit < 0)
		return 0;
	else
		return unitsIndexOccuped[indexEquip][indexPlayer][indexUnit]->id;
}
unsigned long long InOut::getBuildFromPos(State &state, float x, float y)
{
	float nearDist = 99999999;
	int indexEquip = -1;
	int indexPlayer = -1;
	int indexBuilding = -1;
	for(int i=0; i<EQUIPS; i++)
	{
		for(int j=0; j<MAX_PLAYERS_BY_EQUIP; j++)
		{
			if(state.players[i][j].active)
			{
				for(int k=0; k<MAX_BUILDINGS_PLAYER; k++)
				{
					if(state.players[i][j].buildings[k].active)
					{
						float deltaX = state.players[i][j].buildings[k].pos.x - x;
						float deltaY = state.players[i][j].buildings[k].pos.y - y;
						float dist = deltaX*deltaX + deltaY*deltaY;
						if( dist < nearDist && dist < DIST_MAX_TO_CONSIDER_NEAR_PICK_OBJECT)
						{
							nearDist = dist;
							indexEquip = i;
							indexPlayer = j;
							indexBuilding = k;
						}
					}
				}
			}
		}
	}
	
	if(indexBuilding < 0)
		return 0;
	else
		return buildingsIndexOccuped[indexEquip][indexPlayer][indexBuilding]->id;
}
unsigned long long InOut::getObjectMapFromPos(State &state, float x, float y)
{
	float nearDist = 99999999;
	int indexObjectMap = -1;
	for(int i=0; i<MAX_MINERALS_MAP; i++)
	{
		if(state.objectMap[i].active)
		{
			float deltaX = state.objectMap[i].pos.x - x;
			float deltaY = state.objectMap[i].pos.y - y;
			float dist = deltaX*deltaX + deltaY*deltaY;
			if( dist < nearDist && dist < DIST_MAX_TO_CONSIDER_NEAR_PICK_OBJECT)
			{
				nearDist = dist;
				indexObjectMap = i;
			}
		}
	}
	
	if(indexObjectMap < 0)
		return 0;
	else
		return mineralsIndexOccuped[indexObjectMap]->id;
}












