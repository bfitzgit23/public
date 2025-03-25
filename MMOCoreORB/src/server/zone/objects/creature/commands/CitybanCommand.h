/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef CITYBANCOMMAND_H_
#define CITYBANCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"

class CitybanCommand : public QueueCommand {
public:

	CitybanCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		ZoneServer* zoneServer = creature->getZoneServer();
		if (zoneServer == nullptr)
			return GENERALERROR;

		ManagedReference<CreatureObject*> targetCreature = nullptr;

		if (target != 0) { //Infinity: They have something targeted

			ManagedReference<SceneObject*> targetObject = zoneServer->getObject(target);

			if (targetObject == nullptr || !targetObject->isPlayerCreature() || targetObject == creature) {
				return INVALIDTARGET;
			}

			targetCreature = cast<CreatureObject*>(targetObject.get());

		}
		else { //Infinity:  They have nothing targeted, look for player name argument
			
			String targetName;

			try {
				UnicodeTokenizer args(arguments);
				args.getStringToken(targetName);

			} catch (Exception& e) {
				creature->sendSystemMessage("SYNTAX: /cityBan <playerName>");
				return INVALIDPARAMETERS;
			}

			ManagedReference<PlayerManager*> playerManager = server->getPlayerManager();
			if (playerManager == nullptr)
				return GENERALERROR;

			targetCreature = playerManager->getPlayer(targetName);

			if (targetCreature == nullptr) {
				creature->sendSystemMessage("The specified player does not exist.");
				return INVALIDTARGET;
			}
		}

		ManagedReference<CityRegion*> city = creature->getCityRegion().get();

		if (city == nullptr) {
			creature->sendSystemMessage("@city/city:not_in_city"); //You must be in a city to use this command.
			return GENERALERROR;
		}

		Locker lock(city, creature);

		if (city->isBanned(targetCreature->getObjectID()))
			return INVALIDTARGET; //They are already banned.

		if (!city->isMilitiaMember(creature->getObjectID())) {
			creature->sendSystemMessage("@city/city:not_militia"); //You must be a member of the city militia to use this command.
			return GENERALERROR;
		}

		ManagedReference<PlayerObject*> ghost = targetCreature->getPlayerObject();

		if (ghost != nullptr && ghost->hasGodMode()) {
			//Can't ban a CSR
			creature->sendSystemMessage("@city/city:not_csr_ban"); //You cannot ban a Customer Service Representative from the city!

			StringIdChatParameter params("city/city", "csr_ban_attempt_msg");
			params.setTT(creature->getDisplayedName());
			params.setTO(city->getCityRegionName());

			targetCreature->sendSystemMessage(params); //%TT tried to /cityBan you from %TO!
			return GENERALERROR;
		}

		if (city->isCitizen(targetCreature->getObjectID())) {
			creature->sendSystemMessage("@city/city:not_citizen_ban"); //You can't city ban a citizen of the city!
			return GENERALERROR;
		}

		city->addBannedPlayer(targetCreature->getObjectID());
		city->removeSpecializationModifiers(targetCreature);

		Logger::console.error("Player " + creature->getDisplayedName() + " has banned player " + targetCreature->getDisplayedName() + " from city " + city->getCityRegionName());

		StringIdChatParameter params("city/city", "city_banned");
		params.setTO(city->getCityRegionName());
		targetCreature->sendSystemMessage(params); //You have been banned from the city %TO.  You may no longer use any city services. 

		StringIdChatParameter params1("city/city", "city_ban_done");
		params1.setTT(targetCreature->getDisplayedName());
		creature->sendSystemMessage(params1); //%TT has been banned from the city and is no longer able to access city services.

		return SUCCESS;
	}

};

#endif //CITYBANCOMMAND_H_
