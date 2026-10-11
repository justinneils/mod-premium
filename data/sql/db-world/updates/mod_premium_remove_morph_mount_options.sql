-- The Morph, Demorph and Mount options were removed from the premium menu (see
-- src/premium.cpp), so their option text -- OptionIDs 0, 1 and 2 of menu 62001,
-- inserted by 2023_08_11_04.sql -- goes too. The remaining options keep their ids:
-- PremiumGossip pins them rather than counting from 0.
DELETE FROM `gossip_menu_option` WHERE `MenuID` = 62001 AND `OptionID` IN (0, 1, 2);
DELETE FROM `gossip_menu_option_locale` WHERE `MenuID` = 62001 AND `OptionID` IN (0, 1, 2);
