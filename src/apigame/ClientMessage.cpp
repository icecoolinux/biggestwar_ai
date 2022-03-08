
#include "ClientMessage.h"


ClientMessage::ClientMessage()
{
	clear();
}

ClientMessage::~ClientMessage()
{
}

void ClientMessage::clear()
{
	login_ = false;
	loginGame_ = false;
	play_ = false;
	surrender_ = false;
	update_ = false;
	intent_ = false;
	cancelAction_ = false;
	miniMap_ = false;
	selection_ = false;
}


int ClientMessage::set(char* msgS, int len)
{
	char command[200];
	int pos = 0;
	
	clear();
	
	Net::getString(msgS, &command[0], pos);
	
	if(strcmp(command,"login")==0)
	{
		login_ = true;
		Net::getString(msgS, &username_Login[0], pos);
		Net::getString(msgS, &pass_Login[0], pos);
	}
	else if(strcmp(command,"login_game")==0)
	{
		loginGame_ = true;
		Net::getString(msgS, &username_LoginGame[0], pos);
		Net::getString(msgS, &token_LoginGame[0], pos);
	}
	else if(strcmp(command,"play")==0)
	{
		play_ = true;
	}
	else if(strcmp(command,"surrender")==0)
	{
		surrender_ = true;
	}
	else if(strcmp(command,"update")==0)
	{
		update_ = true;
		Net::getInt(msgS, xMin_Update, pos);
		Net::getInt(msgS, xMax_Update, pos);
		Net::getInt(msgS, yMin_Update, pos);
		Net::getInt(msgS, yMax_Update, pos);
	}
	else if(strcmp(command,"intent")==0)
	{
		intent_ = true;
		Net::getInt(msgS, type_Intent, pos);
		Net::getInt(msgS, typeType_Intent, pos);
		Net::getLong(msgS, unitId_Intent, pos);
		Net::getLong(msgS, unit2Id_Intent, pos);
		Net::getLong(msgS, buildId_Intent, pos);
		Net::getLong(msgS, build2Id_Intent, pos);
		Net::getLong(msgS, objectMapId_Intent, pos);
		Net::getFloat(msgS, pos_Intent.x, pos);
		Net::getFloat(msgS, pos_Intent.y, pos);
		Net::getInt(msgS, make_Intent, pos);
	}
	else if(strcmp(command,"cancel_action")==0)
	{
		cancelAction_ = true;
		Net::getLong(msgS, objectId_CancelAction, pos);
	}
	else if(strcmp(command,"minimap")==0)
	{
		miniMap_ = true;
		Net::getInt(msgS, res_MiniMap, pos);
		Net::getInt(msgS, zoom_MiniMap, pos);
		Net::getFloat(msgS, posWorld_MiniMap.x, pos);
		Net::getFloat(msgS, posWorld_MiniMap.y, pos);
	}
	else if(strcmp(command,"selection")==0)
	{
		selection_ = true;
		Net::getInt(msgS, cantSelection, pos);
		for(int i=0; i<cantSelection; i++)
			Net::getLong(msgS, idSelection[i], pos);
	}
	
	return pos;
}


std::string ClientMessage::get()
{
	char buf[3000];

	if(login_)
		sprintf(buf, "login,%s,%s;", username_Login, pass_Login);
	else if(loginGame_)
		sprintf(buf, "login_game,%s,%s;", username_LoginGame, token_LoginGame);
	else if(play_)
		strcpy(buf, "play;");
	else if(surrender_)
		strcpy(buf, "surrender;");
	else if(update_)
		sprintf(buf, "update,%d,%d,%d,%d;", xMin_Update, xMax_Update, yMin_Update, yMax_Update);
	else if(intent_)
		sprintf(buf, "intent,%d,%d,%llu,%llu,%llu,%llu,%llu,%f,%f,%d;", type_Intent, typeType_Intent, unitId_Intent, unit2Id_Intent, buildId_Intent, build2Id_Intent, objectMapId_Intent, pos_Intent.x, pos_Intent.y, make_Intent);
	else if(cancelAction_)
		sprintf(buf, "cancel_action,%llu;", objectId_CancelAction);
	else if(miniMap_)
		sprintf(buf, "minimap,%d,%d,%f,%f;", res_MiniMap, zoom_MiniMap, posWorld_MiniMap.x, posWorld_MiniMap.y);
	
	string s = buf;
	return s;
}

void ClientMessage::login(char* name, char* pass)
{
	clear();
	login_ = true;
	strcpy(username_Login, name);
	strcpy(pass_Login, pass);
}

void ClientMessage::loginGame(char* user, char* token)
{
	clear();
	loginGame_ = true;
	strcpy(username_LoginGame, user);
	strcpy(token_LoginGame, token);
}

void ClientMessage::play()
{
	clear();
	play_ = true;
}

void ClientMessage::surrender()
{
	clear();
	surrender_ = true;
}

void ClientMessage::update(int xMin, int xMax, int yMin, int yMax)
{
	clear();
	update_ = true;
	xMin_Update = xMin;
	xMax_Update = xMax;
	yMin_Update = yMin;
	yMax_Update = yMax;
}

void ClientMessage::intent(int type, int typeType, unsigned long long unitId, unsigned long long unit2Id, unsigned long long buildId, unsigned long long build2Id, unsigned long long objectMapId, vec2 pos, int make)
{
	clear();
	intent_ = true;
	type_Intent = type;
	typeType_Intent = typeType;
	unitId_Intent = unitId;
	unit2Id_Intent = unit2Id;
	buildId_Intent = buildId;
	build2Id_Intent = build2Id;
	objectMapId_Intent = objectMapId;
	pos_Intent = pos;
	make_Intent = make;
}

void ClientMessage::cancelAction(unsigned long long objectId)
{
	clear();
	cancelAction_ = true;
	objectId_CancelAction = objectId;
}

void ClientMessage::miniMap(int res, int zoom, vec2 posWorld)
{
	clear();
	miniMap_ = true;
	res_MiniMap = res;
	zoom_MiniMap = zoom;
	posWorld_MiniMap = posWorld;
}
