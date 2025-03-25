/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef FISHCOMMAND_H_
#define FISHCOMMAND_H_

#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/managers/minigames/FishingManager.h"

class FishCommand : public QueueCommand {
public:
	FishCommand(const String& name, ZoneProcessServer* server) : QueueCommand(name, server) {
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {
		if (!checkStateMask(creature)) {
            if (creature->isSwimming()) {
                creature->sendSystemMessage("@fishing:player_swimming");
            }
            if (creature->isRidingMount()) {
                creature->sendSystemMessage("@fishing:player_mounted");
            }
			return INVALIDSTATE;
		}

		if (!checkInvalidLocomotions(creature)) {
			return INVALIDLOCOMOTION;
		}

		if (creature->isPlayerCreature()) {
			auto fishingManager = server->getFishingManager();

			if (fishingManager == nullptr)
				return GENERALERROR;

			// ENABLE FOR TESTING
			// manager->freeBait(creature);
			// ENDENABLE

			if ((fishingManager->getFishingState(creature) != FishingManager::NOTFISHING) || (fishingManager->isPlaying(creature))) {
				creature->sendSystemMessage("@fishing:already_fishing");

				return GENERALERROR;
			}

			fishingManager->startFishing(creature);
		}
		return SUCCESS;
	}
};

#endif // FISHCOMMAND_H_