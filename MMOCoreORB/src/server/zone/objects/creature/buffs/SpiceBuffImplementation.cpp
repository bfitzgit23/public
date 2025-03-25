/*
 * SpiceBuffImplementation.cpp
 *
 *  Created on: 15/08/2010
 *      Author: victor
 */

#include "server/zone/objects/creature/buffs/SpiceBuff.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/creature/events/SpiceDownerAnimationTask.h"
#include "server/zone/objects/creature/buffs/SpiceDownerBuff.h"

void SpiceBuffImplementation::deactivate(bool removeModifiers) {

	BuffImplementation::deactivate(true);

	if (removeModifiers) {
		//StackTrace::printStackTrace();

		ManagedReference<CreatureObject*> creo = creature.get();

		if (creo == nullptr) {
			error("Creature object is nullptr.");
			return;
		}

		Buff* foodBuff = creo->getBuff(BuffCRC::FOOD_REDUCE_SPICE_DOWNTIME);

		if (foodBuff != nullptr && foodBuff->getSkillModifierValue("reduce_spice_downtime") >= 100) {// Skip downer if Smuggler's Delight is 100+
			debug("skipping downer");
			creo->sendSystemMessage("@spice/spice:" + buffName + "_done");
			creo->notifyObservers(ObserverEventType::SPICEDOWNERACTIVATED, creo, 0);
			return;
		}

		debug("activating downer");
		uint32 crc = String::hashCode("spice." + buffName + ".down");
		ManagedReference<Buff*> downer = new SpiceDownerBuff(creo, buffName, crc, 120);

		Locker locker(downer);

		setDownerAttributes(creo, downer);
		creo->addBuff(downer);

		// Puke now and every 15 seconds while debuff is active
		creo->doAnimation("heavy_cough_vomit");
		Reference<Task*> downerAnimationTask = new SpiceDownerAnimationTask(creo, crc);
		creo->addPendingTask("spice_downer_animation", downerAnimationTask, 15000);
	}
}

void SpiceBuffImplementation::setDownerAttributes(CreatureObject* creature, Buff* buff) {
	for (int i = 0; i < attributeModifiers.size(); ++i) {
		VectorMapEntry<uint8, int>* entry = &attributeModifiers.elementAt(i);

		uint8 attribute = entry->getKey();
		int value = entry->getValue();

		if (value <= 0)
			continue;

		int attributemax = creature->getMaxHAM(attribute) - creature->getWounds(attribute);

		int projvalue = attributemax - value;

		if (projvalue < 1)
			value += projvalue - 1;

		buff->setAttributeModifier(attribute, -value);
	}
}
