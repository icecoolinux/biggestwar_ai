
#include "ServerMessage.h"

ServerMessage::ServerMessage()
{
	clear();
}

ServerMessage::~ServerMessage()
{
	if(pngData_MiniMap != NULL)
		delete[] pngData_MiniMap;
}

void ServerMessage::clear()
{
	pngData_MiniMap = NULL;
		
	login = false;
	config = false;
	basesZones = false;
	newPlayerWorld = false;
	newPlayerCurrent = false;
	newPlayer = false;
	removePlayer = false;
	delete_ = false;
	resources = false;
	new_ = false;
	change = false;
	change_pos = false; change_life = false; change_fullLife = false; change_creada = false; change_construccionCreando = false; change_collected = false; change_amount = false;
	deleteAction_Change = false;
	newAction_Change = false;
	area = false;
	endGame = false;
	error = false;
	login_Error = false;
	loginGame_Error = false;
	play_Error = false;
	surrender_Error = false;
	notEnoughMineral_Error = false;
	miniMap = false;
}

int ServerMessage::set(char* msgS, int len)
{
	char command[200];
	int pos = 0;
	
	clear();
	
	Net::getString(msgS, &command[0], pos);
	
	if(strcmp(command,"login")==0)
	{
		login = true;

		Net::getInt(msgS, wins_Login, pos);
		Net::getInt(msgS, lost_Login, pos);
		Net::getInt(msgS, points_Login, pos);
		
		char gameLogged[100];
		Net::getString(msgS, &gameLogged[0], pos);
		gameLogged_Login = strcmp(gameLogged, "true")==0;
		
		Net::getString(msgS, &uri_Login[0], pos);
		Net::getString(msgS, &token_Login[0], pos);
		
		Net::getInt(msgS, amountResults_Login, pos);

		for(int i=0; i<amountResults_Login; i++)
		{
			int equip, rank, points;
			Net::getInt(msgS, equip, pos);
			Net::getInt(msgS, rank, pos);
			Net::getInt(msgS, points, pos);
			
			char stringTemp[20];
			Net::getString(msgS, &stringTemp[0], pos);
			bool surrender = strcmp(stringTemp,"true")==0;
			Net::getString(msgS, &stringTemp[0], pos);
			bool teamWin = strcmp(stringTemp,"true")==0;
			Net::getString(msgS, &stringTemp[0], pos);
			bool winToEnd = strcmp(stringTemp,"true")==0;
			if(i<20)
			{
				results[i].equip = equip;
				results[i].rank = rank;
				results[i].points = points;
				results[i].surrender = surrender;
				results[i].teamWin = teamWin;
				results[i].winToEnd = winToEnd;
			}
		}
	}
	else if(strcmp(command,"config")==0)
	{
		config = true;
		
		char buf[200];
		float f;
		int d;
		while(msgS[pos-1] != ';')
		{
			Net::getString(msgS, &buf[0], pos);
			if(strcmp(buf, "radio_mineral")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "radio_base")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "radio_barraca")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "radio_torreta")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "radio_recolector")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "radio_soldadoraso")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "radio_soldadoentrenado")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "radio_tanque")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "radio_tanquepesado")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "max_amount_recollect_recolector")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "visibility_distance")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "vel_recolector")==0)
				Net::getFloat(msgS, f, pos);
			else if(strcmp(buf, "full_life_base")==0)
				Net::getInt(msgS, d, pos);
			else if(strcmp(buf, "full_life_barraca")==0)
				Net::getInt(msgS, d, pos);
			else if(strcmp(buf, "full_life_torreta")==0)
				Net::getInt(msgS, d, pos);
			else if(strcmp(buf, "full_life_recolector")==0)
				Net::getInt(msgS, d, pos);
			else if(strcmp(buf, "full_life_soldadoraso")==0)
				Net::getInt(msgS, d, pos);
			else if(strcmp(buf, "full_life_soldadoentrenado")==0)
				Net::getInt(msgS, d, pos);
			else if(strcmp(buf, "full_life_tanque")==0)
				Net::getInt(msgS, d, pos);
			else if(strcmp(buf, "full_life_tanquepesado")==0)
				Net::getInt(msgS, d, pos);
		}
	}
	else if(strcmp(command,"baseszones")==0)
	{
		basesZones = true;
		
		Net::getInt(msgS, amountBasesZones, pos);

		for(int i=0; i<amountBasesZones; i++)
		{
			int valueInt;
			char subCommand[20];
			
			Net::getString(msgS, &subCommand[0], pos);
			
			if(strcmp(subCommand, "add")==0)
			{
				isAddBasesZones[i] = true;
				Net::getInt(msgS, equipBasesZones[i], pos);

				Net::getInt(msgS, valueInt, pos);
				posBasesZones[i].x = valueInt;
				
				Net::getInt(msgS, valueInt, pos);
				posBasesZones[i].y = valueInt;
				
				Net::getInt(msgS, radioBasesZones[i], pos);
			}
			// Delete
			else
			{
				isAddBasesZones[i] = false;
				
				Net::getInt(msgS, valueInt, pos);
				posBasesZones[i].x = valueInt;
				
				Net::getInt(msgS, valueInt, pos);
				posBasesZones[i].y = valueInt;
			}
		}
	}
	else if(strcmp(command,"newplayerworld")==0)
	{
		newPlayerWorld = true;

		Net::getInt(msgS, worldNumber_NewPlayer, pos);
		Net::getString(msgS, &uri_NewPlayer[0], pos);
		Net::getInt(msgS, equip_NewPlayerWorld, pos);
		Net::getString(msgS, &token_NewPlayerWorld[0], pos);
	}
	else if(strcmp(command,"newplayercurrent")==0)
	{
		newPlayerCurrent = true;
		
		Net::getString(msgS, &playerName_NewPlayerCurrent[0], pos);
		Net::getInt(msgS, equip_NewPlayerCurrent, pos);
		Net::getInt(msgS, xView_NewPlayerCurrent, pos);
		Net::getInt(msgS, yView_NewPlayerCurrent, pos);
	}
	else if(strcmp(command,"newplayer")==0)
	{
		newPlayer = true;

		Net::getString(msgS, &playerName_NewPlayer[0], pos);
		Net::getInt(msgS, equip_NewPlayer, pos);
	}
	else if(strcmp(command,"removeplayer")==0)
	{
		removePlayer = true;
		
		Net::getString(msgS, &playerName_RemovePlayer[0], pos);
	}
	else if(strcmp(command,"delete")==0)
	{
		delete_ = true;
		
		Net::getString(msgS, &command[0], pos);
		if(strcmp(command,"destroyed")==0)
			destroyed_Delete = true;
		else
			destroyed_Delete = false;
		
		Net::getLong(msgS, id_Delete, pos);
	}
	else if(strcmp(command,"resources")==0)
	{
		resources = true;

		Net::getInt(msgS, mineral_Resources, pos);
		Net::getInt(msgS, oil_Resources, pos);
	}
	else if(strcmp(command,"new")==0)
	{
		new_ = true;
		
		Net::getLong(msgS, id_New, pos);
		
		char buf[1000];
		float f;
		int d;
		while(msgS[pos-1] != ';')
		{
			Net::getString(msgS, &buf[0], pos);
	
			if(strcmp(buf, "player")==0)
				Net::getString(msgS, &player_New[0], pos);
			else if(strcmp(buf, "type")==0)
				Net::getInt(msgS, type_New, pos);
			else if(strcmp(buf, "pos")==0)
			{
				Net::getFloat(msgS, pos_New.x, pos);
				Net::getFloat(msgS, pos_New.y, pos);
			}
			else if(strcmp(buf, "life")==0)
				Net::getInt(msgS, life_New, pos);
			else if(strcmp(buf, "fulllife")==0)
				Net::getInt(msgS, fullLife_New, pos);
			else if(strcmp(buf, "creada")==0)
				Net::getInt(msgS, creada_New, pos);
			else if(strcmp(buf, "construccioncreando")==0)
				Net::getInt(msgS, construccionCreando_New, pos);
			else if(strcmp(buf, "collected")==0)
				Net::getInt(msgS, collected_New, pos);
			else if(strcmp(buf, "amount")==0)
				Net::getInt(msgS, amount_New, pos);
		}
	}
	else if(strcmp(command,"change")==0)
	{
		change = true;
		
		Net::getLong(msgS, id_Change, pos);
		
		char buf[1000];
		float f;
		int d;
		while(msgS[pos-1] != ';')
		{
			Net::getString(msgS, &buf[0], pos);
			
			if(strcmp(buf, "pos")==0)
			{
				change_pos = true;
				Net::getFloat(msgS, pos_Change.x, pos);
				Net::getFloat(msgS, pos_Change.y, pos);
			}
			else if(strcmp(buf, "life")==0)
			{
				change_life = true;
				Net::getInt(msgS, life_Change, pos);
			}
			else if(strcmp(buf, "fulllife")==0)
			{
				change_fullLife = true;
				Net::getInt(msgS, fullLife_Change, pos);
			}
			else if(strcmp(buf, "creada")==0)
			{
				change_creada = true;
				Net::getInt(msgS, creada_Change, pos);
			}
			else if(strcmp(buf, "construccioncreando")==0)
			{
				change_construccionCreando = true;
				Net::getInt(msgS, construccionCreando_Change, pos);
			}
			else if(strcmp(buf, "collected")==0)
			{
				change_collected = true;
				Net::getInt(msgS, collected_Change, pos);
			}
			else if(strcmp(buf, "amount")==0)
			{
				change_amount = true;
				Net::getInt(msgS, amount_Change, pos);
			}
			else if(strcmp(buf, "deleteaction")==0)
				deleteAction_Change = true;
			else if(strcmp(buf, "newaction")==0)
			{
				newAction_Change = true;
				
				while(msgS[pos-1] != ';')
				{
					Net::getString(msgS, &buf[0], pos);
					
					if(strcmp(buf, "actiontype")==0)
						Net::getInt(msgS, actionType_NewAction, pos);
					else if(strcmp(buf, "actionsubtype")==0)
						Net::getInt(msgS, actionSubType_NewAction, pos);
					else if(strcmp(buf, "actionunitid")==0)
						Net::getLong(msgS, actionUnitID_NewAction, pos);
					else if(strcmp(buf, "actionunit2id")==0)
						Net::getLong(msgS, actionUnit2ID_NewAction, pos);
					else if(strcmp(buf, "actionbuildid")==0)
						Net::getLong(msgS, actionBuildID_NewAction, pos);
					else if(strcmp(buf, "actionbuild2id")==0)
						Net::getLong(msgS, actionBuild2ID_NewAction, pos);
					else if(strcmp(buf, "actionobjectmapid")==0)
						Net::getLong(msgS, actionObjectMapID_NewAction, pos);
					else if(strcmp(buf, "actionpos")==0)
					{
						Net::getFloat(msgS, actionPos_NewAction.x, pos);
						Net::getFloat(msgS, actionPos_NewAction.y, pos);
					}
					else if(strcmp(buf, "actionmake")==0)
						Net::getInt(msgS, actionMake_NewAction, pos);
				}
			}
		}
	}
	else if(strcmp(command,"area")==0)
	{
		area = true;
		
		int temp;
		Net::getInt(msgS, temp, pos);
		isClosing_Area = temp==1;
		Net::getInt(msgS, msToClose_Area, pos);
		
		Net::getInt(msgS, temp, pos);
		futureCenter_Area.x = temp;
		Net::getInt(msgS, temp, pos);
		futureCenter_Area.y = temp;
		
		Net::getInt(msgS, currentBottom_Area, pos);
		Net::getInt(msgS, currentTop_Area, pos);
		Net::getInt(msgS, currentLeft_Area, pos);
		Net::getInt(msgS, currentRight_Area, pos);
		Net::getInt(msgS, futureBottom_Area, pos);
		Net::getInt(msgS, futureTop_Area, pos);
		Net::getInt(msgS, futureLeft_Area, pos);
		Net::getInt(msgS, futureRight_Area, pos);
		Net::getFloat(msgS, speedCloseSecBottom_Area, pos);
		Net::getFloat(msgS, speedCloseSecTop_Area, pos);
		Net::getFloat(msgS, speedCloseSecLeft_Area, pos);
		Net::getFloat(msgS, speedCloseSecRight_Area, pos);
	}
	else if(strcmp(command,"endgame")==0)
	{
		endGame = true;
		
		char stringTemp[20];
		Net::getString(msgS, &stringTemp[0], pos);
		surrender_EndGame = strcmp(stringTemp,"true")==0;
		
		Net::getString(msgS, &stringTemp[0], pos);
		win_EndGame = strcmp(stringTemp,"true")==0;
		
		Net::getString(msgS, &stringTemp[0], pos);
		lose_EndGame = strcmp(stringTemp,"true")==0;
		
		Net::getString(msgS, &stringTemp[0], pos);
		gameFinish_EndGame = strcmp(stringTemp,"true")==0;
		
		Net::getInt(msgS, rankEquip_EndGame, pos);
	}
	else if(strcmp(command,"error")==0)
	{
		error = true;

		Net::getString(msgS, &command[0], pos);
		if(strcmp(command,"login")==0)
			login_Error = true;
		else if(strcmp(command,"play")==0)
			play_Error = true;
		else if(strcmp(command,"login_game")==0)
			loginGame_Error = true;
		else if(strcmp(command,"newplayer")==0)
			newPlayer_Error = true;
		else if(strcmp(command,"surrender")==0)
			surrender_Error = true;
		else if(strcmp(command,"not_enough_mineral")==0)
			notEnoughMineral_Error = true;
	}
	else if(strcmp(command,"minimap")==0)
	{
		miniMap = true;
		lenPngData = len-strlen("minimap,");
		pngData_MiniMap = new char[lenPngData];
		for(int i=0; i<lenPngData; i++)
		{
			pngData_MiniMap[i] = msgS[pos];
			pos++;
		}
	}
	
	return pos;
}



