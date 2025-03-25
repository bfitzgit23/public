/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef CREDITSCOMMAND_H_
#define CREDITSCOMMAND_H_

#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/objects/transaction/TransactionLog.h"

class CreditsCommand : public QueueCommand {
public:

	CreditsCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		try {

			ManagedReference<SceneObject* > object = server->getZoneServer()->getObject(target);

			ManagedReference<CreatureObject*> player = nullptr;

			StringTokenizer args(arguments.toString());

			if (object == nullptr || !object->isPlayerCreature()) {

				String firstName;
				if(args.hasMoreTokens()) {
					args.getStringToken(firstName);
					player = server->getZoneServer()->getPlayerManager()->getPlayer(firstName);
				}

			} else {
				player = cast<CreatureObject*>(object.get());
			}

			if (player == nullptr) {
				creature->sendSystemMessage("invalid arguments for credits command:  /credits <firstname> <add/subtract> <amount> <bank/cash>");
				return GENERALERROR;
			}

			auto ghost = creature->getPlayerObject();
			if (ghost == nullptr) {
				creature->sendSystemMessage("Could not load player object.");
				return GENERALERROR;
			}

			Locker clocker(player, creature);

			String action;
			args.getStringToken(action);
			action = action.toLowerCase();

			int amount;
			amount = args.getIntToken();

			String location;
			args.getStringToken(location);
			location = location.toLowerCase();

			bool success = false;
			bool cleared = false;
			bool maxxed = false;

			if ((amount <= 0) | (amount > 2000000000)) {
				creature->sendSystemMessage("Amount must be a positive integer less than or equal to 2 billion.");
				return GENERALERROR;
			}

			if (action == "add") {

				if (!ghost->isAdmin()) {
					creature->sendSystemMessage("Only admin level 15 players can use the /credits add option.");
					return GENERALERROR;
				}

				if (location.toLowerCase() == "cash") {
					uint64 currentCashCredits = player->getCashCredits();

					if (currentCashCredits + amount > 2000000000) {
						maxxed = true;
						int delta = 2000000000 - currentCashCredits;
						if (delta > 0) {
							TransactionLog trx(TrxCode::CUSTOMERSERVICE, player, delta, true);
							player->addCashCredits(delta);
						}
					} else {
						TransactionLog trx(TrxCode::CUSTOMERSERVICE, player, amount, true);
						player->addCashCredits(amount);
					}
					success = true;
				}

				if (location.toLowerCase() == "bank") {
					uint64 currentBankCredits = player->getBankCredits();

					if (currentBankCredits + amount > 2000000000) {
						maxxed = true;
						int delta = 2000000000 - currentBankCredits;
						if (delta > 0) {
							TransactionLog trx(TrxCode::CUSTOMERSERVICE, player, delta, true);
							player->addBankCredits(delta);
						}
					} else {
						TransactionLog trx(TrxCode::CUSTOMERSERVICE, player, amount, true);
						player->addBankCredits(amount);
					}
					success = true;
				}

			} else if (action == "subtract") {

				if (location.toLowerCase() == "cash") {
					if (player->verifyCashCredits(amount)) {
						TransactionLog trx(player, TrxCode::CUSTOMERSERVICE, amount, true);
						player->subtractCashCredits(amount);
					} else {
						TransactionLog trx(player, TrxCode::CUSTOMERSERVICE, player->getCashCredits(), true);
						player->clearCashCredits();
						cleared = true;
					}

					success = true;
				}

				if (location == "bank") {
					if (player->verifyBankCredits(amount)) {
						TransactionLog trx(player, TrxCode::CUSTOMERSERVICE, amount, false);
						player->subtractBankCredits(amount);
					} else {
						TransactionLog trx(player, TrxCode::CUSTOMERSERVICE, player->getBankCredits(), false);
						player->clearBankCredits();
						cleared = true;
					}

					success = true;
				}
			}

			if (success) {
				if (cleared) {
					creature->sendSystemMessage("All " + location + " credits have been subtracted successfully for " + player->getFirstName() + ".");
				}
				else if (maxxed) {
					location[0] = toupper(location[0]);
					creature->sendSystemMessage(location + " credits have been maximized successfully for " + player->getFirstName() + ".");	
				}
				else {
					creature->sendSystemMessage(String::withCommas(amount) + " " + location + " credits have been " + action + "ed successfully for " + player->getFirstName() + ".");
				}
			}
			else {
				creature->sendSystemMessage("invalid arguments for credits command:  /credits <firstname> <add/subtract> <amount> <bank/cash>");
			}

		} catch (Exception& e) {
			creature->sendSystemMessage("invalid arguments for credits command:  /credits <firstname> <add/subtract> <amount> <bank/cash>");
		}

		return SUCCESS;
	}

};

#endif //CREDITSCOMMAND_H_
