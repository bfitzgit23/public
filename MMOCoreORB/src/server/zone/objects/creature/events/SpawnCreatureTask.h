/*
 * SpawnCreatureTask.h
 *
 *  Created on: 02/08/2015
 *      Author: Miztah
 */

#ifndef SPAWNCREATURETASK_H_
#define SPAWNCREATURETASK_H_

#include "server/zone/Zone.h"
#include "server/ServerCore.h"
#include "server/zone/objects/creature/ai/AiAgent.h"
#include "server/zone/managers/creature/CreatureManager.h"

class SpawnCreatureTask : public Task {
	uint32 templateCRC, creatureMask;
	float x, z, y, respawn, directionDegrees;
	uint64 parentID;
	bool randomRespawn, forceBaby;
	String zoneName;

public:
	SpawnCreatureTask(uint32 temp, float resp, String zone, float xPos, float zPos, float yPos, uint64 par, bool rand, float dir = 0, unsigned int oldCreatureMask = 0) {
		templateCRC = temp;
		respawn = resp;
		zoneName = zone;
		x = xPos;
		z = zPos;
		y = yPos;
		parentID = par;
		randomRespawn = rand;
		directionDegrees = dir;  //In Degrees
		creatureMask = oldCreatureMask;
	}

	void run() {
		ZoneServer* zoneServer = ServerCore::getZoneServer();

		if (zoneServer == nullptr)
			return;

		Zone* zone = zoneServer->getZone(zoneName);

		if (zone == nullptr)
			return;

		CreatureManager* creatureManager = zone->getCreatureManager();

		CreatureObject* creature = nullptr;

		//Logger::console.error("SpawnCreatureTask::run() - creatureMask = " + String::valueOf(creatureMask));

     	if (creatureMask & CreatureFlag::BABYONLY) {
			creature = creatureManager->spawnCreatureAsBaby(templateCRC, x, z, y, parentID, directionDegrees);
		}
		else {
			creature = creatureManager->spawnCreature(templateCRC, 0, x, z, y, parentID, directionDegrees);
		}

		if (creature != nullptr && creature->isAiAgent()) {
			AiAgent* ai = cast<AiAgent*>(creature);

			Locker locker(ai);

			ai->setRespawnTimer(respawn);

			if (randomRespawn)
				ai->setRandomRespawn(true);

			//Infinity:  Check for special creature flags
			if (creatureMask & CreatureFlag::STATIONARY) {
				ai->addCreatureFlag(CreatureFlag::STATIONARY);
			}

			if (creatureMask & CreatureFlag::STATIC) {
				ai->addCreatureFlag(CreatureFlag::STATIC);
			}

			if (creatureMask & CreatureFlag::BABYONLY) {
				ai->addCreatureFlag(CreatureFlag::BABYONLY);
			}

			if (creatureMask & CreatureFlag::NOAIAGGRO) {
				ai->addCreatureFlag(CreatureFlag::NOAIAGGRO);
			}
		
			ai->setAITemplate();
		}
	}
};


#endif /* SPAWNCREATURETASK_H_ */
