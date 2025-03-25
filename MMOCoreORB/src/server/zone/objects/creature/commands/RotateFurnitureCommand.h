/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef ROTATEFURNITURECOMMAND_H_
#define ROTATEFURNITURECOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/tangible/components/EventPerkDataComponent.h"
#include "server/zone/objects/tangible/components/CityDecorationMenuComponent.h"
#include "server/zone/objects/tangible/components/vendor/VendorDataComponent.h"

class RotateFurnitureCommand : public QueueCommand {
public:
	RotateFurnitureCommand(const String& name, ZoneProcessServer* server) : QueueCommand(name, server) {
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {
		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		auto ghost = creature->getPlayerObject();
		if (ghost == nullptr)
			return GENERALERROR;

		if (target == 0) {
			creature->sendSystemMessage("No target is selected.  What do you want to rotate?");
			return GENERALERROR;
		}

		auto zoneServer = server->getZoneServer();
		if (zoneServer == nullptr)
			return GENERALERROR;

		ManagedReference<SceneObject*> object = zoneServer->getObject(target);

		if (object == nullptr || !object->isTangibleObject()) {
			creature->sendSystemMessage("Target object is null or not a tangible object.");
			return GENERALERROR;
		}

		bool isTrainer = object->getObjectTemplate()->getFullTemplateString().contains("trainer");

		if (object->isPlayerCreature() || object->isPet() || (object->isCreatureObject() && !object->isVendor() && !isTrainer)) {
			creature->sendSystemMessage("Players and non-vendor/non-trainer creature objects cannot be moved with this command.");
			return GENERALERROR;
		}

		ManagedReference<SceneObject*> creatureRootParent = creature->getRootParent();
		ManagedReference<SceneObject*> objectRootParent = object->getRootParent();
		ManagedReference<BuildingObject*> buildingObject = creatureRootParent != nullptr ? (creatureRootParent->isBuildingObject() ? cast<BuildingObject*>(creatureRootParent.get()) : nullptr) : nullptr;
		ManagedReference<CityRegion*> city = nullptr;

		if (ghost->getAdminLevel() != 15) { //// Restrictions for non-admin characters

			EventPerkDataComponent* data = cast<EventPerkDataComponent*>(object->getDataObjectComponent()->get());
			if (data != nullptr) {
				EventPerkDeed* deed = data->getDeed();

				if (deed == nullptr) {
					return GENERALERROR;
				}

				ManagedReference<CreatureObject*> owner = deed->getOwner().get();
				if (owner == nullptr || owner != creature) {
					creature->sendSystemMessage("You must be the owner of an event perk to rotate the item.");
					return GENERALERROR;
				}

			} else if (buildingObject == nullptr) { // We're outside

				if (creatureRootParent != nullptr && objectRootParent != nullptr) {
					creature->sendSystemMessage("The player and the object both need to be outside.");
					return GENERALERROR;
				}

				city = creature->getCityRegion().get();
				if (city == nullptr) { // Not in a city
					creature->sendSystemMessage("You are not located in a player city.");
					return GENERALERROR;
				}

				if (!city->isMayor(creature->getObjectID())) { // Not mayor
					creature->sendSystemMessage("You must be mayor to rotate objects in a city.");
					return GENERALERROR;
				}

				if (!creature->hasSkill("social_politician_master")) {
					creature->sendSystemMessage("You must be a master politician to rotate objects in your city.");
					return GENERALERROR;
				}

				CityDecorationMenuComponent* decorationMenu = cast<CityDecorationMenuComponent*>(object->getObjectMenuComponent());
				DestructibleCityDecorationMenuComponent* destructibleDecorationMenu = cast<DestructibleCityDecorationMenuComponent*>(object->getObjectMenuComponent());

				bool isDeco = decorationMenu != nullptr || destructibleDecorationMenu != nullptr;
				bool isValidCityItem = isDeco | object->isMissionTerminal() || isTrainer;

				if (!isValidCityItem) {
					creature->sendSystemMessage("You can only move objects that are city decorations, mission terminals, or trainers.");
					return GENERALERROR;
				}
			} else {
				bool onAdmin = buildingObject != nullptr && buildingObject->isOnAdminList(creature);
				bool onVendor = buildingObject != nullptr && buildingObject->isOnPermissionList("VENDOR", creature);

				if (object->isVendor()) {
					if (!onAdmin && !onVendor) {
						creature->sendSystemMessage("You must have admin or vendor access rights to the structure to rotate a vendor.");
						return GENERALERROR;
					}

					DataObjectComponentReference* data = object->getDataObjectComponent();
					if (data == nullptr || data->get() == nullptr || !data->get()->isVendorData()) {
						creature->sendSystemMessage("Cannot determine the owner of vendor so unable to move vendor.");
						return GENERALERROR;
					}

					VendorDataComponent* vendorData = cast<VendorDataComponent*>(data->get());
					if (vendorData == nullptr) {
						creature->sendSystemMessage("Cannot determine the owner of vendor so unable to move vendor.");
						return GENERALERROR;
					}

					if (!vendorData->isVendorOwner(creature)) {
						creature->sendSystemMessage("You are not the owner of the vendor and cannot move the vendor.");
						return GENERALERROR;
					}

				} else if (!onAdmin) {
					creature->sendSystemMessage("@player_structure:admin_move_only"); // You must be an admin to manipulate objects.
					return GENERALERROR;
				}

				if (objectRootParent != buildingObject || buildingObject->containsChildObject(object)) {
					creature->sendSystemMessage("Error rotating object " + object->getCustomObjectName().toString());
					creature->sendSystemMessage("@player_structure:rotate_what"); // What do you want to rotate?
					return GENERALERROR;
				}

				if (buildingObject->isGCWBase()) {
					creature->sendSystemMessage("@player_structure:no_move_hq"); // You may not move or rotate objects inside a factional headquarters.
					return GENERALERROR;
				}
			}
		}

		String dir;
		int degrees = 0;
		bool rotateYaw = false;
		bool rotatePitch = false;
		bool rotateRoll = false;
		bool resetRotate = false;

		try {
			UnicodeTokenizer tokenizer(arguments.toString());
			tokenizer.getStringToken(dir);

			dir = dir.toLowerCase();

			if (dir == "yaw" || dir == "right" || dir == "left")
				rotateYaw = true;
			else if (dir == "pitch")
				rotatePitch = true;
			else if (dir == "roll")
				rotateRoll = true;
			else if (dir == "reset")
				resetRotate = true;
			else {
				StringBuffer message;
				message << "Invalid direction parameter " << '"' << dir << '"' << " received.  Targeted object is " << '"' << object->getCustomObjectName().toString() << '"' << ", template is " << object->getObjectTemplate()->getTemplateFileName() << ".";
				creature->sendSystemMessage(message.toString());
				throw Exception();
			}

			degrees = tokenizer.getIntToken();

		} catch (Exception& e) {
			creature->sendSystemMessage("Format: /rotateFurniture <right|left|yaw/pitch/roll> [degrees]. Degrees can be -180 to 180 when using this format.");
			creature->sendSystemMessage("Reset position: /rotateFurniture reset 1");
			return INVALIDPARAMETERS;
		}

		if ((rotatePitch || rotateRoll || rotateYaw) && (degrees < -180 || degrees > 180)) {
			creature->sendSystemMessage("The amount to rotate must be between -180 and 180.");
			return INVALIDPARAMETERS;
		}

		if (rotateYaw) {
			if (dir == "right")
				object->rotate(-degrees);
			else if (dir == "left")
				object->rotate(degrees);
			else
				object->rotate(degrees);
		} else if (rotatePitch) {
			object->rotatePitch(degrees);
		} else if (rotateRoll) {
			object->rotateRoll(degrees);
		} else if (resetRotate) {
			object->setDirection(1, 0, 0, 0);
		}

		// object->incrementMovementCounter();

		ManagedReference<SceneObject*> objectParent = object->getParent().get();
		if (objectParent != nullptr)
			object->teleport(object->getPositionX(), object->getPositionZ(), object->getPositionY(), objectParent->getObjectID());
		else
			object->teleport(object->getPositionX(), object->getPositionZ(), object->getPositionY());

		return SUCCESS;
	}
};

#endif // ROTATEFURNITURECOMMAND_H_
