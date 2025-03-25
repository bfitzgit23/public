/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef SAMPLEDNACOMMAND_H_
#define SAMPLEDNACOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/creature/ai/Creature.h"

class SampleDNACommand : public QueueCommand {
public:

	SampleDNACommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		if (creature->isDead()){
			return INVALIDTARGET;
		}

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		if (ghost == nullptr) {
			return GENERALERROR;
		}

		if (ghost->isAFK()) {
			return GENERALERROR;
		}

		auto zoneServer = server->getZoneServer();
		if (zoneServer == nullptr) {
			return GENERALERROR;
		}

		ManagedReference<SceneObject* > object = zoneServer->getObject(target);
		if (object == nullptr) {
			creature->sendSystemMessage("@bio_engineer:harvest_dna_need_target"); // You need to target the creature you wish to take a DNA sample from.
			return INVALIDTARGET;
		}

		if (!creature->hasSkill("outdoors_bio_engineer_novice")) {
			creature->sendSystemMessage("@bio_engineer:harvest_dna_skill_too_low"); // You are not sufficiently skilled to take DNA samples.
			return GENERALERROR;
		}

		if (!object->isCreatureObject() || object->isPlayerCreature()){
			creature->sendSystemMessage("@bio_engineer:harvest_dna_invalid_target"); // You cannot sample DNA from that target.
			return INVALIDTARGET;
		}

		ManagedReference<CreatureObject*> targetCreo = cast<CreatureObject*>(object.get());
		ManagedReference<Creature*> targetCreature = cast<Creature*>(object.get());

		if (targetCreo == nullptr || targetCreature == nullptr){
			creature->sendSystemMessage("@bio_engineer:harvest_dna_invalid_target"); // You cannot sample DNA from that target.
			return INVALIDTARGET;
		}

		Locker crosslocker(targetCreo,creature);

		// SOE Patch notes Sept 4, 2003 all pet sampling disabled, added as radial to pet deed
		if (targetCreature->isPet()) {
			// allow them to sample their own be pets
			creature->sendSystemMessage("@bio_engineer:harvest_dna_target_pet"); // You cannot sample DNA from that pet.
			return INVALIDTARGET;
		}

		if (!CollisionManager::checkLineOfSight(creature, targetCreo)) {
			creature->sendSystemMessage("@container_error_message:container18"); // You can't see that object. You may have to move closer to it.
			return GENERALERROR;
		}

		if (!targetCreature->isAttackableBy(creature)){
			creature->sendSystemMessage("@bio_engineer:harvest_dna_invalid_target"); // You cannot sample DNA from that target.
			return INVALIDTARGET;
		}

		// Sample DNA is a 16M max range
		if (!checkDistance(targetCreature, creature, 16.0f)){
			creature->sendSystemMessage("@bio_engineer:harvest_dna_out_of_range"); // Your target is too far away to be able to sample from.
			return TOOFAR;
		}

		if (targetCreature->isDead()){
			creature->sendSystemMessage("@bio_engineer:harvest_dna_target_corpse"); // You cannot sample DNA from a corpse.
			return INVALIDTARGET;
		}

		if (targetCreature->isBaby()) {
			creature->sendSystemMessage("@bio_engineer:harvest_dna_target_baby"); // That creature is too young to get a viable DNA sample.
			return INVALIDTARGET;
		}

		if (targetCreature->getLevel() > 350) {
			creature->sendSystemMessage("@bio_engineer:harvest_dna_level_too_high"); // You cannot sample DNA from a creature over level 350.
			return INVALIDTARGET;
		}

		if (targetCreature->isInCombat()) {
			creature->sendSystemMessage("@bio_engineer:harvest_dna_creature_in_combat"); // It is too dangerous to sample DNA from a creature while it's in combat.
			return INVALIDTARGET;
		}

		if (targetCreature->canCollectDna(creature)) {

			auto zone = creature->getZone();
			if (zone == nullptr) {
				return GENERALERROR;
			}

			ManagedReference<CreatureManager*> creatureManager = zone->getCreatureManager();
			if (creatureManager == nullptr) {
				return GENERALERROR;
			}

			creatureManager->sample(targetCreature, creature);
		}
		else {
			creature->sendSystemMessage("@bio_engineer:harvest_dna_cant_harvest"); // You cannot sample DNA from that target.
		}

		return SUCCESS;
	}
};

#endif //SAMPLEDNACOMMAND_H_
