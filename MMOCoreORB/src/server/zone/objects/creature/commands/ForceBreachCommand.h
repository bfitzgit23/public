/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef FORCEBREACHCOMMAND_H_
#define FORCEBREACHCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "ForcePowersQueueCommand.h"


class ForceBreachCommand : public ForcePowersQueueCommand {
public:

	ForceBreachCommand(const String& name, ZoneProcessServer* server)
		: ForcePowersQueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		if (isWearingArmor(creature)) {
			return NOJEDIARMOR;
		}

		ManagedReference<SceneObject*> targetObject = server->getZoneServer()->getObject(target);

		if (targetObject == nullptr || !targetObject->isCreatureObject()) {
			return INVALIDTARGET;
		}

		ManagedReference<CreatureObject*> creatureTarget = targetObject->asCreatureObject();

		if (creatureTarget == nullptr)
			return INVALIDTARGET;

		int res = doCombatAction(creature, target);

		Locker clocker(creatureTarget, creature);  //Need to crosslocker before checking cooldowns, etc that can be updated by other threads

		if (!creatureTarget->checkCooldownRecovery("forceBreach")){
			creature->sendSystemMessage("The target's defenses have been breached too recently.");
			return GENERALERROR;
		}
		
		if (res == SUCCESS) {

			if (creatureTarget->hasBuff(BuffCRC::JEDI_FORCEBREACH)){
				creature->sendSystemMessage("The target's defenses are already breached.");
				return GENERALERROR;
			}

			int duration = 10 * creature->getFrsMod("control");
			if (!creatureTarget->isPlayerCreature())
				duration *= 2;
			ManagedReference<Buff*> buff = nullptr;
			buff = new Buff(creatureTarget, BuffCRC::JEDI_FORCEBREACH, duration, BuffType::JEDI);

			Locker blocker(buff);

			int dizzyDef = creatureTarget->getSkillMod("dizzy_defense");
			int stunDef = creatureTarget->getSkillMod("stun_defense");
			int blindDef = creatureTarget->getSkillMod("blind_defense");
			int knockdownDef = creatureTarget->getSkillMod("knockdown_defense");
			int posturedownDef = creatureTarget->getSkillMod("posture_change_down_defense");
			int jediStateDef = creatureTarget->getSkillMod("jedi_state_defense");
			int forceDef = creatureTarget->getSkillMod("force_defense");
			int forceArmor = creatureTarget->getSkillMod("force_armor");
			int forceShield = creatureTarget->getSkillMod("force_shield");
			float groupModifier = 1.0f;

			if (creatureTarget->isPlayerCreature()) {	//Check for how many enemies players are in 64 meter range of the target

				auto zone = creatureTarget->getZone();
				if (zone != nullptr) {

					CloseObjectsVector* closeObjectsVector = creatureTarget->getCloseObjects();
					SortedVector<ManagedReference<QuadTreeEntry*> > closeObjects;

					if (closeObjectsVector != nullptr) {
						closeObjectsVector->safeCopyReceiversTo(closeObjects, CloseObjectsVector::PLAYERTYPE);
					} else {
						zone->getInRangeObjects(creatureTarget->getWorldPositionX(), creatureTarget->getWorldPositionY(), 64, &closeObjects, true);
					}

					int numPlayers = 0;

					for (int i = 0; i < closeObjects.size(); ++i) {

						ManagedReference<SceneObject*> object = cast<SceneObject*>(closeObjects.get(i).get());

						if (object == nullptr || !object->isPlayerCreature())
							continue;

						ManagedReference<CreatureObject*> areaPlayer = object->asCreatureObject();

						if (areaPlayer == nullptr || areaPlayer == creature)
							continue;

						if (areaPlayer->isInvisible() || !areaPlayer->isAttackableBy(creatureTarget) || (closeObjectsVector != nullptr && !areaPlayer->isInRange(creatureTarget, 64)))
							continue;

						numPlayers++;
					}

					switch (numPlayers) {
						case 21 ... 500:
							groupModifier = 0.50f;		//50% reduction is breach strength
							break;
						case 16 ... 20:
							groupModifier = 0.75f;		//25% reduction is breach strength
							break;
						case 11 ... 15:
							groupModifier = 0.85f;		//15% reduction is breach strength
							break;
						case 0 ... 10:
							groupModifier = 1.0f;		// 0% reduction is breach strength
							break;	
						default:
							groupModifier = 1.0f;											
							break;
					}
				}
			}

			if (dizzyDef > 0)
				buff->setSkillModifier("dizzy_defense", (dizzyDef >= 15) ? -15 : (0 - dizzyDef));
			if (stunDef > 0)
				buff->setSkillModifier("stun_defense", (stunDef >= 15) ? -15 : (0 - stunDef));
			if (blindDef > 0)
				buff->setSkillModifier("blind_defense", (blindDef >= 15) ? -15 : (0 - blindDef));
			if (knockdownDef > 0)
				buff->setSkillModifier("knockdown_defense", (knockdownDef >= 15) ? -15 : (0 - knockdownDef));
			if (posturedownDef > 0)
				buff->setSkillModifier("posture_change_down_defense", (posturedownDef >= 15) ? -15 : (0 - posturedownDef));
			if (jediStateDef > 0)
				buff->setSkillModifier("jedi_state_defense", (jediStateDef >= 15) ? -15 : (0 - jediStateDef));

			float debuffPercentage = 0;
			float masterHealerReduction = 0.5f; //Master healers 50% less effect of Force Breach

			if (forceDef > 0) {
				debuffPercentage = 0.45f;
				if (creatureTarget->isPlayerCreature() && creatureTarget->hasSkill("force_discipline_healing_master")) {//Master healers 50% less
					debuffPercentage *= masterHealerReduction;
				}
				buff->setSkillModifier("force_defense",forceDef * -debuffPercentage * groupModifier); 		//now breaches 45% of force defense
			}
			if (forceArmor > 0) {
				debuffPercentage = 0.35f;
				if (creatureTarget->isPlayerCreature() && creatureTarget->hasSkill("force_discipline_healing_master")) {
					debuffPercentage *= masterHealerReduction;
				}

				buff->setSkillModifier("force_armor",forceArmor * -debuffPercentage * groupModifier); 		//now breaches 35% of force armor
			}
			if (forceShield > 0) {
				debuffPercentage = 0.50f;
				if (creatureTarget->isPlayerCreature() && creatureTarget->hasSkill("force_discipline_healing_master")) {
					debuffPercentage *= masterHealerReduction;
				}

				buff->setSkillModifier("force_shield",forceShield * -debuffPercentage * groupModifier); 	//now breaches 50% of force shield
			}

			creatureTarget->addBuff(buff);
			blocker.release();

			creatureTarget->playEffect("clienteffect/pl_force_resist_bleeding_self.cef", "");
			creatureTarget->playEffect("clienteffect/general_greivous_gutsack.cef", "");
			CombatManager::instance()->broadcastCombatSpam(creature, creatureTarget, nullptr, 0, "cbt_spam", combatSpam + "_hit", 1);
			creatureTarget->updateCooldownTimer("forceBreach", 45 * 1000); // 45 second cooldown.
		}
		return res;
	}
	float getCommandDuration(CreatureObject* object, const UnicodeString& arguments) const {
		return defaultTime;  // 1.0 Second
	}

};

#endif //FORCEBREACHCOMMAND_H_
