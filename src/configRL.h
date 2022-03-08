#ifndef _configRL_h_
#define _configRL_h_


#define SIDE_MINIMAP_CONV_V2 	128
#define SIDE_MINIMAP_CONV_V3 	256
#define SIDE_MINIMAP_CONV_V4 	256

#define MAX_MINERALS_RL	 		50000.0f
#define MAX_OILS_RL 			50000.0f
#define MAX_LIFE_RL 			500.0f

#define ANCHO_UPDATE 400
#define ALTO_UPDATE 400


#define SIDE_MINIMAP 256
#define CANT_MINIMAPS 9
#define SIDE_LAYERS 128
#define CANT_LAYERS_PLAYERS 16
#define CANT_INFO_LAYERS_MAP 2
#define CANT_LOCAL_LAYERS ((3*CANT_LAYERS_PLAYERS + CANT_INFO_LAYERS_MAP) * 2)

#define CANT_ACTIONS 9
#define CANT_UNITS 8

// When the agent want to select an object by a position then find the near object.
// But if the object is farther than this define then not select anything
#define DIST_MAX_TO_CONSIDER_NEAR_PICK_OBJECT 300

// I define a probably max second to the end of the game.
#define MAX_SECONDS_GAME (32*60)

#define MS_AGENT_STEP 200

#define MS_SLEEP_LOOP 20

#endif
