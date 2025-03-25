/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions. */

#ifndef SNOOPCOMMAND_H_
#define SNOOPCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/player/sui/messagebox/SuiMessageBox.h"
#include "server/zone/objects/tangible/components/vendor/VendorDataComponent.h"
#include "server/zone/managers/mission/MissionManager.h"

#include "server/zone/managers/auction/AuctionManager.h"
#include "server/zone/managers/auction/AuctionsMap.h"
#include "server/zone/managers/director/ScreenPlayTask.h"
//Infinity:  Custom includes
#include "server/zone/objects/creature/ai/AiAgent.h"
#include "server/zone/objects/player/badges/Badge.h"
#include "server/zone/managers/player/BadgeList.h"
#include "server/zone/managers/stringid/StringIdManager.h"

class SnoopCommand : public QueueCommand {
public:

	SnoopCommand(const String& name, ZoneProcessServer* server)
	: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();

		if (ghost == nullptr)
			return GENERALERROR;

		StringTokenizer args(arguments.toString());
		String targetName = "";
		String container = "";

		PlayerManager* playerManager = server->getZoneServer()->getPlayerManager();
		ManagedReference<CreatureObject*> targetCreo = nullptr;

		if (creature->getTargetID() != 0) {
			targetCreo = server->getZoneServer()->getObject(creature->getTargetID()).castTo<CreatureObject*>();
		} else {
			if(!args.hasMoreTokens())
				return GENERALERROR;

			args.getStringToken(targetName);

			targetCreo = playerManager->getPlayer(targetName);
		}

		if (targetCreo == nullptr)
			return INVALIDTARGET;

		Locker guard(targetCreo, creature);

		ManagedReference<PlayerObject*> targetGhost = targetCreo->getPlayerObject();

		if (args.hasMoreTokens())
			args.getStringToken(container);

		container = container.toLowerCase();

		//Infinity:  Allow snoop buffs for creatures and pets
		if (targetGhost == nullptr && container != "buffs")
			return GENERALERROR;

		if (container == "equipment") {
			targetCreo->sendWithoutParentTo(creature);
			targetCreo->openContainerTo(creature);
		} else if (container == "datapad") {
			SceneObject* creatureDatapad = targetCreo->getSlottedObject("datapad");

			if (creatureDatapad == nullptr)
				return GENERALERROR;

			creatureDatapad->sendWithoutParentTo(creature);
			creatureDatapad->openContainerTo(creature);
		}  else if (container == "bank") {
			SceneObject* creatureBank = targetCreo->getSlottedObject("bank");

			if (creatureBank == nullptr)
				return GENERALERROR;

			creatureBank->sendWithoutParentTo(creature);
			creatureBank->openContainerTo(creature);
		} else if (container == "credits") {
			int cash = targetCreo->getCashCredits();
			int bank = targetCreo->getBankCredits();
			StringBuffer body;

			body << "Player Name:\t" << targetCreo->getFirstName();
			body << "\nCash Credits:\t" << String::withCommas(cash);
			body << "\nBank Credits:\t" << String::withCommas(bank);
			body << "\nBank Location:\t" << targetGhost->getBankLocation();

			ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, SuiWindowType::ADMIN_PLAYER_CREDITS);
			box->setPromptTitle("Player Credits");
			box->setPromptText(body.toString());
			box->setUsingObject(targetCreo);
			box->setForceCloseDisabled();

			ghost->addSuiBox(box);
			creature->sendMessage(box->generateMessage());
		} else if (container == "jeditrainer") {
			if (targetGhost->getJediState() < 2 || !targetCreo->hasSkill("force_title_jedi_rank_02")) {
				creature->sendSystemMessage(targetCreo->getFirstName() + " does not have a jedi state of 2+ or does not have the padawan skill box.");
				return GENERALERROR;
			}

			String planet = targetGhost->getTrainerZoneName();
			Vector3 coords = targetGhost->getJediTrainerCoordinates();

			creature->sendSystemMessage(targetCreo->getFirstName() + "'s jedi trainer is located at " + coords.toString() + " on " + planet);
		} else if (container == "ham") {
			return sendHam(creature, targetCreo);
		} else if (container == "lots") {
			return sendLots(creature, targetCreo);
		} else if (container == "vendors") {
			return sendVendorInfo(creature, targetCreo);
		} else if (container == "veteranrewards") {
			return sendVeteranRewardInfo(creature, targetCreo);
		} else if(container == "faction") {
			return sendFactionInfo(creature, targetCreo);
		} else if (container == "screenplaydata") {
			if (!args.hasMoreTokens()) {
				creature->sendSystemMessage("SYNTAX: /snoop [player] screenplaydata <screenplay> [variable]");
				return INVALIDPARAMETERS;
			}

			String playName, varName;
			args.getStringToken(playName);

			if (!args.hasMoreTokens()) {
				creature->sendSystemMessage("SYNTAX: /snoop [player] screenplaydata <screenplay> [variable]");
				return INVALIDPARAMETERS;
			}

			args.getStringToken(varName);

			ManagedReference<PlayerObject*> targetGhost = targetCreo->getPlayerObject();

			if (targetGhost == nullptr)
				return GENERALERROR;

			String result = targetGhost->getScreenPlayData(playName, varName);

			creature->sendSystemMessage(targetCreo->getFirstName() + "'s screenplay data value for screenplay " + playName + " and variable " + varName + " is: " + result);
		} else if (container == "stringdata") {
			if (!args.hasMoreTokens()) {
				creature->sendSystemMessage("SYNTAX: /snoop [player] stringdata <variable>");
				return INVALIDPARAMETERS;
			}

			String varName;
			args.getStringToken(varName);

			ManagedReference<PlayerObject*> targetGhost = targetCreo->getPlayerObject();

			if (targetGhost == nullptr)
				return GENERALERROR;

			String result = DirectorManager::instance()->readStringSharedMemory(String::valueOf(targetCreo->getObjectID()) + varName);

			creature->sendSystemMessage(targetCreo->getFirstName() + "'s string data value for variable " + varName + " = " + result);

		} else if (container == "screenplaystate") {
			if (!args.hasMoreTokens()) {
				creature->sendSystemMessage("SYNTAX: /snoop [player] screenplaystate <stateName> [state]");
				return INVALIDPARAMETERS;
			}
			String stateName;
			args.getStringToken(stateName);

			uint64 state = targetCreo->getScreenPlayState(stateName);
			if (args.hasMoreTokens()) {
				uint64 stateToCheck = args.getIntToken();
				if (state & stateToCheck)
					creature->sendSystemMessage(targetCreo->getFirstName() + " state check of '" + String::valueOf(stateToCheck) + "' for screenplayState '" + stateName + "': TRUE.");
				else
					creature->sendSystemMessage(targetCreo->getFirstName() + " state check of '" + String::valueOf(stateToCheck) + "' for screenplayState '" + stateName + "': FALSE.");
			} else {
				creature->sendSystemMessage(targetCreo->getFirstName() + " state check for screenplayState '" + stateName + "': " + String::valueOf(state) + ".");
			}
		} else if (container == "activescreenplay") {
			String key = String::valueOf(targetCreo->getObjectID()) + ":activeScreenPlay";
			String data = DirectorManager::instance()->getStringSharedMemory(key);
			creature->sendSystemMessage(targetCreo->getFirstName() + " active screenplay: " + data);
		} else if (container == "luaevents") {
			return sendLuaEvents(creature, targetCreo);
		} else if (container == "buffs") {
			return sendBuffs(creature, targetCreo);
		} else if (container == "visibility") {
			MissionManager* missionManager = creature->getZoneServer()->getMissionManager();

			if (missionManager->sendPlayerBountyDebug(creature, targetCreo))
				return SUCCESS;
			else
				return GENERALERROR;
		} else if (container == "frs") {
			FrsData* playerData = targetGhost->getFrsData();
			int playerRank = playerData->getRank();
			int playerCouncil = playerData->getCouncilType();

			String councilString;
			if (playerCouncil == 1)
				councilString = "Light";
			else if (playerCouncil == 2)
				councilString = "Dark";
			else
				councilString = "Unknown";

			StringBuffer msg;
			
			msg << targetCreo->getFirstName() << " is a rank " << playerRank << " " << councilString << " Jedi with " << String::withCommas(targetGhost->getExperience("force_rank_xp")) << " FRS XP.";
			creature->sendSystemMessage(msg.toString());

		} else if (container == "export") {
			StringBuffer reason = "/snoop " + targetCreo->getFirstName() + " export by " + creature->getFirstName();

			if (args.hasMoreTokens()) {
				String note;
				args.finalToken(note);
				reason << "; Admin Note: " << note;
			}

			String exportFile = targetCreo->exportJSON(reason.toString());
			creature->sendSystemMessage(targetCreo->getFirstName() + " exported to " + exportFile + " ask a server admin to review the file for you.");
		} else if (container == "skills") {
			return sendSkillsInfo(creature, targetCreo, args.hasMoreTokens());

		} else if (container == "experience" || container == "xp") {
			return sendExperienceInfo(creature, targetCreo, args.hasMoreTokens());

		} else if (container == "activepets") {
			return sendActivePets(creature, targetCreo);

		} else if (container == "skillmods") {
			return sendSkillModsInfo(creature, targetCreo, args.hasMoreTokens());

		} else if (container == "badges") {
			return sendBadgeInfo(creature, targetCreo, args.hasMoreTokens());

		} else if (container == "permissiongroups") {

			SortedVector<String>* groups = targetGhost->getPermissionGroups();

			if (groups != nullptr && groups->size() > 0) {

				StringBuffer body;

				body << endl << "Permission groups for player " << targetCreo->getDisplayedName() << endl;

				for (int i = 0; i < groups->size(); ++i) {
					const String group = groups->get(i);

					body << "  " << group << endl;
				}

				creature->sendSystemMessage(body.toString());

				if (args.hasMoreTokens()) {	// Send an email if more tokens after containers
					auto chatManager = server->getZoneServer()->getChatManager();
					if (chatManager != nullptr) {
						chatManager->sendMail("System", "Permission groups for " + targetCreo->getDisplayedName(), body.toString(), creature->getFirstName());
					}
				}
			}
			else {
				creature->sendSystemMessage(targetCreo->getDisplayedName() + " has no permission groups.");
			}

		} else {
			SceneObject* creatureInventory = targetCreo->getSlottedObject("inventory");

			if (creatureInventory == nullptr)
				return GENERALERROR;

			creatureInventory->sendWithoutParentTo(creature);
			creatureInventory->openContainerTo(creature);
		}

		return SUCCESS;
	}

	int sendLuaEvents(CreatureObject* creature, CreatureObject* target) const {

		if (creature == nullptr || target == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();

		if (ghost == nullptr)
			return GENERALERROR;

		Vector<Reference<ScreenPlayTask*> > eventList = DirectorManager::instance()->getObjectEvents(target);

		ManagedReference<SuiListBox*> box = new SuiListBox(creature, 0);
		box->setPromptTitle("LUA Events");
		Time currentTime;
		box->setPromptText("Below are the LUA Events currently scheduled for the player.\n\nCurrent server time: " + currentTime.getFormattedTime());
		box->setUsingObject(target);
		box->setForceCloseDisabled();

		for (int i = 0; i < eventList.size(); i++) {
			Reference<ScreenPlayTask*> task = eventList.get(i);

			if (task == nullptr)
				continue;

			String buffer = task->getScreenPlay() + ":" + task->getTaskKey();
			String args = task->getArgs();

			if (args != "")
				buffer += " (Args: " + args + ")";

			AtomicTime nextExecutionTime;
			Core::getTaskManager()->getNextExecutionTime(task, nextExecutionTime);
			int64 miliDiff = nextExecutionTime.miliDifference();

			buffer += ", Execution (server time): " + nextExecutionTime.getFormattedTime() + " (" + getTimeString(-miliDiff) + " from now)";

			box->addMenuItem(buffer);
		}

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		return SUCCESS;
	}

	String getTimeString(uint64 timestamp) const {
		int seconds = timestamp / 1000;

		int hours = seconds / 3600;
		seconds -= hours * 3600;

		int minutes = seconds / 60;
		seconds -= minutes * 60;

		StringBuffer buffer;

		if (hours > 0)
			buffer << hours << "h ";

		if (minutes > 0)
			buffer << minutes << "m ";

		if (seconds > 0)
			buffer << seconds << "s";

		return buffer.toString();
	}

	int sendVeteranRewardInfo(CreatureObject* creature, CreatureObject* target) const {

		if (creature == nullptr || target == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> targetGhost = target->getPlayerObject();
		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		auto playerManager = server->getZoneServer()->getPlayerManager();

		if (targetGhost == nullptr || ghost == nullptr || playerManager == nullptr)
			return GENERALERROR;

		StringBuffer body;
		body << "Player Name:\t" << target->getFirstName() << endl;
		body << "Claimed Rewards:" << endl;
		body << "\tMilestone\tReward"<< endl;
		for( int i = 0; i < playerManager->getNumVeteranRewardMilestones(); i++ ){
			int milestone = playerManager->getVeteranRewardMilestone(i);
			body << "\t" << String::valueOf(milestone);
			String claimedReward = targetGhost->getChosenVeteranReward(milestone);
			if( claimedReward.isEmpty() ){
				body << "\t\t" << "Unclaimed" << endl;
			}
			else{
				body << "\t\t" << claimedReward << endl;
			}
		}

		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);
		box->setPromptTitle("Veteran Reward Info");
		box->setPromptText(body.toString());
		box->setUsingObject(target);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		return SUCCESS;

	}

	int sendFactionInfo(CreatureObject* creature, CreatureObject* target) const {

		if (creature == nullptr || target == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> targetGhost = target->getPlayerObject();
		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		auto playerManager = server->getZoneServer()->getPlayerManager();

		if (targetGhost == nullptr || ghost == nullptr || playerManager == nullptr)
			return GENERALERROR;

		StringBuffer body;
		body << "Player Name:\t" << target->getFirstName() << endl;
		body << "Affiliation:\t";

		if (target->isImperial())
			body << "Imperial";
		else if (target->isRebel())
			body << "Rebel";
		else
			body << "Neutral" << endl;

		int rank = 0;

		if (target->isImperial() || target->isRebel()) {
			if (target->getFactionStatus() == FactionStatus::ONLEAVE)
				body << " (On Leave)" << endl;
			else if (target->getFactionStatus() == FactionStatus::OVERT)
				body << " (Overt)" << endl;
			else if (target->getFactionStatus() == FactionStatus::COVERT)
				body << " (Covert)" << endl;

			rank = target->getFactionRank();
			body << "Rank:\t" << FactionManager::instance()->getRankName(rank) << " (Rank " << rank + 1 << ")" << endl;
		}
		body << "Imperial Points:\t" << targetGhost->getFactionStanding("imperial") << " (Max: " << (target->isImperial() ? FactionManager::instance()->getFactionPointsCap(rank) : 1000) << ")" << endl;
		body << "Rebel Points:\t" << targetGhost->getFactionStanding("rebel") << " (Max: " << (target->isRebel() ? FactionManager::instance()->getFactionPointsCap(rank) : 1000) << ")" << endl;

		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);
		box->setPromptTitle("Faction Information");
		box->setPromptText(body.toString());
		box->setUsingObject(target);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		return SUCCESS;

	}

	int sendVendorInfo(CreatureObject* creature, CreatureObject* target) const {

		if (creature == nullptr || target == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> targetGhost = target->getPlayerObject();
		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		auto auctionManager = server->getZoneServer()->getAuctionManager();

		if (targetGhost == nullptr || ghost == nullptr || auctionManager == nullptr)
			return GENERALERROR;

		ManagedReference<AuctionsMap*> auctionsMap = auctionManager->getAuctionMap();
		if(auctionsMap == nullptr)
			return GENERALERROR;

		StringBuffer body;
		body << "Player Name:\t" << target->getFirstName() << endl;
		body << "Max # of vendors:\t" << target->getSkillMod("manage_vendor") << endl;
		body << "Max # of items:\t" << target->getSkillMod("vendor_item_limit") << endl;
		body << "Total # of items:\t" << auctionsMap->getPlayerItemCount(target) << endl << endl;
		body << "Vendors:" << endl;

		const SortedVector<unsigned long long>* ownedVendors = targetGhost->getOwnedVendors();
		for (int i = 0; i < ownedVendors->size(); i++) {
			ManagedReference<SceneObject*> vendor = creature->getZoneServer()->getObject(ownedVendors->elementAt(i));

			int num = i + 1;
			body << endl << String::valueOf(num) << ". ";

			if (vendor == nullptr) {
				body << "nullptr Vendor" << endl << endl;
				continue;
			}

			body << "VendorID:\t" << vendor->getObjectID() << endl;

			DataObjectComponentReference* data = vendor->getDataObjectComponent();
			if(data == nullptr || data->get() == nullptr || !data->get()->isVendorData()) {
				body << "    nullptr Data Component" << endl << endl;
				continue;
			}

			VendorDataComponent* vendorData = cast<VendorDataComponent*>(data->get());
			if(vendorData == nullptr) {
				body << "    nullptr Vendor Data Component" << endl << endl;
				continue;
			}

			bool init = false;
			if (vendorData->isInitialized())
				init = true;

			body << "    Initialized?\t" << (init ? "Yes" : "No");
			body << endl << "    # of items:\t" << auctionsMap->getVendorItemCount(vendor) << endl;

			body << "    ParentID:\t";

			ManagedReference<SceneObject*> parent = vendor->getParent().get();
			if (parent == nullptr)
				body << "nullptr" << endl;
			else
				body << parent->getObjectID() << endl;

			body << "    Zone:\t";

			Zone* zone = vendor->getZone();
			if (zone == nullptr) {
				body << "nullptr" << endl;
			} else {
				body << zone->getZoneName() << endl;
				body << "    World Position:\t" << vendor->getWorldPositionX() << ", " << vendor->getWorldPositionY() << endl;
			}

		}

		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);

		box->setPromptTitle("Vendor Info");
		box->setPromptText(body.toString());
		box->setUsingObject(target);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		return SUCCESS;
	}

	int sendLots(CreatureObject* creature, CreatureObject* target) const {

		if (creature == nullptr || target == nullptr)
			return GENERALERROR;

		auto targetGhost = target->getPlayerObject();
		auto ghost = creature->getPlayerObject();
        auto zoneServer = creature->getZoneServer();

		if (targetGhost == nullptr || ghost == nullptr || zoneServer == nullptr)
			return GENERALERROR;

		StringBuffer body;

		body << "Player Name:\t" << target->getFirstName() << endl;
		body << "Unused Lots:\t" << targetGhost->getLotsRemaining() << endl << endl;
		body << "Player Structures:";

		for (int i = 0; i < targetGhost->getTotalOwnedStructureCount(); i++) {

            uint64 oid = targetGhost->getOwnedStructure(i);

		    auto structure = zoneServer->getObject(oid).castTo<StructureObject*>();

			int num = i + 1;
			body << endl << String::valueOf(num) << ". ";

			if (structure == nullptr) {
				body << "nullptr Structure.  OID: " << oid << "." << endl << endl;
				continue;
			}

			body << "StructureID:\t" << structure->getObjectID() << endl;
			body << "    Name:\t" << structure->getCustomObjectName().toString() << endl;
			body << "    Type:\t" << structure->getObjectTemplate()->getFullTemplateString() << endl;
			body << "    Lots:\t" << String::valueOf(structure->getLotSize()) << endl;

			body << "    Zone:\t";
			Zone* zone = structure->getZone();
			if (zone == nullptr) {
				body << "nullptr" << endl;
			} else {
				body << zone->getZoneName() << endl;
				body << "    World Position:\t" << structure->getWorldPositionX() << ", " << structure->getWorldPositionY() << endl;
			}
		}

		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);
		box->setPromptTitle("Player Lots");
		box->setPromptText(body.toString());
		box->setUsingObject(target);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		return SUCCESS;
	}

	int sendHam(CreatureObject* creature, CreatureObject* target) const {

		if (creature == nullptr || target == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		ManagedReference<PlayerObject*> targetGhost = target->getPlayerObject();

		if (ghost == nullptr) {
			return GENERALERROR;
		}

		StringBuffer body;
		body << "Player Name:\t" << target->getFirstName() << endl << endl;

		for (int i = 0; i < 9; i++) {

			switch (i) {
			case 0:
				body << "Health: ";
				break;
			case 1:
				body << "Strength: ";
				break;
			case 2:
				body << "Constitution: ";
				break;
			case 3:
				body << "Action: ";
				break;
			case 4:
				body << "Quickness: ";
				break;
			case 5:
				body << "Stamina: ";
				break;
			case 6:
				body << "Mind: ";
				break;
			case 7:
				body << "Focus: ";
				break;
			case 8:
				body << "Willpower: ";
				break;
			default:
				break;
			}

			body << target->getHAM(i) << " / " << target->getMaxHAM(i) << "\n";
			body << "Wounds: " << target->getWounds(i) << "\n";
			body << "Modifiers: " << (target->getMaxHAM(i) - target->getBaseHAM(i) + target->getEncumbrance(i / 3)) << "\n";
			body << "Encumbrance: " << target->getEncumbrance(i / 3) << "\n";

			body << "\n\n";
		}

		if (targetGhost != nullptr) { // if we're not a PlayerObject, we don't hold force values
			body << "Force Power:\t" << targetGhost->getForcePower() << " / " << targetGhost->getForcePowerMax() << endl;
			body << "Force Regen:\t" << target->getSkillMod("jedi_force_power_regen") << endl;
		}

		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);
		box->setPromptTitle("Player HAM");
		box->setPromptText(body.toString());
		box->setUsingObject(target);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		return SUCCESS;
	}

	int sendBuffs(CreatureObject* creature, CreatureObject* target) const {

		if (creature == nullptr || target == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		if (ghost == nullptr) {
			return GENERALERROR;
		}

		const BuffList* bList = target->getBuffList();
		if (bList == nullptr || bList->getBuffListSize() == 0) {
			creature->sendSystemMessage("No Buffs to Display.");
			return SUCCESS;
		}

		StringBuffer buffText;

		for (int i = 0; i < bList->getBuffListSize(); i++) {
			Buff* buff = bList->getBuffByIndex(i);
			buffText << buff->getBuffName() << ":" <<endl;
			buffText << "\tCRC: 0x" << hex << buff->getBuffCRC() << endl;

			const Vector<uint64>* secondaryCRCs = buff->getSecondaryBuffCRCs();
			if (secondaryCRCs != nullptr && secondaryCRCs->size() > 0) {
				buffText << "\tSecondary CRCs: "<< endl;
				for (int j = 0; j < secondaryCRCs->size(); j++) {
					buffText << "\t\t 0x" << hex << buff->getSecondaryBuffCRCs() << endl;
				}
			}

			buffText << "\tDuration (" << buff->getBuffDuration() << ") Time Left (" << buff->getTimeLeft() << ")" << endl;

			buffText << "\tAttribute Mods: " << buff->getAttributeModifierString() << endl;
			buffText << "\tSkill Mods: " << buff->getSkillModifierString() << endl;
		}

		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);
		box->setPromptTitle("Creature Buffs");
		box->setPromptText(buffText.toString());
		box->setUsingObject(target);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		return SUCCESS;
	}

	int sendSkillsInfo(CreatureObject* creature, CreatureObject* targetCreo, bool eMail = false) const {

		if (creature == nullptr || targetCreo == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		ManagedReference<PlayerObject*> targetGhost = targetCreo->getPlayerObject();

		if (targetGhost == nullptr || ghost == nullptr)
			return GENERALERROR;

		Locker locker(targetCreo,creature);

		SkillManager* skillManager = SkillManager::instance();
		const SkillList* skillList = targetCreo->getSkillList();
        StringBuffer body;
		String targetName = targetCreo->getDisplayedName();

		if (skillList == nullptr){
			return GENERALERROR;
		}

		body << "Player Name:\t" << targetName;
		body << "\nSkills on Character:\n";
		String skillName = "";
		Vector<String> listOfNames;
		skillList->getStringList(listOfNames);
		SkillList copyOfList;
		copyOfList.loadFromNames(listOfNames);

		for (int i = 0; i < copyOfList.size(); i++) {
			Skill* skill = copyOfList.get(i);
			String skillName = skill->getSkillName();
			body <<  skillName << "\n";
		}
		
		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);
		box->setPromptTitle("Skill Info");
		box->setPromptText(body.toString());
		box->setUsingObject(targetCreo);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		if (eMail) {	// Send an email if more tokens after experience
			auto chatManager = server->getZoneServer()->getChatManager();
			chatManager->sendMail("System", "Skill information for " + targetName, body.toString(), creature->getFirstName());
		}

		return SUCCESS;
	}

	int sendExperienceInfo(CreatureObject* creature, CreatureObject* targetCreo, bool eMail = false) const {

		if (creature == nullptr || targetCreo == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		ManagedReference<PlayerObject*> targetGhost = targetCreo->getPlayerObject();

		if (targetGhost == nullptr || ghost == nullptr)
			return GENERALERROR;

		Locker locker(targetCreo,creature);

		VectorMap<String, int>* xpCapList = targetGhost->getXpTypeCapList();

		if (xpCapList== nullptr)
			return GENERALERROR;
		
		StringBuffer body;
		String targetName = targetCreo->getDisplayedName();

		body << "Player Name:\t" << targetName;
		body << "\nExperience Values:\n";

		for (int i = 0; i < xpCapList->size(); ++i) {

			String xpType = xpCapList->elementAt(i).getKey();
			int xpValue = targetGhost->getExperience(xpType);
			if (xpValue != 0)
				body << xpType <<  ":  " << String::withCommas(xpValue) << "\n";
		}
					
		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);
		box->setPromptTitle("Experience Info");
		box->setPromptText(body.toString());
		box->setUsingObject(targetCreo);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		if (eMail) {	// Send an email if more tokens after experience
			auto chatManager = server->getZoneServer()->getChatManager();
			if (chatManager != nullptr)
				chatManager->sendMail("System", "Experience information for " + targetName, body.toString(), creature->getFirstName());
		}
			
		return SUCCESS;
	}

	int sendActivePets(CreatureObject* creature, CreatureObject* targetCreo) const {

		if (creature == nullptr || targetCreo == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		ManagedReference<PlayerObject*> targetGhost = targetCreo->getPlayerObject();

		if (targetGhost == nullptr || ghost == nullptr)
			return GENERALERROR;

		String targetName = targetCreo->getDisplayedName();

		Locker locker(targetCreo, creature);

		if (targetGhost->getActivePetsSize() > 0) {

			StringBuffer msg;
			msg << "\n" << "Active Pets for player " << targetName << "\n";
			msg << "-----------------------------------------------------------------------------------------------\n";

			for (int i = 0; i < targetGhost->getActivePetsSize(); i++) {
				ManagedReference<AiAgent*> pet = targetGhost->getActivePet(i);
				if (pet != nullptr) {
					msg << "#" << i+1 << " - OID = " << pet->getObjectID() << ".  Pet name = " << pet->getDisplayedName() << ".\n";
				}
			}

			creature->sendSystemMessage(msg.toString());
		}
		else {
			creature->sendSystemMessage("No active pets found for player " + targetName);
		}	
			
		return SUCCESS;
	}

	int sendSkillModsInfo(CreatureObject* creature, CreatureObject* targetCreo, bool eMail = false) const {

		if (creature == nullptr || targetCreo == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		ManagedReference<PlayerObject*> targetGhost = targetCreo->getPlayerObject();
		if (targetGhost == nullptr || ghost == nullptr)
			return GENERALERROR;

		String targetName = targetCreo->getDisplayedName();

		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);

		box->setPromptTitle("Skill Modifer Info For " + targetName);

		Locker smodLocker(targetCreo->getSkillModMutex());

		const SkillModList* skillModList = targetCreo->getSkillModList();

		smodLocker.release();

		StringBuffer promptText;

		promptText << "SkillMods" << endl << "------------" << endl;
		promptText << skillModList->getPrintableSkillModList();

		box->setPromptText(promptText.toString());
		creature->sendMessage(box->generateMessage());

		if (eMail) {	// Send an email if more tokens after experience
			auto chatManager = server->getZoneServer()->getChatManager();
			chatManager->sendMail("System", "Skill Modifier information for " + targetName, promptText.toString(), creature->getFirstName());
		}

		return SUCCESS;
	}

	int sendBadgeInfo(CreatureObject* creature, CreatureObject* targetCreo, bool eMail = false) const {

		if (creature == nullptr || targetCreo == nullptr)
			return GENERALERROR;

		ManagedReference<PlayerObject*> ghost = creature->getPlayerObject();
		ManagedReference<PlayerObject*> targetGhost = targetCreo->getPlayerObject();

		if (targetGhost == nullptr || ghost == nullptr)
			return GENERALERROR;

		Locker locker(targetCreo, creature);

        auto zoneServer = server->getZoneServer();

        if (zoneServer != nullptr) {
            auto playerManager = zoneServer->getPlayerManager();
            if (playerManager != nullptr) {
                playerManager->validateBadges(targetGhost, true); //Validate badges and count
            }
        }

		StringBuffer body;
		String targetName = targetCreo->getDisplayedName();

		body << "Player Name:\t\t" << targetName << endl;
		body << "Number of badges:\t" << targetGhost->getNumBadges() << endl;
		body << "Badge List:" << endl << endl;

		const BadgeList* badgeList = BadgeList::instance();

		if (badgeList == nullptr)
			return GENERALERROR;

		for (uint8 i = 0; i <= 250; ++i) { //Infinity:  Allow up to 251 badges
			if (targetGhost->hasBadge(i)) {

				const Badge* badge = badgeList->get(i);

				if (badge != nullptr) {
					String lookup = "@badge_n:" + badge->getKey();
					String type = badge->getTypeString();
					uint32 typeHash = type.hashCode();

					switch (typeHash) {
						case STRING_HASHCODE("events"):
							type = "Events\t\t\t\t";
							break;

						case STRING_HASHCODE("master"):
							type = "Master\t\t\t\t";
							break;

						case STRING_HASHCODE("content"):
							type = "Content\t\t\t\t";
							break;

						case STRING_HASHCODE("interest"):
							type = "Interest\t\t\t\t";
							break;

						case STRING_HASHCODE("accumulation"):
							type = "Accumulation\t\t\t";
							break;

						case STRING_HASHCODE("master_nojedi"):
							type = "Master (Non-Glowy)\t";
							break;

						case STRING_HASHCODE("exploration_jedi"):
							type = "Jedi POI\t\t\t\t";
							break;

						case STRING_HASHCODE("exploration_easy"):
							type = "Easy POI\t\t\t\t";
							break;

						case STRING_HASHCODE("exploration_hard"):
							type = "Hard POI\t\t\t\t";
							break;

						case STRING_HASHCODE("exploration_dangerous"):
							type = "Dangerous POI\t\t";
							break;

						default:
							type = "Unknown Type\t\t";
							break;
					}

					body << "\tType: " << type << "  Badge:  " << StringIdManager::instance()->getStringId(lookup.hashCode()).toString() << endl;
				}
			}
		}
				
		ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, 0);
		box->setPromptTitle("Badge Info");
		box->setPromptText(body.toString());
		box->setUsingObject(targetCreo);
		box->setForceCloseDisabled();

		ghost->addSuiBox(box);
		creature->sendMessage(box->generateMessage());

		if (eMail) {	// Send an email if more tokens after experience
			auto chatManager = server->getZoneServer()->getChatManager();
			if (chatManager != nullptr)
				chatManager->sendMail("System", "Badge information for " + targetName, body.toString(), creature->getFirstName());
		}
			
		return SUCCESS;
	}
	
};

#endif //SNOOPCOMMAND_H_
