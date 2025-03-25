/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef REVOKESKILLCOMMAND_H_
#define REVOKESKILLCOMMAND_H_

#include "server/zone/managers/skill/SkillManager.h"

class RevokeSkillCommand : public QueueCommand {
public:
	RevokeSkillCommand(const String& name, ZoneProcessServer* server) : QueueCommand(name, server) {
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		auto zoneServer = server->getZoneServer();
		if (zoneServer == nullptr)
			return GENERALERROR;

		auto playerManager = zoneServer->getPlayerManager();
		if (playerManager == nullptr)
			return GENERALERROR;

		String errorString = "Invalid arguments for revokeSkill command. usage: /revokeSkill [firstName] <skillName>";

		ManagedReference<SceneObject* > object = zoneServer->getObject(target);
		ManagedReference<CreatureObject*> targetCreature = (object == nullptr || !object->isPlayerCreature()) ? nullptr : object->asCreatureObject();

		try {

			StringTokenizer args(arguments.toString());

			if (targetCreature == nullptr) {

				String firstName;
				if (args.hasMoreTokens()) {
					args.getStringToken(firstName);
					targetCreature = playerManager->getPlayer(firstName);
				}
			}

			if (targetCreature == nullptr) {
				creature->sendSystemMessage(errorString);
				return INVALIDTARGET;
			}

			String firstName = targetCreature->getFirstName();
			String skillName = "";

			if (args.hasMoreTokens()) {
				args.getStringToken(skillName);
			}

			if (skillName.isEmpty()) {
				creature->sendSystemMessage(errorString);
				return INVALIDTARGET;
			}

			skillName = skillName.toLowerCase();

			if (targetCreature->hasSkill(skillName)) {

				SkillManager* skillManager = SkillManager::instance();
				bool skillRemoved = skillManager != nullptr && skillManager->surrenderSkill(skillName, targetCreature, true, true);  //Check FRS

				if (skillRemoved) {
					targetCreature->sendSystemMessage("An admin removed your skill '" + skillName + "'.");
					creature->sendSystemMessage("Removed the skill '" + skillName + "' from player " + firstName);
				}
				else {
					//targetCreature->sendSystemMessage("An admin failed to remove your skill """ + skillName + """.");
					creature->sendSystemMessage("Failed to remove the skill '" + skillName + "' from player " + firstName);
				}
			}
			else {
				creature->sendSystemMessage("Player " + firstName + " does not have the skill '" + skillName + "'.");
			}

		}
		catch (Exception& e) {
			creature->sendSystemMessage(errorString);
			return GENERALERROR;
		}

		return SUCCESS;
	}
};

#endif // REVOKESKILLCOMMAND_H_
