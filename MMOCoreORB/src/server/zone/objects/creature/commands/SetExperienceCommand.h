/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef SETEXPERIENCECOMMAND_H_
#define SETEXPERIENCECOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/player/PlayerObject.h"
#include "server/zone/managers/player/PlayerManager.h"

class SetExperienceCommand : public QueueCommand {
public:

	SetExperienceCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		try {

			ManagedReference<SceneObject* > object =
					server->getZoneServer()->getObject(target);

			ManagedReference<CreatureObject*> player = nullptr;

			StringTokenizer args(arguments.toString());

			if (object == nullptr || !object->isPlayerCreature()) {

				String firstName;

				if (args.hasMoreTokens()) {
					args.getStringToken(firstName);
					player = server->getZoneServer()->getPlayerManager()->getPlayer(
									firstName);
				}

			} else {
				player = cast<CreatureObject*>(object.get());
			}

			if (player == nullptr) {
				creature->sendSystemMessage("invalid arguments for setExperience command. usage: setExperience <firstName> <experienceType> <amount>");
				return GENERALERROR;
			}

			auto zoneServer = player->getZoneServer();
			if (zoneServer == nullptr)
				return GENERALERROR;

			auto playerManager = zoneServer->getPlayerManager();
			if (playerManager == nullptr)
				return GENERALERROR;

			String xpType;
			args.getStringToken(xpType);
			int newXp = args.getIntToken();

			ManagedReference<PlayerObject*> ghost = player->getPlayerObject();
			if (ghost == nullptr) {
				creature->sendSystemMessage("Error retrieving player object.");
				return GENERALERROR;
			}

			int currentXp = ghost->getExperience(xpType);

			int delta = newXp - currentXp;

			playerManager->awardExperience(player, xpType, delta, true, 1.0f, false);  //Don't use modifiers

			creature->sendSystemMessage("Experience successfully changed for player " + player->getFirstName());

			if (delta < 0) {
				StringIdChatParameter message("base_player","prose_revoke_xp");
				message.setDI(delta * -1);
				message.setTO("exp_n", xpType);
				player->sendSystemMessage(message);
			}

		} catch (Exception& e) {
			creature->sendSystemMessage("invalid arguments for setExperience command. usage: setExperience <firstName> <experienceType> <amount>");
		}


		return SUCCESS;
	}

};

#endif //SETEXPERIENCECOMMAND_H_
