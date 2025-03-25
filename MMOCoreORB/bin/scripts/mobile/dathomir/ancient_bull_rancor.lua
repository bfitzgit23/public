ancient_bull_rancor = Creature:new {
	objectName = "@mob/creature_names:ancient_bull_rancor",
	socialGroup = "rancor",
	faction = "",
	mobType = MOB_CARNIVORE,
	level = 140,
	chanceHit = 2.95,
	damageMin = 800,
	damageMax = 1400,
	baseXp = 9336,
	baseHAM = 120000,
	baseHAMmax = 125000,
	armor = 1,
	resists = {185,185,185,195,180,185,185,190,135},
	meatType = "meat_carnivore",
	meatAmount = 1000,
	hideType = "hide_leathery",
	hideAmount = 1000,
	boneType = "bone_mammal",
	boneAmount = 950,
	milk = 0,
	tamingChance = 0,
	ferocity = 20,
	pvpBitmask = AGGRESSIVE + ATTACKABLE + ENEMY,
	creatureBitmask = PACK + KILLER,
	optionsBitmask = AIENABLED,
	diet = CARNIVORE,

	templates = {"object/mobile/bull_rancor.iff"},
	hues = { 16, 17, 18, 19, 20, 21, 22, 23 },
	scale = 1.25,
	lootGroups = {
		{
			groups = {
				{group = "rancor_common",	chance = 40 * (100000)},
				{group = "armor_all",		chance = 20 * (100000)},
				{group = "weapons_all",		chance = 25 * (100000)},
				{group = "wearables_all",	chance = 15 * (100000)}
			},
			lootChance = 30 * (100000)
		},
		{
			groups = {
				{group = "captain_ring_group", chance = 10000000},
			},
			lootChance = .25 * (100000)
		}
	},

	-- Primary and secondary weapon should be different types (rifle/carbine, carbine/pistol, rifle/unarmed, etc)
	-- Unarmed should be put on secondary unless the mobile doesn't use weapons, in which case "unarmed" should be put primary and "none" as secondary
	primaryWeapon = "unarmed",
	secondaryWeapon = "none",
	conversationTemplate = "",
	
	-- primaryAttacks and secondaryAttacks should be separate skill groups specific to the weapon type listed in primaryWeapon and secondaryWeapon
	-- Use merge() to merge groups in creatureskills.lua together. If a weapon is set to "none", set the attacks variable to empty brackets
	primaryAttacks = { {"creatureareableeding",""}, {"creatureareacombo",""} },
	secondaryAttacks = { }
}

CreatureTemplates:addCreatureTemplate(ancient_bull_rancor, "ancient_bull_rancor")
