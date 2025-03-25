/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.
 */

#include "server/zone/objects/tangible/consumable/DelayedBuffObserver.h"
#include "server/zone/objects/creature/buffs/DelayedBuff.h"

int DelayedBuffObserverImplementation::notifyObserverEvent(unsigned int eventType, Observable* observable, ManagedObject* arg1, int64 arg2) {

	ManagedReference<DelayedBuff*> strongBuff = buff.get();	
	if (strongBuff == nullptr) {
		return 1;
	}

	Locker locker(strongBuff);

	strongBuff->useCharge();

	return 0;
}
