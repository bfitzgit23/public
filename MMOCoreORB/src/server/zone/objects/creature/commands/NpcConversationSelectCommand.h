/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef NPCCONVERSATIONSELECTCOMMAND_H_
#define NPCCONVERSATIONSELECTCOMMAND_H_

class NpcConversationSelectCommand : public QueueCommand {
public:

	NpcConversationSelectCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		if (!creature->isPlayerCreature())
			return GENERALERROR;

		if (creature == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		if (ghost == nullptr)
			return GENERALERROR;

		uint64 conversationCreatureOid = ghost->getConversatingCreature();
		ManagedReference<CreatureObject*> agent = server->getZoneServer()->getObject(conversationCreatureOid).castTo<CreatureObject*>();

		if (agent != nullptr) {
			int option = Integer::valueOf(arguments.toString());

			try {
				Locker clocker(agent, creature);

				/*
				ValidatedPosition* validPosition = ghost->getLastValidatedPosition();

				if (validPosition == nullptr)
					return GENERALERROR;

				Vector3 creaturePos = validPosition->getPosition();
				uint64 playerParentID = validPosition->getParent();
				*/

				Vector3 agentPos = agent->getPosition();
				uint64 agentParentID = agent->getParentID();

				Vector3 creaturePos = creature->getPosition();
				uint64 creatureParentID = creature->getParentID();

				if (creatureParentID != agentParentID) {
					StringIdChatParameter params;
					params.setStringId("@ui:sui_different_cell_start");
					params.setTT(agent->getDisplayedName());
					creature->sendSystemMessage(params);
					return 0;
				}

				// If the conversing NPC is outdoors, we will account for distance based on x, y only. LoS also checked below
				if (agentParentID == 0) {
					agentPos.setZ(0);
					creaturePos.setZ(0);
				}

				if (creaturePos.distanceTo(agentPos) <= 7.0f) {
					agent->selectConversationOption(option, creature);

					agent->notifyObservers(ObserverEventType::SELECTCONVERSATION, creature, option);
				} else {
					StringIdChatParameter params;
					params.setStringId("@ui:radial_out_of_range_prose");
					params.setTT(agent->getDisplayedName());
					params.setTO("@ui_radial:converse_start");
					creature->sendSystemMessage(params);
					return TOOFAR;
				}
			} catch (Exception& e) {

			}
		}

		return SUCCESS;
	}

};

#endif //NPCCONVERSATIONSELECTCOMMAND_H_
