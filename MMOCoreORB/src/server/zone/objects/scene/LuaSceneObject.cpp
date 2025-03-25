/*
 * LuaSceneObject.cpp
 *
 *  Created on: 27/05/2011
 *      Author: victor
 */

#include "server/zone/objects/scene/LuaSceneObject.h"
#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/managers/stringid/StringIdManager.h"
#include "server/zone/managers/director/DirectorManager.h"
#include "server/zone/Zone.h"
#include "server/zone/managers/director/ScreenPlayTask.h"
#include "engine/lua/LuaPanicException.h"
#include "server/zone/objects/tangible/Container.h"
// Infinity:  Custom
#include "server/zone/objects/region/CityRegion.h"
#include "server/zone/objects/creature/VehicleObject.h"
#include "server/zone/objects/factorycrate/FactoryCrate.h"
#include "server/zone/objects/intangible/PetControlDevice.h"
#include "server/zone/objects/creature/ai/AiAgent.h"
#include "server/zone/objects/player/PlayerObject.h"
#include "server/zone/managers/planet/PlanetManager.h"
#include "server/zone/managers/collision/CollisionManager.h"

const char LuaSceneObject::className[] = "LuaSceneObject";

Luna<LuaSceneObject>::RegType LuaSceneObject::Register[] = {
		{ "_setObject", &LuaSceneObject::_setObject },
		{ "_getObject", &LuaSceneObject::_getObject },
		{ "getParent", &LuaSceneObject::getParent },
		{ "getObjectID", &LuaSceneObject::getObjectID },
		{ "getPositionX", &LuaSceneObject::getPositionX },
		{ "getPositionY", &LuaSceneObject::getPositionY },
		{ "getPositionZ", &LuaSceneObject::getPositionZ },
		{ "getDirectionAngle", &LuaSceneObject::getDirectionAngle },
		{ "getWorldPositionX", &LuaSceneObject::getWorldPositionX },
		{ "getWorldPositionY", &LuaSceneObject::getWorldPositionY },
		{ "getWorldPositionZ", &LuaSceneObject::getWorldPositionZ },
		{ "getParentID", &LuaSceneObject::getParentID },
		{ "isInRange", &LuaSceneObject::isInRange },
		{ "isInRangeWithObject", &LuaSceneObject::isInRangeWithObject },
		{ "isInRangeWithObject3d", &LuaSceneObject::isInRangeWithObject3d },
		{ "setCustomObjectName", &LuaSceneObject::setCustomObjectName},
		{ "getDistanceTo", &LuaSceneObject::getDistanceTo },
		{ "getDistanceToPosition", &LuaSceneObject::getDistanceToPosition },
		{ "updateDirection", &LuaSceneObject::updateDirection },
		{ "getServerObjectCRC", &LuaSceneObject::getServerObjectCRC },
		{ "showFlyText", &LuaSceneObject::showFlyText },
		{ "getContainerObject", &LuaSceneObject::getContainerObject },
		{ "getContainerObjectById", &LuaSceneObject::getContainerObjectById },
		{ "getContainerObjectsSize", &LuaSceneObject::getContainerObjectsSize },
		{ "getCountableObjectsRecursive", &LuaSceneObject::getCountableObjectsRecursive },
		{ "getContainerVolumeLimit", &LuaSceneObject::getContainerVolumeLimit },
		{ "isContainerFull", &LuaSceneObject::isContainerFull },
		{ "isContainerFullRecursive", &LuaSceneObject::isContainerFullRecursive },
		{ "getSlottedObject", &LuaSceneObject::getSlottedObject },
		{ "transferObject", &LuaSceneObject::transferObject },
//		{ "removeObject", &LuaSceneObject::removeObject },
		{ "getGameObjectType", &LuaSceneObject::getGameObjectType },
		{ "faceObject", &LuaSceneObject::faceObject },
		{ "isFacingObject", &LuaSceneObject::isFacingObject },
		{ "destroyObjectFromWorld", &LuaSceneObject::destroyObjectFromWorld },
		{ "destroyObjectFromDatabase", &LuaSceneObject::destroyObjectFromDatabase },
		{ "isCreatureObject", &LuaSceneObject::isCreatureObject },
		{ "isAiAgent", &LuaSceneObject::isAiAgent },
		{ "isPlayerCreature", &LuaSceneObject::isPlayerCreature },
		{ "isCreature", &LuaSceneObject::isCreature },
		{ "isBuildingObject", &LuaSceneObject::isBuildingObject },
		{ "isActiveArea", &LuaSceneObject::isActiveArea },
		{ "sendTo", &LuaSceneObject::sendTo },
		{ "getCustomObjectName", &LuaSceneObject::getCustomObjectName },
		{ "getDisplayedName", &LuaSceneObject::getDisplayedName },
		{ "getObjectName", &LuaSceneObject::getObjectName },
		{ "setDirectionalHeading", &LuaSceneObject::setDirectionalHeading },
		{ "getZoneName", &LuaSceneObject::getZoneName },
		{ "getTemplateObjectPath", &LuaSceneObject::getTemplateObjectPath },
		{ "teleport", &LuaSceneObject::teleport },
		{ "setObjectMenuComponent", &LuaSceneObject::setObjectMenuComponent },
		{ "setContainerComponent", &LuaSceneObject::setContainerComponent },
		{ "switchZone", &LuaSceneObject::switchZone },
		{ "setContainerInheritPermissionsFromParent", &LuaSceneObject::setContainerInheritPermissionsFromParent },
		{ "setContainerAllowPermission", &LuaSceneObject::setContainerAllowPermission },
		{ "setContainerDenyPermission", &LuaSceneObject::setContainerDenyPermission },
		{ "setContainerDefaultAllowPermission", &LuaSceneObject::setContainerDefaultAllowPermission },
		{ "clearContainerDefaultAllowPermission", &LuaSceneObject::clearContainerDefaultAllowPermission },
		{ "setContainerDefaultDenyPermission", &LuaSceneObject::setContainerDefaultDenyPermission },
		{ "clearContainerDefaultDenyPermission", &LuaSceneObject::clearContainerDefaultDenyPermission },
		{ "setContainerOwnerID", &LuaSceneObject::setContainerOwnerID },
		{ "setContainerLockedStatus", &LuaSceneObject::setContainerLockedStatus },
		{ "setObjectName", &LuaSceneObject::setObjectName },
		{ "setRadius", &LuaSceneObject::setRadius },
		{ "isASubChildOf", &LuaSceneObject::isASubChildOf },
		{ "isOwned", &LuaSceneObject::isOwned },
		{ "playEffect", &LuaSceneObject::playEffect },
		{ "addPendingTask", &LuaSceneObject::addPendingTask },
		{ "hasPendingTask", &LuaSceneObject::hasPendingTask },
		{ "cancelPendingTask", &LuaSceneObject::cancelPendingTask },
		{ "getChildObject", &LuaSceneObject::getChildObject },
		{ "getContainerOwnerID", &LuaSceneObject::getContainerOwnerID },
		{ "info", &LuaSceneObject::info },
		{ "getPlayersInRange", &LuaSceneObject::getPlayersInRange },
		{ "isInNavMesh", &LuaSceneObject::isInNavMesh },
		// Infinity:  Custom hooks below
		{ "getCreaturesInRange", &LuaSceneObject::getCreaturesInRange },
		{ "getCityRegion", &LuaSceneObject::getCityRegion },
		{ "isPet", &LuaSceneObject::isPet },
		{ "isVehicleObject", &LuaSceneObject::isVehicleObject },
		{ "repairVehicle", &LuaSceneObject::repairVehicle },
		{ "getInRangeObjects", &LuaSceneObject::getInRangeObjects },
		{ "rotateReset", &LuaSceneObject::rotateReset },
		{ "rotate", &LuaSceneObject::rotate },
		{ "rotateRoll", &LuaSceneObject::rotateRoll },
		{ "rotatePitch", &LuaSceneObject::rotatePitch },
		{ "getDirectionX", &LuaSceneObject::getDirectionX },
		{ "getDirectionY", &LuaSceneObject::getDirectionY },
		{ "getDirectionZ", &LuaSceneObject::getDirectionZ },
		{ "getDirectionW", &LuaSceneObject::getDirectionW },
		{ "setDirection", &LuaSceneObject::setDirection },
		{ "extractObjectToInventory", &LuaSceneObject::extractObjectToInventory },
		{ "isFactoryCrate", &LuaSceneObject::isFactoryCrate },
		{ "isArmorObject", &LuaSceneObject::isArmorObject },
		{ "isWeaponObject", &LuaSceneObject::isWeaponObject },
		{ "getPrototypeTemplatePath", &LuaSceneObject::getPrototypeTemplatePath },
		{ "isVendorObject", &LuaSceneObject::isVendorObject },
        { "getNPCsInRange", &LuaSceneObject::getNPCsInRange },
        { "getTemplateObjectFilename", &LuaSceneObject::getTemplateObjectFilename },
		{ "isPetControlDevice", &LuaSceneObject::isPetControlDevice },
		{ "getTerrainHeight", &LuaSceneObject::getTerrainHeight },
		{ "getDistanceTo3d", &LuaSceneObject::getDistanceTo3d },
		{ "getDistanceToPosition3d", &LuaSceneObject::getDistanceToPosition3d },
		{ "getRootParent", &LuaSceneObject::getRootParent },
		{ "getRootParentID", &LuaSceneObject::getRootParentID },
		{ "isCellObject", &LuaSceneObject::isCellObject },
		{ "isTangibleObject", &LuaSceneObject::isTangibleObject },
		{ "isIntangibleObject", &LuaSceneObject::isIntangibleObject },
		{ "checkLineOfSight", &LuaSceneObject::checkLineOfSight },
		{ 0, 0 }

};

LuaSceneObject::LuaSceneObject(lua_State *L) {
	realObject = reinterpret_cast<SceneObject*>(lua_touserdata(L, 1));
}

LuaSceneObject::~LuaSceneObject(){
}

int LuaSceneObject::_getObject(lua_State* L) {
	if (realObject == nullptr)
		lua_pushnil(L);
	else
		lua_pushlightuserdata(L, realObject.get());

	return 1;
}

int LuaSceneObject::_setObject(lua_State* L) {
	auto obj = reinterpret_cast<SceneObject*>(lua_touserdata(L, -1));

	if (obj == nullptr)
		throw LuaPanicException("nullptr in LuaSceneObject::_setObject");

	if (obj != realObject)
		realObject = obj;

	return 0;
}

int LuaSceneObject::setObjectMenuComponent(lua_State* L) {
	String value = lua_tostring(L, -1);

	realObject->setObjectMenuComponent(value);

	return 0;
}

int LuaSceneObject::setContainerComponent(lua_State* L) {
	String value = lua_tostring(L, -1);

	realObject->setContainerComponent(value);

	return 0;
}

int LuaSceneObject::setCustomObjectName(lua_State* L) {
	int numberOfArguments = lua_gettop(L) - 1;

	String value;

	if (numberOfArguments == 2) {
		String file = lua_tostring(L, -2);
		String key = lua_tostring(L, -1);
		String fullPath = "@" + file + ":" + key;
		value = StringIdManager::instance()->getStringId(fullPath.hashCode()).toString();
	} else {
		value = lua_tostring(L, -1);
	}

	realObject->setCustomObjectName(value, true);

	return 0;
}

int LuaSceneObject::switchZone(lua_State* L) {
	uint64 parentid = lua_tointeger(L, -1);
	float y = lua_tonumber(L, -2);
	float z = lua_tonumber(L, -3);
	float x = lua_tonumber(L, -4);
	String planet = lua_tostring(L, -5);

	Locker locker(realObject);

	realObject->switchZone(planet, x, z, y, parentid);

	return 0;
}

int LuaSceneObject::getTemplateObjectPath(lua_State* L) {
	if (realObject != nullptr) {
		String tempPath = realObject->getObjectTemplate()->getFullTemplateString();

		lua_pushstring(L, tempPath.toCharArray());
	} else {
		lua_pushstring(L, "");
	}

	return 1;
}

int LuaSceneObject::teleport(lua_State* L) {
	uint64 parentID = lua_tointeger(L, -1);
	float y = lua_tonumber(L, -2);
	float z = lua_tonumber(L, -3);
	float x = lua_tonumber(L, -4);

	realObject->teleport(x, z, y, parentID);

	return 0;
}

int LuaSceneObject::getZoneName(lua_State* L) {
	Zone* zone = realObject->getZone();

	String name = "";

	if (zone != nullptr) {
		name = zone->getZoneName();
	}

	lua_pushstring(L, name.toCharArray());

	return 1;
}

int LuaSceneObject::getPositionX(lua_State* L) {
	lua_pushnumber(L, realObject->getPositionX());

	return 1;
}

int LuaSceneObject::getPositionZ(lua_State* L) {
	lua_pushnumber(L, realObject->getPositionZ());

	return 1;
}

int LuaSceneObject::getDirectionAngle(lua_State* L) {
	lua_pushnumber(L, realObject->getDirectionAngle());

	return 1;
}

int LuaSceneObject::getWorldPositionY(lua_State* L) {
	lua_pushnumber(L, realObject->getWorldPositionY());

	return 1;
}

int LuaSceneObject::getWorldPositionX(lua_State* L) {
	lua_pushnumber(L, realObject->getWorldPositionX());

	return 1;
}

int LuaSceneObject::getWorldPositionZ(lua_State* L) {
	lua_pushnumber(L, realObject->getWorldPositionZ());

	return 1;
}

int LuaSceneObject::getPositionY(lua_State* L) {
	lua_pushnumber(L, realObject->getPositionY());

	return 1;
}

int LuaSceneObject::getObjectID(lua_State* L) {
	lua_pushinteger(L, realObject->getObjectID());

	return 1;
}

int LuaSceneObject::getParentID(lua_State* L) {
	lua_pushinteger(L, realObject->getParentID());

	return 1;
}

int LuaSceneObject::isInRange(lua_State* L) {
//public boolean isInRange(float x, float y, float range) {
	float range = lua_tonumber(L, -1);
	float y = lua_tonumber(L, -2);
	float x = lua_tonumber(L, -3);

	bool res = (static_cast<QuadTreeEntry*>(realObject))->isInRange(x, y, range);

	lua_pushnumber(L, res);

	return 1;
}

int LuaSceneObject::getGameObjectType(lua_State* L) {
	uint32 type = realObject->getGameObjectType();

	lua_pushnumber(L, type);

	return 1;
}

int LuaSceneObject::getDistanceTo(lua_State* L) {
	SceneObject* obj = (SceneObject*)lua_touserdata(L, -1);

	float res = realObject->getDistanceTo(obj);

	lua_pushnumber(L, res);

	return 1;
}

int LuaSceneObject::getDistanceToPosition(lua_State* L) {
	float y = lua_tonumber(L, -1);
	float z = lua_tonumber(L, -2);
	float x = lua_tonumber(L, -3);

	Coordinate position;
	position.setPosition(x, z, y);

	float dist = realObject->getDistanceTo(&position);

	lua_pushnumber(L, dist);

	return 1;
}

int LuaSceneObject::getServerObjectCRC(lua_State* L) {
	uint32 crc = realObject->getServerObjectCRC();

	lua_pushnumber(L, crc);

	return 1;
}

int LuaSceneObject::faceObject(lua_State* L) {
	bool notifyClient = lua_toboolean(L, -1);
	SceneObject* obj = (SceneObject*)lua_touserdata(L, -2);

	realObject->faceObject(obj, notifyClient);

	return 0;
}

int LuaSceneObject::isFacingObject(lua_State* L) {
	SceneObject* obj = (SceneObject*)lua_touserdata(L, -1);

	if (obj == nullptr) {
		lua_pushboolean(L, false);

		return 1;
	}

	bool res = realObject->isFacingObject(obj);

	lua_pushboolean(L, res);

	return 1;
}

int LuaSceneObject::isInRangeWithObject(lua_State* L) {
	float range = lua_tonumber(L, -1);
	SceneObject* obj = (SceneObject*)lua_touserdata(L, -2);

	bool res = obj != nullptr && realObject->isInRange(obj, range);

	lua_pushboolean(L, res);

	return 1;
}

int LuaSceneObject::isInRangeWithObject3d(lua_State* L) {
	float range = lua_tonumber(L, -1);
	SceneObject* obj = (SceneObject*)lua_touserdata(L, -2);

	bool res = obj != nullptr && realObject->isInRange3d(obj, range);

	lua_pushboolean(L, res);

	return 1;
}

int LuaSceneObject::getParent(lua_State* L) {
	SceneObject* obj = realObject->getParent().get().get();

	if (obj == nullptr) {
		lua_pushnil(L);
	} else {
		obj->_setUpdated(true);
		lua_pushlightuserdata(L, obj);
	}

	return 1;
}

int LuaSceneObject::getContainerObject(lua_State* L) {
	int idx = lua_tonumber(L, -1);

	SceneObject* obj = realObject->getContainerObject(idx);

	if (obj == nullptr) {
		lua_pushnil(L);
	} else {
		obj->_setUpdated(true);
		lua_pushlightuserdata(L, obj);
	}

	return 1;
}

int LuaSceneObject::getContainerObjectById(lua_State* L) {
	uint64 objectID = lua_tointeger(L, -1);

	SceneObject* obj = realObject->getContainerObject(objectID);

	if (obj != nullptr) {
		obj->_setUpdated(true);
		lua_pushlightuserdata(L, obj);
	} else {
		lua_pushnil(L);
	}

    return 1;
}

int LuaSceneObject::getContainerObjectsSize(lua_State* L) {
	int num = realObject->getContainerObjectsSize();

	lua_pushnumber(L, num);

	return 1;
}

int LuaSceneObject::getCountableObjectsRecursive(lua_State* L) {
	int num = realObject->getCountableObjectsRecursive();

	lua_pushnumber(L, num);

	return 1;
}

int LuaSceneObject::getContainerVolumeLimit(lua_State* L) {
	int num = realObject->getContainerVolumeLimit();

	lua_pushnumber(L, num);

	return 1;
}

int LuaSceneObject::isContainerFull(lua_State* L) {
	bool full = realObject->isContainerFull();

	lua_pushboolean(L, full);

	return 1;
}

int LuaSceneObject::isContainerFullRecursive(lua_State* L) {
	bool full = realObject->isContainerFullRecursive();

	lua_pushboolean(L, full);

	return 1;
}

int LuaSceneObject::getSlottedObject(lua_State* L) {
	String slot = lua_tostring(L, -1);

	SceneObject* obj = realObject->getSlottedObject(slot);
	if (obj == nullptr) {
		lua_pushnil(L);
	} else {
		obj->_setUpdated(true);
		lua_pushlightuserdata(L, obj);
	}

	return 1;
}

int LuaSceneObject::transferObject(lua_State* L) {
	//transferObject(SceneObject object, int containmentType, boolean notifyClient = false);

	bool notifyClient = lua_tonumber(L, -1);
	int containmentType = lua_tonumber(L, -2);
	SceneObject* obj = (SceneObject*) lua_touserdata(L, -3);

	realObject->transferObject(obj, containmentType, notifyClient);

	return 0;
}

/*int LuaSceneObject::removeObject(lua_State* L) {

	//removeObject(SceneObject object, boolean notifyClient = false);

	bool notifyClient = lua_tonumber(L, -1);
	SceneObject* obj = (SceneObject*) lua_touserdata(L, -2);

	realObject->removeObject(obj, notifyClient);

	return 0;
}*/

int LuaSceneObject::showFlyText(lua_State* L) {
	//final string file, final string uax, byte red, byte green, byte blue

	byte blue = lua_tonumber(L, -1);
	byte green = lua_tonumber(L, -2);
	byte red = lua_tonumber(L, -3);

	String aux = lua_tostring(L, -4);
	String file = lua_tostring(L, -5);

	realObject->showFlyText(file, aux, red, green, blue);

	return 0;
}

int LuaSceneObject::playEffect(lua_State* L) {
	//public native void playEffect(final string file, final string aux);

	String aux = lua_tostring(L, -1);
	String file = lua_tostring(L, -2);

	realObject->playEffect(file, aux);

	return 0;
}


int LuaSceneObject::updateDirection(lua_State* L) {
	int numberOfArguments = lua_gettop(L) - 1;

	if (numberOfArguments == 1) {
		float angle = lua_tonumber(L, -1);
		realObject->updateDirection(angle);
	} else {
		float fz = lua_tonumber(L, -1);
		float fy = lua_tonumber(L, -2);
		float fx = lua_tonumber(L, -3);
		float fw = lua_tonumber(L, -4);

		realObject->updateDirection(fw, fx, fy, fz);
	}

	return 0;
}

int LuaSceneObject::destroyObjectFromWorld(lua_State* L) {
	Locker locker(realObject);

	realObject->destroyObjectFromWorld(true);

	return 0;
}

int LuaSceneObject::destroyObjectFromDatabase(lua_State* L) {
	realObject->destroyObjectFromDatabase(true);

	return 0;
}

int LuaSceneObject::isCreatureObject(lua_State* L) {
	bool val = realObject->isCreatureObject();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::isAiAgent(lua_State* L) {
	bool val = realObject->isAiAgent();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::isPlayerCreature(lua_State* L) {
	bool val = realObject->isPlayerCreature();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::isCreature(lua_State* L) {
	bool val = realObject->isCreature();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::isBuildingObject(lua_State* L) {

	bool val = false;

	if (realObject != nullptr) {
		val = realObject->isBuildingObject();
	}
	
	lua_pushboolean(L, val);

	return 1;
}


int LuaSceneObject::isActiveArea(lua_State* L) {
	bool val = realObject->isActiveArea();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::wlock(lua_State* L) {
	return 0;
}

int LuaSceneObject::unlock(lua_State* L) {
	return 0;
}

int LuaSceneObject::sendTo(lua_State* L) {
	SceneObject* obj = (SceneObject*) lua_touserdata(L, -1);

	realObject->sendTo(obj, true);

	return 0;
}

int LuaSceneObject::getCustomObjectName(lua_State* L) {
	String objname = realObject->getCustomObjectName().toString();

	lua_pushstring(L, objname.toCharArray());

	return 1;
}

int LuaSceneObject::getDisplayedName(lua_State* L) {
	String objname = realObject->getDisplayedName();

	lua_pushstring(L, objname.toCharArray());

	return 1;
}

int LuaSceneObject::getObjectName(lua_State* L) {
	String objname = realObject->getObjectName()->getStringID();

	lua_pushstring(L, objname.toCharArray());

	return 1;
}

int LuaSceneObject::setDirectionalHeading(lua_State* L) {
	float heading = lua_tonumber(L, -1);

	//SceneObject* obj = (SceneObject*) lua_touserdata(L, -2);

	realObject->setDirection(heading);

	return 0;
}

int LuaSceneObject::setContainerInheritPermissionsFromParent(lua_State* L) {
	bool val = lua_toboolean(L, -1);

	Locker locker(realObject);

	realObject->setContainerInheritPermissionsFromParent(val);

	return 0;
}

int LuaSceneObject::setContainerAllowPermission(lua_State* L) {
	String group = lua_tostring(L, -2);
	uint16 perm = lua_tointeger(L, -1);

	Locker locker(realObject);

	realObject->setContainerAllowPermission(group, perm);

	return 0;
}

int LuaSceneObject::setContainerDenyPermission(lua_State* L) {
	String group = lua_tostring(L, -2);
	uint16 perm = lua_tointeger(L, -1);

	Locker locker(realObject);

	realObject->setContainerDenyPermission(group, perm);

	return 0;
}

int LuaSceneObject::setContainerDefaultAllowPermission(lua_State* L) {
	uint16 perm = lua_tointeger(L, -1);

	Locker locker(realObject);

	realObject->setContainerDefaultAllowPermission(perm);

	return 0;
}

int LuaSceneObject::clearContainerDefaultAllowPermission(lua_State* L) {
	uint16 perm = lua_tointeger(L, -1);

	Locker locker(realObject);

	realObject->clearContainerDefaultAllowPermission(perm);

	return 0;
}

int LuaSceneObject::setContainerDefaultDenyPermission(lua_State* L) {
	uint16 perm = lua_tointeger(L, -1);

	Locker locker(realObject);

	realObject->setContainerDefaultDenyPermission(perm);

	return 0;
}

int LuaSceneObject::clearContainerDefaultDenyPermission(lua_State* L) {
	uint16 perm = lua_tointeger(L, -1);

	Locker locker(realObject);

	realObject->clearContainerDefaultDenyPermission(perm);

	return 0;
}

int LuaSceneObject::setContainerOwnerID(lua_State* L) {
	uint64 ownerID = lua_tointeger(L, -1);

	Locker locker(realObject);

	realObject->setContainerOwnerID(ownerID);

	return 0;
}

int LuaSceneObject::setContainerLockedStatus(lua_State* L) {
	int numberOfArguments = lua_gettop(L) - 1;

	if (numberOfArguments != 1) {
		realObject->error() << "Improper number of arguments in setContainerLockedStatus in LuaSceneObject.";
		return 0;
	}

	bool status = lua_toboolean(L, -1);

	if (!realObject->isContainerObject())
		return 0;

	ManagedReference<Container*> container = realObject.castTo<Container*>();

	if (container == nullptr)
		return 0;

	Locker locker(container);

	container->setLockedStatus(status);

	return 0;
}

int LuaSceneObject::setObjectName(lua_State* L) {
	String file = lua_tostring(L, -3);
	String key = lua_tostring(L, -2);
	bool notifyClient = lua_toboolean(L, -1);

	StringId stringid(file, key);

	realObject->setObjectName(stringid, notifyClient);

	return 0;
}

int LuaSceneObject::setRadius(lua_State* L) {
	float radius = lua_tonumber(L, -1);

	if (radius < 0.5f || radius > ZoneServer::CLOSEOBJECTRANGE * 4.0f) {
		realObject->error() << __FUNCTION__ << "() invalid radius of " << radius;
		lua_pushnil(L);
		return 0;
	}

	realObject->setRadius(radius);

	lua_pushboolean(L, true);

	return 1;
}

int LuaSceneObject::isASubChildOf(lua_State* L) {
	SceneObject* obj = (SceneObject*) lua_touserdata(L, -1);

	lua_pushboolean(L, realObject->isASubChildOf(obj));

	return 1;
}

int LuaSceneObject::isOwned(lua_State* L) {
	lua_pushboolean(L, realObject->isPet() || realObject->isVehicleObject());

	return 1;
}

int LuaSceneObject::addPendingTask(lua_State* L) {
	uint32 mili = lua_tonumber(L, -3);
	String play = lua_tostring(L, -2);
	String key = lua_tostring(L, -1);

	Reference<ScreenPlayTask*> task = new ScreenPlayTask(realObject, key, play, "");

	String name = play + ":" + key;

	realObject->addPendingTask(name, task, mili);

	return 0;
}

int LuaSceneObject::hasPendingTask(lua_State* L) {
	String play = lua_tostring(L, -2);
	String key = lua_tostring(L, -1);

	String name = play + ":" + key;

	lua_pushboolean(L, realObject->containsPendingTask(name));

	return 1;
}


int LuaSceneObject::cancelPendingTask(lua_State* L) {
	String play = lua_tostring(L, -2);
	String key = lua_tostring(L, -1);

	String name = play + ":" + key;

	if (realObject->containsPendingTask(name)) {
		Reference<ScreenPlayTask*> task = realObject->getPendingTask(name).castTo<ScreenPlayTask*>();

		if (task != nullptr && task->isScheduled()) {
			task->cancel();
		}

		realObject->removePendingTask(name);
	}

	return 0;
}

int LuaSceneObject::getChildObject(lua_State* L) {
	int index = lua_tonumber(L, -1);

	SceneObject* obj = realObject->getChildObjects()->get(index);

	if (obj == nullptr) {
		lua_pushnil(L);
	} else {
		obj->_setUpdated(true);
		lua_pushlightuserdata(L, obj);
	}

	return 1;
}

int LuaSceneObject::getContainerOwnerID(lua_State *L) {

	lua_pushnumber(L, realObject->getContainerPermissions()->getOwnerID());

	return 1;
}

int LuaSceneObject::info(lua_State* L) {
	String msg = lua_tostring(L, -1);

	realObject->info(msg, true);

	return 0;
}

int LuaSceneObject::getPlayersInRange(lua_State *L) {

	if (realObject == nullptr) {
		lua_pushnil(L);
		Logger::console.error("LuaSceneObject::getPlayersInRange - realObject is nullptr");
		return 1;
	}

	int range = lua_tonumber(L, -1);

	auto zone = realObject->getZone();

	if (zone == nullptr) {
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);


	SortedVector<ManagedReference<QuadTreeEntry*> > playerObjects;
	zone->getInRangePlayers(realObject->getWorldPositionX(), realObject->getWorldPositionY(), range, &playerObjects);
	
	int numPlayers = 0;

	for (int i = 0; i < playerObjects.size(); ++i) {
		
		SceneObject* object = cast<SceneObject*>(playerObjects.get(i).get());

		if (object == nullptr) {
			continue;
		}

		numPlayers++;
		lua_pushlightuserdata(L, object);
		lua_rawseti(L, -2, numPlayers);
	}

	return 1;
}

int LuaSceneObject::isInNavMesh(lua_State* L) {
	bool val = realObject->isInNavMesh();

	lua_pushboolean(L, val);

	return 1;
}

// Infinity:  Custom Hooks below
//******************************************************************************************
int LuaSceneObject::getCreaturesInRange(lua_State *L) {

	if (realObject == nullptr) {
		lua_pushnil(L);
		Logger::console.error("LuaSceneObject::getCreaturesInRange - realObject is nullptr");
		return 1;
	}

	int range = lua_tonumber(L, -1);

	auto zone = realObject->getZone();

	if (zone == nullptr) {
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);

	SortedVector<ManagedReference<QuadTreeEntry*> > closeObjects;
	zone->getInRangeObjects(realObject->getWorldPositionX(), realObject->getWorldPositionY(), range, &closeObjects, true);

	int numCreatures = 0;

	for (int i = 0; i < closeObjects.size(); ++i) {
		SceneObject* object = cast<SceneObject*>(closeObjects.get(i).get());

		if (object == nullptr || !object->isCreatureObject()) {
			continue;
		}

		CreatureObject* creo = object->asCreatureObject();

		if (creo == nullptr || creo->isInvisible())
			continue;

		numCreatures++;
		lua_pushlightuserdata(L, object);
		lua_rawseti(L, -2, numCreatures);
	}

	return 1;
}

int LuaSceneObject::getCityRegion(lua_State* L) {

	ManagedReference<CityRegion*> city = nullptr;
	
	if (realObject != nullptr) {
		city = realObject->getCityRegion().get();
	}

	if (city == nullptr) {
		lua_pushnil(L);
	} else {
		lua_pushlightuserdata(L, city);
	}

	return 1;
}

int LuaSceneObject::isPet(lua_State* L) {
	
	bool val = false;
	
	if (realObject != nullptr) {
		val = realObject->isPet();
	}

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::isVehicleObject(lua_State* L) {
	
	bool val = false;
	
	if (realObject != nullptr)
		val = realObject->isVehicleObject();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::repairVehicle(lua_State* L) {
	
	if (realObject != nullptr && realObject->isVehicleObject()) {

		VehicleObject* vehicle = realObject.castTo<VehicleObject*>();

		if (vehicle != nullptr) {

			int conditionDamage = vehicle->getConditionDamage();

			if (conditionDamage > 0) {

				ManagedReference<CreatureObject* > owner = vehicle->getLinkedCreature();

				Locker locker(vehicle);
				vehicle->healDamage(owner, 0, conditionDamage, true);

				if (vehicle->isDisabled())
					vehicle->setDisabled(false);

				locker.release();

				if (owner != nullptr)
					owner->sendSystemMessage("Your vehicle has been repaired.");
			}
		}
	}

	return 0;
}

int LuaSceneObject::getInRangeObjects(lua_State *L) {
	int range = lua_tonumber(L, -1);

	Zone* thisZone = realObject->getZone();

	if (thisZone == nullptr) {
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);

	SortedVector<ManagedReference<QuadTreeEntry*> > closeObjects;
	thisZone->getInRangeObjects(realObject->getWorldPositionX(), realObject->getWorldPositionY(), range, &closeObjects, true);
	int numObjects = 0;

	for (int i = 0; i < closeObjects.size(); ++i) {
		SceneObject* object = cast<SceneObject*>(closeObjects.get(i).get());

		if (object == nullptr)
			continue;

		numObjects++;
		lua_pushlightuserdata(L, object);
		lua_rawseti(L, -2, numObjects);
	}

	return 1;
}

int LuaSceneObject::rotateReset(lua_State* L) {

	if (realObject != nullptr) {
		realObject->setDirection(1, 0, 0, 0);
	}
	else {
		Logger::console.error("LuaSceneObject::rotateReset - realObject is nullptr");
	}

	return 0;
}

int LuaSceneObject::rotate(lua_State* L) {

	int degrees = lua_tointeger(L, -1);

	if (degrees > 360 || degrees < -360) {
		Logger::console.error("LuaSceneObject::rotate - value for degrees must be between -360 and 360");
		return 0;
	}

	if (realObject != nullptr) {
		realObject->rotate(degrees);
	}
	else {
		Logger::console.error("LuaSceneObject::rotate - realObject is nullptr");
	}

	return 0;
}

int LuaSceneObject::rotateRoll(lua_State* L) {

	int degrees = lua_tointeger(L, -1);

	if (degrees > 360 || degrees < -360) {
		Logger::console.error("LuaSceneObject::rotateRoll - value for degrees must be between -360 and 360");
		return 0;
	}

	if (realObject != nullptr) {
		realObject->rotateRoll(degrees);
	}
	else {
		Logger::console.error("LuaSceneObject::rotateRoll - realObject is nullptr");
	}

	return 0;
}

int LuaSceneObject::rotatePitch(lua_State* L) {

	int degrees = lua_tointeger(L, -1);

	if (degrees > 360 || degrees < -360) {
		Logger::console.error("LuaSceneObject::rotatePitch - value for degrees must be between -360 and 360");
		return 0;
	}

	if (realObject != nullptr) {
		realObject->rotatePitch(degrees);
	}
	else {
		Logger::console.error("LuaSceneObject::rotatePitch - realObject is nullptr");
	}

	return 0;
}

int LuaSceneObject::getDirectionX(lua_State* L) {

	if (realObject != nullptr) {
		lua_pushnumber(L, realObject->getDirectionX());
	}
	else {
		Logger::console.error("LuaSceneObject::getDirectionX - realObject is nullptr");
		lua_pushnil(L);
	}

	return 1;
}

int LuaSceneObject::getDirectionY(lua_State* L) {

	if (realObject != nullptr) {
		lua_pushnumber(L, realObject->getDirectionY());
	}
	else {
		Logger::console.error("LuaSceneObject::getDirectionY - realObject is nullptr");
		lua_pushnil(L);
	}

	return 1;
}

int LuaSceneObject::getDirectionZ(lua_State* L) {

	if (realObject != nullptr) {
		lua_pushnumber(L, realObject->getDirectionZ());
	}
	else {
		Logger::console.error("LuaSceneObject::getDirectionZ - realObject is nullptr");
		lua_pushnil(L);
	}

	return 1;
}

int LuaSceneObject::getDirectionW(lua_State* L) {

	if (realObject != nullptr) {
		lua_pushnumber(L, realObject->getDirectionW());
	}
	else {
		Logger::console.error("LuaSceneObject::getDirectionW - realObject is nullptr");
		lua_pushnil(L);
	}

	return 1;
}

int LuaSceneObject::setDirection(lua_State* L) {


	int numberOfArguments = lua_gettop(L) - 1;

	if (numberOfArguments != 4) {
		Logger::console.error("LuaSceneObject::setDirection - invalid number of arguments");
		return 0;
	}

	float ow = lua_tonumber(L, -4);
	float ox = lua_tonumber(L, -3);
	float oy = lua_tonumber(L, -2);
	float oz = lua_tonumber(L, -1);

	if (realObject != nullptr) {
		realObject->setDirection(ow, ox, oy, oz);
	}
	else {
		Logger::console.error("LuaSceneObject::setDirection - realObject is nullptr");
	}

	return 0;
}


int LuaSceneObject::extractObjectToInventory(lua_State* L) {

	if (realObject != nullptr) {

		if (!realObject->isFactoryCrate()) {
			Logger::console.error("LuaSceneObject::extractObjectToInventory - scene object is not factory crate");
			lua_pushnil(L);
			return 1;
		}

		int numberOfArguments = lua_gettop(L) - 1;

		if (numberOfArguments != 1) {
			Logger::console.error("LuaSceneObject::extractObjectToInventory - incorrect number of arguments");
			lua_pushnil(L);
			return 1;
		}

		CreatureObject* enhancer = (CreatureObject*) lua_touserdata(L, -1);

		if (enhancer == nullptr || !enhancer->isPlayerCreature()) {
			Logger::console.error("LuaSceneObject::extractObjectToInventory - invalid creature object");
			lua_pushnil(L);
			return 1;
		}

		FactoryCrate* crate = cast<FactoryCrate*>(realObject.get());

		if (crate == nullptr) {
			Logger::console.error("LuaSceneObject::extractObjectToInventory - factory crate is nullptr");
			lua_pushnil(L);
			return 1;
		}

		bool result = crate->extractObjectToInventory(enhancer);
		lua_pushboolean(L, result);
		return 1;
	}
	else {
		Logger::console.error("LuaSceneObject::extractObjectToInventory - realObject is nullptr");
		lua_pushnil(L);
		return 1;
	}
	return 1;
}

int LuaSceneObject::isFactoryCrate(lua_State* L) {

	if (realObject != nullptr) {
		bool res = realObject->isFactoryCrate();
		lua_pushboolean(L, res);
	}
	else {
		Logger::console.error("LuaSceneObject::isFactoryCrate - realObject is nullptr");
		lua_pushnil(L);
	}

	return 1;
}


int LuaSceneObject::isArmorObject(lua_State* L) {
	
	bool val = false;
	
	if (realObject != nullptr)
		val = realObject->isArmorObject();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::isWeaponObject(lua_State* L) {
	
	bool val = false;
	
	if (realObject != nullptr)
		val = realObject->isWeaponObject();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::getPrototypeTemplatePath(lua_State* L) {

	if (realObject != nullptr) {

		if (!realObject->isFactoryCrate()) {
			Logger::console.error("LuaSceneObject::getPrototypeTemplatePath - scene object is not factory crate");
			lua_pushnil(L);
			return 1;
		}

		FactoryCrate* crate = cast<FactoryCrate*>(realObject.get());

		if (crate == nullptr) {
			Logger::console.error("LuaSceneObject::getPrototypeTemplatePath - factory crate is nullptr");
			lua_pushnil(L);
			return 1;
		}

		TangibleObject* prototype = crate->getPrototype();

		if (prototype == nullptr) {
			Logger::console.error("LuaSceneObject::getPrototypeTemplatePath - prototype is nullptr");
			lua_pushnil(L);
			return 1;
		}

		String templateName = prototype->getObjectTemplate()->getFullTemplateString();
		lua_pushstring(L, templateName.toCharArray());
		return 1;
	}
	else {
		Logger::console.error("LuaSceneObject::getPrototypeTemplatePath - realObject is nullptr");
		lua_pushnil(L);
		return 1;
	}
	return 1;
}

int LuaSceneObject::isVendorObject(lua_State* L) {
	
	bool val = false;
	
	if (realObject != nullptr)
		val = realObject->isVendor();

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::getNPCsInRange(lua_State *L) {
	int range = lua_tonumber(L, -1);

	Zone* thisZone = realObject->getZone();

	if (thisZone == nullptr) {
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);

	SortedVector<ManagedReference<QuadTreeEntry*> > closeObjects;
	thisZone->getInRangeObjects(realObject->getWorldPositionX(), realObject->getWorldPositionY(), range, &closeObjects, true);
	int numNPCs = 0;

	for (int i = 0; i < closeObjects.size(); ++i) {
		SceneObject* object = cast<SceneObject*>(closeObjects.get(i).get());

		if (object == nullptr || !object->isCreatureObject() || object->isPlayerCreature())
			continue;

		CreatureObject* creature = object->asCreatureObject();

		if (creature == nullptr || creature->isInvisible() || !creature->isNonPlayerCreatureObject())
			continue;

		numNPCs++;
		lua_pushlightuserdata(L, object);
		lua_rawseti(L, -2, numNPCs);
	}

	return 1;
}

int LuaSceneObject::getTemplateObjectFilename(lua_State* L) {
	if (realObject != nullptr) {
		String filename = realObject->getObjectTemplate()->getTemplateFileName(); 

		lua_pushstring(L, filename.toCharArray());
	} else {
		lua_pushstring(L, "");
	}

	return 1;
}

int LuaSceneObject::isPetControlDevice(lua_State* L) {
	
	bool val = false;
	
	if (realObject != nullptr) {
		val = realObject->isPetControlDevice();
	}

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::getTerrainHeight(lua_State* L) {

	if (realObject != nullptr) {
		auto zone = realObject->getZone();
		if (zone != nullptr) {
			auto planetManager = zone->getPlanetManager();
			if (planetManager != nullptr) {
				auto terrainManager = planetManager->getTerrainManager();
				if (terrainManager != nullptr) {
					float x = realObject->getWorldPositionX();
					float y = realObject->getWorldPositionY();
					lua_pushnumber(L, terrainManager->getHeight(x, y));
				}
			}
		}
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int LuaSceneObject::getDistanceTo3d(lua_State* L) {
	SceneObject* obj = (SceneObject*)lua_touserdata(L, -1);

	float res = realObject->getDistanceTo3d(obj);

	lua_pushnumber(L, res);

	return 1;
}

int LuaSceneObject::getDistanceToPosition3d(lua_State* L) {
	float y = lua_tonumber(L, -1);
	float z = lua_tonumber(L, -2);
	float x = lua_tonumber(L, -3);

	Coordinate position;
	position.setPosition(x, z, y);

	float dist = realObject->getDistanceTo3d(&position);

	lua_pushnumber(L, dist);

	return 1;
}

int LuaSceneObject::getRootParent(lua_State* L) {

	if (realObject != nullptr) {

		ManagedReference<SceneObject*> rootParent = realObject->getRootParent();

		if (rootParent == nullptr) {
			lua_pushnil(L);
		}
		else {
			lua_pushlightuserdata(L, rootParent);
		}
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int LuaSceneObject::getRootParentID(lua_State* L) {

	uint64 rootParentID = 0;

	if (realObject != nullptr) {

		ManagedReference<SceneObject*> rootParent = realObject->getRootParent();

		if (rootParent != nullptr) {
			rootParentID = rootParent->getObjectID();
		}
	}

	lua_pushinteger(L, rootParentID);

	return 1;
}

int LuaSceneObject::isCellObject(lua_State* L) {

	bool val = false;

	if (realObject != nullptr) {
		val = realObject->isCellObject();
	}

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::isTangibleObject(lua_State* L) {
	bool val = false;
	
	if (realObject != nullptr) {
		val = realObject->isTangibleObject();
	}

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::isIntangibleObject(lua_State* L) {
	bool val = false;
	
	if (realObject != nullptr) {
		val = realObject->isIntangibleObject();
	}

	lua_pushboolean(L, val);

	return 1;
}

int LuaSceneObject::checkLineOfSight(lua_State* L) {

	int numberOfArguments = lua_gettop(L) - 1;

	if (numberOfArguments != 1) {
		realObject->error() << "Improper number of arguments in checkLineOfSight in LuaSceneObject.";
		return 0;
	}

	bool res = false;

	SceneObject* obj = (SceneObject*)lua_touserdata(L, -1);

	if (realObject != nullptr && obj != nullptr) {

		res = CollisionManager::checkLineOfSight(realObject, obj);

	}

	lua_pushboolean(L, res);

	return 1;
}