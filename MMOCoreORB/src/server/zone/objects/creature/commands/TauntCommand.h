/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef TAUNTCOMMAND_H_
#define TAUNTCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/tangible/threat/ThreatMap.h"

class TauntCommand : public CombatQueueCommand {
public:
	TauntCommand(const String& name, ZoneProcessServer* server) : CombatQueueCommand(name, server) {
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {
		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		ManagedReference<SceneObject*> targetObject = creature->getZoneServer()->getObject(target);

		if (targetObject == nullptr || !targetObject->isCreatureObject() || targetObject->isPlayerCreature())
			return INVALIDTARGET;

		CreatureObject* targetCreature = cast<CreatureObject*>(targetObject.get());

		if (targetCreature == nullptr)
			return INVALIDTARGET;

		if (!targetCreature->isAttackableBy(creature))
			return INVALIDTARGET;

		if (creature->isPlayerCreature() && !creature->checkCooldownRecovery("taunt"))  {
			const Time* cooldownTime = creature->getCooldownTime("taunt");
			if (cooldownTime != nullptr) {
				float timeLeft = fabs(cooldownTime->miliDifference()) / 1000.0f;
				creature->sendSystemMessage("You can use the taunt command again in " + String::format("%.1f", timeLeft) + " second" + ((timeLeft == 1.0f) ? "." : "s."));
				return GENERALERROR;
			} 
			else {
				creature->sendSystemMessage("You can only use the taunt command once every 3 seconds.");
                return GENERALERROR;
			}
		}	

		AiAgent* agent = targetCreature->asAiAgent();

		if (agent == nullptr)
			return INVALIDTARGET;

		if (!agent->isTauntable()) {
			if (creature->isPlayerCreature()) {
				creature->sendSystemMessage("@cbt_spam:taunt_fail_single");
			}
			return INVALIDTARGET;
		}

		int res = doCombatAction(creature, target);

		if (res == SUCCESS) {

			if (creature->isPlayerCreature()) {
				creature->updateCooldownTimer("taunt", 3 * 1000); // 3 seconds
			}

			Locker clocker(targetCreature, creature);

			auto threatMap = targetCreature->getThreatMap();
			if (threatMap != nullptr) {
				int tauntMod = creature->getSkillMod("taunt");
				int levelCombine = targetCreature->getLevel() + creature->getLevel(); //Infinity:  Review this

				if (System::random(levelCombine + tauntMod) >= System::random(levelCombine - tauntMod)) {  //Infinity:  Review this
					threatMap->setThreatState(creature, ThreatStates::TAUNTED, (uint64)tauntMod * 1000, (uint64)tauntMod * 1000);    //Infinity:  Review this, maybe remove threatstate cooldown (last arg)
					threatMap->addAggro(creature, tauntMod * 10, (uint64)tauntMod * 1000);   //Infinity:  Review this

					/* Returns no grammar data
					CombatManager::instance()->broadcastCombatSpam(creature, agent, nullptr, 0, "cbt_spam", "taunt_success", 0);
					stringFiles[29].addEntry("taunt_success", "~~~~~You taunt your enemy, enraging them!~~%TU taunts his enemy!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
					*/

					if (creature->isPlayerCreature())
						creature->sendSystemMessage("@cbt_spam:taunt_success_single");
				} else {
					creature->sendSystemMessage("@cbt_spam:taunt_fail_single");

					/* Returns no grammar data
					stringFiles[29].addEntry("taunt_fail", "~~~~~You try to go taunt your enemy, but they don't fall for it.~~%TU taunts his enemy to no avail.~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
					CombatManager::instance()->broadcastCombatSpam(creature, agent, nullptr, 0, "cbt_spam", "taunt_fail", 0);
					*/;
				}
			}
		}

		return res;
	}
};

#endif // TAUNTCOMMAND_H_
