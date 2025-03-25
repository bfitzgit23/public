
#ifndef PETFOLLOWCOMMAND_H_
#define PETFOLLOWCOMMAND_H_

#include "server/zone/objects/creature/commands/QueueCommand.h"
#include "server/zone/objects/creature/ai/AiAgent.h"
#include "server/zone/objects/creature/ai/DroidObject.h"
#include "server/zone/managers/combat/CombatManager.h"
#include "templates/params/ObserverEventType.h"
#include "server/zone/managers/creature/PetManager.h"
#include "server/zone/objects/tangible/components/droid/DroidDetonationModuleDataComponent.h"

class PetFollowCommand : public QueueCommand {
public:
	PetFollowCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {
	}


	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (creature == nullptr) {
			return GENERALERROR;
		}

		ManagedReference<PetControlDevice*> controlDevice = creature->getControlDevice().get().castTo<PetControlDevice*>();

		if (controlDevice == nullptr) {
			return GENERALERROR;
		}

		ManagedReference<AiAgent*> pet = cast<AiAgent*>(creature);

		if (pet == nullptr) {
			return GENERALERROR;
		}

		if (pet->hasRidingCreature()) {
			return GENERALERROR;
		}

		ManagedReference<SceneObject*> targetObject = server->getZoneServer()->getObject(target, true);
		if (targetObject == nullptr || !targetObject->isCreatureObject() ) { // pets should be able to follow other mobiles as a command. i found multiple references to this. -- washu
			pet->showFlyText("npc_reaction/flytext","confused", 204, 0, 0);  // "?!!?!?!"
			return GENERALERROR;
		}

		StringTokenizer tokenizer(arguments.toString());

		if (!tokenizer.hasMoreTokens()) {
			return GENERALERROR;
		}

		uint64 playerID = tokenizer.getLongToken();

		ManagedReference<CreatureObject*> player = server->getZoneServer()->getObject(playerID, true).castTo<CreatureObject*>();

		if (player == nullptr) {
			return GENERALERROR;
		}

		CreatureObject* targetCreature = cast<CreatureObject*>(targetObject.get());

		if (targetCreature == nullptr) {
			return GENERALERROR;
		}

		ManagedReference<DroidObject*> droidPet = nullptr;
		// Droid Pet Checks
		if (controlDevice->getPetType() == PetManager::DROIDPET) {
			droidPet = cast<DroidObject*>(pet.get());
			if (droidPet == nullptr) {
				return GENERALERROR;
			}
			if (!droidPet->hasPower()) {
				pet->showFlyText("npc_reaction/flytext","low_power", 204, 0, 0);  // "*Low Power*"
				return GENERALERROR;
			}
		}

		if (targetCreature != player) { //This is follow other, not follow owner

			if (!CollisionManager::checkLineOfSight(player, targetObject) || !playerEntryCheck(player, targetCreature)) {
				pet->showFlyText("npc_reaction/flytext","confused", 204, 0, 0);  // "?!!?!?!"
				return INVALIDTARGET;
			}

			if (targetCreature->isAttackableBy(creature)) { // Can't follow attackable creature unless using bomb droid
				if (droidPet == nullptr) { // Not a droid, fail
					pet->showFlyText("npc_reaction/flytext","confused", 204, 0, 0);  // "?!!?!?!"
					return INVALIDTARGET;
				}
				else {  // Bomb droid check
					auto detonationModule = droidPet->getModule("detonation_module").castTo<DroidDetonationModuleDataComponent*>();
					if (detonationModule == nullptr) { // Not a bomb droid, fail
						pet->showFlyText("npc_reaction/flytext","confused", 204, 0, 0);  // "?!!?!?!"
						return INVALIDTARGET;
					}
				}		
			}

		}

		// attempt peace if the pet is in combat
		if (pet->isInCombat())
			CombatManager::instance()->attemptPeace(pet);

		Locker clocker(controlDevice, creature);
		controlDevice->setLastCommandTarget(targetCreature);
		clocker.release();

		pet->setFollowObject(targetCreature);
		pet->storeFollowObject();

		if (pet->isResting()) {
			pet->setMovementState(AiAgent::FOLLOWING);
		}

		pet->notifyObservers(ObserverEventType::STARTCOMBAT, pet->getLinkedCreature().get());

		return SUCCESS;
	}

};

#endif /* PETFOLLOWCOMMAND_H_ */
