/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef CITYPARDONCOMMAND_H_
#define CITYPARDONCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"

class CitypardonCommand : public QueueCommand {
public:

	CitypardonCommand(const String& name, ZoneProcessServer* server)
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
		else {  //Infinity:  They have nothing targeted, look for player name argument
		
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

		if (!city->isBanned(targetCreature->getObjectID()))
			return INVALIDTARGET; //They aren't even banned...

		if (!city->isMilitiaMember(creature->getObjectID())) {
			creature->sendSystemMessage("@city/city:not_militia"); //You must be a member of the city militia to use this command.
			return GENERALERROR;
		}

		city->removeBannedPlayer(targetCreature->getObjectID());
		city->applySpecializationModifiers(targetCreature);

		StringIdChatParameter params("city/city", "city_pardoned"); //You have been pardoned by the city %TO and are once again able to use city services.
		params.setTO(city->getCityRegionName());
		targetCreature->sendSystemMessage(params);

		StringIdChatParameter params1("city/city", "city_pardon_done"); //%TT has been pardoned and is now able to use city services.
		params1.setTT(targetCreature->getDisplayedName());
		creature->sendSystemMessage(params1);

		return SUCCESS;
	}

};

#endif //CITYPARDONCOMMAND_H_
