#ifndef _ServerMessage_h_
#define _ServerMessage_h_

class ServerMessage;

#include "../../../server/src/config.h"
#include "net.h"
#include "vec2.h"
#include <string.h>

using namespace std;

class ServerMessage
{
	private:
		
	public:
		ServerMessage();
		~ServerMessage();
		
		void clear();
		
		
		bool login;
		int wins_Login;
		int lost_Login;
		int points_Login;
		bool gameLogged_Login;
		char uri_Login[LEN_URI+1];
		char token_Login[LEN_TOKEN+1];
		int amountResults_Login;
		struct {
			int equip;
			int rank;
			int points;
			bool surrender;
			bool teamWin;
			bool winToEnd;
		} results[20];
		
		
		bool config;
		
		bool basesZones;
		int amountBasesZones;
		bool isAddBasesZones[EQUIPS*MAX_PLAYERS_BY_EQUIP];
		int equipBasesZones[EQUIPS*MAX_PLAYERS_BY_EQUIP];
		vec2 posBasesZones[EQUIPS*MAX_PLAYERS_BY_EQUIP];
		int radioBasesZones[EQUIPS*MAX_PLAYERS_BY_EQUIP];
		
		bool newPlayerWorld;
		int worldNumber_NewPlayer;
		char uri_NewPlayer[LEN_URI+1];
		int equip_NewPlayerWorld;
		char token_NewPlayerWorld[LEN_TOKEN+1];

		bool newPlayerCurrent;
		char playerName_NewPlayerCurrent[LEN_NAME+1];
		int equip_NewPlayerCurrent;
		int xView_NewPlayerCurrent;
		int yView_NewPlayerCurrent;
		
		bool newPlayer;
		char playerName_NewPlayer[LEN_NAME+1];
		int equip_NewPlayer;
		
		bool removePlayer;
		char playerName_RemovePlayer[LEN_NAME+1];
		
		bool delete_;
		bool destroyed_Delete;
		unsigned long long id_Delete;
		
		bool resources;
		int mineral_Resources;
		int oil_Resources;
		
		bool new_;
		unsigned long long id_New;
		char player_New[LEN_NAME+1];
		int type_New;
		vec2 pos_New;
		int life_New;
		int fullLife_New;
		int creada_New;
		int construccionCreando_New;
		int collected_New;
		int amount_New;
		
		bool change;
		unsigned long long id_Change;
		vec2 pos_Change; bool change_pos;
		int life_Change; bool change_life;
		int fullLife_Change; bool change_fullLife;
		int creada_Change; bool change_creada;
		int construccionCreando_Change; bool change_construccionCreando;
		int collected_Change; bool change_collected;
		int amount_Change; bool change_amount;
		bool deleteAction_Change;
		bool newAction_Change;
		int actionType_NewAction;
		int actionSubType_NewAction;
		unsigned long long actionUnitID_NewAction;
		unsigned long long actionUnit2ID_NewAction;
		unsigned long long actionBuildID_NewAction;
		unsigned long long actionBuild2ID_NewAction;
		unsigned long long actionObjectMapID_NewAction;
		vec2 actionPos_NewAction;
		int actionMake_NewAction;

		bool area;
		bool isClosing_Area;
		int msToClose_Area;
		vec2 futureCenter_Area;
		int currentBottom_Area, currentTop_Area, currentLeft_Area, currentRight_Area;
		int futureBottom_Area, futureTop_Area, futureLeft_Area, futureRight_Area;
		float speedCloseSecBottom_Area, speedCloseSecTop_Area, speedCloseSecLeft_Area, speedCloseSecRight_Area;
		
		bool endGame;
		bool surrender_EndGame;
		bool win_EndGame;
		bool lose_EndGame;
		bool gameFinish_EndGame;
		int rankEquip_EndGame;
		
		bool error;
		bool login_Error;
		bool play_Error;
		bool loginGame_Error;
		bool newPlayer_Error;
		bool surrender_Error;
		bool notEnoughMineral_Error;
		
		bool miniMap;
		char* pngData_MiniMap;
		int lenPngData;
		
		
		
		
		int set(char* msgS, int len);
};

#endif
