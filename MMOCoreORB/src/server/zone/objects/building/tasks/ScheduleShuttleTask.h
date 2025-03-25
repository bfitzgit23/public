
#ifndef SCHEDULESHUTTLETASK_H_
#define SCHEDULESHUTTLETASK_H_

#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/managers/planet/PlanetManager.h"

class ScheduleShuttleTask : public Task, public Logger {
	ManagedWeakReference<CreatureObject*> shuttleObject;
	Zone* zone;

public:
	ScheduleShuttleTask(CreatureObject* shuttle, Zone* zon) : Task() {
		shuttleObject = shuttle;
		zone = zon;

		Logger::setLoggingName("ScheduleShuttleTask");
	}

	void run() {
		if (zone == nullptr) {
			error() << " zone has a nullptr.";
			return;
		}

		String zoneName = zone->getZoneName();

		auto zoneServer = zone->getZoneServer();
		if (zoneServer == nullptr) {
			error() << " zoneServer is nullptr.";
			return;
		}

		if (zoneServer->isServerLoading()) {
			schedule((10 + System::random(10)) * 1000);
			return;
		}

		ManagedReference<CreatureObject*> strongShuttle = shuttleObject.get();
		if (strongShuttle == nullptr) {
			error() << " Shuttle strongShuttle has a nullptr in Zone: " << zoneName;
			return;
		}

		Locker lock(strongShuttle);

		if (!strongShuttle->checkCooldownRecovery("shuttleStart")) {
			//Logger::console.error(" Shuttle already started in Zone: " + zoneName);
			return;
		}

		ManagedReference<PlanetManager*> planetManager = zone->getPlanetManager();
		if (planetManager == nullptr) {
			zone->error() << " planetManager has a nullptr in Zone: " << zoneName;
			return;
		}

		ManagedReference<CityRegion*> cityRegion = strongShuttle->getCityRegion().get();

		float x = strongShuttle->getWorldPositionX();
		float y = strongShuttle->getWorldPositionY();
		float z = strongShuttle->getWorldPositionZ();

		/* Infinity:  Shuttle travel point logging
		bool isHG = false;
		if (x >= -5044 && x <= -5043) {

			isHG = true;
			StringBuffer msg;
			msg << "Checking Highgarden Shuttle" << endl;
			msg << "Zone: " << zoneName << "  X: " << x << "  Y: " << y << endl;

			Logger::console.error(msg.toString());
	
		}
		*/

		// Player City
		if ((cityRegion != nullptr) && !cityRegion->isClientRegion()) {

			Vector3 arrivalVector(x, y, z);

			Locker clocker(cityRegion, strongShuttle);

			cityRegion->setShuttleID(strongShuttle->getObjectID());
			clocker.release();

			PlanetTravelPoint* planetTravelPoint = new PlanetTravelPoint(zoneName, cityRegion->getCityRegionName(), arrivalVector, arrivalVector, strongShuttle, 6.f);

			/* Infinity:  Shuttle travel point logging
			if (isHG) {
				Logger::console.error("Created planetTravelPoint for HG shuttle");
			}
			*/

			planetManager->addPlayerCityTravelPoint(planetTravelPoint);
			planetManager->scheduleShuttle(strongShuttle, PlanetManager::SHUTTLEPORT);

			strongShuttle->updateCooldownTimer("shuttleStart", 2000);

		} else {
			Reference<PlanetTravelPoint*> travelPoint = planetManager->getNearestPlanetTravelPoint(strongShuttle, 128.f);

			if (travelPoint == nullptr) {
				error() << " Planet Travel Point (travelPoint) has a nullptr in Zone: " << zoneName;
				return;
			}

			/* Infinity:  Shuttle travel point logging
			if (isHG) {
				Logger::console.error("Should never get here, something is FUBAR and it is probably assigning Theed Shuttle B");
			}
			*/

			auto oldShuttle = travelPoint->getShuttle();

			if (oldShuttle == nullptr) {
				travelPoint->setShuttle(strongShuttle);

				if (travelPoint->isInterplanetary()) {
					planetManager->scheduleShuttle(strongShuttle, PlanetManager::STARPORT);
				} else {
					planetManager->scheduleShuttle(strongShuttle, PlanetManager::SHUTTLEPORT);
				}
			} else if (oldShuttle != strongShuttle) {
				/* Infinity:  Shuttle travel point logging
				if (isHG) {
					Logger::console.error("Deleting shuttle, something is FUBAR");
				}
				*/
				strongShuttle->destroyObjectFromWorld(true);
				strongShuttle->destroyObjectFromDatabase(true);
			}
		}
	}
};

#endif /* SCHEDULESHUTTLETASK_H_ */
