/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef REMOVESHUTTLEIMMUNITYTASK_H_
#define REMOVESHUTTLEIMMUNITYTASK_H_

#include "server/zone/objects/creature/CreatureObject.h"

class RemoveShuttleImmunityTask : public Task {
	ManagedReference<CreatureObject*> player;
	String taskName;

public:
	RemoveShuttleImmunityTask(CreatureObject* creo, const String& task) : Task() {
		player = creo;
	}

	void run() {

		if (player == nullptr)
			return;

		Locker locker(player);
		player->removePendingTask("shuttle_immunity");
		player->broadcastPvpStatusBitmask();	

	}
};

#endif /*REMOVESHUTTLEIMMUNITYTASK_H_*/
