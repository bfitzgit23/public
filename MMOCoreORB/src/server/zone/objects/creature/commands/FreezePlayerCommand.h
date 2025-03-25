/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef FREEZEPLAYERCOMMAND_H_
#define FREEZEPLAYERCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"

class FreezePlayerCommand : public QueueCommand {
public:

	FreezePlayerCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		if (!creature->isPlayerCreature())
			return GENERALERROR;


		ManagedReference<SceneObject*> object = server->getZoneServer()->getObject(target);

		ManagedReference<CreatureObject*> targetPlayer = NULL;
		ManagedReference<CreatureObject*> player = cast<CreatureObject*>(creature);
		StringTokenizer args(arguments.toString());
		StringBuffer fullReason;
		String syntaxError = "Invalid arguments: /freezePlayer <firstname> <reason>";

		if (object == nullptr || !object->isPlayerCreature()) {

			String firstName;
			if (args.hasMoreTokens()) {
				args.getStringToken(firstName);
				targetPlayer = server->getZoneServer()->getPlayerManager()->getPlayer(firstName);
			}

		} else {
			targetPlayer = cast<CreatureObject*>( object.get());
		}

		if (targetPlayer == nullptr) {
			player->sendSystemMessage(syntaxError);
			return INVALIDPARAMETERS;
		}

		ManagedReference<PlayerObject*> targetGhost = targetPlayer->getPlayerObject();
		if (targetGhost == nullptr) {
			player->sendSystemMessage(syntaxError);
			return INVALIDPARAMETERS;
		}

		bool muteOnly = false;

		while (args.hasMoreTokens()) {
			String reason;
			args.getStringToken(reason);

			if (reason.toLowerCase() == "muteonly")
				muteOnly = true;
			else
				fullReason << reason << " ";
		}

		try {

			Locker playerlocker(targetPlayer);

			targetGhost->setMutedState(true);  // This will mute the player

			StringBuffer muteMessage, targetMuteMessage;

			if (!muteOnly) {  //Standard freeze and mute

				targetPlayer->setRootedState(3600);   // This will apply frozen state
				targetPlayer->setSpeedMultiplierBase(0.f, true);

				if(fullReason.toString().isEmpty()) {

					targetPlayer->sendSystemMessage("You have been frozen and muted by " + player->getFirstName() + ".");
					player->sendSystemMessage(targetPlayer->getFirstName() + " is now frozen and muted.");

				} else {

					String reason = fullReason.toString();
					targetGhost->setMutedReason(reason);

					targetMuteMessage << "You have been frozen and muted by \'" << player->getFirstName() << "\' for '" << reason << "'";
					targetPlayer->sendSystemMessage(targetMuteMessage.toString());

					muteMessage << targetPlayer->getFirstName() << " is now frozen and muted for \'" << reason << "'";
					player->sendSystemMessage(muteMessage.toString());
				}
			}
			else {  // Mute only

				if(fullReason.toString().isEmpty()) {

					targetPlayer->sendSystemMessage("You have been muted by " + player->getFirstName() + ".");
					player->sendSystemMessage(targetPlayer->getFirstName() + " is now muted.");
				}
				else {

					String reason = fullReason.toString();
					targetGhost->setMutedReason(reason);

					targetMuteMessage << "You have been muted by \'" << player->getFirstName() << "\' for '" << reason << "'";
					targetPlayer->sendSystemMessage(targetMuteMessage.toString());

					muteMessage << targetPlayer->getFirstName() << " is now muted for \'" << reason << "'";
					player->sendSystemMessage(muteMessage.toString());
				}
			}

		} catch (Exception& e) {
			player->sendSystemMessage(syntaxError);
		}

		return SUCCESS;
	}

};

#endif //FREEZEPLAYERCOMMAND_H_
