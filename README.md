# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore

## Premium Account

- Latest Premium Account build status with azerothcore:

[![Build Status](https://github.com/azerothcore/mod-premium/workflows/core-build/badge.svg?branch=master&event=push)](https://github.com/azerothcore/mod-premium)

This is a module for [AzerothCore](http://www.azerothcore.org) that adds Premium account features to players.

![script screenshot](/screenshots/script.png?raw=true "script screenshot")

Current features:

This module allows players to open a menu with the `.premium` chat command, from which they can:
- Mobile Bank
- Mobile Auction
- Summon your Guild Bank (a Guild Vault to click, since the guild bank needs a real object)
- Summon a Food & Drink vendor stocking every rank of mana-regen drink, health-and-mana ration and health-regen food, faction-matched
- Summon the faction's own Bag Vendor -- Alyssa Griffith (Stormwind) or Gotri (Orgrimmar) -- who sell bags from 6 up to 12 slots
- Summon a forge and an anvil, for the mining and blacksmithing recipes that need them
- Summon a Stable Master (hunters only) -- the faction's own, Jenova Stoneshield or Xon'cha -- to swap pets in and out of the stable or buy a stable slot
- Trainer players will now spawn their class trainer also depending on faction.
- Summon a Mailbox (a real one to click, since the mail window alone only works for GMs)
- One summoned NPC at a time: summoning another dismisses the previous one

Upcoming features:
- Teleport players to major citys
- Talent Reset features

## Requirements

Premium module currently requires: players to import the sql into character database

AzerothCore v1.0.1+

## How to install

###1) Simply place the module under the `modules` folder of your AzerothCore source folder.

You can do clone it via git under the azerothcore/modules directory:

`cd path/to/azerothcore/modules`

`git clone https://github.com/azerothcore/mod-premium.git`

or you can manually [download the module](https://github.com/azerothcore/mod-premium/archive/master.zip), unzip the premium folder and place it under the `azerothcore/modules` directory.

Players open the premium menu with the `.premium` chat command. Only accounts with an active row in the characters database's `premium` table can use it.

###2) Re-run cmake and launch a clean build of AzerothCore

**That's it.**

### (Optional) Edit module configuration

If you need to change the module configuration, go to your server configuration folder (e.g. **etc**), copy `premium.conf.dist` to `premium.conf` and edit it as you prefer.

