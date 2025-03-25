/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions. */

#ifndef GMREVIVECOMMAND_H_
#define GMREVIVECOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"

class GmReviveCommand : public QueueCommand {
public:

	GmReviveCommand(const String& name, ZoneProcessServer* server) : QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		try {
			StringTokenizer args(arguments.toString());
			ManagedReference<CreatureObject*> patient = nullptr;

			auto zoneServer = server->getZoneServer();
			if (zoneServer == nullptr)
				return GENERALERROR;

			auto playerManager = zoneServer->getPlayerManager();
			if (playerManager == nullptr)
				return GENERALERROR;

			ManagedReference<SceneObject*> object = zoneServer->getObject(creature->getTargetID());

			if (!args.hasMoreTokens()) { // No arguments passed

				if (object != nullptr && (object->isPlayerCreature() || object->isPet())) { // Target is a player or pet, rez target
					patient = cast<CreatureObject*>( object.get());
					revivePatient(creature, patient);

				} else if (object == nullptr) { // No target, rez self
					patient = creature;
					revivePatient(creature, patient);

				} else { // Target is not a player or pet
					creature->sendSystemMessage("Syntax: /gmrevive [buff | removeskillmod | resetmodifiers] <amount> [ [<name>] | [area [<range>] [imperial | rebel | neutral]] ]");
					return INVALIDTARGET;
				}

			}
			else { // Has arguments

				String firstArg;
				String firstName = "";
				String modName = "";
				String buffToken;
				int buffAmount = 0;
				bool buff = false;
				bool removeSkillmod = false;
				bool resetModifiers = false;
				args.getStringToken(firstArg);

				if (firstArg.toLowerCase() == "buff") { // First argument is buff, get second argument
					buff = true;
					if (args.hasMoreTokens()) {
						args.getStringToken(buffToken);
						if (Character::isDigit(buffToken.charAt(0))) {        // If the argument is an int, it's buff amount
								for (int i = 0; i < buffToken.length(); i++)
									if (!Character::isDigit(buffToken.charAt(i)))
										throw Exception();
					
								buffAmount = Integer::valueOf(buffToken);
								if (args.hasMoreTokens())
									args.getStringToken(firstName);
						}
						else {
								firstName = buffToken;
						}
					}				
				}
				else if (firstArg.toLowerCase() == "removeskillmod") {
						removeSkillmod = true;
						if (args.hasMoreTokens())
							args.getStringToken(modName);
						else
							return GENERALERROR;
				}
				else if (firstArg.toLowerCase() == "resetmodifiers") {
						resetModifiers = true;
				}
				else  // First argument is not buff, must be a name or area
					firstName = firstArg;

				if (firstName != "") { // There's an argument for a name or area

					if (firstName.toLowerCase() == "area") { // Area argument found, check for range argument
						int range = 32;
						String faction = "";

						if (args.hasMoreTokens()) { // Can be range or faction
							String token;
							args.getStringToken(token);

							if (Character::isDigit(token.charAt(0))) { // If the argument is an int, it's range
								for (int i = 0; i < token.length(); i++) {
									if (!Character::isDigit(token.charAt(i)))
										throw Exception();
								}
								range = Integer::valueOf(token);

							} else { // Otherwise it's faction
								faction = token;
							}
						}

						if (range > 50) // We don't want the range to get crazy, so hard caps of 5-50
							range = 50;
						else if (range < 5)
							range = 5;

						if (faction == "" && args.hasMoreTokens()) { // Last argument must be faction
							args.getStringToken(faction);
						}

						SortedVector<QuadTreeEntry*> closeObjects;
						Zone* zone = creature->getZone();

						if (creature->getCloseObjects() == nullptr) {
#ifdef COV_DEBUG
							creature->info("Null closeobjects vector in GmReviveCommand::doQueueCommand", true);
#endif
							zone->getInRangeObjects(creature->getPositionX(), creature->getPositionY(), range, &closeObjects, true);
						}
						else {
							CloseObjectsVector* closeVector = (CloseObjectsVector*) creature->getCloseObjects();
							closeVector->safeCopyTo(closeObjects);
						}

						for (int i = 0; i < closeObjects.size(); ++i) {
							SceneObject* sceneObject = static_cast<SceneObject*>(closeObjects.get(i));

							if ((sceneObject->isPlayerCreature() || sceneObject->isPet()) && creature->isInRange(sceneObject, range)) {
								ManagedReference<CreatureObject*> patientObject = cast<CreatureObject*>(sceneObject);

								if (patientObject != nullptr) {
									if (faction == "" || faction.hashCode() == patientObject->getFaction() || (faction == "neutral" && patientObject->getFaction() == 0)) {
										if (buff) {
											Locker clocker(patientObject, creature);

											if (patientObject->isPlayerCreature()) {
												if (buffAmount > 0)
													playerManager->gmenhanceCharacter(patientObject,buffAmount);
												else
													playerManager->enhanceCharacter(patientObject);
												creature->sendSystemMessage(patientObject->getFirstName() + " has been enhanced.");
											} else if (patientObject->isCreature()) {
												if (buffAmount > 0)
													playerManager->gmenhanceCharacter(patientObject,buffAmount);
												else
													playerManager->enhanceCharacter(patientObject);
												creature->sendSystemMessage(patientObject->getDisplayedName() + " has been enhanced.");
											}

										} else {
											revivePatient(creature, patientObject);
										}
									}
								}
							}
						}

					}
					else { // Not area
						patient = server->getZoneServer()->getChatManager()->getPlayer(firstName);

						if (patient != nullptr) {
							if (buff) {
								Locker clocker(patient, creature);
								if (buffAmount > 0)
									playerManager->gmenhanceCharacter(patient,buffAmount);
								else
									playerManager->enhanceCharacter(patient);
								creature->sendSystemMessage(patient->getFirstName() + " has been enhanced.");

							} else {
								revivePatient(creature, patient);
							}
						}
					}
				}
				else if (buff) {  // Buff was the only argument

					if (object != nullptr && (object->isPlayerCreature() || object->isPet())) { // Target is a player or pet, buff target
						patient = cast<CreatureObject*>( object.get());
						Locker clocker(patient, creature);

						if (patient->isPlayerCreature()) {
							if (buffAmount > 0)
								playerManager->gmenhanceCharacter(patient,buffAmount);
							else
								playerManager->enhanceCharacter(patient);
							creature->sendSystemMessage(patient->getFirstName() + " has been enhanced.");
						}
						else if (patient->isCreature()) {
							if (buffAmount > 0)
								playerManager->gmenhanceCharacter(patient, buffAmount);
							else
								playerManager->enhanceCharacter(patient);
							creature->sendSystemMessage(patient->getDisplayedName() + " has been enhanced.");
						}

					} else if (object == nullptr) { // No target, buff self
						if (buffAmount > 0)
							playerManager->gmenhanceCharacter(creature,buffAmount);
						else
							playerManager->enhanceCharacter(creature);

					}
					else { // Target is not a player or pet
						creature->sendSystemMessage("Invalid target for /gmrevive buff command.");
						return INVALIDTARGET;
					}

				}
				else if (removeSkillmod) {
					if (object != nullptr && object->isPlayerCreature()) {
						patient = cast<CreatureObject*>(object.get());
						Locker clocker(patient, creature);
						patient->removeSkillMod(SkillModManager::BUFF, modName, patient->getSkillMod(modName), true);
					}
					else {
						creature->sendSystemMessage("Invalid target for GMRevive removeskillmod command.");
						return INVALIDTARGET;
					}
				}
				else if (resetModifiers) {
					if (object == nullptr) {  //Infinity:  Allow this for all creature objects, not just players
						creature->sendSystemMessage("Invalid target for /gmrevive resetmodifiers command.");
						return INVALIDTARGET;
					}

					patient = cast<CreatureObject*>(object.get());

					if (patient == nullptr) {
						creature->sendSystemMessage("Invalid target for for /gmrevive resetmodifiers command.");
						return GENERALERROR;
					}

					Locker clocker(patient, creature);
					patient->clearBuffs(true, true);

					for (int i = 0; i < 9; i++) {
						int base = patient->getBaseHAM(i);
						patient->setMaxHAM(i, base, true);
					}

					StringBuffer msg;
					msg << "GMRevive modifiers: Creature object buffs removed and HAM values reset to base values for: " << patient->getDisplayedName();
					creature->sendSystemMessage(msg.toString());
				}
				else { // Shouldn't ever end up here
					creature->sendSystemMessage("Syntax: /gmrevive [buff | removeskillmod | resetmodifiers] <amount> [ [<name>] | [area [<range>] [imperial | rebel | neutral]] ]");
					return INVALIDTARGET;
				}
			}

		} catch (Exception& e) {
			creature->sendSystemMessage("Syntax: /gmrevive [buff | removeskillmod | resetmodifiers] <amount> [ [<name>] | [area [<range>] [imperial | rebel | neutral]] ]");
		}

		return SUCCESS;
	}

	void revivePatient(CreatureObject* creature, CreatureObject* patient) const {
		Locker clocker(patient, creature);

		ManagedReference<PlayerObject*> targetGhost = patient->getPlayerObject();

		if (targetGhost != nullptr) {

			if(targetGhost->getJediState() > 1)
				targetGhost->setForcePower(targetGhost->getForcePowerMax());

			if(patient->isDead())
				targetGhost->removeSuiBoxType(SuiWindowType::CLONE_REQUEST);
		}

		patient->clearDots();

		if (patient->isDizzied())
				patient->removeStateBuff(CreatureState::DIZZY);
		if (patient->isStunned())
				patient->removeStateBuff(CreatureState::STUNNED);
		if (patient->isIntimidated())
				patient->removeStateBuff(CreatureState::INTIMIDATED);
		if (patient->isBlinded())
				patient->removeStateBuff(CreatureState::BLINDED);

		for (int i = 0; i < 9; ++i) {
			patient->setWounds(i, 0);
		}
		patient->setShockWounds(0);

		//Infinity:  Increase for big pets and weird stuff
		patient->healDamage(creature, CreatureAttribute::HEALTH, 500000);
		patient->healDamage(creature, CreatureAttribute::ACTION, 500000);
		patient->healDamage(creature, CreatureAttribute::MIND, 500000);

		patient->removeFeignedDeath();

		patient->setPosture(CreaturePosture::UPRIGHT);

		patient->notifyObservers(ObserverEventType::CREATUREREVIVED, creature, 0);

		patient->broadcastPvpStatusBitmask();

		if (patient->isPlayerCreature()) {
			patient->sendSystemMessage("You have been restored.");
			creature->sendSystemMessage(patient->getFirstName() + " has been restored.");
		}
		else {
			creature->sendSystemMessage(patient->getDisplayedName() + " has been restored.");
		}
	}
};

#endif //GMREVIVECOMMAND_H_
