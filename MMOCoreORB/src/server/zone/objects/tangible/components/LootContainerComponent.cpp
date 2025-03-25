/*
 * LootContainerComponent.cpp
 *
 *  Created on: Jan 24, 2012
 *      Author: xyborn
 */

#include "LootContainerComponent.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/group/GroupObject.h"

int LootContainerComponent::notifyObjectRemoved(SceneObject* sceneObject, SceneObject* object, SceneObject* destination) const {
	if (destination != nullptr) {
		ManagedReference<SceneObject*> rootParent = destination->getParent().get();

		if (rootParent != nullptr && rootParent->isCreatureObject()) {
			CreatureObject* creature = cast<CreatureObject*>(rootParent.get());

			if (creature != nullptr) {

				//Send system message to the looter.
				StringIdChatParameter params("base_player", "prose_item_looted_self_detail"); //You looted: %TO from %TT.
				params.setTO(object->getDisplayedName());
				params.setTT(sceneObject->getParent().get()->getDisplayedName());
				creature->sendSystemMessage(params);

				//Send system message to everyone in group except the looter.	
				auto group = creature->getGroup();
				if (group != nullptr) {
					StringIdChatParameter params("group", "notify_single_loot"); //[GROUP] %TU looted %TO from %TT.
					params.setTO(object->getDisplayedName());
					params.setTU(creature->getDisplayedName());
					params.setTT(sceneObject->getParent().get()->getDisplayedName());
					group->sendSystemMessage(params, creature, 2);   //Infinity:  messageType 1 = Harvest, 2 = Loot
				}
			}
		}
	}

	return 0;
}


bool LootContainerComponent::checkContainerPermission(SceneObject* sceneObject, CreatureObject* creature, uint16 permission) const {
	auto permissions = sceneObject->getContainerPermissions();
	if(permission == ContainerPermissions::MOVEIN)
		return false;
	else if (permission == ContainerPermissions::MOVEOUT ){
		return (permissions->getOwnerID() == creature->getObjectID() || permissions->getOwnerID() == creature->getGroupID());
	}

	return false;
}

int LootContainerComponent::canAddObject(SceneObject* sceneObject, SceneObject* object, int containmentType, String& errorDescription) const {

	if(sceneObject->getContainerObjectsSize() >= 0){
		errorDescription = "@error_message:remove_only_corpse"; //You cannot place items into a corpse.
		 return TransferErrorCode::INVALIDTYPE;
	}

	return 1;
}

