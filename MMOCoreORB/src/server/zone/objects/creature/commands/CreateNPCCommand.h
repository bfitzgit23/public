/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef CREATENPCCOMMAND_H_
#define CREATENPCCOMMAND_H_

#include "server/zone/managers/director/DirectorManager.h"
#include "server/zone/objects/creature/ai/AiAgent.h"
#include "server/zone/objects/creature/commands/QueueCommand.h"
//Infinity:  Custom includes
#include "server/zone/objects/area/ActiveArea.h"

class CreateNPCCommand : public QueueCommand {
public:

	CreateNPCCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		StringTokenizer args(arguments.toString());

		if (!args.hasMoreTokens())
			return INVALIDPARAMETERS;

		String arg = "";

		args.getStringToken(arg);

		if (arg == "tools") {
			Lua* lua = DirectorManager::instance()->getLuaInstance();

			Reference<LuaFunction*> staffTools = lua->createFunction("StaffTools", "openToolsSUI", 0);
			*staffTools << creature;

			staffTools->callFunction();
		} else if (arg == "toggledebug") {
			ManagedReference<AiAgent*> aiAgent = server->getZoneServer()->getObject(creature->getTargetID()).castTo<AiAgent*>();

			if (aiAgent == nullptr)
				return GENERALERROR;

			Locker clocker(aiAgent, creature);

			bool curDebug = aiAgent->getAIDebug();

			aiAgent->setAIDebug(!curDebug);

			StringBuffer msg;
			msg << "AiAgent " << aiAgent->getObjectID() << " debug set to " << aiAgent->getAIDebug();

#ifndef DEBUG_AI
			msg << " not compiled with DEBUG_AI, using LogLevel";
#endif // DEBUG_AI

			String logFileName = aiAgent->getLogFileName();

			if (!logFileName.isEmpty()) {
				msg << " logging to " << logFileName;
			}

			creature->sendSystemMessage(msg.toString());
		} else if (arg == "debugregions") {
			if (creature->isDebuggingRegions()) {
				creature->setDebuggingRegions(false);
				creature->sendSystemMessage("Region System Message Debug Disabled");
			} else {
				creature->setDebuggingRegions(true);
				creature->sendSystemMessage("Region System Message Debug Enabled");
			}

		} else if (arg == "activeareas") {

			auto zone = creature->getZone();
			if (zone == nullptr) {
				return GENERALERROR;
			}

			StringBuffer msg;

			msg << endl << "List of active areas at current position" << endl;
			msg << "-----------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;


			SortedVector<ManagedReference<ActiveArea*> >* activeAreas = creature->getActiveAreas();

			//zone->getInRangeActiveAreas(creature->getWorldPositionX(), creature->getWorldPositionY(), &activeAreas, true);

			for (int i = 0; i < activeAreas->size(); ++i) {
				ActiveArea* area = activeAreas->get(i);

				if (area == nullptr) {
					continue;
				}

				String areaName = (area->getAreaName().isEmpty()) ? "Undefined" : area->getAreaName();

				msg << " Area name: " << areaName << "  ";

				String shape = "Undefined";
				if (area->isCircularAreaShape())
					shape = "Circle";
				if (area->isRectangularAreaShape())
					shape = "Rectangle";
				else if (area->isRingAreaShape())
					shape = "Ring";
				msg << "Shape: " << shape << "  ";

				Vector3 coords = area->getWorldPosition();
				msg << "Position: " << coords.getX() <<  ", " << coords.getZ() << ", " << coords.getY() << "  ";

				msg << "Radius: " << area->getRadius() << endl;

				StringBuffer regionTypes;

				if (area->isUndefinedRegion())
					regionTypes << "UNDEFINEDAREA ";
				if (area->isSpawnArea())
					regionTypes << "SPAWNAREA ";
				if (area->isNoSpawnArea())
					regionTypes << "NOSPAWNAREA ";
				if (area->isWorldSpawnArea())
					regionTypes << "WORLDSPAWNAREA ";
				if (area->isNoWorldSpawnArea())
					regionTypes << "NOWORLDSPAWNAREA ";
				if (area->isNoBuildZone())
					regionTypes << "NOBUILDZONEAREA ";
				if (area->isCampingArea())
					regionTypes << "CAMPINGAREA ";
				if (area->isCityRegion())
					regionTypes << "CITY ";
				if (area->shouldBuildNavmesh())
					regionTypes << "NAVAREA ";
				if (area->isNamedRegion())
					regionTypes << "NAMEDREGION ";
				if (area->isLockedArea())
					regionTypes << "LOCKEDAREA";

				msg << " Region Types: (" << regionTypes.toString() << ")" << endl;

				msg << "-----------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
			}

			creature->sendSystemMessage(msg.toString());
		}

		return SUCCESS;
	}

};

#endif //CREATENPCCOMMAND_H_
