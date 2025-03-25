/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef NPCCONVERSATIONSTARTCOMMAND_H_
#define NPCCONVERSATIONSTARTCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"

class NpcConversationStartCommand : public QueueCommand {
public:

	NpcConversationStartCommand(const String& name, ZoneProcessServer* server) : QueueCommand(name, server) {
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {
		if (!checkStateMask(creature)) {
			return INVALIDSTATE;
		}

		if (!checkInvalidLocomotions(creature)) {
			return INVALIDLOCOMOTION;
		}

		if (!creature->isPlayerCreature()) {
			return GENERALERROR;
		}

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		if (ghost == nullptr)
			return GENERALERROR;

		auto zoneServer = creature->getZoneServer();
		if (zoneServer == nullptr)
			return GENERALERROR;

		ManagedReference<SceneObject*> object = zoneServer->getObject(target);
		if (object == nullptr || !object->isAiAgent())
			return INVALIDTARGET;

		AiAgent* agent = object->asAiAgent();
		if (agent == nullptr)
			return INVALIDTARGET;

		try {
			Locker clocker(agent, creature);

			/*
			ValidatedPosition* validPosition = ghost->getLastValidatedPosition();

			if (validPosition == nullptr)
				return GENERALERROR;

			Vector3 creaturePos = validPosition->getPosition();
			uint64 playerParentID = validPosition->getParent();
			*/

			Vector3 creaturePos = creature->getPosition();
			uint64 creatureParentID = creature->getParentID();

			Vector3 agentPos = agent->getPosition();
			uint64 agentParentID = agent->getParentID();

			// No conversing from different cells
			if (creatureParentID != agentParentID) {
				StringIdChatParameter params;
				params.setStringId("@ui:sui_different_cell_start");
				params.setTT(agent->getDisplayedName());
				creature->sendSystemMessage(params);
				return TOOFAR;
			}

			// If the conversing NPC is outdoors, we will account for distance based on x, y only. LoS also checked below
			if (agentParentID == 0) {
				agentPos.setZ(0);
				creaturePos.setZ(0);
			}

			if (creaturePos.distanceTo(agentPos) > 5.0f) {
				StringIdChatParameter params;
				params.setStringId("@ui:radial_out_of_range_prose");
				params.setTT(agent->getDisplayedName());
				params.setTO("@ui_radial:converse_start");
				creature->sendSystemMessage(params);
				return TOOFAR;
			}

			// No conversing without LoS
			if (!CollisionManager::checkLineOfSight(agent, creature)) {
				StringIdChatParameter params;
				params.setStringId("@ui:sui_los_start");
				params.setTT(agent->getDisplayedName());
				creature->sendSystemMessage(params);
				return GENERALERROR;
			}

			ghost->setConversatingCreature(agent);

			if (agent->sendConversationStartTo(creature)) {
				agent->notifyObservers(ObserverEventType::STARTCONVERSATION, creature);
			}
		} catch (Exception& e) {
			e.printStackTrace();
			creature->error("unreported ObjectControllerMessage::parseNpcStartConversation(creature* creature, Message* pack) exception");
		}

		return SUCCESS;
	}
};

#endif //NPCCONVERSATIONSTARTCOMMAND_H_
