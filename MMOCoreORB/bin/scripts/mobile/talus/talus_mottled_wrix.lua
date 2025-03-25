talus_mottled_wrix = Creature:new {
	objectName = "@mob/creature_names:mottled_wrix",
	socialGroup = "wrix",
	faction = "",
	mobType = MOB_CARNIVORE,
	level = 84,
	chanceHit = 0.62,
	damageMin = 978,
	damageMax = 1725,
	baseXp = 6866,
	baseHAM = 6316,
	baseHAMmax = 7821,
	armor = 1,
	resists = {55,155,40,60,20,20,50,40,-1},
	meatType = "meat_carnivore",
	meatAmount = 123,
	hideType = "hide_bristley",
	hideAmount = 123,
	boneType = "bone_mammal",
	boneAmount = 123,
	milk = 2 * 0,
	tamingChance = 0.10,
	ferocity = 4,
	pvpBitmask = AGGRESSIVE + ATTACKABLE + ENEMY,
	creatureBitmask = PACK + HERD + KILLER,
	optionsBitmask = AIENABLED,
	diet = CARNIVORE,

	templates = {"object/mobile/wrix.iff"},
	controlDeviceTemplate = "object/intangible/pet/mottled_wrix_hue.iff",
	scale = 1.05,
	-- Primary and secondary weapon should be different types (rifle/carbine, carbine/pistol, rifle/unarmed, etc)
	-- Unarmed should be put on secondary unless the mobile doesn't use weapons, in which case "unarmed" should be put primary and "none" as secondary
	primaryWeapon = "unarmed",
	secondaryWeapon = "none",
	conversationTemplate = "",
	
	-- primaryAttacks and secondaryAttacks should be separate skill groups specific to the weapon type listed in primaryWeapon and secondaryWeapon
	-- Use merge() to merge groups in creatureskills.lua together. If a weapon is set to "none", set the attacks variable to empty brackets
	primaryAttacks = {{"stunattack",""},{"strongdisease",""}},
	secondaryAttacks = { }

}

CreatureTemplates:addCreatureTemplate(talus_mottled_wrix, "talus_mottled_wrix")
