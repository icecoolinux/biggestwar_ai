#ifndef _inout_h_
#define _inout_h_

#include "../../../server/src/config.h"
#include "../../../server/src/defs.h"
#include "ServerMessage.h"
#include "ClientMessage.h"
#include "../configRL.h"
#include "img.h"

#include <unordered_map>

#define LAYER_BASE 0
#define LAYER_BARRACA 1
#define LAYER_TORRETA 2
#define LAYER_RECOLECTOR 3
#define LAYER_SOLDADORASO 4
#define LAYER_SOLDADOENTRENADO 5
#define LAYER_TANQUE 6
#define LAYER_TANQUEPESADO 7
#define LAYER_LIFE 8
#define LAYER_CREADA 9
#define LAYER_CONSTRUYENDO_UNIDAD 10
#define LAYER_CONSTRUYENDO_BUILDING 11
#define LAYER_MOVIENDOSE 12
#define LAYER_RECOLECTANDO 13
#define LAYER_ATACANDO 14
#define LAYER_YENDO_A_ATACAR 15

#define CANT_INFO_LAYERS 16

#define LAYER_MINERAL 0
#define LAYER_BASES_ZONES 1
#define CANT_INFO_LAYERS_MAP 2

#define INFO_LAYER_SIDE_PIXELS 128
#define INFO_LAYER_METERS_BY_PIXELS 3

#define GLOBAL_LAYER_MINIMAP_R 0
#define GLOBAL_LAYER_MINIMAP_G 1
#define GLOBAL_LAYER_MINIMAP_B 2
#define GLOBAL_LAYER_VIEWLOCAL_UPDATE 3
#define GLOBAL_LAYER_VIEWLOCAL_FIRST 4
#define GLOBAL_LAYER_VIEWLOCAL_SECOND 5
#define GLOBAL_LAYER_CURRENTCLOSED 6
#define GLOBAL_LAYER_FUTURECLOSED 7
#define GLOBAL_LAYER_PIXELSTIMEUPDATED 8
#define CANT_LAYERS_GLOBAL 9

extern char* NAMES_LAYERS[];
extern char* NAMES_LAYERS_OBJECTMAP[];
extern char* NAMES_LAYERS_GLOBALS[];


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
			unsigned long long areaStartToCloseTs;
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
		
		
		struct LocalState
		{
			float timestamp;
			float minerals;
			float oils;
			float isClosingArea;
			float msToStartToCloseArea;
			
			// Use the cartesian coords: [x][y]
			float layersCurrentFirst[CANT_INFO_LAYERS][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS];
			float layersAliadesFirst[CANT_INFO_LAYERS][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS];
			float layersEnemiesFirst[CANT_INFO_LAYERS][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS];
			float layersObjectMapFirst[CANT_INFO_LAYERS_MAP][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS];
			
			float layersCurrentSecond[CANT_INFO_LAYERS][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS];
			float layersAliadesSecond[CANT_INFO_LAYERS][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS];
			float layersEnemiesSecond[CANT_INFO_LAYERS][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS];
			float layersObjectMapSecond[CANT_INFO_LAYERS_MAP][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS];
			
			float layerGlobal[CANT_LAYERS_GLOBAL][SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4];
		};
		
		struct Action 
		{
			bool noAction;
			bool surrender;
			bool update;
			bool move;
			bool recollect;
			bool buildBuilding;
			bool buildUnit;
			bool attack;
			bool cancelAction;
			
			float units[8];
			float local[4];
			float global[4];
			
			bool theresUnits[8];
			bool theresLocal[4];
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

		bool isEnemy(char* playerName);
		
		void initState(State &state);
		void copyState(State &src, State &dst);
		bool put(unsigned long long tsIn, ServerMessage* msgIn, State &stateOut, float (&pixelsTimeUpdatedOut)[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4]);
		
		void getLocalState(State &globalIn, 
							float (&minimapIn)[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4][3], 
							float (&pixelsTimeUpdatedIn)[SIDE_MINIMAP_CONV_V4][SIDE_MINIMAP_CONV_V4], 
							vec2 global1In, vec2 global2In, 
							LocalState &localOut);
		void standardize(LocalState &state);
		void destandardize(LocalState &state);
		static void serializeLocalState(LocalState &local, 
										float* localLayersOut, // CANT_LOCAL_LAYERS x SIDE_LAYERS x SIDE_LAYERS
										float* globalLayersOut); // CANT_MINIMAPS x SIDE_MINIMAP x SIDE_MINIMAP
		
		bool inverse_get(State &stateIn, ClientMessage &msgIn, Action &actionOut);
		bool get(State &stateIn, Action &actionIn, ClientMessage* msgOut); // Return false if no action.
		void setNoAction(vec2 global1In, vec2 global2In, Action &actionOut);
		
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
		
		
		void initLocalState(LocalState &state);
		void putObjectToLayer(int x, int y, int ratio, float (&layers)[INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS], float value);
		void generateLayers(float xView, float yView, Player &player, float (&layers)[CANT_INFO_LAYERS][INFO_LAYER_SIDE_PIXELS][INFO_LAYER_SIDE_PIXELS]);
		
		unsigned long long getMyUnitFromPos(State &state, float x, float y);
		unsigned long long getMyBuildFromPos(State &state, float x, float y);
		unsigned long long getUnitFromPos(State &state, float x, float y);
		unsigned long long getBuildFromPos(State &state, float x, float y);
		unsigned long long getObjectMapFromPos(State &state, float x, float y);

		void mapLocalToPos(float xLocal, float yLocal, float xGlobal, float yGlobal, float &x, float &y);
		void posToMapLocal(float x, float y, float &xLocal, float &yLocal, float &xGlobal, float &yGlobal);
};
#endif
