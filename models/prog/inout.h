#ifndef _inout_h_
#define _inout_h_

#include "../../../server/src/config.h"
#include "../../../server/src/defs.h"

#include "../../src/apigame/ServerMessage.h"
#include "../../src/apigame/ClientMessage.h"
#include "../../src/configRL.h"
#include "../../src/apigame/img.h"

#include <unordered_map>


class InOut
{
	public:
		
		struct Building
		{
			bool active;
			bool visible;
			unsigned long long updated;
			unsigned long long id;
			
			int type;
			vec2 pos;
			int life;
			int fullLife;
			int creada;
			int construccionCreando;
			
			int action;
			int subAction;
			int actionMake;
			
			int actionUnit2ID;
			int actionBuild2ID;
			int actionObjectMap2ID;
		};
		struct Unit
		{
			bool active;
			bool visible;
			unsigned long long updated;
			unsigned long long id;
			
			int type;
			vec2 pos;
			int life;
			int fullLife;
			int creada;
			int collected;
			
			int action;
			int subAction;
			vec2 actionPos;
			int actionMake;
			
			int actionUnit2ID;
			int actionBuild2ID;
			int actionObjectMap2ID;
		};
		struct Player
		{
			bool active;
			Building buildings[MAX_BUILDINGS_PLAYER];
			Unit units[MAX_UNITS_PLAYER];
		};
		struct ObjectMap
		{
			bool active;
			bool visible;
			unsigned long long updated;
			unsigned long long id;
			
			vec2 pos;
			int amount;
		};
		struct BaseZone
		{
			vec2 pos;
			float radio;
		};
		struct State
		{
			// Area
			bool areaIsClosing;
			int areaFutureCenterX;
			int areaFutureCenterY;
			int areaCurrentBottom;
			int areaCurrentTop;
			int areaCurrentLeft;
			int areaCurrentRight;
			int areaFutureBottom;
			int areaFutureTop;
			int areaFutureLeft;
			int areaFutureRight;
			float areaSpeedCloseSecBottom;
			float areaSpeedCloseSecTop;
			float areaSpeedCloseSecLeft;
			float areaSpeedCloseSecRight;
			
			int amountBasesZones[EQUIPS];
			BaseZone basesZones[EQUIPS][MAX_PLAYERS_BY_EQUIP];
			
			// Player scalars
			unsigned long long timestamp;
			vec2 view;
			int minerals;
			int oils;

			// Players, 2.340.300
			// First equips is aliade, first player is me.
			Player players[EQUIPS][MAX_PLAYERS_BY_EQUIP];
			
			// Minerals, 120.000
			ObjectMap objectMap[MAX_MINERALS_MAP];
		};
		
		struct InfoObject
		{
			InOut::Unit* unit;
			InOut::Building* building;
			InOut::ObjectMap* mineral;
			
			int type;
			vec2 pos;
			unsigned long long id;
			int equip;
			int player;
			int index;
		};
		
		
		InOut();
		~InOut();

		void initState(State &state);
		void copyState(State &src, State &dst);
		void put(unsigned long long ts, ServerMessage* msg, State &state);

		
		list<InfoObject> getObjectByType(State &state, int type, bool mine, bool aliades, bool enemies);
		list<InfoObject> getObjectByTypeAndArea(State &state, int type, vec2 pos, float ratio, bool mine, bool aliades, bool enemies);
		void setObjectsUpdated(State &state, float xmin, float xmax, float ymin, float ymax);
		void getUnUpdatestObject(State &state, InfoObject &unupdatestObject, int &msAgo);
		
	private:
		struct PlayerInfo_{
			int indexEquip;
			int indexPlayer;
		};
		struct ObjectInfo_{
			unsigned long long id;
			int indexEquip;
			int indexPlayer;
			int indexUnit;
			int indexBuilding;
			int indexMineral;
		};
		
		int myEquip;
		char myPlayerName[LEN_NAME+1];
		
		unordered_map<string, PlayerInfo_*> playersInfo;
		unordered_map<unsigned long long, ObjectInfo_*> objectsInfo;
		
		PlayerInfo_* playerIndexOccuped[EQUIPS][MAX_PLAYERS_BY_EQUIP];
		ObjectInfo_* unitsIndexOccuped[EQUIPS][MAX_PLAYERS_BY_EQUIP][MAX_UNITS_PLAYER];
		ObjectInfo_* buildingsIndexOccuped[EQUIPS][MAX_PLAYERS_BY_EQUIP][MAX_BUILDINGS_PLAYER];
		ObjectInfo_* mineralsIndexOccuped[MAX_MINERALS_MAP];
		
		
		PlayerInfo_* getInfoPlayer(char* name);
		PlayerInfo_* insertPlayer(char* name, int equip);
		void removePlayer(char* name);

		ObjectInfo_* insertObject(unsigned long long id, char* player, int type);
		ObjectInfo_* getInfoObject(unsigned long long id);
		void removeObject(unsigned long long id);
		
		
		
		
		void initPlayer(Player &player);
		void initObjectMap(ObjectMap &object);
		void initBuilding(Building &building);
		void initUnit(Unit &unit);

		
		unsigned long long getMyUnitFromPos(State &state, float x, float y);
		unsigned long long getMyBuildFromPos(State &state, float x, float y);
		unsigned long long getUnitFromPos(State &state, float x, float y);
		unsigned long long getBuildFromPos(State &state, float x, float y);
		unsigned long long getObjectMapFromPos(State &state, float x, float y);

};
#endif
