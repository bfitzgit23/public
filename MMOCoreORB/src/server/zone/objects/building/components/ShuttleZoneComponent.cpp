/*
 * ShuttleZoneComponent.cpp
 *
 *  Created on: Aug 19, 2011
 *      Author: crush
 */

#include "ShuttleZoneComponent.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/building/tasks/ScheduleShuttleTask.h"

//#define SHUTTLE_TIMER_DEBUG

void ShuttleZoneComponent::notifyInsertToZone(SceneObject* sceneObject, Zone* zone) const {
	ZoneComponent::notifyInsertToZone(sceneObject, zone);

	if (sceneObject == nullptr) {
		error() << "ShuttleZoneComponent::notifyInsertToZone -- inserted object is null";
		return;
	}

	if (!sceneObject->isCreatureObject()) {
		error() << "ShuttleZoneComponent::notifyInsertToZone -- inserted object is not a Creature Object";
		return;
	}

	auto shuttle = sceneObject->asCreatureObject();
	if (shuttle == nullptr)
		return;

	auto zoneServer = zone->getZoneServer();
	if (zoneServer == nullptr)
		return;

	float x = shuttle->getWorldPositionX();
	float y = shuttle->getWorldPositionY();

	Reference<ScheduleShuttleTask*> task = new ScheduleShuttleTask(shuttle, zone);
	if (task == nullptr)
		return;

	int delay = 500;

	if (zoneServer->isServerLoading()) {
		uint32 startDiff = zone->getZoneServer()->getStartTimestamp()->miliDifference();

		// Shuttles delayed 5 minutes for server start
		int bootDelay = ConfigManager::instance()->getInt("Core3.ShuttleZoneComponent.BootDelay", 5 * 60 * 1000);

		delay = bootDelay - startDiff;

		if (delay <= 0)
			delay = 500;

#ifdef SHUTTLE_TIMER_DEBUG
		info(true) << "ScheduleShuttleTask for " << zone->getZoneName() << " scheduled due to server loading: militime since server start - " << startDiff << "  shuttle delay time - " << delay << " miliseconds.";
#endif
	}

	/* Infinity:  HG Shuttle logging for testing
	if (x >= -5044 && x <= -5043) {
			StringBuffer msg;
			msg << "Scheduling start of Highgarden Shuttle" << endl;
			msg << "Zone: " << zone->getZoneName() << "  X: " << x << "  Y: " << y << endl;

			Logger::console.error(msg.toString());
	}
	*/

	task->schedule(delay);
}

void ShuttleZoneComponent::notifyRemoveFromZone(SceneObject* sceneObject) const {
	ZoneComponent::notifyRemoveFromZone(sceneObject);
}
