-- A single vendor stocking the full food-and-drink ladder: 11 tiers of mana-regen
-- drink, 3 health-and-mana rations, and 9 tiers of health-regen food.
--
-- Casters otherwise have to re-find a drink vendor every ten levels, and the good
-- ones are scattered across capitals and zones the party may not be in. This is
-- summoned from the premium item's gossip menu (see src/premium.cpp,
-- RefreshmentVendor).
--
-- Two entries, one per faction, following the same entryA/entryH pattern the module
-- already uses for its profession trainers, trade-goods vendors and ammo vendor.
-- SummonTempNPC overrides the summoned creature's faction to the player's, so
-- hostility is never the issue -- but the model is fixed by the creature entry, so a
-- single shared entry means Alliance players summon a Horde-race NPC. The models are
-- taken from real food/drink vendors of each faction: Myra Tyrngaarde, the Ironforge
-- bread vendor, and Larhka, the Orgrimmar beverage merchant.
--
-- WHAT COUNTS AS "MANA FOOD" HERE WAS READ OUT OF Spell.dbc, NOT ASSUMED. Every item
-- below was checked by following its use spell to the auras it actually applies:
-- aura 84 (SPELL_AURA_MOD_REGEN) is health, aura 85 (SPELL_AURA_MOD_POWER_REGEN) is
-- mana. That check moved two obvious-looking candidates off the list:
--
--   Dried Fruit Rations (24009) and Skyguard Rations (32721) look like mana rations
--   and are not. Both apply aura 84 plus aura 29 (MOD_STAT) -- they are ordinary
--   food carrying a stat buff, with no mana component at all. The three genuine
--   health-and-mana items trigger a MOD_REGEN spell and a MOD_POWER_REGEN spell as a
--   pair, which is what "restores health and mana" is actually built out of.
--
-- Alterac Manna Biscuit (19301) is also deliberately absent. Unlike everything below,
-- ALL four of its vendors are gated behind Alterac Valley reputation, so stocking it
-- here would be more permissive than the stock game rather than merely more
-- convenient.
--
-- No conditions are attached, for the same reason the ammo vendor attaches none:
-- mod-individual-progression gates these items on *specific expansion-zone vendors*,
-- not on the items themselves. Every item below was verified to have ungated vendor
-- sources elsewhere on the map -- the most gated of them, Smoked Talbuk Venison, is
-- still sold without any condition by 45 of its 54 vendors. The items keep their own
-- RequiredLevel, which is what actually stops a level 20 drinking Honeymint Tea.
--
-- Everything is priced in gold (ExtendedCost 0). The one item that normally carries an
-- extended cost is Star's Lament, at ExtendedCost 460 from the Sha'tar; that is dropped
-- here rather than reproduced, exactly as the ammo vendor drops the honor cost on Ice
-- Threaded Arrow. It is the only vendor-sold drink of its tier, so the alternative was
-- a hole in the ladder between levels 45 and 60.
--
-- Neither creature is ever spawned in the world -- they exist only to be summoned.

SET @REFRESH_VENDOR_A := 900019;
SET @REFRESH_VENDOR_H := 900020;

DELETE FROM `creature_template` WHERE `entry` IN (@REFRESH_VENDOR_A, @REFRESH_VENDOR_H);
INSERT INTO `creature_template`
    (`entry`, `name`, `subname`, `gossip_menu_id`, `minlevel`, `maxlevel`, `faction`,
     `npcflag`, `unit_class`, `unit_flags`, `type`, `AIName`, `MovementType`,
     `RegenHealth`, `flags_extra`, `ScriptName`) VALUES
    -- npcflag 128 = UNIT_NPC_FLAG_VENDOR. No UNIT_NPC_FLAG_VENDOR_AMMO here: that
    -- second flag is what puts the ammo tab on the ammo vendor, and food is a plain
    -- vendor inventory.
    -- faction 35 is friendly-to-all; SummonTempNPC replaces it with the player's
    -- anyway, so this only matters if that ever stops happening.
    -- flags_extra 2 = CREATURE_FLAG_EXTRA_CIVILIAN.
    (@REFRESH_VENDOR_A, 'Refreshment Provisioner', 'Food & Drink', 0, 80, 80, 35,
     128, 1, 512, 7, '', 0, 1, 2, ''),
    (@REFRESH_VENDOR_H, 'Refreshment Provisioner', 'Food & Drink', 0, 80, 80, 35,
     128, 1, 512, 7, '', 0, 1, 2, '');

DELETE FROM `creature_template_model` WHERE `CreatureID` IN (@REFRESH_VENDOR_A, @REFRESH_VENDOR_H);
INSERT INTO `creature_template_model`
    (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`) VALUES
    (@REFRESH_VENDOR_A, 0, 3050, 1, 1), -- human female, as used by Myra Tyrngaarde (Ironforge Bread Vendor)
    (@REFRESH_VENDOR_H, 0, 4525, 1, 1); -- troll female, as used by Larhka (Orgrimmar Beverage Merchant)

DELETE FROM `npc_vendor` WHERE `entry` IN (@REFRESH_VENDOR_A, @REFRESH_VENDOR_H);
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`) VALUES
    -- Drinks -- restore MANA. One per drink rank, level 1 to 75.
    (@REFRESH_VENDOR_A,  1,   159, 0, 0, 0), -- Refreshing Spring Water  (req 1)
    (@REFRESH_VENDOR_A,  2,  1179, 0, 0, 0), -- Ice Cold Milk            (req 5)
    (@REFRESH_VENDOR_A,  3,  1205, 0, 0, 0), -- Melon Juice              (req 15)
    (@REFRESH_VENDOR_A,  4,  1708, 0, 0, 0), -- Sweet Nectar             (req 25)
    (@REFRESH_VENDOR_A,  5,  1645, 0, 0, 0), -- Moonberry Juice          (req 35)
    (@REFRESH_VENDOR_A,  6,  8766, 0, 0, 0), -- Morning Glory Dew        (req 45)
    (@REFRESH_VENDOR_A,  7, 32455, 0, 0, 0), -- Star's Lament            (req 55)
    (@REFRESH_VENDOR_A,  8, 28399, 0, 0, 0), -- Filtered Draenic Water   (req 60)
    (@REFRESH_VENDOR_A,  9, 27860, 0, 0, 0), -- Purified Draenic Water   (req 65)
    (@REFRESH_VENDOR_A, 10, 33444, 0, 0, 0), -- Pungent Seal Whey        (req 70)
    (@REFRESH_VENDOR_A, 11, 33445, 0, 0, 0), -- Honeymint Tea            (req 75)
    -- Rations -- restore health AND mana (verified: each triggers a MOD_REGEN spell
    -- and a MOD_POWER_REGEN spell). This is the "mana food" proper.
    (@REFRESH_VENDOR_A, 12, 13724, 0, 0, 0), -- Enriched Manna Biscuit   (req 45)
    (@REFRESH_VENDOR_A, 13, 32722, 0, 0, 0), -- Enriched Terocone Juice  (req 65)
    (@REFRESH_VENDOR_A, 14, 34780, 0, 0, 0), -- Naaru Ration             (req 65)
    -- Food -- restores HEALTH. One per food rank, level 1 to 75.
    (@REFRESH_VENDOR_A, 15,  4540, 0, 0, 0), -- Tough Hunk of Bread      (req 1)
    (@REFRESH_VENDOR_A, 16,  2287, 0, 0, 0), -- Haunch of Meat           (req 5)
    (@REFRESH_VENDOR_A, 17,  3770, 0, 0, 0), -- Mutton Chop              (req 15)
    (@REFRESH_VENDOR_A, 18,  3771, 0, 0, 0), -- Wild Hog Shank           (req 25)
    (@REFRESH_VENDOR_A, 19,  4599, 0, 0, 0), -- Cured Ham Steak          (req 35)
    (@REFRESH_VENDOR_A, 20,  8952, 0, 0, 0), -- Roasted Quail            (req 45)
    (@REFRESH_VENDOR_A, 21, 27854, 0, 0, 0), -- Smoked Talbuk Venison    (req 55)
    (@REFRESH_VENDOR_A, 22, 33449, 0, 0, 0), -- Crusty Flatbread         (req 65)
    (@REFRESH_VENDOR_A, 23, 35950, 0, 0, 0); -- Sweet Potato Bread       (req 75)

-- The Horde vendor carries exactly the same stock. Mirrored from the rows above
-- rather than repeated, so the two can never drift apart.
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`)
SELECT @REFRESH_VENDOR_H, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`
FROM `npc_vendor` WHERE `entry` = @REFRESH_VENDOR_A;
