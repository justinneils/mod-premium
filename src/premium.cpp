#include "Chat.h"
#include "CommandScript.h"
#include "Config.h"
#include "Define.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "TemporarySummon.h"

enum Vendors
{
    NPC_AUCTION_H   = 9856,
    NPC_AUCTION_A   = 8670,

    // Custom creatures added by this module, stocking every vendor-sold arrow and
    // bullet in one place. Defined in
    // data/sql/db-world/updates/mod_premium_ammo_vendor.sql -- never spawned in the
    // world, only summoned from the gossip menu. Two entries with identical stock,
    // for the same reason the trainers and trade-goods vendors are paired:
    // SummonTempNPC fixes the faction but not the model, so one shared entry would
    // send an Alliance player an orc. Models are lifted from the Ironforge gunsmith
    // and the Orgrimmar bowyer respectively.
    NPC_AMMO_VENDOR_A = 900017,
    NPC_AMMO_VENDOR_H = 900018,

    // Likewise custom, and paired for the same reason: the full food-and-drink
    // ladder in one place -- every rank of mana-regen drink, the three rations that
    // restore health and mana together, and every rank of health-regen food.
    // Defined in data/sql/db-world/updates/mod_premium_refreshment_vendor.sql.
    // Models are lifted from the Ironforge bread vendor and the Orgrimmar beverage
    // merchant.
    NPC_REFRESHMENT_VENDOR_A = 900019,
    NPC_REFRESHMENT_VENDOR_H = 900020,

    // The game's own bag vendors -- Alyssa Griffith in Stormwind and Gotri in
    // Orgrimmar, both of whom carry the subname "Bag Vendor" in the stock data.
    // Real NPCs rather than custom ones, because between them they already sell
    // every bag a vendor sells outside the profession bags: Small Brown Pouch,
    // Brown Leather Satchel, Heavy Brown Bag and Huge Brown Sack, identical stock
    // on both, priced in gold and gated by nothing. Paired per faction for the
    // model, exactly like the trainers and trade-goods vendors above.
    NPC_BAG_VENDOR_A = 1321,
    NPC_BAG_VENDOR_H = 3369,

    // The game's own capital-city stable masters -- Jenova Stoneshield in Stormwind
    // and Xon'cha in Orgrimmar. Real NPCs, and paired per faction for the model like
    // everything above, but here the entry matters for a second reason: the stable
    // window is only opened by a creature that carries UNIT_NPC_FLAG_STABLEMASTER
    // (0x400000) and a gossip menu with a GOSSIP_OPTION_STABLEPET row, and the server
    // re-checks that flag on every stable packet (WorldSession::CheckStableMaster).
    // A summoned copy of a real stable master satisfies both; a custom creature would
    // have to reproduce them.
    NPC_STABLE_MASTER_A = 11069,
    NPC_STABLE_MASTER_H = 9988
};

// The game objects this module puts on the ground. Objects rather than NPCs because
// each of these three IS an object in the world: there is no banker that opens a
// guild vault and no smith that lends you an anvil.
enum SummonedObjects
{
    // Guild Vault, GAMEOBJECT_TYPE_GUILD_BANK. The personal bank opens with a packet
    // and no object at all (see the Bank case below), but the guild bank cannot:
    // every guild-bank message the client sends names the vault it is talking to, and
    // WorldSession::HandleGuildBanker* drops the message unless that GUID is a guild
    // bank object within interaction range. So the vault is summoned and the player
    // clicks it, exactly as they would in a capital city. 187299 is the most widely
    // spawned of the vault entries.
    GO_GUILD_VAULT = 187299,

    // Mailbox, GAMEOBJECT_TYPE_MAILBOX -- the one in Gadgetzan, picked because it is
    // friendly to both factions (faction 35). The mail window cannot be opened by
    // packet alone for a player: sent against the player's own GUID it is accepted
    // only from accounts holding RBAC_PERM_COMMAND_MAILBOX (WorldSession::
    // CanOpenMailBox), so for everybody else the window opened but every send, take
    // and delete in it was dropped as a cheat. A real mailbox in range passes.
    GO_MAILBOX = 144112,

    // Blacksmithing recipes require spell focus 1 and mining's smelting recipes
    // require spell focus 3 (Spell.dbc RequiresSpellFocus, checked in Spell::CheckCast
    // against nearby GAMEOBJECT_TYPE_SPELL_FOCUS objects). These two provide them,
    // each over a 10 yard radius, which is the whole of what a smith needs to work
    // away from a city: an anvil to hammer on and a forge to smelt at.
    GO_ANVIL = 1684,    // Blacksmith's Anvil, spell focus 1
    GO_FORGE = 1685     // Forge, spell focus 3
};

// Where a summoned object lands: this far in front of the player, and this far to
// either side of their facing so a pair does not land inside itself.
static constexpr float GO_SUMMON_DISTANCE = 2.5f;
static constexpr float GO_SUMMON_SPREAD = 0.8f;     // radians

enum Trainers
{
    // Alliance
    DRUID_A     = 5504,
    HUNTER_A    = 5515,
    MAGE_A      = 5497,
    PALADIN_A   = 928,
    PRIEST_A    = 376,
    ROGUE_A     = 918,
    SHAMAN_A    = 20407,
    WARLOCK_A   = 461,
    WARRIOR_A   = 5479,

    // Horde
    DRUID_H     = 3033,
    HUNTER_H    = 3406,
    MAGE_H      = 5883,
    PALADIN_H   = 23128,
    PRIEST_H    = 3045,
    ROGUE_H     = 3401,
    SHAMAN_H    = 3344,
    WARLOCK_H   = 3324,
    WARRIOR_H   = 3354,

    DEATHKNIGHT_AH  = 28472
};

enum PremiumGossip
{
    PREMIUM_MENU = 62001,
    PREMIUM_MENU_TEXT = 90003,
    // Option ids into gossip_menu_option's rows for menu 62001. Rows 0-2 (Morph,
    // Demorph, Mount) were deleted and row 5 (Vendor) is no longer offered, so the
    // ids are pinned rather than counted.
    GOSSIP_TRAIN_ME = 3,
    GOSSIP_PLAYER,
    GOSSIP_MAIL = 6,
    GOSSIP_BANK,
    GOSSIP_AUCTION_HOUSE,
    GOSSIP_FACTION
};

template <typename T, std::size_t N>
static constexpr uint32 CountOf(T const (&)[N])
{
    return static_cast<uint32>(N);
}

struct TrainerTier
{
    char const* label;
    uint32 entryA;  // Alliance-race trainer for this proficiency
    uint32 entryH;  // Horde-race trainer for this proficiency
};

struct ProfessionTrainerData
{
    char const* name;
    TrainerTier const* tiers;
    uint32 tierCount;
};

// Profession trainers by proficiency tier, per faction. SummonTempNPC overrides
// the summoned creature's faction to the player's so it is never hostile, but the
// model/race is fixed by the creature entry -- so an Alliance player must summon an
// Alliance-race trainer and a Horde player a Horde-race one. Journeyman/Expert/Artisan
// use capital-city trainers of each faction; Master uses the neutral Shattrath (Sha'tar)
// trainers and Grand Master the neutral Dalaran trainers (both acceptable to either side).
// Where a faction lacks a distinct proficiency trainer, a same-faction trainer that teaches
// up to the same skill cap is reused (3.3.5 capital trainers teach through Artisan/300).
static TrainerTier const AlchemyTiers[] =
{
    { "Journeyman",   1215,  11046 },
    { "Expert",       5499,  3347 },
    { "Artisan",      4160,  1386 },
    { "Master",       33674, 33674 },
    { "Grand Master", 26903, 26903 }
};
static TrainerTier const BlacksmithingTiers[] =
{
    { "Journeyman",   514,   3174 },
    { "Expert",       3136,  1383 },
    { "Artisan",      4258,  3355 },
    { "Master",       33675, 33675 },
    { "Grand Master", 26904, 26904 }
};
static TrainerTier const EnchantingTiers[] =
{
    { "Journeyman",   3606,  3345 },
    { "Expert",       1317,  3011 },
    { "Artisan",      11072, 11074 },
    { "Master",       33676, 33676 },
    { "Grand Master", 26906, 26906 }
};
static TrainerTier const EngineeringTiers[] =
{
    { "Journeyman",   1702,  2857 },
    { "Expert",       5518,  3412 },
    { "Artisan",      5174,  11017 },
    { "Master",       33677, 33677 },
    { "Grand Master", 26907, 26907 }
};
static TrainerTier const HerbalismTiers[] =
{
    { "Journeyman",   812,   3013 },
    { "Master",       33678, 33678 },
    { "Grand Master", 26910, 26910 }
};
static TrainerTier const InscriptionTiers[] =
{
    { "Journeyman",   30713, 30706 },
    { "Master",       33679, 33679 },
    { "Grand Master", 26916, 26916 }
};
static TrainerTier const JewelcraftingTiers[] =
{
    { "Journeyman",   19778, 15501 },
    { "Master",       33680, 33680 },
    { "Grand Master", 26915, 26915 }
};
static TrainerTier const LeatherworkingTiers[] =
{
    { "Journeyman",   1632,  5811 },
    { "Expert",       5564,  1385 },
    { "Artisan",      4212,  3007 },
    { "Master",       33681, 33681 },
    { "Grand Master", 26911, 26911 }
};
static TrainerTier const MiningTiers[] =
{
    { "Journeyman",   1681,  3357 },
    { "Master",       33682, 33682 },
    { "Grand Master", 26912, 26912 }
};
static TrainerTier const SkinningTiers[] =
{
    { "Journeyman",   1292,  7088 },
    { "Master",       33683, 33683 },
    { "Grand Master", 26913, 26913 }
};
static TrainerTier const TailoringTiers[] =
{
    { "Journeyman",   1103,  2855 },
    { "Expert",       5567,  3363 },
    { "Artisan",      1346,  3363 },
    { "Master",       33684, 33684 },
    { "Grand Master", 26914, 26914 }
};
static TrainerTier const CookingTiers[] =
{
    { "Journeyman",   3087,  3026 },
    { "Master",       19186, 19186 },
    { "Grand Master", 26905, 26905 }
};
static TrainerTier const FirstAidTiers[] =
{
    { "Journeyman",   2327,  3373 },
    { "Master",       22477, 22477 },
    { "Grand Master", 23734, 23734 }
};
static TrainerTier const FishingTiers[] =
{
    { "Journeyman",   1680,  3332 },
    { "Grand Master", 26909, 26909 }
};

static ProfessionTrainerData const ProfessionTrainerList[] =
{
    { "Alchemy",        AlchemyTiers,        CountOf(AlchemyTiers) },
    { "Blacksmithing",  BlacksmithingTiers,  CountOf(BlacksmithingTiers) },
    { "Enchanting",     EnchantingTiers,     CountOf(EnchantingTiers) },
    { "Engineering",    EngineeringTiers,     CountOf(EngineeringTiers) },
    { "Herbalism",      HerbalismTiers,      CountOf(HerbalismTiers) },
    { "Inscription",    InscriptionTiers,    CountOf(InscriptionTiers) },
    { "Jewelcrafting",  JewelcraftingTiers,  CountOf(JewelcraftingTiers) },
    { "Leatherworking", LeatherworkingTiers, CountOf(LeatherworkingTiers) },
    { "Mining",         MiningTiers,         CountOf(MiningTiers) },
    { "Skinning",       SkinningTiers,       CountOf(SkinningTiers) },
    { "Tailoring",      TailoringTiers,      CountOf(TailoringTiers) },
    { "Cooking",        CookingTiers,        CountOf(CookingTiers) },
    { "First Aid",      FirstAidTiers,       CountOf(FirstAidTiers) },
    { "Fishing",        FishingTiers,        CountOf(FishingTiers) }
};

static constexpr uint32 ProfessionTrainerCount =
    sizeof(ProfessionTrainerList) / sizeof(ProfessionTrainerList[0]);

// Trade-goods / supply vendors by profession, per faction. As with the profession
// trainers, SummonTempNPC overrides the summoned creature's faction to the player's,
// so hostility is never an issue -- the entry only fixes the race/model, so Alliance
// players summon an Alliance-race capital vendor and Horde players a Horde-race one.
// Each entry is a well-stocked capital-city supply vendor (verified to have npc_vendor
// inventory). Gathering professions (Herbalism, Skinning) are omitted -- they have no
// supply vendor; First Aid cloth is covered by the General Trade Goods vendor.
struct TradeGoodsVendorData
{
    char const* name;
    uint32 entryA;  // Alliance-race vendor
    uint32 entryH;  // Horde-race vendor
};

static TradeGoodsVendorData const TradeGoodsVendorList[] =
{
    { "General Trade Goods", 8934,  5817 },  // Christopher Hewen  / Shimra
    { "Alchemy",             1313,  3348 },  // Maria Lumere       / Kor'geld
    { "Blacksmithing",       5512,  2999 },  // Kaita Deepforge    / Taur Stonehoof
    { "Cooking",             16718, 16677 }, // Phea               / Quelis
    { "Enchanting",          1318,  3346 },  // Jessara Cordell    / Kithas
    { "Engineering",         5175,  3413 },  // Gearcutter Cogspinner / Sovik
    { "Fishing",             16708, 3333 },  // Dekin              / Shankys
    { "Inscription",         30730, 30723 }, // Stanly McCormick   / Xantili
    { "Jewelcrafting",       17512, 16624 }, // Arred              / Gelanthis
    { "Leatherworking",      5565,  3366 },  // Jillian Tanner     / Tamar
    { "Mining",              16751, 16664 }, // Merran             / Zelan
    { "Tailoring",           1347,  3364 }   // Alexandra Bolero   / Borya
};

static constexpr uint32 TradeGoodsVendorCount =
    sizeof(TradeGoodsVendorList) / sizeof(TradeGoodsVendorList[0]);

// Gossip action ranges (GOSSIP_ACTION_INFO_DEF == 1000):
//   +100 .. +100+profCount   -> open a profession's tier submenu
//   +200 .. +200+profCount*10 -> summon (profIndex * 10 + tierIndex)
static constexpr uint32 PROFESSION_TIER_MENU_BASE = GOSSIP_ACTION_INFO_DEF + 100;
static constexpr uint32 PROFESSION_SUMMON_BASE = GOSSIP_ACTION_INFO_DEF + 200;

// Trade-goods vendor summon actions: +400 .. +400+TradeGoodsVendorCount. Kept clear of
// the profession summon range (+200 .. +200+ProfessionTrainerCount*10) above.
static constexpr uint32 TRADE_GOODS_SUMMON_BASE = GOSSIP_ACTION_INFO_DEF + 400;

// The premium menu, opened by the .premium command (premium_command). It is sent
// from the player's own GUID, so its clicks come back through premium_menu_player.
class PremiumMenu
{
public:
    // Why the menu cannot be opened for this player right now, or nullptr if it can.
    static char const* CannotOpenReason(Player* player)
    {
        if (!sConfigMgr->GetOption<bool>("PremiumAccount", true))
            return "The premium menu is disabled.";

        QueryResult result = CharacterDatabase.Query("SELECT `AccountId` FROM `premium` WHERE `active`=1 AND `AccountId`={}", player->GetSession()->GetAccountId());

        if (!result)
            return "This account is not a premium account.";

        if (player->IsInCombat())
            return "You can't do that while in combat.";

        return nullptr;
    }

    // Menus sent from the player's own GUID are only answered while the menu id the
    // client echoes back matches the one the server last sent (see
    // WorldSession::HandleGossipSelectOptionOpcode), and that id is what tells
    // premium_menu_player its clicks apart from any other module's. ClearMenus resets
    // it to 0, so it is set again on every send.
    static void SendMenu(Player* player)
    {
        player->PlayerTalkClass->GetGossipMenu().SetMenuId(PREMIUM_MENU);
        SendGossipMenuFor(player, PREMIUM_MENU_TEXT, player->GetGUID());
    }

    static void ShowMainMenu(Player* player)
    {
        // No "is an NPC already standing here" check: one used to refuse the whole
        // menu while any class trainer or auctioneer was within 10 yards -- which
        // includes the one the menu just summoned, so it went dead for the full
        // Premium.NpcDuration. Stacking is handled where it matters instead, in
        // SummonTempNPC, which dismisses whatever NPC the menu summoned before.
        ClearGossipMenuFor(player);

        if (sConfigMgr->GetOption<bool>("Trainers", true))
            AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_TRAIN_ME, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);

        if (sConfigMgr->GetOption<bool>("ProfessionTrainers", true))
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Profession Trainers", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);

        if (sConfigMgr->GetOption<bool>("TradeGoodsVendors", true))
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Trade Goods Vendors", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 11);

        if (sConfigMgr->GetOption<bool>("AmmoVendor", true))
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Ammunition Vendor", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 12);

        if (sConfigMgr->GetOption<bool>("RefreshmentVendor", true))
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Food & Drink Vendor", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 13);

        if (sConfigMgr->GetOption<bool>("BagVendor", true))
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "Bag Vendor", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 14);

        if (sConfigMgr->GetOption<bool>("ForgeAndAnvil", true))
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Forge & Anvil", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 16);

        // Hunters only: the stable is the hunter pet system, and the client-side
        // gossip option is itself gated on the class (Player::PrepareGossipMenu drops
        // GOSSIP_OPTION_STABLEPET for anybody else), so to any other class the
        // summoned NPC would only offer the lost-companion chat.
        if (sConfigMgr->GetOption<bool>("StableMaster", true) && player->getClass() == CLASS_HUNTER)
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Stable Master", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 17);

        if (sConfigMgr->GetOption<bool>("PlayerInteraction", true))
            AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_PLAYER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);

        SendMenu(player);
    }

    static void HandleSelect(Player* player, uint32 action)
    {
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 3: /*Show Bank*/
            {
                player->GetSession()->SendShowBank(player->GetGUID());
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 4: /*Mail Box*/
            {
                // Dropped in front of the player to right-click: see GO_MAILBOX for
                // why opening the mail window directly does not work.
                SummonTempGO(player, GO_MAILBOX);
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 7: /*Auction House*/
            {
                uint32 auctionId = 0;
                std::string salute = "";
                if (player->GetTeamId() == TEAM_HORDE)
                {
                    auctionId = NPC_AUCTION_H;
                    switch (player->GetSession()->GetSessionDbLocaleIndex())
                    {
                    case LOCALE_enUS:
                    case LOCALE_koKR:
                    case LOCALE_frFR:
                    case LOCALE_deDE:
                    case LOCALE_zhCN:
                    case LOCALE_zhTW:
                    case LOCALE_ruRU:
                    {
                        salute = "I will go shortly, I need to get back to Orgrimmar";
                        break;
                    }
                    case LOCALE_esES:
                    case LOCALE_esMX:
                    {
                        salute = "Me iré en breve, necesito volver a Orgrimmar.";
                    }
                    default:
                        break;
                    }
                }
                else
                {
                    auctionId = NPC_AUCTION_A;
                    switch (player->GetSession()->GetSessionDbLocaleIndex())
                    {
                    case LOCALE_enUS:
                    case LOCALE_koKR:
                    case LOCALE_frFR:
                    case LOCALE_deDE:
                    case LOCALE_zhCN:
                    case LOCALE_zhTW:
                    case LOCALE_ruRU:
                    {
                        salute = "I will go shortly, I need to get back to Stormwind City";
                        break;
                    }
                    case LOCALE_esES:
                    case LOCALE_esMX:
                    {
                        salute = "Me iré en breve, necesito volver a la Ciudad de Ventormenta.";
                    }
                    default:
                        break;
                    }
                }

                SummonTempNPC(player, auctionId, salute.c_str());
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 8: /* Class Trainers*/
            {
                uint32 trainerId = 0;
                switch (player->getClass())
                {
                    case CLASS_ROGUE:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? ROGUE_A : ROGUE_H;
                        break;
                    case CLASS_WARRIOR:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? WARRIOR_A : WARRIOR_H;
                        break;
                    case CLASS_PRIEST:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? PRIEST_A : PRIEST_H;
                        break;
                    case CLASS_MAGE:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? MAGE_A : MAGE_H;
                        break;
                    case CLASS_PALADIN:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? PALADIN_A : PALADIN_H;
                        break;
                    case CLASS_HUNTER:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? HUNTER_A : HUNTER_H;
                        break;
                    case CLASS_DRUID:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? DRUID_A : DRUID_H;
                        break;
                    case CLASS_SHAMAN:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? SHAMAN_A : SHAMAN_H;
                        break;
                    case CLASS_WARLOCK:
                        trainerId = player->GetTeamId() == TEAM_ALLIANCE ? WARLOCK_A : WARLOCK_H;
                        break;
                    case CLASS_DEATH_KNIGHT:
                        trainerId = DEATHKNIGHT_AH;
                        break;
                }

                SummonTempNPC(player, trainerId);
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 9: /*Player Interactions*/
            {
                ClearGossipMenuFor(player);

                if (sConfigMgr->GetOption<bool>("MailBox", true))
                    AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_MAIL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

                if (sConfigMgr->GetOption<bool>("Bank", true))
                    AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_BANK, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

                // Offered only to players who are in a guild: to anybody else the
                // vault is an object that answers "you are not in a guild" when
                // clicked, and there is no point summoning it for that.
                if (sConfigMgr->GetOption<bool>("GuildBank", true) && player->GetGuildId())
                    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "Guild Bank", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 15);

                if (sConfigMgr->GetOption<bool>("Auction", true))
                    AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_AUCTION_HOUSE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);

                SendMenu(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 10: /*Profession Trainers*/
            {
                ClearGossipMenuFor(player);

                for (uint32 i = 0; i < ProfessionTrainerCount; ++i)
                    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, ProfessionTrainerList[i].name, GOSSIP_SENDER_MAIN, PROFESSION_TIER_MENU_BASE + i);

                SendMenu(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 11: /*Trade Goods Vendors*/
            {
                ClearGossipMenuFor(player);

                for (uint32 i = 0; i < TradeGoodsVendorCount; ++i)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, TradeGoodsVendorList[i].name, GOSSIP_SENDER_MAIN, TRADE_GOODS_SUMMON_BASE + i);

                SendMenu(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 12: /*Ammunition Vendor*/
            {
                uint32 entry = (player->GetTeamId() == TEAM_ALLIANCE) ? NPC_AMMO_VENDOR_A : NPC_AMMO_VENDOR_H;
                SummonTempNPC(player, entry);
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 13: /*Food & Drink Vendor*/
            {
                uint32 entry = (player->GetTeamId() == TEAM_ALLIANCE) ? NPC_REFRESHMENT_VENDOR_A : NPC_REFRESHMENT_VENDOR_H;
                SummonTempNPC(player, entry);
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 14: /*Bag Vendor*/
            {
                uint32 entry = (player->GetTeamId() == TEAM_ALLIANCE) ? NPC_BAG_VENDOR_A : NPC_BAG_VENDOR_H;
                SummonTempNPC(player, entry);
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 15: /*Guild Bank*/
            {
                // Dropped in front of the player rather than opened for them: see
                // GO_GUILD_VAULT for why the guild bank needs a real object where the
                // personal bank needs none. Right-clicking it opens the guild bank.
                SummonTempGO(player, GO_GUILD_VAULT);
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 16: /*Forge & Anvil*/
            {
                // Both, always. Blacksmithing wants the anvil and smelting wants the
                // forge, but a blacksmith is usually also the miner who smelted the
                // bars, and summoning one without the other would just mean coming
                // back to the menu.
                SummonTempGO(player, GO_FORGE, GO_SUMMON_SPREAD);
                SummonTempGO(player, GO_ANVIL, -GO_SUMMON_SPREAD);
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 17: /*Stable Master*/
            {
                // Right-click the summoned stable master and pick "I'd like to stable
                // my pet here" to get the usual stable window: swap the active pet for
                // a stabled one, or buy another stable slot.
                uint32 entry = (player->GetTeamId() == TEAM_ALLIANCE) ? NPC_STABLE_MASTER_A : NPC_STABLE_MASTER_H;
                SummonTempNPC(player, entry);
                CloseGossipMenuFor(player);
                break;
            }
            default:
            {
                if (action >= PROFESSION_TIER_MENU_BASE && action < PROFESSION_TIER_MENU_BASE + ProfessionTrainerCount)
                {
                    uint32 profIndex = action - PROFESSION_TIER_MENU_BASE;
                    ProfessionTrainerData const& prof = ProfessionTrainerList[profIndex];

                    ClearGossipMenuFor(player);

                    for (uint32 i = 0; i < prof.tierCount; ++i)
                        AddGossipItemFor(player, GOSSIP_ICON_TRAINER, prof.tiers[i].label, GOSSIP_SENDER_MAIN, PROFESSION_SUMMON_BASE + profIndex * 10 + i);

                    SendMenu(player);
                }
                else if (action >= PROFESSION_SUMMON_BASE && action < PROFESSION_SUMMON_BASE + ProfessionTrainerCount * 10)
                {
                    uint32 code = action - PROFESSION_SUMMON_BASE;
                    uint32 profIndex = code / 10;
                    uint32 tierIndex = code % 10;

                    if (tierIndex < ProfessionTrainerList[profIndex].tierCount)
                    {
                        TrainerTier const& tier = ProfessionTrainerList[profIndex].tiers[tierIndex];
                        uint32 entry = (player->GetTeamId() == TEAM_ALLIANCE) ? tier.entryA : tier.entryH;
                        SummonTempNPC(player, entry);
                        CloseGossipMenuFor(player);
                    }
                }
                else if (action >= TRADE_GOODS_SUMMON_BASE && action < TRADE_GOODS_SUMMON_BASE + TradeGoodsVendorCount)
                {
                    TradeGoodsVendorData const& vendor = TradeGoodsVendorList[action - TRADE_GOODS_SUMMON_BASE];
                    uint32 entry = (player->GetTeamId() == TEAM_ALLIANCE) ? vendor.entryA : vendor.entryH;
                    SummonTempNPC(player, entry);
                    CloseGossipMenuFor(player);
                }
                break;
            }
        }
    }

    // Put a temporary game object on the ground in front of the player. `angle` is
    // relative to the player's facing, so a pair of objects can be placed either side
    // of it instead of inside one another. The object belongs to the player -- it goes
    // with them when they log out or die -- and lives for the same Premium.NpcDuration
    // as a summoned NPC.
    //
    // SetSpawnedByDefault(false) is what makes it a temporary object rather than a
    // world spawn, and it is not optional: GameObject::isSpawned() is FALSE for a
    // default-spawned object that has a respawn timer, and an object that is not
    // spawned is invisible to every client (GameObject::IsInvisibleDueToDespawn). The
    // same flag routes GameObject::Update down the despawn path when the timer runs
    // out instead of the respawn path. Visibility is refreshed by hand because the
    // flag changes after the object has already been added to the map.
    static GameObject* SummonTempGO(Player* player, uint32 entry, float angle = 0.0f)
    {
        if (!player || entry == 0)
            return nullptr;

        int32 duration = sConfigMgr->GetOption<int32>("Premium.NpcDuration", 60);
        if (duration <= 0) // Safeguard
            duration = 60;

        float x, y, z;
        player->GetClosePoint(x, y, z, player->GetObjectSize(), GO_SUMMON_DISTANCE, angle);

        GameObject* go = player->SummonGameObject(entry, x, y, z, player->GetOrientation(),
                                                  0.0f, 0.0f, 0.0f, 0.0f, duration);
        if (!go)
            return nullptr;

        go->SetSpawnedByDefault(false);
        go->UpdateObjectVisibility(true);
        return go;
    }

    // Every creature entry this menu can summon, gathered from the tables above.
    static std::vector<uint32> const& SummonableEntries()
    {
        static std::vector<uint32> const entries = []
        {
            std::vector<uint32> list =
            {
                NPC_AUCTION_A, NPC_AUCTION_H,
                NPC_AMMO_VENDOR_A, NPC_AMMO_VENDOR_H,
                NPC_REFRESHMENT_VENDOR_A, NPC_REFRESHMENT_VENDOR_H,
                NPC_BAG_VENDOR_A, NPC_BAG_VENDOR_H,
                NPC_STABLE_MASTER_A, NPC_STABLE_MASTER_H,
                DRUID_A, HUNTER_A, MAGE_A, PALADIN_A, PRIEST_A, ROGUE_A, SHAMAN_A, WARLOCK_A, WARRIOR_A,
                DRUID_H, HUNTER_H, MAGE_H, PALADIN_H, PRIEST_H, ROGUE_H, SHAMAN_H, WARLOCK_H, WARRIOR_H,
                DEATHKNIGHT_AH
            };

            for (ProfessionTrainerData const& prof : ProfessionTrainerList)
                for (uint32 i = 0; i < prof.tierCount; ++i)
                {
                    list.push_back(prof.tiers[i].entryA);
                    list.push_back(prof.tiers[i].entryH);
                }

            for (TradeGoodsVendorData const& vendor : TradeGoodsVendorList)
            {
                list.push_back(vendor.entryA);
                list.push_back(vendor.entryH);
            }

            return list;
        }();

        return entries;
    }

    // One summoned NPC at a time: whatever the menu summoned for this player before
    // is dismissed when they summon the next, whichever menu either came from. Only
    // the player's own summons are touched -- the same entries are real world spawns
    // (the class trainers and bag vendors are) and other players' summons too. The
    // search radius is generous because a summon follows its owner, so it is never
    // far; one left behind on another map simply times out as before.
    static void DespawnOwnSummons(Player* player)
    {
        std::list<Creature*> existing;
        player->GetCreatureListWithEntryInGrid(existing, SummonableEntries(), 100.0f);

        for (Creature* creature : existing)
            if (TempSummon* summon = creature->ToTempSummon())
                if (summon->GetSummonerGUID() == player->GetGUID())
                    summon->DespawnOrUnsummon();
    }

    static void SummonTempNPC(Player* player, uint32 entry, const char* salute = "")
    {
        if (!player || entry == 0)
            return;

        int npcDuration = sConfigMgr->GetOption<int32>("Premium.NpcDuration", 60) * IN_MILLISECONDS;
        if (npcDuration <= 0) // Safeguard
            npcDuration = 60 * IN_MILLISECONDS;

        DespawnOwnSummons(player);

        Creature* npc = player->SummonCreature(entry, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, npcDuration);
        if (!npc)
            return;

        npc->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        npc->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, player->GetFollowAngle());
        npc->SetFaction(player->GetFaction());

        if (salute && !(salute[0] == '\0'))
            npc->Whisper(salute, LANG_UNIVERSAL, player, false);
    }
};

// Clicks in the menu: sent from the player's own GUID, they arrive at this player
// hook rather than at any creature, object or item script.
class premium_menu_player : public PlayerScript
{
public:
    premium_menu_player() : PlayerScript("premium_menu_player", { PLAYERHOOK_ON_GOSSIP_SELECT }) { }

    void OnPlayerGossipSelect(Player* player, uint32 menuId, uint32 /*sender*/, uint32 action) override
    {
        if (menuId == PREMIUM_MENU)
            PremiumMenu::HandleSelect(player, action);
    }
};

using namespace Acore::ChatCommands;

// .premium -- opens the premium menu.
class premium_command : public CommandScript
{
public:
    premium_command() : CommandScript("premium_command") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "premium", HandlePremiumCommand, SEC_PLAYER, Console::No }
        };

        return commandTable;
    }

    static bool HandlePremiumCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (char const* reason = PremiumMenu::CannotOpenReason(player))
        {
            handler->SendErrorMessage(reason);
            return false;
        }

        PremiumMenu::ShowMainMenu(player);
        return true;
    }
};

void AddPremiumAccount()
{
    new premium_menu_player();
    new premium_command();
}
