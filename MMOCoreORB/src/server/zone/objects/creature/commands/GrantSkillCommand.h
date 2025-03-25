/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef GRANTSKILLCOMMAND_H_
#define GRANTSKILLCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/managers/skill/SkillManager.h"

class GrantSkillCommand : public QueueCommand {
public:

	GrantSkillCommand(const String& name, ZoneProcessServer* server) : QueueCommand(name, server) {
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

		String errorString = "Invalid arguments for grantSkill command. usage: /grantSkill [firstName] <skillName>";

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

			if (!targetCreature->hasSkill(skillName)) {

				SkillManager* skillManager = SkillManager::instance();
				if (skillManager == nullptr) {
					return GENERALERROR;
				}

				bool skillGranted = skillManager->awardSkill(skillName, targetCreature, true, true, true);

				if (skillGranted) {
					StringIdChatParameter params;
					params.setTO(skillName);
					params.setStringId("skill_teacher", "skill_terminal_grant");
					targetCreature->sendSystemMessage(params);
					creature->sendSystemMessage("Granted the skill '" + skillName + "' to player " + firstName);
				}
				else {
					StringIdChatParameter params;
					params.setTO(skillName);
					params.setStringId("skill_teacher", "prose_train_failed");
					targetCreature->sendSystemMessage(params);
					creature->sendSystemMessage("Failed to grant the skill '" + skillName + "' to player " + firstName);
				}
			}
			else {
				creature->sendSystemMessage("Player " + firstName + " already has the skill '" + skillName + "'.");
			}
		}
		catch (Exception& e) {
			creature->sendSystemMessage(errorString);
			return GENERALERROR;
		}

		return SUCCESS;
	}

};

#endif //GRANTSKILLCOMMAND_H_
