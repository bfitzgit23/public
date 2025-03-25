/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef THROWGRENADECOMMAND_H_
#define THROWGRENADECOMMAND_H_

#include "engine/core/TaskManager.h"

class ThrowGrenadeCommand : public CombatQueueCommand {
public:
	ThrowGrenadeCommand(const String& name, ZoneProcessServer* server) : CombatQueueCommand(name, server) {
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {
		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		StringTokenizer tokenizer(arguments.toString());

		if (!tokenizer.hasMoreTokens())
			return INVALIDPARAMETERS;

		try {
			uint64 weaponID = tokenizer.getLongToken();

			Reference<WeaponObject*> grenade = server->getZoneServer()->getObject(weaponID).castTo<WeaponObject*>();

			if (grenade == nullptr || !grenade->isThrownWeapon())
				return INVALIDPARAMETERS;

			if (!grenade->isASubChildOf(creature))
				return GENERALERROR;

			ManagedReference<TangibleObject*> targetObject = server->getZoneServer()->getObject(target).castTo<TangibleObject*>();

			if (targetObject == nullptr)
				return GENERALERROR;

			if (!(targetObject->isAttackableBy(creature)))
				return INVALIDTARGET;

			SharedObjectTemplate* templateData = TemplateManager::instance()->getTemplate(grenade->getServerObjectCRC());
			if (templateData == nullptr)
				return GENERALERROR;

			SharedWeaponObjectTemplate* grenadeData = cast<SharedWeaponObjectTemplate*>(templateData);
			if (grenadeData == nullptr)
				return GENERALERROR;

			bool isCortosis = grenadeData->getCombatSpam() == "throw_cortosis";
			ManagedReference<CreatureObject*> targetCreature = targetObject->asCreatureObject();

			if (isCortosis) {

				if(!creature->hasSkill("combat_bountyhunter_master")) {	
					creature->sendSystemMessage("You must be a master bounty hunter to use a Cortosis grenade.");
					return GENERALERROR;
				}

				if (targetCreature == nullptr)
					return INVALIDTARGET;

				ManagedReference<PlayerObject*> targetGhost = targetCreature->getPlayerObject();
				if (targetGhost == nullptr)
					return INVALIDTARGET;

				if (!targetGhost->isJedi()) {
					creature->sendSystemMessage("The target of a Cortosis grenade must be a player Jedi target.");
					return INVALIDTARGET;
				}

				if (!creature->hasBountyMissionFor(targetCreature)) {
					creature->sendSystemMessage("You must have a bounty mission for a Jedi target to use a Cortosis grenade.");
					return INVALIDTARGET;
				}
			}

			UnicodeString args = "combatSpam=" + grenadeData->getCombatSpam() + ";";

			int result = doCombatAction(creature, target, args, grenade);

			if (result == SUCCESS) {
				// We need to give some time for the combat animation to start playing before destroying the tano
				// otherwise our character will play the wrong animations

				Core::getTaskManager()->scheduleTask([grenade] {
					Locker locker(grenade);
					grenade->decreaseUseCount();
				}, "ThrowGrenadeTanoDecrementTask", 100);

				uint32 buffCRC = BuffCRC::JEDI_CORTOSIS;
				if (isCortosis && targetCreature != nullptr && !targetCreature->hasBuff(buffCRC)){
					Core::getTaskManager()->scheduleTask([creature, targetCreature] {
						if (creature != nullptr and targetCreature != nullptr) {
							Locker locker(targetCreature);
							uint32 buffCRC = BuffCRC::JEDI_CORTOSIS;
							ManagedReference<Buff*> buff = new Buff(targetCreature, buffCRC, 60, BuffType::JEDI);	
							Locker blocker(buff, targetCreature);
							buff->setSkillModifier("jedi_force_power_regen", -10);
							targetCreature->addBuff(buff);
							creature->sendSystemMessage("Your Cortosis grenade successfully poisons the target and reduces the target's force regeneration.");
							targetCreature->sendSystemMessage("You feel the effect of Cortosis poison reducing your Force regeneration.");
						}
					}, "CortosisEffectApply", 1000);										
				}
			}

			return result;

		} catch (Exception& e) {
		}

		return GENERALERROR;
	}

	String getAnimation(TangibleObject* attacker, TangibleObject* defender, WeaponObject* weapon, uint8 hitLocation, int damage) const {
		SharedWeaponObjectTemplate* weaponData = cast<SharedWeaponObjectTemplate*>(weapon->getObjectTemplate());

		if (weaponData == nullptr) {
			warning("Null weaponData in ThrowGrenadeCommand::getAnimation");
			return "";
		}

		String type = weaponData->getAnimationType();

		if (type.isEmpty())
			return "throw_grenade";

		int range = attacker->getWorldPosition().distanceTo(defender->getWorldPosition());

		String distance = "";

		if (range < 10) {
			distance = "_near_";
		} else if (range < 20) {
			distance = "_medium_";
		} else {
			distance = "_far_";
		}

		return "throw_grenade" + distance + type;
	}

	float getCommandDuration(CreatureObject* object, const UnicodeString& arguments) const {
		StringTokenizer tokenizer(arguments.toString());

		if (!tokenizer.hasMoreTokens()) {
			return 10.f;
		}

		uint64 weaponID = tokenizer.getLongToken();

		auto zoneServer = server->getZoneServer();

		if (zoneServer == nullptr) {
			return 10.f;
		}

		Reference<WeaponObject*> grenade = zoneServer->getObject(weaponID).castTo<WeaponObject*>();

		if (grenade == nullptr) {
			return 10.f;
		}

		return CombatManager::instance()->calculateWeaponAttackSpeed(object, grenade, speedMultiplier);
	}
};

#endif // THROWGRENADECOMMAND_H_
