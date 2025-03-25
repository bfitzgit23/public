/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef FINDOBJECTCOMMAND_H_
#define FINDOBJECTCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/player/sui/callbacks/FindObjectSuiCallback.h"
#include "server/zone/objects/tangible/components/CityDecorationMenuComponent.h"
#include "server/zone/objects/tangible/components/DestructibleCityDecorationMenuComponent.h"

class FindObjectCommand : public QueueCommand {
public:

	FindObjectCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (creature == nullptr)
			return GENERALERROR;

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		if (!creature->isPlayerCreature())
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		if (ghost == nullptr)
			return GENERALERROR;

		auto zoneServer = creature->getZoneServer();
		if (zoneServer == nullptr)
			return GENERALERROR;

		try {
			StringTokenizer tokenizer(arguments.toString());

			Reference<SceneObject*> targetObject = nullptr;

			if (!tokenizer.hasMoreTokens()) {
				targetObject = zoneServer->getObject(creature->getTargetID());

				if (targetObject != nullptr) {
					Locker crossLocker(targetObject, creature);

					Vector3 worldPosition = targetObject->getWorldPosition();

					ManagedReference<WaypointObject*> obj = zoneServer->createObject(0xc456e788, 1).castTo<WaypointObject*>();

					Locker waypointGuard(obj);

					obj->setPlanetCRC(targetObject->getPlanetCRC());
					obj->setPosition(worldPosition.getX(), 0, worldPosition.getY());
					obj->setActive(true);
					obj->setCustomObjectName(targetObject->getDisplayedName(), false);

					ghost->addWaypoint(obj, false, true);
					return SUCCESS;
				}
			}

			auto zone = creature->getZone();
			if (zone == nullptr)
				return GENERALERROR;

			float x = creature->getWorldPositionX();
			float y = creature->getWorldPositionY();
			float cornerX = zone->getMaxX();
			float cornerY = zone->getMaxY();

			if (x >= 0) 
				cornerX *= -1;

			if (y >= 0) 
				cornerY *= -1;

			float maxRange = Vector3(x,0,y).distanceTo(Vector3(cornerX, 0, cornerY));

			float range = maxRange;

			String objectFilter;
			tokenizer.getStringToken(objectFilter);

			if (tokenizer.hasMoreTokens())
				range = tokenizer.getFloatToken();

			if (range > maxRange || range <= 0)
				range = maxRange;

			ManagedReference<SuiListBox*> findResults = new SuiListBox(creature, SuiWindowType::ADMIN_FIND_OBJECT);
			findResults->setCallback(new FindObjectSuiCallback(zoneServer));
			findResults->setPromptTitle("Find Object");
			findResults->setPromptText("Here are the objects that match your search:");
			findResults->setCancelButton(true, "");
			findResults->setOkButton(true, "@treasure_map/treasure_map:store_waypoint");
			findResults->setOtherButton(true, "@go");

			SortedVector<ManagedReference<QuadTreeEntry*> > objects(512, 512);
			zone->getInRangeObjects(x, y, range, &objects, true);

			for (int i = 0; i < objects.size(); ++i) {
				ManagedReference<SceneObject*> object = cast<SceneObject*>(objects.get(i).get());

				if (object == nullptr)
					continue;

				if (object == creature)
					continue;

				Locker crlocker(object, creature);

                ManagedReference<TangibleObject*> tano = cast<TangibleObject*>(object.get());		

				String serial = "";
				
				if (tano != nullptr)
					serial = tano->getSerialNumber();

				String name = object->getDisplayedName();

				if (objectFilter == "-p") {
					if (!object->isPlayerCreature()) {
						continue;
					}
				}
				else if (objectFilter == "-d") { // deco 

					CityDecorationMenuComponent* decorationMenu = cast<CityDecorationMenuComponent*>(object->getObjectMenuComponent());
					DestructibleCityDecorationMenuComponent* destructibleDecorationMenu = cast<DestructibleCityDecorationMenuComponent*>(object->getObjectMenuComponent());

					if (decorationMenu == nullptr && destructibleDecorationMenu == nullptr) { // Not city deco
						continue;
					}
					else if (object->getCityRegion() != nullptr || object->isClientObject()) {	//skip anything inside city
						continue;
					}
					else if (object->getRootParent() != nullptr) {  //Skip anything inside a cell
						continue;
					}

				}
				else if (!name.toLowerCase().contains(objectFilter.toLowerCase()) && (!serial.toLowerCase().contains(objectFilter.toLowerCase()))) {
					continue;
				}

				StringBuffer results;
				results << name;
				results << " (" << String::valueOf(object->getWorldPositionX());
				results << ", " << String::valueOf(object->getWorldPositionY()) << ")";

				findResults->addMenuItem(results.toString(), object->getObjectID());
			}

			if (findResults->getMenuSize() < 1) {
				creature->sendSystemMessage("No objects were found that matched that filter.");
				return SUCCESS;
			}

			ghost->addSuiBox(findResults);
			creature->sendMessage(findResults->generateMessage());

		} catch (Exception& e) {
			creature->sendSystemMessage("Syntax: /findobject <string filter> <range>");
			creature->sendSystemMessage("Syntax: /findobject <-d | -p> <range>");
		}

		return SUCCESS;
	}

};

#endif //FINDOBJECTCOMMAND_H_