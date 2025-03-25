/*
 * FireHeavyWeaponMenuComponent.cpp
 *
 *  Created on: May 10, 2012
 *      Author: chris.rush
 *	Edited on: July 12, 2022
 *		WickedHangover
 */

#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/packets/object/ObjectMenuResponse.h"
#include "FireHeavyWeaponMenuComponent.h"
#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/managers/director/DirectorManager.h"

void FireHeavyWeaponMenuComponent::fillObjectMenuResponse(SceneObject* sceneObject, ObjectMenuResponse* menuResponse, CreatureObject* player) const {
	if (sceneObject == NULL || !sceneObject->isWeaponObject() || player == NULL)
		return;

	WeaponObjectMenuComponent::fillObjectMenuResponse(sceneObject, menuResponse, player);

	if (sceneObject->getObjectTemplate()->getTemplateFileName() == "heavy_rocket_launcher") {
		if (player->hasSkill("combat_demolition_pyro_3")) {
			menuResponse->addRadialMenuItem(232, 3, "Paint Target"); // Place Decoration
		}
	}
	if (sceneObject->getObjectTemplate()->getTemplateFileName() == "heavy_particle_beam") {
		if (player->hasSkill("combat_demolition_master")) {
			menuResponse->addRadialMenuItem(232, 3, "Disintegrate Target"); // Place Decoration
		}
	}
	
}

int FireHeavyWeaponMenuComponent::handleObjectMenuSelect(SceneObject* sceneObject, CreatureObject* player, byte selectedID) const {
	if (!sceneObject->isWeaponObject() || !player->isPlayerCreature() || !sceneObject->isASubChildOf(player))
		return 0;

	if (selectedID == 20) {
		player->sendCommand(STRING_HASHCODE("fireheavyweapon"), String::valueOf(sceneObject->getObjectID()), player->getTargetID());
		
		
	}
	else if (selectedID == 232) {
		if (player->hasSkill("combat_demolition_pyro_3")) {
			Lua* lua = DirectorManager::instance()->getLuaInstance();
			Reference<LuaFunction*> luaObtainData = lua->createFunction("demolitions", "rocketLauncherFire", 0);
			*luaObtainData << player;
			*luaObtainData << sceneObject;

			luaObtainData->callFunction();
		}
		return 1;
	}

	return WeaponObjectMenuComponent::handleObjectMenuSelect(sceneObject, player, selectedID);
}