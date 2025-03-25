ForceShrineMenuComponent = {}

function ForceShrineMenuComponent:fillObjectMenuResponse(pSceneObject, pMenuResponse, pPlayer)
	local menuResponse = LuaObjectMenuResponse(pMenuResponse)
	if (CreatureObject(pPlayer):hasSkill("mandalorian_novice")) then
		util:cout(pPlayer, "Your Mandalorian status prevents you from using this shrine. You must decide your path, both is not an option.", "orange")
	else
		if (CreatureObject(pPlayer):hasSkill("force_title_jedi_novice")) then
			menuResponse:addRadialMenuItem(120, 3, "@jedi_trials:meditate") -- Meditate
		end
		if util:hasQuestState(pPlayer, "mand:dropJedi", 1) then
			menuResponse:addRadialMenuItem(122, 3, "I would like Jedi back, I dropped it.")
		end
		if (CreatureObject(pPlayer):hasSkill("force_title_jedi_rank_02")) then
			menuResponse:addRadialMenuItem(121, 3, "@force_rank:recover_jedi_items") -- Recover Jedi Items
		end
	end
end

function ForceShrineMenuComponent:handleObjectMenuSelect(pObject, pPlayer, selectedID)
	if (pPlayer == nil or pObject == nil) then
		return 0
	end
	
	if (selectedID == 120 and CreatureObject(pPlayer):hasSkill("force_title_jedi_novice")) then
		if util:isAdmin(pPlayer) then
			util:cw("[mgmt] do meditate", 1)
			self:doMeditate(pObject, pPlayer)
		else
			if (CreatureObject(pPlayer):getPosture() ~= CROUCHED) then
				CreatureObject(pPlayer):sendSystemMessage("@jedi_trials:show_respect") -- Must respect
			else
				util:cw("do meditate", 1)
				self:doMeditate(pObject, pPlayer)
			end
		end
		
	elseif (selectedID == 122 and util:hasQuestState(pPlayer, "mand:dropJedi", 1)) then
		local pGhost = CreatureObject(pPlayer):getPlayerObject()
		PlayerObject(pGhost):setJediState(2)
		util:giveProfession(pPlayer, "force_title_jedi_rank_01")
		util:giveProfession(pPlayer, "force_title_jedi_rank_02")
		util:removeQuestState(pPlayer, "mand:dropJedi", 1)
	elseif (selectedID == 121 and CreatureObject(pPlayer):hasSkill("force_title_jedi_rank_02")) then
		self:recoverRobe(pPlayer)
	end

	return 0
end

function ForceShrineMenuComponent:doMeditate(pObject, pPlayer)
	if (not CreatureObject(pPlayer):hasSkill("force_title_jedi_rank_02") and CreatureObject(pPlayer):hasScreenPlayState(32, "VillageJediProgression")) then
		local currentTrial = JediTrials:getCurrentTrial(pPlayer)
		util:cw("do meditate 1", 1)
		
		if (not JediTrials:isOnPadawanTrials(pPlayer)) then
			
			-- auto fix
			util:cw("do meditate 1a", 1)
			local showSui = true
			if CreatureObject(pPlayer):hasScreenPlayState(32, "VillageJediProgression") then
				local completedTrials = tonumber(readScreenPlayData(pPlayer, "PadawanTrials", "completedTrials"))
				if completedTrials ~= nil and completedTrials == 1 then
					local pGhost = CreatureObject(pPlayer):getPlayerObject()
					local state = PlayerObject(pGhost):getJediState()
					if state ~= nil and state >= 2 then
						local learnedBranches = VillageJediManagerCommon.getLearnedForceSensitiveBranches(pPlayer)
						util:cw("learnedBranches=" .. learnedBranches, 1)
						if learnedBranches ~= nil and learnedBranches >= NUMBEROFTREESTOMASTER then
							util:cw("at this point fix them", 1)
							if not CreatureObject(pPlayer):hasSkill("force_title_jedi_rank_01") then
								util:giveProfession(pPlayer, "force_title_jedi_rank_01")
							end
							if not CreatureObject(pPlayer):hasSkill("force_title_jedi_rank_02") then
								util:giveProfession(pPlayer, "force_title_jedi_rank_02")
							end
							util:cout(pPlayer, "It appears that you dropped Jedi and are back to pick it up again. This is now fixed and you can once again start gaining Jedi experience. Remember to use /findMyTrainer if you forgot where you Jedi Skill Trainer is located.", "pink")
							util:cout(pPlayer, "If you do not know why you are seeing the above message, please contact support.", "orange")
							showSui = false
							printLuaLog("Player fixed Jedi Status at shrine: " .. CreatureObject(pPlayer):getFirstName())
						else
							util:cw("does not have required qty of village trees", 1)
						end
					else
						-- not qualified
						util:cw("failed jedi state", 1)
					end
				else
					-- not qualified
					util:cw("failed to complete trials", 1)
				end
			else
				-- not qualified
				util:cw("failed VillageJediProgression", 1)
			end

			if showSui then
				util:cw("open sui", 1)
				PadawanTrials:startPadawanTrials(pObject, pPlayer)
			end
		elseif (currentTrial == 0) then
			PadawanTrials:startNextPadawanTrial(pObject, pPlayer)
			util:cw("do meditate 1b", 1)
		else
			PadawanTrials:showCurrentTrial(pObject, pPlayer)
			util:cw("do meditate 1c", 1)
		end
	elseif (JediTrials:isOnKnightTrials(pPlayer)) then
		util:cw("do meditate 2", 1)
		local pPlayerShrine = KnightTrials:getTrialShrine(pPlayer)

		if (pPlayerShrine ~= nil and pObject ~= pPlayerShrine) then
			local correctShrineZone = SceneObject(pPlayerShrine):getZoneName()
			if (correctShrineZone ~= SceneObject(pObject):getZoneName()) then
				local messageString = LuaStringIdChatParameter("@jedi_trials:knight_shrine_reminder")
				messageString:setTO(getStringId("@jedi_trials:" .. correctShrineZone))
				CreatureObject(pPlayer):sendSystemMessage(messageString:_getObject())
			else
				CreatureObject(pPlayer):sendSystemMessage("@jedi_trials:knight_shrine_wrong")
			end
			return
		end

		local currentTrial = JediTrials:getCurrentTrial(pPlayer)
		local trialsCompleted = JediTrials:getTrialsCompleted(pPlayer)

		if (currentTrial == 0 and trialsCompleted == 0) then
			KnightTrials:startNextKnightTrial(pPlayer)
		else
			KnightTrials:showCurrentTrial(pPlayer)
		end
	else
		util:cw("do meditate 3", 1)
		local pGhost = CreatureObject(pPlayer):getPlayerObject()
		local state = PlayerObject(pGhost):getJediState()
		util:cw("jedi state=" .. state, 1)
		local trialsCompleted = tonumber(readScreenPlayData(pPlayer, "KnightTrials", "trialsCompleted"))
		if trialsCompleted ~= nil and trialsCompleted >= 15 then
			if CreatureObject(pPlayer):hasSkill("force_rank_light_novice")
			or CreatureObject(pPlayer):hasSkill("force_rank_dark_novice") then
				util:cw("do nothing, they are knights", 1)
			else
				util:cw("they look like they want to be knight again", 1)
				if util:getCheck(pPlayer, 6) then
					util:cout(pPlayer, "You can only join an FRS faction once every 7 days.", "red")
				else
					if not CreatureObject(pPlayer):hasSkill("force_rank_light_novice") 
					and CreatureObject(pPlayer):getFaction() == FACTIONREBEL then
						util:cw("You were once a Rebel Knight", 1)
						ForceShrineMenuComponent:resetKnight(pPlayer)
					elseif not CreatureObject(pPlayer):hasSkill("force_rank_dark_novice") 
					and CreatureObject(pPlayer):getFaction() == FACTIONIMPERIAL then
						util:cw("You were once an Imperial Knight", 1)
						ForceShrineMenuComponent:resetKnight(pPlayer)
					end
				end
			end
		end
		CreatureObject(pPlayer):sendSystemMessage("@jedi_trials:force_shrine_wisdom_" .. getRandomNumber(1, 15))
	end
	util:cw("do meditate 4", 1)
end

function ForceShrineMenuComponent:resetKnight(pPlayer)
	if pPlayer ~= nil then
		local cooldown = 7 * 24 * 60 * 60
		if not util:isLive() then
			cooldown = 1 * 60
		end
		local pGhost = CreatureObject(pPlayer):getPlayerObject()
		local state = PlayerObject(pGhost):getJediState()
		util:cw("current jedi state=" .. state, 1)
		local changeVis = false
		if CreatureObject(pPlayer):getFaction() == FACTIONREBEL then
			if CreatureObject(pPlayer):villageKnightPrereqsMet("") then
				writeScreenPlayData(pPlayer, "JediTrials", "JediCouncil", 1)
				JediTrials:unlockJediKnight(pPlayer)
				util:cw("set cooldown", 1)
				util:setCheck(pPlayer, 6, cooldown)
				local state = PlayerObject(pGhost):getJediState()
				util:cw("current jedi state=" .. state, 1)
				changeVis = true
				util:cout(pPlayer, "You have rejoined the Force Ranking System as a Light Jedi Knight.  You can only re-join the FRS once every 7 days.", "green")
				printLuaLog("Player has rejoined the FRS as a Light Jedi at a shrine: " .. CreatureObject(pPlayer):getFirstName())
			else
				util:cout(pPlayer, "You do not meet the minimum requirements to return to Knight status.", "red")
			end
		elseif CreatureObject(pPlayer):getFaction() == FACTIONIMPERIAL then
			if CreatureObject(pPlayer):villageKnightPrereqsMet("") then
				writeScreenPlayData(pPlayer, "JediTrials", "JediCouncil", 2)
				JediTrials:unlockJediKnight(pPlayer)
				util:cw("set cooldown", 1)
				util:setCheck(pPlayer, 6, cooldown)
				local state = PlayerObject(pGhost):getJediState()
				util:cw("current jedi state=" .. state, 1)
				changeVis = true
				util:cout(pPlayer, "You have rejoined the Force Ranking System as a Dark Jedi Knight.  You can only re-join the FRS once every 7 days.", "green")
				printLuaLog("Player has rejoined the FRS as a Dark Jedi at a shrine: " .. CreatureObject(pPlayer):getFirstName())
			else
				util:cout(pPlayer, "You do not meet the minimum requirements to return to Knight status.", "red")
			end
		end
		if changeVis then 
			-- Remove Jedi Guardian and Jedi Master Boxes if they have them
			if CreatureObject(pPlayer):hasSkill("force_title_jedi_master") then
				CreatureObject(pPlayer):surrenderSkill("force_title_jedi_master")
			end
			if CreatureObject(pPlayer):hasSkill("force_title_jedi_rank_04") then
				CreatureObject(pPlayer):surrenderSkill("force_title_jedi_rank_04")
			end
			-- Reset FRS XP to Zero
			local frsXP = PlayerObject(pGhost):getExperience("force_rank_xp")
			if frsXP > 0 then
				CreatureObject(pPlayer):awardExperience("force_rank_xp", frsXP * -1, false) -- Don't use modifiers
			end
			local currentVis = PlayerObject(pGhost):getVisibility()
            if currentVis == nil then
                currentVis = 0
            end
            local totalVis = 2000 + currentVis
			if totalVis > 8000 then
				totalVis = 8000
			end
        	PlayerObject(pGhost):setVisibility(totalVis)
			CreatureObject(pPlayer):setFactionStatus(2)
		end
	end
end
function ForceShrineMenuComponent:recoverRobe(pPlayer)
	local pInventory = SceneObject(pPlayer):getSlottedObject("inventory")

	if (pInventory == nil) then
		return
	end

	if (SceneObject(pInventory):isContainerFullRecursive()) then
		CreatureObject(pPlayer):sendSystemMessage("@jedi_spam:inventory_full_jedi_robe")
		return
	end

	local robeTemplate
	if (CreatureObject(pPlayer):hasSkill("force_title_jedi_rank_03")) then
		local councilType = JediTrials:getJediCouncil(pPlayer)

		if (councilType == JediTrials.COUNCIL_LIGHT) then
			robeTemplate = "object/tangible/wearables/robe/robe_jedi_light_s01.iff"
		else
			robeTemplate = "object/tangible/wearables/robe/robe_jedi_dark_s01.iff"
		end
	else
		robeTemplate = "object/tangible/wearables/robe/robe_jedi_padawan.iff"
	end

	giveItem(pInventory, robeTemplate, -1)
	CreatureObject(pPlayer):sendSystemMessage("@force_rank:items_recovered")
end
