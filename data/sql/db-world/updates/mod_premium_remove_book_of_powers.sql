-- The menu used to open from an item, the "Book of Powers", made by overwriting the
-- stock item 9017 (Codex of Holy Protection III) with the premium_account script and
-- a Teleport: Dalaran on-use spell. That item and its script are gone -- the menu now
-- opens only with the .premium command -- so 9017 is put back exactly as the base
-- world data has it (data/sql/base/db_world/item_template.sql). Every column the old
-- mod_premium_item_9017.sql changed is restored; on a world that never had it, this
-- changes nothing.
UPDATE `item_template` SET
    `name` = 'Codex of Holy Protection III',
    `Quality` = 1,
    `Flags` = 80,
    `BuyPrice` = 44000,
    `SellPrice` = 11000,
    `AllowableClass` = 31248,
    `ItemLevel` = 54,
    `RequiredLevel` = 54,
    `spellid_1` = 0,
    `description` = '',
    `ScriptName` = '',
    `VerifiedBuild` = 1
WHERE `entry` = 9017;
