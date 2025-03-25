/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef GETOBJVARSCOMMAND_H_
#define GETOBJVARSCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/chat/ChatManager.h"
#include "server/zone/objects/group/GroupObject.h"
#include "server/zone/objects/player/sui/messagebox/SuiMessageBox.h"

//Infinity:  Custom includes
#include "server/zone/managers/stringid/StringIdManager.h"
#include "server/zone/objects/tangible/weapon/WeaponObject.h"
#include "server/zone/objects/tangible/TangibleObject.h"


class GetObjVarsCommand : public QueueCommand {
public:
	GetObjVarsCommand(const String& name, ZoneProcessServer* server) : QueueCommand(name, server) {
	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {
		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		if (!creature->isPlayerCreature())
			return GENERALERROR;

		PlayerObject* ghost = creature->getPlayerObject();

		if (ghost == nullptr)
			return GENERALERROR;

		uint64 objectID = 0;
		UnicodeTokenizer tokenizer(arguments);
		tokenizer.setDelimeter(" ");

		// if we have an argument passed, use it
		if (tokenizer.hasMoreTokens()) {
			try {
				objectID = tokenizer.getLongToken();
			} catch ( Exception& err ) {
				creature->sendSystemMessage("INVALID OBJECT.  Please specify a valid object name or objectid");
				return INVALIDPARAMETERS;
			}
		} else {
			objectID = target;
		}

		if ( objectID == 0) {
			creature->sendSystemMessage("You need to target an object or specify an object id: /getobjvars <objectID> ");
		}

		ManagedReference<SceneObject*> object = server->getZoneServer()->getObject(objectID, false);

		if (object == nullptr) {
			creature->sendSystemMessage("ERROR GETTING OBJECT - nullptr " + String::valueOf(objectID));
		} else {
			String strClassName = object->getObjectNameStringIdName();
			String strDescription = object->getDetailedDescription();
			bool bMarkedForDelete = object->_isMarkedForDeletion();
			bool bIsUpdated = object->_isUpdated();
			int rCount = object.get()->getReferenceCount();
			uint64 parentID = object->getParentID();
			StringBuffer msg;
			msg << endl << "OBJECTID: " << String::valueOf(objectID) << endl;
			msg << "OBJECTTYPE: " << String::valueOf(object->getGameObjectType()) << endl;

			msg << "Displayed Name:  " << object->getDisplayedName() << endl;
			if (object->isCreatureObject()) {
				msg << "Creature First Name:  " << object.castTo<CreatureObject*>()->getFirstName() << endl;
			}

			msg << "Template Filename: " << object->getObjectTemplate()->getTemplateFileName() << endl;

			if (object->isTangibleObject()) {
				ManagedReference<TangibleObject*> tano = object->asTangibleObject();
				if (tano != nullptr) {
					unsigned faction = tano->getFaction();
					String factionString = "N/A";
					if (faction == Factions::FACTIONIMPERIAL) 
						factionString = "IMPERIAL";
					else if (faction == Factions::FACTIONREBEL)
						factionString = "REBEL";
					else
						factionString = "NEUTRAL";

					int factionStatus = tano->getFactionStatus();
					String statusString = "N/A";
					if (factionStatus == FactionStatus::ONLEAVE) 
						statusString = "ONLEAVE";
					else if (factionStatus == FactionStatus::COVERT)
						statusString = "COVERT";
					else if (factionStatus == FactionStatus::OVERT)
						statusString = "OVERT";
					msg << "Faction: " << factionString << "  Faction Status: " << statusString << endl;
				}
			}

			msg << "CLASS: " << strClassName << endl;
			msg << "Marked for deletion: " << String::valueOf(bMarkedForDelete) << endl;
			msg << "IsUpdated: " <<  String::valueOf(bIsUpdated) << endl;
			msg << "REFERENCE COUNT " << String::valueOf(rCount) << endl;
			msg << "Path: " << object->getObjectTemplate()->getFullTemplateString() << endl;
			msg << "Children: " << String::valueOf(object->getChildObjects()->size()) << endl;
			msg << "PARENT OBJECTID: " << String::valueOf(parentID) << endl;

			if (object->isCreatureObject()) {
				ManagedReference<CreatureObject*> creoObject = object->asCreatureObject();

				if (creoObject != nullptr) {

                    if (creoObject->isPlayerCreature()) {
						auto playerManager = server->getPlayerManager();

						if (playerManager != nullptr) {
							int playerLevel = playerManager->calculatePlayerLevel(creoObject);

							msg << "Player Level: " << playerLevel << endl;
						}
                    }
                    else {
                        int creatureLevel = creoObject->getLevel();
                        msg << "Creature Level: " << creatureLevel << endl;
                    }

					msg << "PvP Status Bitmask: " << creoObject->getPvpStatusBitmask() << endl;
					msg << "Options Bitmask: " << creoObject->getOptionsBitmask() << endl;

					if (object->isAiAgent()) {
						ManagedReference<AiAgent*> objectAgent = object->asAiAgent();

						if (objectAgent != nullptr) {

                            String aiEnabled = (creoObject->getOptionsBitmask() & OptionBitmask::AIENABLED ? "True" : "False");
                            msg << "AI Enabled: " << aiEnabled << endl;

                            msg << "Creature Bitmask: " << objectAgent->getCreatureBitmask() << endl;

                            int mobType = objectAgent->getMobType();

                            msg << "MobType: ";

                            switch (mobType) {

                                case AiAgent::MOB_HERBIVORE:

                                    msg << "MOB_HERBIVORE";
                                    break;

                                case AiAgent::MOB_NPC:

                                    msg << "MOB_NPC" << endl;
                                    break;

                                case AiAgent::MOB_DROID:

                                    msg << "MOB_DROID" << endl;
                                    break;

                                case AiAgent::MOB_ANDROID:

                                    msg << "MOB_ANDROID" << endl;
                                    break;

                                case AiAgent::MOB_VEHICLE:

                                    msg << "MOB_DROID" << endl;
                                    break;

                                default:
                                    msg << "UNKNOWN" << endl;
                                    break;
                            }

							if (objectAgent->hasCreatureFlag(CreatureFlag::ADULTONLY))
								msg << "  ADULTONLY: True" << endl;
							if (objectAgent->hasCreatureFlag(CreatureFlag::BABYONLY))
								msg << "  BABYONLY: True" << endl;
							if (objectAgent->hasCreatureFlag(CreatureFlag::NOBLIND))
								msg << "  NOBLIND: True" << endl;
							if (objectAgent->hasCreatureFlag(CreatureFlag::NODIZZY))
								msg << "  NODIZZY: True" << endl;
							if (objectAgent->hasCreatureFlag(CreatureFlag::NODOT))
								msg << "  NODOT: True" << endl;
							if (objectAgent->hasCreatureFlag(CreatureFlag::NOINTIMIDATE))
								msg << "  NOINTIMIDATE: True" << endl;
							if (objectAgent->hasCreatureFlag(CreatureFlag::NOKNOCKDOWN))
								msg << "  NOKNOCKDOWN: True" << endl;
							if (objectAgent->hasCreatureFlag(CreatureFlag::NOPOSTUREDOWN))
								msg << "  NOPOSTUREDOWN: True" << endl;
							if (objectAgent->hasCreatureFlag(CreatureFlag::NOSTUN))
								msg << "  NOSTUN: True" << endl;

							msg << "Creature Movement State: " << objectAgent->getMovementState() << endl;
							msg << "Aggro Radius: " << objectAgent->getAggroRadius() << endl;
							msg << "Respawn Timer: " << (int)objectAgent->getRespawnTimer() << " seconds" << endl;

							ManagedReference<SceneObject*> followCopy = objectAgent->getFollowObject();
							StringBuffer hasFollow;

							if (followCopy != nullptr) {
								hasFollow << "True - " << " OID: " << followCopy->getObjectID();
							} else {
								hasFollow << "False";
							}

							msg << "Has Follow Object: " << hasFollow.toString() << endl;
							msg << "Current total Patrol Points: " << objectAgent->getPatrolPointSize() << endl;
							msg << "In Navmesh: " << (objectAgent->isInNavMesh() ? "True" : "False") << endl;

							msg << "Weapon Information" << endl;

							ManagedReference<WeaponObject*> currentWeapon = objectAgent->getCurrentWeapon();
							if (currentWeapon != nullptr) {
								msg << "  Current Weapon: " << currentWeapon->getObjectTemplate()->getTemplateFileName() << "  OID: " << currentWeapon->getObjectID() <<  endl;
								msg << "    Min Damage:  " << currentWeapon->getMinDamage() << "  Max Damage:  " << currentWeapon->getMaxDamage() << endl;
							}
							ManagedReference<WeaponObject*> defaultWeapon = objectAgent->getDefaultWeapon();	
							if (defaultWeapon != nullptr) {
								msg << "  Default Weapon: " << defaultWeapon->getObjectTemplate()->getTemplateFileName() << "  OID: " << defaultWeapon->getObjectID() <<  endl;
							}

							ManagedReference<WeaponObject*> primaryWeapon = objectAgent->getPrimaryWeapon();	
							if (primaryWeapon != nullptr) {
								msg << "  Primary Weapon: " << primaryWeapon->getObjectTemplate()->getTemplateFileName() << "  OID: " << primaryWeapon->getObjectID() <<  endl;
							}

							ManagedReference<WeaponObject*> secondaryWeapon = objectAgent->getSecondaryWeapon();	
							if (secondaryWeapon != nullptr) {
								msg << "  Secondary Weapon: " << secondaryWeapon->getObjectTemplate()->getTemplateFileName() << "  OID: " << secondaryWeapon->getObjectID() <<  endl;
							}

							msg << "  AI Agent (Template) Min Damage:  " << objectAgent->getMinDamage() << "  Max Damage:  " << objectAgent->getMaxDamage() << "  Weapon Speed: " << objectAgent->getWeaponSpeed() << endl;

							const CreatureAttackMap* attackMap = objectAgent->getAttackMap();
							if (attackMap != nullptr && attackMap->size() > 0) {

								String map = "Default";
								if (currentWeapon == primaryWeapon) 
									map = "Primary";
								else if (currentWeapon == secondaryWeapon)
									map = "Secondary";
								msg << map << " Weapon Attack Map" << endl;
								for (int i = 0; i < attackMap->size(); i++) {
			
									String cmd = attackMap->getCommand(i);
									String formattedName = "";
									if (cmd.isEmpty()) {
										formattedName = "none";
									}
									else {
										String lookup = "@combat_effects:" + cmd;
										formattedName = StringIdManager::instance()->getStringId(lookup.hashCode()).toString();
									}

									msg << "  Attack #" << i+1 << ":  " << (formattedName != "" ? formattedName : cmd) << endl;
								}
							}

							int specialPercentage = objectAgent->getSpecialPercentage();

							if (specialPercentage > 0 ) {
								msg << "  Special Percentage: " << objectAgent->getSpecialPercentage() << endl;
							}
							else {
								msg << "  Special Percentage: " << 0 << "  Using default value: " << AiAgent::DEFAULT_SPECIAL_PERCENTAGE << endl;
							}

							// Home Object - Lairs
							uint64 homeID = 0;
							String homeName = "none";
							ManagedReference<SceneObject*> homeLair = objectAgent->getHomeObject().get();

							if (homeLair != nullptr) {
								homeID = homeLair->getObjectID();
								homeName = homeLair->getObjectNameStringIdName();
							}

							msg << "Home Object: " << homeName << " ID: " << homeID << endl;

							PatrolPoint* homeLocation = objectAgent->getHomeLocation();

							if (homeLocation != nullptr) {
								msg << "Home Location: X:" << homeLocation->getPositionX() << ", Z:" << homeLocation->getPositionZ() << ", Y:" << homeLocation->getPositionY() << endl;
							}

							if (objectAgent->isCreature()) {

								ManagedReference<Creature*> critter = cast<Creature*>(objectAgent.get());

								if (critter != nullptr) {
									msg << "DNA Sample count: " << critter->getDnaSampleCount() << "  DNA State: " << critter->getDnaState() << "  Milk State: " << critter->getMilkState() << endl;
								}

							}

							msg << "Inventory" << endl;
							ManagedReference<SceneObject*> inventory = objectAgent->getSlottedObject("inventory");
							if (inventory != nullptr) {
													
								if (inventory->getContainerObjectsSize() > 0) {
									for (int i = 0; i < inventory->getContainerObjectsSize(); ++i) {
										ManagedReference<SceneObject*> sceno = inventory->getContainerObject(i);
										if (sceno != nullptr) {
											msg << "  Item #" << i+1 << ": " << sceno->getDisplayedName() << " - " << sceno->getObjectTemplate()->getTemplateFileName() << " - OID: " << sceno->getObjectID()  << endl;
										}
									}
								}
								else {
									msg << "  Inventory is empty" << endl;
								}
							}
							else {
								msg << "  Inventory is nullptr" << endl;
							}
						}
					}

					if (creoObject->isGrouped()) {
						GroupObject* group = creoObject->getGroup();

						if (group != nullptr)
							msg << "Group Level: " << group->getGroupLevel() << endl;
					}

					SortedVector<ManagedReference<ActiveArea*> >* areas = creoObject->getActiveAreas();

					if (areas != nullptr && areas->size() > 0) {

						msg << "Active Areas" << endl;

						for (int i = 0; i < areas->size(); i++) {
							ActiveArea* area = areas->get(i);

							if (area == nullptr)
								continue;

							msg << "  Area #" << i+1 << " -- " << area->getAreaName() << endl;
						}
					}
				}
			}

			if (object->isWeaponObject()) {

				ManagedReference<WeaponObject*> weapon = cast<WeaponObject*>(object.get());
				if (weapon != nullptr) {
					msg << "Weapon Min Damage: " << weapon->getMinDamage() << endl;
					msg << "Weapon Max Damage: " << weapon->getMaxDamage() << endl;
					msg << "Weapon Speed: " << weapon->getAttackSpeed() << endl;
					msg << "Force Cost: " << weapon->getForceCost() << endl;
				}
			}

			if (object->getZone() != nullptr) {
				msg << "Location: " << object->getPosition().toString() << " " << object->getZone()->getZoneName() << endl;
				msg << "Direction Angle - Degrees: " << object->getDirectionAngle() << endl;
			}

			ManagedReference<CityRegion*> city = object->getCityRegion().get();

			if (city != nullptr)
				msg << "City Region oid: " << String::valueOf(city->getObjectID()) << ", name: " << city->getRegionDisplayedName() << endl;

			creature->sendSystemMessage(msg.toString());

			ChatManager* chatManager = server->getZoneServer()->getChatManager();
			String title = "getObjVars - " + String::valueOf(objectID);
			chatManager->sendMail("System", title , msg.toString(), creature->getFirstName());

			ManagedReference<SuiMessageBox*> box = new SuiMessageBox(creature, SuiWindowType::NONE);

			if (box != nullptr) {
				StringBuffer titleStr;
				titleStr << "GetObjVars: " << objectID;

				box->setPromptTitle(titleStr.toString());
				box->setPromptText(msg.toString());

				ghost->addSuiBox(box);
				creature->sendMessage(box->generateMessage());
			}
		}

		return SUCCESS;
	}
};

#endif //GETOBJVARSCOMMAND_H_
