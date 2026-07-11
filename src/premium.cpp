#include "Config.h"
#include "Define.h"
#include "GossipDef.h"
#include "Item.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "Spell.h"

enum Vendors
{
    NPC_VENDOR_A    = 54,
    NPC_VENDOR_H    = 3163,
    NPC_AUCTION_H   = 9856,
    NPC_AUCTION_A   = 8670
};

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

enum Mounts
{
    HUMAN_MOUNT     = 470,
    ORC_MOUNT       = 6653,
    GNOME_MOUNT     = 17454,
    NIGHTELF_MOUNT  = 8394,
    DWARF_MOUNT     = 6899,
    UNEAD_MOUNT     = 17463,
    TAUREN_MOUNT    = 64657,
    TROLL_MOUNT     = 8395,
    BLOODELF_MOUNT  = 35022,
    DRAENEI_MOUNT   = 34406
};

enum PremiumGossip
{
    PREMIUM_MENU = 62001,
    PREMIUM_MENU_TEXT = 90003,
    GOSSIP_MORPH = 0,
    GOSSIP_DEMORPH,
    GOSSIP_MOUNT,
    GOSSIP_TRAIN_ME,
    GOSSIP_PLAYER,
    GOSSIP_VENDOR,
    GOSSIP_MAIL,
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
    uint32 entry;
};

struct ProfessionTrainerData
{
    char const* name;
    TrainerTier const* tiers;
    uint32 tierCount;
};

// Profession trainers by proficiency tier. SummonTempNPC overrides the
// summoned creature's faction to the player's, so a single entry serves
// both factions. A tier only teaches once the player holds the previous
// proficiency (standard trainer gating).
static TrainerTier const AlchemyTiers[] =
{
    { "Journeyman",   1215 },
    { "Expert",       2391 },
    { "Artisan",      4160 },
    { "Master",       16588 },
    { "Grand Master", 26903 }
};
static TrainerTier const BlacksmithingTiers[] =
{
    { "Journeyman",   514 },
    { "Expert",       1383 },
    { "Artisan",      3355 },
    { "Master",       16583 },
    { "Grand Master", 26564 }
};
static TrainerTier const EnchantingTiers[] =
{
    { "Journeyman",   3606 },
    { "Expert",       1317 },
    { "Artisan",      11072 },
    { "Master",       18753 },
    { "Grand Master", 26906 }
};
static TrainerTier const EngineeringTiers[] =
{
    { "Journeyman",   1702 },
    { "Expert",       1676 },
    { "Artisan",      5174 },
    { "Master",       17634 },
    { "Grand Master", 25277 }
};
static TrainerTier const HerbalismTiers[] =
{
    { "Journeyman",   812 },
    { "Master",       18748 },
    { "Grand Master", 26910 }
};
static TrainerTier const InscriptionTiers[] =
{
    { "Journeyman",   30706 },
    { "Master",       30721 },
    { "Grand Master", 26916 }
};
static TrainerTier const JewelcraftingTiers[] =
{
    { "Journeyman",   15501 },
    { "Master",       18751 },
    { "Grand Master", 26915 }
};
static TrainerTier const LeatherworkingTiers[] =
{
    { "Journeyman",   223 },
    { "Expert",       1385 },
    { "Artisan",      3007 },
    { "Master",       18754 },
    { "Grand Master", 26911 }
};
static TrainerTier const MiningTiers[] =
{
    { "Journeyman",   1681 },
    { "Master",       18747 },
    { "Grand Master", 26912 }
};
static TrainerTier const SkinningTiers[] =
{
    { "Journeyman",   1292 },
    { "Master",       18755 },
    { "Grand Master", 26913 }
};
static TrainerTier const TailoringTiers[] =
{
    { "Journeyman",   1103 },
    { "Expert",       2627 },
    { "Artisan",      1346 },
    { "Master",       18749 },
    { "Grand Master", 26914 }
};
static TrainerTier const CookingTiers[] =
{
    { "Journeyman",   1355 },
    { "Master",       19186 },
    { "Grand Master", 26905 }
};
static TrainerTier const FirstAidTiers[] =
{
    { "Journeyman",   2326 },
    { "Master",       12920 },
    { "Grand Master", 23734 }
};
static TrainerTier const FishingTiers[] =
{
    { "Journeyman",   1651 },
    { "Grand Master", 26909 }
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

// Gossip action ranges (GOSSIP_ACTION_INFO_DEF == 1000):
//   +100 .. +100+profCount   -> open a profession's tier submenu
//   +200 .. +200+profCount*10 -> summon (profIndex * 10 + tierIndex)
static constexpr uint32 PROFESSION_TIER_MENU_BASE = GOSSIP_ACTION_INFO_DEF + 100;
static constexpr uint32 PROFESSION_SUMMON_BASE = GOSSIP_ACTION_INFO_DEF + 200;

class premium_account : public ItemScript
{
public:
    premium_account() : ItemScript("premium_account") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!sConfigMgr->GetOption<bool>("PremiumAccount", true))
            return false;

        QueryResult result = CharacterDatabase.Query("SELECT `AccountId` FROM `premium` WHERE `active`=1 AND `AccountId`={}", player->GetSession()->GetAccountId());

        if (!result)
            return false;

        if (player->IsInCombat())
            return false;

        float rangeCheck = 10.0f;

        if (player->FindNearestCreature(NPC_AUCTION_A, rangeCheck) ||
            player->FindNearestCreature(NPC_AUCTION_H, rangeCheck) ||
            player->FindNearestCreature(NPC_VENDOR_A, rangeCheck) ||
            player->FindNearestCreature(NPC_VENDOR_H, rangeCheck) ||
            player->FindNearestCreature(ROGUE_A, rangeCheck) ||
            player->FindNearestCreature(WARRIOR_A, rangeCheck) ||
            player->FindNearestCreature(HUNTER_A, rangeCheck) ||
            player->FindNearestCreature(PRIEST_A, rangeCheck) ||
            player->FindNearestCreature(PALADIN_A, rangeCheck) ||
            player->FindNearestCreature(DRUID_A, rangeCheck) ||
            player->FindNearestCreature(SHAMAN_A, rangeCheck) ||
            player->FindNearestCreature(MAGE_A, rangeCheck) ||
            player->FindNearestCreature(WARLOCK_A, rangeCheck) ||
            player->FindNearestCreature(HUNTER_H, rangeCheck) ||
            player->FindNearestCreature(WARRIOR_H, rangeCheck) ||
            player->FindNearestCreature(SHAMAN_H, rangeCheck) ||
            player->FindNearestCreature(PALADIN_H, rangeCheck) ||
            player->FindNearestCreature(ROGUE_H, rangeCheck) ||
            player->FindNearestCreature(WARLOCK_H, rangeCheck) ||
            player->FindNearestCreature(MAGE_H, rangeCheck) ||
            player->FindNearestCreature(PRIEST_H, rangeCheck) ||
            player->FindNearestCreature(DRUID_H, rangeCheck) ||
            player->FindNearestCreature(DEATHKNIGHT_AH, rangeCheck))
            return false;

        ClearGossipMenuFor(player);

        if (sConfigMgr->GetOption<bool>("Morph", true))
        {
            AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_MORPH, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_DEMORPH, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        }

        if (sConfigMgr->GetOption<bool>("Mount", true))
            AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_MOUNT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);

        if (sConfigMgr->GetOption<bool>("Trainers", true))
            AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_TRAIN_ME, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);

        if (sConfigMgr->GetOption<bool>("ProfessionTrainers", true))
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Profession Trainers", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);

        if (sConfigMgr->GetOption<bool>("PlayerInteraction", true))
            AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_PLAYER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);

        SendGossipMenuFor(player, PREMIUM_MENU_TEXT, item->GetGUID());
        return false; // Cast the spell on use normally
    }

    void OnGossipSelect(Player* player, Item* item, uint32 /*sender*/, uint32 action) override
    {
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1: /*Morph*/
            {
                CloseGossipMenuFor(player);
                ApplyRandomMorph(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 2: /*Demorph*/
            {
                player->DeMorph();
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 3: /*Show Bank*/
            {
                player->GetSession()->SendShowBank(player->GetGUID());
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 4: /*Mail Box*/
            {
                player->GetSession()->SendShowMailBox(player->GetGUID());
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 5: /*Vendor*/
            {
                uint32 vendorId = 0;
                std::string salute;

                if (player->GetTeamId() == TEAM_ALLIANCE)
                {
                    vendorId = NPC_VENDOR_A;
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
                        salute = "Greetings";
                        break;
                    }
                    case LOCALE_esES:
                    case LOCALE_esMX:
                    {
                        salute = "Saludos.";
                    }
                    default:
                        break;
                    }
                }
                else
                {
                    vendorId = NPC_VENDOR_H;
                    salute = "Zug zug";
                }

                SummonTempNPC(player, vendorId, salute.c_str());
                CloseGossipMenuFor(player);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 6: /*Mount*/
            {
                CloseGossipMenuFor(player);
                switch (player->getRace())
                {
                    case RACE_HUMAN:         player->CastSpell(player, HUMAN_MOUNT); break;
                    case RACE_ORC:           player->CastSpell(player, ORC_MOUNT); break;
                    case RACE_GNOME:         player->CastSpell(player, GNOME_MOUNT); break;
                    case RACE_NIGHTELF:      player->CastSpell(player, NIGHTELF_MOUNT); break;
                    case RACE_DWARF:         player->CastSpell(player, DWARF_MOUNT); break;
                    case RACE_DRAENEI:       player->CastSpell(player, DRAENEI_MOUNT); break;
                    case RACE_UNDEAD_PLAYER: player->CastSpell(player, UNEAD_MOUNT); break;
                    case RACE_TAUREN:        player->CastSpell(player, TAUREN_MOUNT); break;
                    case RACE_TROLL:         player->CastSpell(player, TROLL_MOUNT); break;
                    case RACE_BLOODELF:      player->CastSpell(player, BLOODELF_MOUNT); break;
                }
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

                if (sConfigMgr->GetOption<bool>("Vendor", true))
                    AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_VENDOR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);

                if (sConfigMgr->GetOption<bool>("MailBox", true))
                    AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_MAIL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

                if (sConfigMgr->GetOption<bool>("Bank", true))
                    AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_BANK, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

                if (sConfigMgr->GetOption<bool>("Auction", true))
                    AddGossipItemFor(player, PREMIUM_MENU, GOSSIP_AUCTION_HOUSE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);

                SendGossipMenuFor(player, PREMIUM_MENU_TEXT, item->GetGUID());
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 10: /*Profession Trainers*/
            {
                ClearGossipMenuFor(player);

                for (uint32 i = 0; i < ProfessionTrainerCount; ++i)
                    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, ProfessionTrainerList[i].name, GOSSIP_SENDER_MAIN, PROFESSION_TIER_MENU_BASE + i);

                SendGossipMenuFor(player, PREMIUM_MENU_TEXT, item->GetGUID());
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

                    SendGossipMenuFor(player, PREMIUM_MENU_TEXT, item->GetGUID());
                }
                else if (action >= PROFESSION_SUMMON_BASE && action < PROFESSION_SUMMON_BASE + ProfessionTrainerCount * 10)
                {
                    uint32 code = action - PROFESSION_SUMMON_BASE;
                    uint32 profIndex = code / 10;
                    uint32 tierIndex = code % 10;

                    if (tierIndex < ProfessionTrainerList[profIndex].tierCount)
                    {
                        SummonTempNPC(player, ProfessionTrainerList[profIndex].tiers[tierIndex].entry);
                        CloseGossipMenuFor(player);
                    }
                }
                break;
            }
        }
    }

    void ApplyRandomMorph(Player* player)
    {
        uint32 random = (urand(1, 26)); // Change this line when adding more morphs
        {
            switch (random)
            {
                case 1: player->SetDisplayId(10134); break;     // Troll Female                 'Orb of Deception'
                case 2: player->SetDisplayId(10135); break;     // Troll Male                   'Orb of Deception'
                case 3: player->SetDisplayId(10136); break;     // Tauren Male                  'Orb of Deception'
                case 4: player->SetDisplayId(10137); break;     // Human Male                   'Orb of Deception'
                case 5: player->SetDisplayId(10138); break;     // Human Female                 'Orb of Deception'
                case 6: player->SetDisplayId(10139); break;     // Orc Male                     'Orb of Deception'
                case 7: player->SetDisplayId(10140); break;     // Orc Female                   'Orb of Deception'
                case 8: player->SetDisplayId(10141); break;     // Dwarf Male                   'Orb of Deception'
                case 9: player->SetDisplayId(10142); break;     // Dwarf Female                 'Orb of Deception'
                case 10: player->SetDisplayId(10143); break;    // NightElf Male                'Orb of Deception'
                case 11: player->SetDisplayId(10144); break;    // NightElf Female              'Orb of Deception'
                case 12: player->SetDisplayId(10145); break;    // Undead Female                'Orb of Deception'
                case 13: player->SetDisplayId(10146); break;    // Undead Male                  'Orb of Deception'
                case 14: player->SetDisplayId(10147); break;    // Tauren Female                'Orb of Deception'
                case 15: player->SetDisplayId(10148); break;    // Gnome Male                   'Orb of Deception'
                case 16: player->SetDisplayId(10149); break;    // Gnome Female                 'Orb of Deception'
                case 17: player->SetDisplayId(4527); break;     // Thrall                       'Orgrimmar Boss'
                case 18: player->SetDisplayId(11657); break;    // Lady Sylvanas                'Undercity Boss'
                case 19: player->SetDisplayId(4307); break;     // Cairne Bloodhoof             'Thunderbluff Boss'
                case 20: player->SetDisplayId(17122); break;    // Lor'themar Theron            'Silvermoon City Boss'
                case 21: player->SetDisplayId(3597); break;     // King Magni Bronzebeard       'Ironforge Boss'
                case 22: player->SetDisplayId(5566); break;     // Highlord Bolvar Fordragon    'Stormwind Boss'
                case 23: player->SetDisplayId(7006); break;     // High Tinker Mekkatorque      'Gnomer Boss'
                case 24: player->SetDisplayId(7274); break;     // Tyrande Whisperwind          'Darnassus Boss'
                case 25: player->SetDisplayId(21976); break;    // Arthus Small                 'Arthus'
                case 26: player->SetDisplayId(24641); break;    // Arthus Ghost                 'Arthus Ghost'

                default:
                    break;
            }
        }
    }

    void SummonTempNPC(Player* player, uint32 entry, const char* salute = "")
    {
        if (!player || entry == 0)
            return;

        int npcDuration = sConfigMgr->GetOption<int32>("Premium.NpcDuration", 60) * IN_MILLISECONDS;
        if (npcDuration <= 0) // Safeguard
            npcDuration = 60;

        Creature* npc = player->SummonCreature(entry, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, npcDuration);
        npc->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        npc->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, player->GetFollowAngle());
        npc->SetFaction(player->GetFaction());

        if (salute && !(salute[0] == '\0'))
            npc->Whisper(salute, LANG_UNIVERSAL, player, false);
    }
};

void AddPremiumAccount()
{
    new premium_account();
}
