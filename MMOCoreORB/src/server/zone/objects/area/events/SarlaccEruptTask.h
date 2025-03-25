/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef SARLACCERUPTTASK_H_
#define SARLACCERUPTTASK_H_

#include "engine/engine.h"
#include "server/zone/Zone.h"
#include "server/zone/objects/area/SarlaccArea.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "templates/params/creature/CreatureAttribute.h"

class SarlaccEruptTask: public Task {
	ManagedReference<SarlaccArea*> sarlaccArea;


public:
	SarlaccEruptTask(SarlaccArea* area) {
		sarlaccArea = area;
	}

	void run() {
		if (sarlaccArea == nullptr) {
			return;
		}

		SortedVector<ManagedReference<QuadTreeEntry*> > closeObjects;
		CloseObjectsVector* closeObjectsVector = (CloseObjectsVector*) sarlaccArea->getCloseObjects();
		if (closeObjectsVector == nullptr) {
			auto zone = sarlaccArea->getZone();

			if (zone == nullptr) {
				return;
			}
			zone->getInRangeObjects(sarlaccArea->getWorldPositionX(), sarlaccArea->getWorldPositionY(), 60, &closeObjects, true);
		} else {
			closeObjectsVector->safeCopyReceiversTo(closeObjects, CloseObjectsVector::PLAYERTYPE);
		}

		for (int i = 0; i < closeObjects.size(); ++i) {
			ManagedReference<SceneObject*> scno = cast<SceneObject*>(closeObjects.get(i).get());

			if (scno != nullptr && scno->isPlayerCreature()) {

				if (!scno->isInRange(sarlaccArea, 60)) { //Check range in case using closeObjects vector
					continue;
				}

				ManagedReference<CreatureObject*> playerCreature = cast<CreatureObject*>(scno.get());

				if (playerCreature == nullptr)
					continue;

				playerCreature->sendSystemMessage("@mob/sarlacc:sarlacc_erupt"); // The Sarlacc suddenly erupts, spewing a diseased and corrosive substance into the air!

				if (playerCreature->getSkillMod("resistance_disease") < 24) {
					Locker locker(playerCreature);
					playerCreature->addDotState(playerCreature, CreatureState::DISEASED, 0, 30 + System::random(20), CreatureAttribute::HEALTH, 30 * 60, 2000, 0);
					playerCreature->sendSystemMessage("@mob/sarlacc:sarlacc_dot"); // You suddenly feel weak and sick.
				}
			}
		}

	}
};

#endif /* SARLACCERUPTTASK_H_ */
