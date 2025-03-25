/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef AREATAUNTCOMMAND_H_
#define AREATAUNTCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/tangible/threat/ThreatStates.h"

class AreaTauntCommand : public CombatQueueCommand {
public:

	AreaTauntCommand(const String& name, ZoneProcessServer* server)
		: CombatQueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {
		
		if (!checkStateMask(creature)) {
			return INVALIDSTATE;
		}

		if (!checkInvalidLocomotions(creature)) {
			return INVALIDLOCOMOTION;
		}

		ManagedReference<SceneObject*> targetObject = creature->getZoneServer()->getObject(target);

		if (targetObject == nullptr || !targetObject->isAiAgent()) {
			return INVALIDTARGET;
		}

		ManagedReference<CreatureObject*> targetCreature = cast<CreatureObject*>(targetObject.get());

		if (targetCreature == nullptr) {
			return INVALIDTARGET;
		}

		if (!targetCreature->isAttackableBy(creature)) {
			return INVALIDTARGET;
		}
			
		if (creature->isPlayerCreature() && !creature->checkCooldownRecovery("areaTaunt"))  {
			const Time* cooldownTime = creature->getCooldownTime("areaTaunt");
			if (cooldownTime != nullptr) {
				float timeLeft = fabs(cooldownTime->miliDifference()) / 1000.0f;
				creature->sendSystemMessage("You can use the areaTaunt command again in " + String::format("%.1f", timeLeft) + " second" + ((timeLeft == 1.0f) ? "." : "s."));
				return GENERALERROR;
			}
			else {
				creature->sendSystemMessage("You can only use the areaTaunt command once every 10 seconds.");
                return GENERALERROR;
			}
		}

		ManagedReference<AiAgent*> agent = targetCreature->asAiAgent();
		if (agent == nullptr) {
			return INVALIDTARGET;
		}

		if (!agent->isTauntable()) {
			if (creature->isPlayerCreature()) {
				creature->sendSystemMessage("@cbt_spam:taunt_fail_single");
			}
			return INVALIDTARGET;
		}

		int numberTaunted = 0;
		int tauntMod = creature->getSkillMod("taunt");

		int res = doCombatAction(creature, target);

		if (res == SUCCESS) {

			if (creature->isPlayerCreature()) {
				creature->updateCooldownTimer("areaTaunt", 10 * 1000);   // Ten seconds
			}
			creature->doCombatAnimation(creature, STRING_HASHCODE("taunt"), 0, 0xFF);
			creature->doAnimation("taunt");

			Locker clocker(targetCreature, creature);

			auto threatMap = targetCreature->getThreatMap();

			if (threatMap != nullptr) {
				int levelCombine = targetCreature->getLevel() + creature->getLevel(); //Infinity:  Review this

				if (System::random(levelCombine + tauntMod) >= System::random(levelCombine - tauntMod)) {  //Infinity:  Review this
					threatMap->setThreatState(creature, ThreatStates::TAUNTED, (uint64)tauntMod * 1000, (uint64)tauntMod * 1000);    //Infinity:  Review this, maybe remove threatstate cooldown (last arg)
					threatMap->addAggro(creature, tauntMod * 10, (uint64)tauntMod * 1000);   //Infinity:  Review this
					numberTaunted++;
				}
			}
		}

		SortedVector<ManagedReference<QuadTreeEntry*> > closeObjects;
		CloseObjectsVector* closeObjectsVector = (CloseObjectsVector*) creature->getCloseObjects();

		if (closeObjectsVector == nullptr) {
			auto zone = creature->getZone();
			if (zone == nullptr) {
				return GENERALERROR;
			}
			zone->getInRangeObjects(creature->getWorldPositionX(), creature->getWorldPositionY(), 32, &closeObjects, true, true);
		}
		else {
			closeObjectsVector->safeCopyReceiversTo(closeObjects, CloseObjectsVector::CREOTYPE);
		}
			
		for (int i = 0; i < closeObjects.size(); i++) {
			ManagedReference<SceneObject*> object = static_cast<SceneObject*>(closeObjects.get(i).get());
			
			if (object == nullptr) {
				continue;
			}
			
			if (!object->isAiAgent()) {
				continue;
			}

			if (object == creature || object == targetCreature) {
				continue;
			}

			ManagedReference<CreatureObject*> areaCreature = cast<CreatureObject*>(object.get());		
			if (areaCreature == nullptr)
				continue;

			ManagedReference<AiAgent*> areaAgent = areaCreature->asAiAgent();
			if (areaAgent == nullptr) {
				continue;
			}

			if (!areaAgent->isTauntable()) {
				continue;
			}

			if (!areaAgent->isAttackableBy(creature)) {
				continue;
			}

			if (areaAgent->isDead() || areaAgent->isIncapacitated()) {
				continue;
			}

			if (!creature->isInRange(areaAgent, 30.f + areaAgent->getTemplateRadius())) { //Check range plus template adjustment
				continue;
			}

			if (!CollisionManager::checkLineOfSight(creature, areaAgent)) {  //Must be in LOS
				continue;
			}
	
			Locker alocker(areaAgent, creature);
			auto threatMap = areaAgent->getThreatMap();

			if (threatMap != nullptr) {
				int levelCombine = areaAgent->getLevel() + creature->getLevel(); //Infinity:  Review this

				if (System::random(levelCombine + tauntMod) >= System::random(levelCombine - tauntMod)) {  //Infinity:  Review this
					threatMap->setThreatState(creature, ThreatStates::TAUNTED, (uint64)tauntMod * 1000, (uint64)tauntMod * 1000);    //Infinity:  Review this, maybe remove threatstate cooldown (last arg)
					threatMap->addAggro(creature, tauntMod * 10, (uint64)tauntMod * 1000);   //Infinity:  Review this
					CombatManager* combatManager = CombatManager::instance();
					if (combatManager != nullptr && (!areaAgent->hasDefender(creature) || !areaAgent->isInCombat())) {
						combatManager->startCombat(creature, areaAgent);
					}
					numberTaunted++;
				}
			}
		}

		if (numberTaunted > 0) {
			if (creature->isPlayerCreature())
				creature->sendSystemMessage("You successfully taunted a total of " + String::valueOf(numberTaunted) + " targets.");
			return SUCCESS;
		}
		else {
			if (creature->isPlayerCreature())
				creature->sendSystemMessage("You were unable to successfully taunt any targets.");	
			return res;
		}
	}

	void sendAttackCombatSpam(TangibleObject* attacker, TangibleObject* defender, int attackResult, int damage, const CreatureAttackData& data) const {
		if (attacker == nullptr)
			return;

		Zone* zone = attacker->getZone();
		if (zone == nullptr)
			return;

		String stringName = data.getCombatSpam();
		byte color = 1;

		switch (attackResult) {
		case CombatManager::HIT:
			stringName += "_success";
			break;
		case CombatManager::MISS:
		case CombatManager::DODGE:
		case CombatManager::COUNTER:
		case CombatManager::BLOCK:
		case CombatManager::RICOCHET:
			stringName += "_fail";
			color = 0;
			break;
		default:
			break;
		}

		CombatManager::instance()->broadcastCombatSpam(attacker, nullptr, nullptr, damage, "cbt_spam", stringName, color);

	}

};

#endif //AREATAUNTCOMMAND_H_
