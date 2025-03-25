/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#include "QueueCommand.h"

//Infinity: Custom includes
#include "server/zone/managers/collision/CollisionManager.h"
#include "server/zone/managers/planet/PlanetManager.h"

#ifndef UNSTICKCOMMAND_H_
#define UNSTICKCOMMAND_H_

class UnstickCommand : public QueueCommand {
public:

	UnstickCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (creature == nullptr)
			return GENERALERROR;

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		if (!creature->checkCooldownRecovery("unstick")) {

			const Time* cooldownTime = creature->getCooldownTime("unstick");
			if (cooldownTime != nullptr) {
				float timeLeft = fabs(cooldownTime->miliDifference()) / 1000.f /60.f;
				creature->sendSystemMessage("You can use the /unstick command again in " + String::format("%.1f", timeLeft) + " minute" + ((timeLeft == 1.0f) ? "." : "s."));
				return GENERALERROR;
			} 
			else {
				creature->sendSystemMessage("You can only use the /unstick command once every 5 minutes.");
                return GENERALERROR;
			}
		}

        auto zone = creature->getZone();

	    if (zone == nullptr) {
            creature->sendSystemMessage("The /unstick command could not find player zone.  Please create a support ticket for assistance.");
			return GENERALERROR;
		}

        uint32 cellID = 0;
        Vector3 worldPosition = creature->getWorldPosition();
        float x = worldPosition.getX();
        float y = worldPosition.getY();
        float z = worldPosition.getZ();

        auto parent = creature->getParent().get();

        if (parent != nullptr) {

            if (parent->isCellObject()) {
                cellID = parent->getObjectID();
            }
        }

        if (cellID == 0) { // Outside

            auto planetManager = zone->getPlanetManager();

            if (planetManager != nullptr) {
                auto terrainManager = planetManager->getTerrainManager();

                if (terrainManager != nullptr) {

                    bool found = false;

                    float randomX = x;
                    float randomY = y;

                    for (int i = 0; i < 20; i++) {

                        randomX = x + System::random(10) - 5;
                        randomY = y + System::random(10) - 5;
                        float collisionZ = CollisionManager::getWorldFloorCollision(randomX, randomY, zone, false);
                        float terrainZ = terrainManager->getHeight(randomX, randomY);

                        /*
                        StringBuffer msg;
                        msg << "X:" << x << "  Y:" << y << "  Z:" << z << "  ranX:" << randomX << "  ranY:" << randomY << "  collisionZ:" << collisionZ << "  terrainZ:" << terrainZ << "  deltaZ:" << fabs(collisionZ - z);
                        creature->sendSystemMessage(msg.toString());
                        */

                        if (collisionZ - z < 3) {  //Can't go up more than 3 meters, can go down farther if stuck on roof
                            x = randomX;
                            y = randomY;
                            z = collisionZ;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        creature->sendSystemMessage("Could not find a new location Z coordinate due to collisions.  Attempting teleport to current Z coordinate to unstick.");
                        x = randomX;
                        y = randomY;
                    }

                }
                else {
                    creature->sendSystemMessage("The /unstick command could not find a new position.  Please create a support ticket for assistance.");
                    return GENERALERROR;
                }
            }           
        }
        else { //Inside
            creature->sendSystemMessage("The /unstick command cannot be used inside a structure.  Please create a support ticket for assistance.");
            return GENERALERROR;
        }

        creature->teleport(x, z, y, cellID);

		creature->addCooldown("unstick", 15 * 60 * 1000); // Wait 15 mins before they can try again

		creature->error("Player " + creature->getFirstName() + " used /unstick " + arguments.toString());

		creature->sendSystemMessage("@cmd_err:unstick_request_complete"); // Unstick complete

		    /*
            string/en/cmd_err.stf	7	unstick_in_progress	Unstick in progress
            string/en/cmd_err.stf	8	unstick_request_complete	Unstick complete
            string/en/cmd_err.stf	9	unstick_request_cancelled	Unstick request was cancelled
		    */

		return SUCCESS;
	}

};

#endif //UNSTICKCOMMAND_H_

