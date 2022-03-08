
#include "agent.h"

#define MAX_RECOLLECTORS 30
#define SECONDS_TO_UPDATE_MY_OBJECTS (((2.0f*VISIBILITY_DISTANCE) / VEL_RECOLECTOR )*0.8f)

Agent::Agent(char* username_)
{
	strcpy(this->username, username_);
	inout = new InOut();
	this->state = new InOut::State;
	inout->initState(*state);
	thereIsCurrentPlayer = false;
	didFirstUpdate = false;
}

Agent::~Agent()
{
	delete inout;
	delete state;
}

void Agent::init()
{
	this->tsStart = Time::currentMs();
}

void Agent::set(ServerMessage *msg)
{
	// Is minimap, save it.
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
	}

	// Update state.
	inout->put(Time::currentMs()-this->tsStart, msg, *state);
	
	if(msg->newPlayerCurrent)
		thereIsCurrentPlayer = true;
}

int Agent::step(ClientMessage* msgs, int max)
{
	
if(DEBUG)
	printf("STEP\n");

	int ret = 0;

	// Set curretTime to updated to current view objects.
	inout->setObjectsUpdated(*state, state->view.x-ANCHO_UPDATE/2, state->view.x+ANCHO_UPDATE/2, state->view.y-ALTO_UPDATE/2, state->view.y+ALTO_UPDATE/2);

	// Check update.
	if(thereIsCurrentPlayer && !didFirstUpdate)
	{
		didFirstUpdate = true;
		msgs[ret].update(state->view.x-ANCHO_UPDATE/2, state->view.x+ANCHO_UPDATE/2, state->view.y-ALTO_UPDATE/2, state->view.y+ALTO_UPDATE/2);
		ret++;
		return ret;
	}

	/// UPDATE MY OBJECTS
	// There is my object that not update 5 seconds ago.
	InOut::InfoObject unupdatestObject;
	int msAgo;
	inout->getUnUpdatestObject(*state, unupdatestObject, msAgo);
	if(msAgo > SECONDS_TO_UPDATE_MY_OBJECTS*1000.0f)
	{
		state->view.x = unupdatestObject.pos.x;
		state->view.y = unupdatestObject.pos.y;
		
		// Set curretTime to updated to current view objects.
		inout->setObjectsUpdated(*state, state->view.x-ANCHO_UPDATE/2, state->view.x+ANCHO_UPDATE/2, state->view.y-ALTO_UPDATE/2, state->view.y+ALTO_UPDATE/2);
	
		msgs[ret].update(state->view.x-ANCHO_UPDATE/2, state->view.x+ANCHO_UPDATE/2, state->view.y-ALTO_UPDATE/2, state->view.y+ALTO_UPDATE/2);
		ret++;
		return ret;
	}

	
	
	vec2 pos_;
	list<InOut::InfoObject>::iterator it;
	list<InOut::InfoObject>::iterator it2;
	list<InOut::InfoObject>::iterator it3;

	// Get and make some calcs.
	list<InOut::InfoObject> bases = inout->getObjectByType(*state, OT_BASE, true, false, false);
	list<InOut::InfoObject> recollects = inout->getObjectByType(*state, OT_RECOLECTOR, true, false, false);
	list<InOut::InfoObject> barracas = inout->getObjectByType(*state, OT_BARRACA, true, false, false);
	list<InOut::InfoObject> soldiers;
	list<InOut::InfoObject> mineralsBases[MAX_BUILDINGS_PLAYER];
	list<InOut::InfoObject> recollectsBases[MAX_BUILDINGS_PLAYER];
	int basesWithMineralsNear = 0;
	int mineralsNearBases[MAX_BUILDINGS_PLAYER];
	int recollectsNearBases[MAX_BUILDINGS_PLAYER];
	int recollectsRecollectingNearBases[MAX_BUILDINGS_PLAYER];
	vec2 knowNearCenterPos = vec2(999999, 999999);
	vec2 centerMap = vec2(SIDE_MAP_METERS/2, SIDE_MAP_METERS/2);
	vec2 centerBases = vec2(0,0);
	float ratioCenterBases = 0;
	vec2 areaFutureLB, areaFutureLT, areaFutureRT, areaFutureRB, nearestAreaFutureCornerToCenter;
	int index = 0;
	it = bases.begin();
	while(it != bases.end())
	{
		mineralsBases[index] = inout->getObjectByTypeAndArea(*state, OT_MINERAL, it->pos, DISTANCE_LIMIT_FIND_ANOTHER_MINERAL_RECOLECTOR-RADIO_BASE*1.5f, true, false, false);
		recollectsBases[index] = inout->getObjectByTypeAndArea(*state, OT_RECOLECTOR, it->pos, DISTANCE_LIMIT_FIND_ANOTHER_MINERAL_RECOLECTOR-RADIO_BASE*1.5f, true, false, false);
		
		if(mineralsBases[index].size() > 0)
			basesWithMineralsNear ++;
		mineralsNearBases[index] = mineralsBases[index].size();
		recollectsNearBases[index] = recollectsBases[index].size();
		
		recollectsRecollectingNearBases[index] = 0;
		list<InOut::InfoObject>::iterator it2 = recollectsBases[index].begin();
		while(it2 != recollectsBases[index].end())
		{
			if(it2->unit->action == AT_RECOLLECT)
				recollectsRecollectingNearBases[index]++;
			it2++;
		}
		
		centerBases.x += it->building->pos.x;
		centerBases.y += it->building->pos.y;
				
		index++;
		it++;
	}
	centerBases.x /= ((float)bases.size());
	centerBases.y /= ((float)bases.size());
	// Calculate ratio bases from center.
	index = 0;
	it = bases.begin();
	while(it != bases.end())
	{
		float dist = centerBases.dist(it->building->pos);
		if(dist > ratioCenterBases)
			ratioCenterBases = dist;
		index++;
		it++;
	}
	// Calculate the knowed nearest to center position.
	it = recollects.begin();
	while(it != recollects.end())
	{
		if(centerMap.dist(it->pos) < centerMap.dist(knowNearCenterPos))
			knowNearCenterPos = it->pos;
		it++;
	}
	// Get soldiers.
	list<InOut::InfoObject> temp = inout->getObjectByType(*state, OT_SOLDADO_RASO, true, false, false);
	soldiers.insert(soldiers.end(), temp.begin(), temp.end());
	temp = inout->getObjectByType(*state, OT_SOLDADO_ENTRENADO, true, false, false);
	soldiers.insert(soldiers.end(), temp.begin(), temp.end());
	temp = inout->getObjectByType(*state, OT_TANQUE, true, false, false);
	soldiers.insert(soldiers.end(), temp.begin(), temp.end());
	temp = inout->getObjectByType(*state, OT_TANQUE_PESADO, true, false, false);
	soldiers.insert(soldiers.end(), temp.begin(), temp.end());
	
	if(state->areaIsClosing)
	{
		areaFutureLB.x = state->areaFutureLeft;
		areaFutureLB.y = state->areaFutureBottom;
		areaFutureLT.x = state->areaFutureLeft;
		areaFutureLT.y = state->areaFutureTop;
		areaFutureRT.x = state->areaFutureRight;
		areaFutureRT.y = state->areaFutureTop;
		areaFutureRB.x = state->areaFutureRight;
		areaFutureRB.y = state->areaFutureBottom;
		
		float distNearestCorner = centerMap.dist(areaFutureLB);
		nearestAreaFutureCornerToCenter = areaFutureLB;
		if(centerMap.dist(areaFutureLT) < distNearestCorner)
		{
			distNearestCorner = centerMap.dist(areaFutureLT);
			nearestAreaFutureCornerToCenter = areaFutureLT;
		}
		if(centerMap.dist(areaFutureRT) < distNearestCorner)
		{
			distNearestCorner = centerMap.dist(areaFutureRT);
			nearestAreaFutureCornerToCenter = areaFutureRT;
		}
		if(centerMap.dist(areaFutureRB) < distNearestCorner)
		{
			distNearestCorner = centerMap.dist(areaFutureRB);
			nearestAreaFutureCornerToCenter = areaFutureRB;
		}
	}

			
	// Is area closing.
		// My bases are in the area
			// Defend the base with my soldiers.
		// My bases aren't in the area.
			// I have a soldier that isn't in or he is not go to the center area.
				// Send it to the area center (attacking).
	if(state->areaIsClosing)
	{
		// My bases are in the area
		if(state->areaFutureLeft < centerBases.x && centerBases.x < state->areaFutureRight &&
			state->areaFutureBottom < centerBases.y && centerBases.y < state->areaFutureTop)
		{
			// Send all soldiers to attack into the area on the nearest center edge.
			vec2 targetPos;
			targetPos.x = (nearestAreaFutureCornerToCenter.x + state->areaFutureCenterX)/2.0f;
			targetPos.y = (nearestAreaFutureCornerToCenter.y + state->areaFutureCenterY)/2.0f;
			
			it = soldiers.begin();
			while(it != soldiers.end())
			{
				// If he's not attacking and isn't in the position OR he's going to attack but not to the position.
				if( (it->unit->action != AT_ATACK && targetPos.dist(it->pos) > 200.0f) || (it->unit->action == AT_ATACK && targetPos.dist(it->unit->actionPos) > 200.0f) )
				{
					// Go go go.
					msgs[ret].intent(AT_ATACK, AST_ATACK_GOTOATACK, it->id, 0, 0, 0, 0, targetPos, 0);
					ret++;
					if(ret >= max)
						return ret;
				}
				it++;
			}
		}
		else
		{
			// Send soldiers to the center of area
			vec2 targetPos = vec2(state->areaFutureCenterX, state->areaFutureCenterY);
			it = soldiers.begin();
			while(it != soldiers.end())
			{
				// If he's not attacking and isn't in the position OR he's going to attack but not to the position.
				if(it->unit->action != AT_ATACK && targetPos.dist(it->pos) > 200.0f || (it->unit->action == AT_ATACK && targetPos.dist(it->unit->actionPos) > 200.0f) )
				{
					// Go go go.
					msgs[ret].intent(AT_ATACK, AST_ATACK_GOTOATACK, it->id, 0, 0, 0, 0, targetPos, 0);
					ret++;
					if(ret >= max)
						return ret;
				}
				it++;
			}
		}
		if(ret > 0)
			return ret;
	}



	/// RECOLLECT MINERALS
	// In the bases. Check if there are same amount recollects recollecting that minerals near, if not:
		// Send to recollet those recollects that don't do nothing.
			// If not, do recollect.
	it = bases.begin();
	index = 0;
	while(it != bases.end())
	{
		// Is created.
		if(it->building->creada >= it->building->fullLife)
		{
			if(mineralsBases[index].size() > recollectsRecollectingNearBases[index])
			{
				// Get the near recollector that isn't do nothing.
				unsigned long long unusedRecollectId = getNearUnUsedRecollector(recollects, it->building->pos);
				
				// If there isn't unused recollect then find a recollect that are two or more in the same mineral.
				if(unusedRecollectId == 0)
				{
					it2 = recollects.begin();
					while(it2 != recollects.end())
					{
						if(it2->unit->action == AT_RECOLLECT)
						{
							it3 = it2;
							it3++;
							while(it3 != recollects.end())
							{
								if(it3->unit->action == AT_RECOLLECT && it2->unit->actionObjectMap2ID == it3->unit->actionObjectMap2ID)
								{
									unusedRecollectId = it2->id;
									break;
								}
								it3++;
							}
						}
						if(unusedRecollectId > 0)
							break;
						it2++;
					}
				}
				
				// Send unused to recollect.
				if(unusedRecollectId > 0)
				{
					// Check mineral that isn't recollecting.
					unsigned long long mineralUnRecollectingId = 0;
					it2 = mineralsBases[index].begin();
					while(it2 != mineralsBases[index].end())
					{
						bool rec = true;
						it3 = recollectsBases[index].begin();
						while(it3 != recollectsBases[index].end())
						{
							if(it3->unit->action == AT_RECOLLECT && it3->unit->actionObjectMap2ID == it2->id)
							{
								rec = false;
								break;
							}
							it3++;
						}
						if(rec)
						{
							mineralUnRecollectingId = it2->id;
							break;
						}
						it2++;
					}
					if(mineralUnRecollectingId > 0)
					{
						// Send recollector to recollect.
						pos_.x = 0; pos_.y = 0;
						msgs[ret].intent(AT_RECOLLECT, AST_RECOLLECT_GORESOURCE, unusedRecollectId, 0, 0, 0, mineralUnRecollectingId, pos_, 0);
						ret++;
						return ret;
					}
				}
				// Make a recollector.
				else
				{
					// There are few recollectors.
					if(recollects.size() < MAX_RECOLLECTORS)
					{
						// There is resources and It isn't making.
						if(state->minerals > MINERALS_COST_RECOLECTOR && it->building->action != AT_NEWUNIT)
						{
							// Make new recollector.
							pos_.x = 0; pos_.y = 0;
							msgs[ret].intent(AT_NEWUNIT, AST_NEWUNIT_MAKEUNIT, 0, 0, it->id, 0, 0, pos_, OT_RECOLECTOR);
							ret++;
							return ret;
						}
					}
				}
			}
		}
		
		index++;
		it++;
	}


	
	
	/// MAKE BASE NEAR MINERALS
	// There are two or less bases with minerals near, I have 5000 or less minerals and there's not recollect go to making a base.
		// Find a place that there are six minerals near.
			// Send the nearest recollect to make new base.
	if(basesWithMineralsNear < 3 && state->minerals >= MINERALS_COST_BASE && state->minerals < 5000)
	{
		bool goToMakingBase = false;
		it = recollects.begin();
		while(it != recollects.end())
		{
			if(it->unit->action == AT_BUILD && it->unit->actionMake == OT_BASE)
			{
				goToMakingBase = true;
				break;
			}
			it++;
		}
		if(!goToMakingBase)
		{
			// Get minerals near to camp but there isn't base near to them.
			list<InOut::InfoObject> minerals = inout->getObjectByTypeAndArea(*state, OT_MINERAL, centerBases, ratioCenterBases+6*DISTANCE_LIMIT_FIND_ANOTHER_MINERAL_RECOLECTOR, true, false, false);
			for(int i=0; i<bases.size(); i++)
			{
				it2 = mineralsBases[i].begin();
				while(it2 != mineralsBases[i].end())
				{
					it3 = minerals.begin();
					while(it3 != minerals.end())
					{
						if(it2->id == it3->id)
						{
							minerals.erase(it3);
							break;
						}
						it3++;
					}
					it2++;
				}
			}
			
			// There are at least six minerals.
			if(minerals.size() >= 6)
			{
				// Find the position that's the most grouped mineral.
				// Loop algorithm, each mineral attract same quantite of distance for him.
				vec2 posNewBase = centerBases;
				for(int i=0; i<10; i++)
				{
					it2 = minerals.begin();
					while(it2 != minerals.end())
					{
						vec2 v = it2->pos - posNewBase;
						v.normalize();
						v *= 10.0f;
						posNewBase += v;
						it2++;
					}
				}
				
				// Check minerals near is more than six.
				int cantMineralsNear = 0;
				it2 = minerals.begin();
				while(it2 != minerals.end())
				{
					float dist = it2->pos.dist(posNewBase);
					if( dist < 0.7f*DISTANCE_LIMIT_FIND_ANOTHER_MINERAL_RECOLECTOR)
						cantMineralsNear++;
					it2++;
				}
				if(cantMineralsNear >= 6)
				{
					unsigned long long nearRecId = getNearUnUsedRecollector(recollects, posNewBase);
					if(nearRecId > 0)
					{
						posNewBase = getPositionToBuild(posNewBase, 2.0f*RADIO_BASE + 1.0f);
						
						msgs[ret].intent(AT_BUILD, AST_BUILD_GOTOBUILD, nearRecId, 0, 0, 0, 0, posNewBase, OT_BASE);
						ret++;
						return ret;
					}
				}
			}
		}
	}
	// There is a base unfinished and there isn't any building it.
	// Send to build it.
	it = bases.begin();
	while(it != bases.end())
	{
		if(it->building->creada < it->building->fullLife)
		{
			bool goToBuild = false;
			it2 = recollects.begin();
			while(it2 != recollects.end())
			{
				if(it2->unit->action == AT_BUILD && it2->unit->actionBuild2ID == it->id)
				{
					goToBuild = true;
					break;
				}
				it2++;
			}
			if(!goToBuild)
			{
				unsigned long long nearRecId = getNearUnUsedRecollector(recollects, it->pos);
				if(nearRecId == 0)
				{
					// Select one that is recollecting.
					it2 = recollects.begin();
					while(it2 != recollects.end())
					{
						if(it2->unit->action == AT_RECOLLECT)
						{
							nearRecId = it2->id;
							break;
						}
						it2++;
					}
				}
				if(nearRecId > 0)
				{
					msgs[ret].intent(AT_BUILD, AST_BUILD_GOTOBUILD, nearRecId, 0, 0, it->id, 0, it->pos, OT_BASE);
					ret++;
					return ret;
				}
			}
		}
		it++;
	}
	
	
	
	/// EXPLORER
	// There isn't recollect with movement,
		// Get the recollect without job or recollecting nearest to center.
			// Send it to explore.
	if(bases.size() > 0)
	{
		// Select the jobless recollector that is the nearest to center.
		// Too the recollect taht is moving.
		// To the nearest to center that is recollecting.
		float distNearestNothing = SIDE_MAP_METERS*99;
		float distNearestRecollecting = SIDE_MAP_METERS*99;
		InOut::Unit* recollectorMoving = NULL;
		InOut::Unit* recollectorNothing = NULL;
		InOut::Unit* recollectorRecollectingNearCenter = NULL;
		it = recollects.begin();
		while(it != recollects.end())
		{
			float dist = centerMap.dist(it->pos);
			if(it->unit->action == 0)
			{
				if(dist < distNearestNothing)
				{
					recollectorNothing = it->unit;
					distNearestNothing = dist;
				}
			}
			else if(it->unit->action == AT_RECOLLECT)
			{
				if(dist < distNearestRecollecting)
				{
					recollectorRecollectingNearCenter = it->unit;
					distNearestRecollecting = dist;
				}
			}
			else if(it->unit->action == AT_MOVE)
			{
				recollectorMoving = it->unit;
				break;
			}
			it++;
		}
		if(recollectorMoving == NULL)
		{
			InOut::Unit* newExplorer = recollectorNothing;
			if(newExplorer == NULL)
				newExplorer = recollectorRecollectingNearCenter;
			if(newExplorer != NULL)
			{
				// Move to explore.
				vec2 projRec;
				float m, b;
				vec2::getLine(centerMap, centerBases, m, b);
				vec2::projPointToLine(newExplorer->pos, m, b, projRec);

				vec2 toCenter = centerMap;
				toCenter -= projRec;
				toCenter.normalize();
				toCenter *= (2.0f * VISIBILITY_DISTANCE);

				vec2 toOtherSide = projRec;
				toOtherSide -= newExplorer->pos;
				toOtherSide.normalize();
				toOtherSide *= 250.0f;

				vec2 targetPos = projRec;
				targetPos += toCenter;
				targetPos += toOtherSide;

				if(targetPos.x < 0) targetPos.x = 1;
				if(targetPos.y < 0) targetPos.y = 1;
				if(targetPos.x > SIDE_MAP_METERS) targetPos.x = SIDE_MAP_METERS-1;
				if(targetPos.y > SIDE_MAP_METERS) targetPos.y = SIDE_MAP_METERS-1;

				// Send to move.
				msgs[ret].intent(AT_MOVE, AST_MOVE_MOVE, newExplorer->id, 0, 0, 0, 0, targetPos, 0);
				ret++;
				
				state->view.x = newExplorer->pos.x;
				state->view.y = newExplorer->pos.y;
				// Set curretTime to updated to current view objects.
				inout->setObjectsUpdated(*state, state->view.x-ANCHO_UPDATE/2, state->view.x+ANCHO_UPDATE/2, state->view.y-ALTO_UPDATE/2, state->view.y+ALTO_UPDATE/2);
		
				// Update to recollect to get fresh update.
				msgs[ret].update(newExplorer->pos.x-ANCHO_UPDATE/2, newExplorer->pos.x+ANCHO_UPDATE/2, newExplorer->pos.y-ALTO_UPDATE/2, newExplorer->pos.y+ALTO_UPDATE/2);
				ret++;
				
				return ret;
			}
			// There aren't recollectors!!
			else
			{
				if(state->minerals >= MINERALS_COST_RECOLECTOR)
				{
					// Make new recollector.
					pos_.x = 0; pos_.y = 0;
					msgs[ret].intent(AT_NEWUNIT, AST_NEWUNIT_MAKEUNIT, 0, 0, bases.front().id, 0, 0, pos_, OT_RECOLECTOR);
					ret++;
					return ret;
				}
			}
		}
	}
	
	
	
	// I haven't a factory.
		// There isn't recollect to go making a factory.
			// Select a recollect without job and make it in a free space close to the nearest map center base.
	if(barracas.size() == 0 && state->minerals >= MINERALS_COST_BARRACA)
	{
		bool goToMakingBarraca = false;
		it = recollects.begin();
		while(it != recollects.end())
		{
			if(it->unit->action == AT_BUILD && it->unit->actionMake == OT_BARRACA)
			{
				goToMakingBarraca = true;
				break;
			}
			it++;
		}
		if(!goToMakingBarraca)
		{
			unsigned long long nearRecId = getNearUnUsedRecollector(recollects, knowNearCenterPos);
			
			// There isn't unused recollect and I have a lot of minerals.
			if(nearRecId == 0 && state->minerals > 1000)
			{
				// Select one that is recollecting.
				it = recollects.begin();
				while(it != recollects.end())
				{
					if(it->unit->action == AT_RECOLLECT)
					{
						nearRecId = it->id;
						break;
					}
					it++;
				}
			}
				
			if(nearRecId > 0)
			{
				vec2 posNewBarraca = getPositionToBuild(knowNearCenterPos, 2.0f*RADIO_BARRACA + 1.0f);
				
				msgs[ret].intent(AT_BUILD, AST_BUILD_GOTOBUILD, nearRecId, 0, 0, 0, 0, posNewBarraca, OT_BARRACA);
				ret++;
				return ret;
			}
		}
	}
	// There is a factory unfinished and there isn't any building.
	// Send to build it.
	if(barracas.size() > 0 && barracas.front().building->creada < barracas.front().building->fullLife)
	{
		bool goToBuild = false;
		it = recollects.begin();
		while(it != recollects.end())
		{
			if(it->unit->action == AT_BUILD && it->unit->actionBuild2ID == barracas.front().id)
			{
				goToBuild = true;
				break;
			}
			it++;
		}
		if(!goToBuild)
		{
			unsigned long long nearRecId = getNearUnUsedRecollector(recollects, barracas.front().pos);
			if(nearRecId == 0)
			{
				// Select one that is recollecting.
				it = recollects.begin();
				while(it != recollects.end())
				{
					if(it->unit->action == AT_RECOLLECT)
					{
						nearRecId = it->id;
						break;
					}
					it++;
				}
			}
			if(nearRecId > 0)
			{
				msgs[ret].intent(AT_BUILD, AST_BUILD_GOTOBUILD, nearRecId, 0, 0, barracas.front().id, 0, barracas.front().pos, OT_BARRACA);
				ret++;
				return ret;
			}
		}
	}
	
	
	// I have soldier without job far to the center.
		// Send it to the center attacking.
	it = soldiers.begin();
	while(it != soldiers.end())
	{
		if(it->unit->action == 0 && centerMap.dist(it->unit->pos) > 400.0f)
		{
			msgs[ret].intent(AT_ATACK, AST_ATACK_GOTOATACK, it->id, 0, 0, 0, 0, centerMap, 0);
			ret++;
			return ret;
		}
		it++;
	}
	
		
		
	// Make a soldier if there isn't recollect goes to make a base and there are almost three bases or enough minerals to build base and factory.
	if(barracas.size() > 0 && state->minerals >= MINERALS_COST_SOLDADO_TANQUE_PESADO && 
		(bases.size() >= 3 || state->minerals >= (MINERALS_COST_BASE+MINERALS_COST_SOLDADO_TANQUE_PESADO)) )
	{
		// Is created.
		if(barracas.front().building->creada >= barracas.front().building->fullLife)
		{
			// This is for not consume the base's resources.
			bool goToMakingBase = false;
			it = recollects.begin();
			while(it != recollects.end())
			{
				if(it->unit->action == AT_BUILD && it->unit->actionMake == OT_BASE)
				{
					goToMakingBase = true;
					break;
				}
				it++;
			}
			if(!goToMakingBase)
			{
				int soldierMaked = OT_SOLDADO_RASO;
				if(rand()%4 == 0)
					soldierMaked = OT_SOLDADO_ENTRENADO;
				else if(rand()%4 == 0)
					soldierMaked = OT_TANQUE;
				else if(rand()%4 == 0)
					soldierMaked = OT_TANQUE_PESADO;
				
				pos_.x = 0; pos_.y = 0;
				msgs[ret].intent(AT_NEWUNIT, AST_NEWUNIT_MAKEUNIT, 0, 0, barracas.front().id, 0, 0, pos_, soldierMaked);
				ret++;
				return ret;
			}
		}
	}
	
	return ret;
}











unsigned long long Agent::getNearUnUsedRecollector(list<InOut::InfoObject> &recollects, vec2 pos)
{
	unsigned long long unusedRecollectId = 0;
	float distUnUsedRecollect = 2*SIDE_MAP_METERS;
	list<InOut::InfoObject>::iterator it = recollects.begin();
	while(it != recollects.end())
	{
		if(it->unit->action == 0)
		{
			float dist = it->unit->pos.dist(pos);
			if( dist < distUnUsedRecollect)
			{
				distUnUsedRecollect = dist;
				unusedRecollectId = it->id;
			}
		}
		it++;
	}
	
	return unusedRecollectId;
}

vec2 Agent::getPositionToBuild(vec2 pos, float radio)
{
	vec2 target = pos;
	list<InOut::InfoObject> near = inout->getObjectByTypeAndArea(*state, OT_ALL, pos, 200.0f, true, true, true);
	
	// This algorithm search like a snail a free space.
	// When find a free space try to insert the object.
	// 0 right, 1 up, 2 left, 3 down.
	int dir = 0;
	int amountSide = 1;
	int remainSide = 1;
	while(true)
	{
		bool ready = true;
		
		if( (target.x-radio) >= 0 && (target.x+radio) < SIDE_MAP_METERS && (target.y-radio) >= 0 && (target.y+radio) < SIDE_MAP_METERS)
		{
			// Check near objects.
			list<InOut::InfoObject>::iterator it = near.begin();
			while(it != near.end())
			{
				float dist = it->pos.dist(target);
				if(dist <= radio)
				{
					ready = false;
					break;
				}
				it++;
			}
			
			// Check base zones.
			for(int e=0; e<EQUIPS; e++)
			{
				for(int j=0; j<state->amountBasesZones[e]; j++)
				{
					if(state->basesZones[e][j].pos.dist(target) <= (radio + state->basesZones[e][j].radio))
					{
						ready = false;
						break;
					}
				}
			}
		}
		else
			ready = false;
		
		if(ready)
			break;
		
		// One step.
		if(remainSide==0)
		{
			if(dir == 0)
				dir = 1;
			else if(dir == 1)
			{
				dir = 2;
				amountSide++;
			}
			else if(dir == 2)
				dir = 3;
			else
			{
				dir = 0;
				amountSide++;
			}
			remainSide = amountSide;
		}
		switch(dir)
		{
			case 0:
				target.x += RADIO_MINERAL;
				break;
			case 1:
				target.y += RADIO_MINERAL;
				break;
			case 2:
				target.x -= RADIO_MINERAL;
				break;
			case 3:
				target.y -= RADIO_MINERAL;
				break;
		}
		remainSide--;
	}
	
	return target;
}









