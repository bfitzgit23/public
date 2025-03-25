/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef OPENCONTAINERCOMMAND_H_
#define OPENCONTAINERCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/tangible/Container.h"
#include "server/zone/objects/tangible/RelockLootContainerEvent.h"

class OpenContainerCommand : public QueueCommand {
public:
	OpenContainerCommand(const String& name, ZoneProcessServer* server) : QueueCommand(name, server) {
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

		// Check if mounted
		if (creature->isRidingMount()) {
			creature->sendSystemMessage("@error_message:survey_on_mount"); // You cannot perform that action while mounted on a creature or driving a vehicle.
			return INVALIDLOCOMOTION;
		}

		auto zoneServer = server->getZoneServer();

		if (zoneServer == nullptr) {
			return GENERALERROR;
		}

		ManagedReference<SceneObject*> objectToOpen = zoneServer->getObject(target);

		if (objectToOpen == nullptr) {
			return GENERALERROR;
		}

		ManagedReference<SceneObject*> objectsParent = objectToOpen->getParent();
		bool craftingStation = objectsParent != nullptr && objectsParent->isCraftingStation();

		bool playerItem = objectToOpen->getParentRecursively(SceneObjectType::PLAYERCREATURE) != nullptr;

		ManagedReference<SceneObject*> rootParent = objectToOpen->getRootParent();
		bool clientObject = rootParent != nullptr && rootParent->isClientObject();

		/* Open Container Debug
		StringBuffer msg;
		msg << endl <<  "OPEN CONTAINER DEBUG" << endl;
		msg << "Object To Open: " << objectToOpen->getDisplayedName() << "  OID: " << objectToOpen->getObjectID() << "  LOS to player: " << CollisionManager::checkLineOfSight(objectToOpen, creature) << endl;
		msg << "Distance Object to Player (2D): " << creature->getDistanceTo(objectToOpen) << "  Distance Object to Player (3D): " << creature->getDistanceTo3d(objectToOpen) << endl;
		if (objectsParent != nullptr) {
			msg << "Object's Parent: " << objectsParent->getDisplayedName() << "  OID: " << objectsParent->getObjectID() << "  LOS to player: " << CollisionManager::checkLineOfSight(objectsParent, creature) << endl;
			msg << "Distance Object's Parent to Player (2D): " << creature->getDistanceTo(objectsParent) << "  Distance Object's Parent to Player (3D): " << creature->getDistanceTo3d(objectsParent) << endl;
		}
		msg << "Crafting Station: " << craftingStation << "  Player Held: " << playerItem << "  Client Object: " << clientObject << "  objectsParent == nullptr: " << (objectsParent == nullptr) << endl;
		creature->sendSystemMessage(msg.toString());
		//creature->info(true) << msg.toString();
		*/

		if (craftingStation) {

			if (!creature->isInRange(objectsParent, 12.0f)) {
				StringIdChatParameter param;
				param.setStringId("@container_error_message:container09_prose"); // You are out of range of %TT.
				param.setTT(objectsParent->getObjectName());
				creature->sendSystemMessage(param);
				return TOOFAR;
			}

			if (!CollisionManager::checkLineOfSight(objectsParent, creature)) {
				StringIdChatParameter msgParam;
				msgParam.setStringId("@container_error_message:container18_prose"); // You can't see %TT. You may have to move closer to it.
				msgParam.setTT(objectsParent->getObjectName());
				creature->sendSystemMessage(msgParam);
				return GENERALERROR;
			}

		} else if (!playerItem && (objectsParent == nullptr || clientObject)) {//Perform checks if not player item and outside, or inside in clientObject structure

			if (!creature->isInRange(objectToOpen, 7.0f)) {
				StringIdChatParameter param;
				param.setStringId("@container_error_message:container09_prose"); // You are out of range of %TT.
				param.setTT(objectToOpen->getObjectName());
				creature->sendSystemMessage(param);
				return TOOFAR;
			}

			if (!CollisionManager::checkLineOfSight(objectToOpen, creature)) {
				StringIdChatParameter msgParam;
				msgParam.setStringId("@container_error_message:container18_prose"); // You can't see %TT. You may have to move closer to it.
				msgParam.setTT(objectToOpen->getObjectName());
				creature->sendSystemMessage(msgParam);
				return GENERALERROR;
			}
		}

		Locker clocker(objectToOpen, creature);

		ManagedReference<Container*> container = objectToOpen.castTo<Container*>();
		if (container != nullptr && container->isContainerLocked()) {
			creature->sendSystemMessage("@slicing/slicing:locked");
			return SUCCESS;
		}

		if (objectToOpen->checkContainerPermission(creature, ContainerPermissions::OPEN)) {
			if (objectToOpen->getGameObjectType() == SceneObjectType::STATICLOOTCONTAINER) {
				if (container != nullptr && container->isRelocking() == false) {
					Reference<RelockLootContainerEvent*> relockEvent = new RelockLootContainerEvent(container);
					relockEvent->schedule(container->getLockTime());
				}
			}

			objectToOpen->openContainerTo(creature);

			objectToOpen->notifyObservers(ObserverEventType::OPENCONTAINER, creature);
		} else {
			// You do not have permission to access this container.
			creature->sendSystemMessage("@error_message:perm_no_open");
		}

		return SUCCESS;
	}
};

#endif // OPENCONTAINERCOMMAND_H_
