
#include "inout.h"

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
void InOut::initPlayer(Player &player)
{
	player.active = false;
	for(int i=0; i<MAX_BUILDINGS_PLAYER; i++)
		initBuilding(player.buildings[i]);
	for(int i=0; i<MAX_UNITS_PLAYER; i++)
		initUnit(player.units[i]);
}
void InOut::initState(State &state)
{
	state.areaIsClosing = false;
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

void InOut::put(unsigned long long ts, ServerMessage* msg, State &state)
{
	state.timestamp = ts;
	
	if(msg->newPlayerCurrent)
	{
		state.players[0][0].active = true;
		state.view.x = msg->xView_NewPlayerCurrent;
		state.view.y = msg->yView_NewPlayerCurrent;
		
		myEquip = msg->equip_NewPlayerCurrent;
		strcpy(myPlayerName, msg->playerName_NewPlayerCurrent);
		
		PlayerInfo_* info = insertPlayer(msg->playerName_NewPlayerCurrent, msg->equip_NewPlayerCurrent);
	}
	else if(msg->newPlayer)
	{
		// Only if i'm not.
		if(strcmp(myPlayerName, msg->playerName_NewPlayer) != 0)
		{
			PlayerInfo_* info = insertPlayer(msg->playerName_NewPlayer, msg->equip_NewPlayer);
		
			state.players[info->indexEquip][info->indexPlayer].active = true;
		}
	}
	else if(msg->removePlayer)
	{
		PlayerInfo_* info = getInfoPlayer(msg->playerName_RemovePlayer);
		
		initPlayer(state.players[info->indexEquip][info->indexPlayer]);
		
		removePlayer(msg->playerName_RemovePlayer);
	}
	else if(msg->delete_)
	{
		ObjectInfo_* info = getInfoObject(msg->id_Delete);

		if(msg->destroyed_Delete)
		{
			if(info->indexMineral >= 0)
				initObjectMap(state.objectMap[info->indexMineral]);
			else
			{
				if(info->indexUnit >= 0)
					initUnit(state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit]);
				if(info->indexBuilding >= 0)
					initBuilding(state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding]);
			}
		
			removeObject(msg->id_Delete);
		}
		else
		{
			if(info->indexMineral >= 0)
				state.objectMap[info->indexMineral].visible = false;
			else
			{
				if(info->indexUnit >= 0)
					state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].visible = false;
				if(info->indexBuilding >= 0)
					state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].visible = false;
			}
		}
	}
	else if(msg->resources)
	{
		state.minerals = msg->mineral_Resources;
		state.oils = msg->oil_Resources;
	}
	else if(msg->new_)
	{
		ObjectInfo_* info = getInfoObject(msg->id_New);
		// Is really new object
		if(info == NULL)
		{
			info = insertObject(msg->id_New, msg->player_New, msg->type_New);
		}

		if(info->indexMineral >= 0)
		{
			state.objectMap[info->indexMineral].active = true;
			state.objectMap[info->indexMineral].visible = true;
			state.objectMap[info->indexMineral].updated = Time::currentMs();
			state.objectMap[info->indexMineral].id = msg->id_New;
			state.objectMap[info->indexMineral].pos.x = msg->pos_New.x;
			state.objectMap[info->indexMineral].pos.y = msg->pos_New.y;
			state.objectMap[info->indexMineral].amount = msg->amount_New;
		}
		else
		{
			if(info->indexUnit >= 0)
			{
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].active = true;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].visible = true;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].updated = Time::currentMs();
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].id = msg->id_New;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].type = msg->type_New;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.x = msg->pos_New.x;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.y = msg->pos_New.y;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].life = msg->life_New;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].fullLife = msg->fullLife_New;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].creada = msg->creada_New;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].collected = msg->collected_New;
			}
			else if(info->indexBuilding >= 0)
			{
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].active = true;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].visible = true;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].updated = Time::currentMs();
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].id = msg->id_New;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].type = msg->type_New;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.x = msg->pos_New.x;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.y = msg->pos_New.y;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].life = msg->life_New;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].fullLife = msg->fullLife_New;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].creada = msg->creada_New;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].construccionCreando = msg->construccionCreando_New;
			}
		}
	}
	else if(msg->change)
	{
		ObjectInfo_* info = getInfoObject(msg->id_Change);

		if(info->indexMineral >= 0)
		{
			state.objectMap[info->indexMineral].updated = Time::currentMs();
			if(msg->change_pos)
			{
				state.objectMap[info->indexMineral].pos.x = msg->pos_Change.x;
				state.objectMap[info->indexMineral].pos.y = msg->pos_Change.y;
			}
			if(msg->change_amount)
				state.objectMap[info->indexMineral].amount = msg->amount_Change;
		}
		else
		{
			if(info->indexUnit >= 0)
			{
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].updated = Time::currentMs();
				if(msg->change_pos)
				{
					state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.x = msg->pos_Change.x;
					state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].pos.y = msg->pos_Change.y;
				}
				if(msg->change_life)
					state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].life = msg->life_Change;
				if(msg->change_fullLife)
					state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].fullLife = msg->fullLife_Change;
				if(msg->change_creada)
					state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].creada = msg->creada_Change;
				if(msg->change_collected)
					state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].collected = msg->collected_Change;
			}
			else if(info->indexBuilding >= 0)
			{
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].updated = Time::currentMs();
				if(msg->change_pos)
				{
					state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.x = msg->pos_Change.x;
					state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].pos.y = msg->pos_Change.y;
				}
				if(msg->change_life)
					state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].life = msg->life_Change;
				if(msg->change_fullLife)
					state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].fullLife = msg->fullLife_Change;
				if(msg->change_creada)
					state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].creada = msg->creada_Change;
				if(msg->change_construccionCreando)
					state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].construccionCreando = msg->construccionCreando_Change;
			}
		}

		if(msg->deleteAction_Change || msg->newAction_Change)
		{
			if(info->indexUnit >= 0)
			{
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].updated = Time::currentMs();
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].action = 0;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].subAction = 0;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionPos.x = 0.0f;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionPos.y = 0.0f;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionMake = 0;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionUnit2ID = 0;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionBuild2ID = 0;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionObjectMap2ID = 0;
			}
			else if(info->indexBuilding >= 0)
			{
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].updated = Time::currentMs();
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].action = 0;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].subAction = 0;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionMake = 0;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionUnit2ID = 0;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionBuild2ID = 0;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionObjectMap2ID = 0;
			}
		}
		
		if(msg->newAction_Change)
		{
			if(info->indexUnit >= 0)
			{
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].updated = Time::currentMs();
				
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].action = msg->actionType_NewAction;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].subAction = msg->actionSubType_NewAction;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionPos.x = msg->actionPos_NewAction.x;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionPos.y = msg->actionPos_NewAction.y;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionMake = msg->actionMake_NewAction;
				
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionUnit2ID = msg->actionUnit2ID_NewAction;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionBuild2ID = msg->actionBuild2ID_NewAction;
				state.players[info->indexEquip][info->indexPlayer].units[info->indexUnit].actionObjectMap2ID = msg->actionObjectMapID_NewAction;
			}
			else if(info->indexBuilding >= 0)
			{
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].updated = Time::currentMs();
				
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].action = msg->actionType_NewAction;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].subAction = msg->actionSubType_NewAction;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionMake = msg->actionMake_NewAction;
				
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionUnit2ID = msg->actionUnit2ID_NewAction;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionBuild2ID = msg->actionBuild2ID_NewAction;
				state.players[info->indexEquip][info->indexPlayer].buildings[info->indexBuilding].actionObjectMap2ID = msg->actionObjectMapID_NewAction;
			}
		}
	}
	else if(msg->basesZones)
	{
		for(int i=0; i<msg->amountBasesZones; i++)
		{
			// Add
			if(msg->isAddBasesZones[i])
			{
				int e = msg->equipBasesZones[i];
				state.basesZones[e][ state.amountBasesZones[e] ].pos = msg->posBasesZones[i];
				state.basesZones[e][ state.amountBasesZones[e] ].radio = msg->radioBasesZones[i];
				state.amountBasesZones[e]++;
			}
			// Del
			else
			{
				// Find the zone.
				bool foundIt = false;
				int e2=0, i2=0;
				while(e2<EQUIPS)
				{
					while(i2<state.amountBasesZones[e2])
					{
						if(state.basesZones[e2][i2].pos.dist(msg->posBasesZones[i]) < state.basesZones[e2][i2].radio)
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
					for(int i3=i2+1; i3<state.amountBasesZones[e2]; i3++)
						state.basesZones[e2][i3-1] = state.basesZones[e2][i3];
					state.amountBasesZones[e2]--;
				}
			}
		}
	}
	else if(msg->area)
	{
		state.areaIsClosing = msg->isClosing_Area;
		state.areaFutureCenterX = msg->futureCenter_Area.x;
		state.areaFutureCenterY = msg->futureCenter_Area.y;
		state.areaCurrentBottom = msg->currentBottom_Area;
		state.areaCurrentTop = msg->currentTop_Area;
		state.areaCurrentLeft = msg->currentLeft_Area;
		state.areaCurrentRight = msg->currentRight_Area;
		state.areaFutureBottom = msg->futureBottom_Area;
		state.areaFutureTop = msg->futureTop_Area;
		state.areaFutureLeft = msg->futureLeft_Area;
		state.areaFutureRight = msg->futureRight_Area;
		state.areaSpeedCloseSecBottom = msg->speedCloseSecBottom_Area;
		state.areaSpeedCloseSecTop = msg->speedCloseSecTop_Area;
		state.areaSpeedCloseSecLeft = msg->speedCloseSecLeft_Area;
		state.areaSpeedCloseSecRight = msg->speedCloseSecRight_Area;
	}
}

list<InOut::InfoObject> InOut::getObjectByType(State &state, int type, bool mine, bool aliades, bool enemies)
{
	list<InfoObject> ret;
	
	unordered_map<unsigned long long, ObjectInfo_*>::iterator it = objectsInfo.begin();
	while(it != objectsInfo.end())
	{
		InfoObject info;
		if(it->second->indexEquip < 0)
		{
			if(type == OT_MINERAL || type == OT_ALL)
			{
				info.unit = NULL;
				info.building = NULL;
				info.mineral = &(state.objectMap[it->second->indexMineral]);
				
				info.type = OT_MINERAL;
				info.pos.x = info.mineral->pos.x;
				info.pos.y = info.mineral->pos.y;
				info.equip = -1;
				info.player = -1;
				info.index = it->second->indexMineral;
				info.id = it->first;
				
				ret.push_back(info);
			}
		}
		else if( (mine && it->second->indexEquip == 0 && it->second->indexPlayer == 0) || (aliades && it->second->indexEquip == 0) || (enemies && (it->second->indexEquip == 1 || it->second->indexEquip == 2) ))
		{
			int indexEquip = it->second->indexEquip;
			int indexPlayer = it->second->indexPlayer;
			
			if(type == OT_BASE || type == OT_BARRACA || type == OT_TORRETA || type == OT_ALL)
			{
				info.unit = NULL;
				info.building = &(state.players[indexEquip][indexPlayer].buildings[it->second->indexBuilding]);
				info.mineral = NULL;
				
				if(info.building->type == type || type == OT_ALL)
				{
					info.type = info.building->type;
					info.pos.x = info.building->pos.x;
					info.pos.y = info.building->pos.y;
					info.equip = indexEquip;
					info.player = indexPlayer;
					info.index = it->second->indexBuilding;
					info.id = it->first;
					ret.push_back(info);
				}
			}
			else if(type == OT_RECOLECTOR || type == OT_SOLDADO_RASO || type == OT_SOLDADO_ENTRENADO || type == OT_TANQUE || type == OT_TANQUE_PESADO || type == OT_ALL)
			{
				info.unit = &(state.players[indexEquip][indexPlayer].units[it->second->indexUnit]);
				info.building = NULL;
				info.mineral = NULL;
				
				if(info.unit->type == type || type == OT_ALL)
				{
					info.type = info.unit->type;
					info.pos.x = info.unit->pos.x;
					info.pos.y = info.unit->pos.y;
					info.equip = indexEquip;
					info.player = indexPlayer;
					info.index = it->second->indexUnit;
					info.id = it->first;
					ret.push_back(info);
				}
			}
		}
		it++;
	}
	
	return ret;
}

list<InOut::InfoObject> InOut::getObjectByTypeAndArea(State &state, int type, vec2 pos, float ratio, bool mine, bool aliades, bool enemies)
{
	list<InfoObject> ret = getObjectByType(state, type, mine, aliades, enemies);
	
	list<InfoObject>::iterator it = ret.begin();
	while(it != ret.end())
	{
		if(pos.dist( it->pos) > ratio)
			it = ret.erase(it);
		else
			it++;
	}
	
	return ret;
}

void InOut::setObjectsUpdated(State &state, float xmin, float xmax, float ymin, float ymax)
{
	unsigned long long now = Time::currentMs();
	for(int i=0; i<MAX_UNITS_PLAYER; i++)
	{
		if(state.players[0][0].units[i].active)
		{
			float x = state.players[0][0].units[i].pos.x;
			float y = state.players[0][0].units[i].pos.y;
			if(xmin <= x && x <= xmax && ymin <= y && y <= ymax)
				state.players[0][0].units[i].updated = now;
		}
	}
	for(int i=0; i<MAX_BUILDINGS_PLAYER; i++)
	{
		if(state.players[0][0].buildings[i].active)
		{
			float x = state.players[0][0].buildings[i].pos.x;
			float y = state.players[0][0].buildings[i].pos.y;
			if(xmin <= x && x <= xmax && ymin <= y && y <= ymax)
				state.players[0][0].buildings[i].updated = now;
		}
	}
}

void InOut::getUnUpdatestObject(State &state, InfoObject &unupdatestObject, int &msAgo)
{
	unsigned long long oldestTs = Time::currentMs();
	for(int i=0; i<MAX_UNITS_PLAYER; i++)
	{
		if(state.players[0][0].units[i].active && state.players[0][0].units[i].updated < oldestTs)
		{
			oldestTs = state.players[0][0].units[i].updated;
			unupdatestObject.unit = &(state.players[0][0].units[i]);
			unupdatestObject.building = NULL;
			unupdatestObject.mineral = NULL;
			unupdatestObject.type = unupdatestObject.unit->type;
			unupdatestObject.pos = unupdatestObject.unit->pos;
			unupdatestObject.id = unupdatestObject.unit->id;
			unupdatestObject.equip = 0;
			unupdatestObject.player = 0;
			unupdatestObject.index = i;
		}
	}
	for(int i=0; i<MAX_BUILDINGS_PLAYER; i++)
	{
		if(state.players[0][0].buildings[i].active && state.players[0][0].buildings[i].updated < oldestTs)
		{
			oldestTs = state.players[0][0].buildings[i].updated;
			unupdatestObject.unit = NULL;
			unupdatestObject.building = &(state.players[0][0].buildings[i]);
			unupdatestObject.mineral = NULL;
			unupdatestObject.type = unupdatestObject.building->type;
			unupdatestObject.pos = unupdatestObject.building->pos;
			unupdatestObject.id = unupdatestObject.building->id;
			unupdatestObject.equip = 0;
			unupdatestObject.player = 0;
			unupdatestObject.index = i;
		}
	}
	
	msAgo = Time::currentMs() - oldestTs;
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
		if(state.players[0][0].units[i].active > 0.5f)
		{
			float deltaX = state.players[0][0].units[i].pos.x - x;
			float deltaY = state.players[0][0].units[i].pos.y - y;
			float dist = deltaX*deltaX + deltaY*deltaY;
			if( dist < nearDist)
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
		if(state.players[0][0].buildings[i].active > 0.5f)
		{
			float deltaX = state.players[0][0].buildings[i].pos.x - x;
			float deltaY = state.players[0][0].buildings[i].pos.y - y;
			float dist = deltaX*deltaX + deltaY*deltaY;
			if( dist < nearDist)
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
			if(state.players[i][j].active > 0.5f)
			{
				for(int k=0; k<MAX_UNITS_PLAYER; k++)
				{
					if(state.players[i][j].units[k].active > 0.5f)
					{
						float deltaX = state.players[i][j].units[k].pos.x - x;
						float deltaY = state.players[i][j].units[k].pos.y - y;
						float dist = deltaX*deltaX + deltaY*deltaY;
						if( dist < nearDist)
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
			if(state.players[i][j].active > 0.5f)
			{
				for(int k=0; k<MAX_BUILDINGS_PLAYER; k++)
				{
					if(state.players[i][j].buildings[k].active > 0.5f)
					{
						float deltaX = state.players[i][j].buildings[k].pos.x - x;
						float deltaY = state.players[i][j].buildings[k].pos.y - y;
						float dist = deltaX*deltaX + deltaY*deltaY;
						if( dist < nearDist)
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
		if(state.objectMap[i].active > 0.5f)
		{
			float deltaX = state.objectMap[i].pos.x - x;
			float deltaY = state.objectMap[i].pos.y - y;
			float dist = deltaX*deltaX + deltaY*deltaY;
			if( dist < nearDist)
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












