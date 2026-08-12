-- A single vendor stocking every arrow and bullet that is sold by a vendor anywhere
-- in the game: 12 arrows and 12 bullets, from Rough Arrow up to Terrorshaft Arrow.
--
-- Hunters otherwise have to keep finding an ammo vendor as they outgrow each tier,
-- and the good ones are scattered across zones the player may not be in. This is
-- summoned from the premium item's gossip menu (see src/premium.cpp, AmmoVendor).
--
-- No conditions are attached, and that is deliberate rather than an oversight.
-- mod-individual-progression gates ammo on *specific expansion-zone vendors*, not on
-- the items: every one of these 24 items has ungated sources elsewhere on the map --
-- even Terrorshaft Arrow, which 56 of its 58 vendors sell with no progression check.
-- Mirroring those conditions here would make this vendor more restrictive than the
-- stock game for no gain. The items keep their own RequiredLevel, which is what
-- actually stops a level 20 using WotLK ammo.
--
-- Everything is priced in gold (ExtendedCost 0). The only items that normally carry
-- an extended cost are Ice Threaded Arrow/Bullet, at 1 honor point from the Alterac
-- Valley quartermasters; that is dropped here rather than reproduced.
--
-- The creature is never spawned in the world -- it exists only to be summoned by
-- SummonTempNPC, which overrides its faction to the player's, so the goblin model is
-- chosen to read as neutral to both sides.

SET @AMMO_VENDOR := 900017;

DELETE FROM `creature_template` WHERE `entry` = @AMMO_VENDOR;
INSERT INTO `creature_template`
    (`entry`, `name`, `subname`, `gossip_menu_id`, `minlevel`, `maxlevel`, `faction`,
     `npcflag`, `unit_class`, `unit_flags`, `type`, `AIName`, `MovementType`,
     `RegenHealth`, `flags_extra`, `ScriptName`) VALUES
    -- npcflag 384 = UNIT_NPC_FLAG_VENDOR (128) | UNIT_NPC_FLAG_VENDOR_AMMO (256), the
    -- same combination Mazk Snipeshot and the other ammo vendors use.
    -- flags_extra 2 = CREATURE_FLAG_EXTRA_CIVILIAN.
    (@AMMO_VENDOR, 'Munitions Quartermaster', 'Arrows & Bullets', 0, 80, 80, 35,
     384, 1, 512, 7, '', 0, 1, 2, '');

DELETE FROM `creature_template_model` WHERE `CreatureID` = @AMMO_VENDOR;
INSERT INTO `creature_template_model`
    (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`) VALUES
    (@AMMO_VENDOR, 0, 4294, 1, 1); -- goblin merchant, as used by Rarck

DELETE FROM `npc_vendor` WHERE `entry` = @AMMO_VENDOR;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`) VALUES
    -- Arrows, in ammo-tier order
    (@AMMO_VENDOR,  1,  2512, 0, 0, 0), -- Rough Arrow          (req 1)
    (@AMMO_VENDOR,  2,  2515, 0, 0, 0), -- Sharp Arrow          (req 10)
    (@AMMO_VENDOR,  3,  3030, 0, 0, 0), -- Razor Arrow          (req 25)
    (@AMMO_VENDOR,  4, 11285, 0, 0, 0), -- Jagged Arrow         (req 40)
    (@AMMO_VENDOR,  5, 19316, 0, 0, 0), -- Ice Threaded Arrow   (req 51)
    (@AMMO_VENDOR,  6, 24417, 0, 0, 0), -- Scout's Arrow        (req 61)
    (@AMMO_VENDOR,  7, 28056, 0, 0, 0), -- Blackflight Arrow    (req 65)
    (@AMMO_VENDOR,  8, 30611, 0, 0, 0), -- Halaani Razorshaft   (req 66)
    (@AMMO_VENDOR,  9, 31949, 0, 0, 0), -- Warden's Arrow       (req 68)
    (@AMMO_VENDOR, 10, 31737, 0, 0, 0), -- Timeless Arrow       (req 70)
    (@AMMO_VENDOR, 11, 34581, 0, 0, 0), -- Mysterious Arrow     (req 70)
    (@AMMO_VENDOR, 12, 41586, 0, 0, 0), -- Terrorshaft Arrow    (req 75)
    -- Bullets, same order
    (@AMMO_VENDOR, 13,  2516, 0, 0, 0), -- Light Shot           (req 1)
    (@AMMO_VENDOR, 14,  2519, 0, 0, 0), -- Heavy Shot           (req 10)
    (@AMMO_VENDOR, 15,  3033, 0, 0, 0), -- Solid Shot           (req 25)
    (@AMMO_VENDOR, 16, 11284, 0, 0, 0), -- Accurate Slugs       (req 40)
    (@AMMO_VENDOR, 17, 19317, 0, 0, 0), -- Ice Threaded Bullet  (req 51)
    (@AMMO_VENDOR, 18, 28061, 0, 0, 0), -- Ironbite Shell       (req 65)
    (@AMMO_VENDOR, 19, 30612, 0, 0, 0), -- Halaani Grimshot     (req 66)
    (@AMMO_VENDOR, 20, 32882, 0, 0, 0), -- Hellfire Shot        (req 68)
    (@AMMO_VENDOR, 21, 32883, 0, 0, 0), -- Felbane Slugs        (req 68)
    (@AMMO_VENDOR, 22, 31735, 0, 0, 0), -- Timeless Shell       (req 70)
    (@AMMO_VENDOR, 23, 34582, 0, 0, 0), -- Mysterious Shell     (req 70)
    (@AMMO_VENDOR, 24, 41584, 0, 0, 0); -- Frostbite Bullets    (req 75)
