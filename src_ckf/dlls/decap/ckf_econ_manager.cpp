// contains all the economy shit that the weapons use
// this initially was based off the tf2 one, but it kinda bran
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "ckf_econ_manager.h"

CEconAttributeMgr* TheEconManager = NULL;

static void ValidateItemDefs(void)
{
    // jakulo REALLY wanted to add this for some reason
    // seems kinda pointless JFL
    // todo: gate this behind developer cvar

    int errors = 0;
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        const ItemDef& def = g_ItemDefs[i];

        if (def.iIndex != i)
        {
            ALERT(at_console, "[Econ] g_ItemDefs[%d] has iIndex %d (entries out of order?)\n", i, def.iIndex);
            errors++;
        }

        if (i == ITEM_NULL)
            continue;

        if (!def.szBaseClass || !def.szBaseClass[0])
        {
            ALERT(at_console, "[Econ] item %d has no base class\n", i);
            errors++;
        }

        if (!def.iAllowedClasses)
        {
            ALERT(at_console, "[Econ] item %d (%s) has no allowed classes\n", i, def.szBaseClass);
            errors++;
        }
    }
    for (int iClass = CLASS_SCOUT; iClass <= CLASS_SPY; iClass++)
    {
        for (int slot = 0; slot < MAX_WEAPON_SLOTS; slot++)
        {
            int iItem = g_StockLoadout[iClass][slot];
            if (iItem == ITEM_NULL)
                continue;

            if (iItem < 0 || iItem >= ITEM_COUNT)
            {
                ALERT(at_console, "[Econ] stock loadout class %d slot %d has invalid item %d\n", iClass, slot, iItem);
                errors++;
                continue;
            }

            if (!(g_ItemDefs[iItem].iAllowedClasses & ClassToAllowedBit(iClass)))
            {
                ALERT(at_console, "[Econ] stock item %d (%s) is not allowed for class %d\n",
                    iItem, g_ItemDefs[iItem].szBaseClass, iClass);
                errors++;
            }
        }
    }

    if (errors)
        ALERT(at_console, "[Econ] %d item table error(s), fix them before testing loadouts\n", errors);
}

const int g_StockLoadout[CLASS_SPY + 1][MAX_WEAPON_SLOTS] =
{
    { 0 },
    { ITEM_SCATTERGUN, ITEM_PISTOL, ITEM_BAT },
    { ITEM_MINIGUN, ITEM_SHOTGUN, ITEM_FISTS },
    { ITEM_ROCKETLAUNCHER, ITEM_SHOTGUN, ITEM_SHOVEL },
    { ITEM_FLAMETHROWER, ITEM_SHOTGUN, ITEM_FIREAXE },
    { ITEM_SNIPERRIFLE, ITEM_SMG, ITEM_KUKRI },
    { ITEM_SYRINGEGUN, ITEM_MEDIGUN, ITEM_BONESAW },
    { ITEM_SHOTGUN, ITEM_PISTOL, ITEM_WRENCH, ITEM_BUILDPDA, ITEM_DESTROYPDA },
    { ITEM_GRENADELAUNCHER, ITEM_STICKYBOMBLAUNCHER, ITEM_BOTTLE },
    { ITEM_REVOLVER, ITEM_SAPPER, ITEM_BUTTERFLY, ITEM_DISGUISEKIT },
    // these are basedd off the class enums so LEAVE THIS ALONE!!!!!!!!!!!!!!
};

ItemDef g_ItemDefs[ITEM_COUNT] =
{
    { ITEM_NULL,              "",                        {}, 0 },
    { ITEM_SCATTERGUN,        "weapon_scattergun",       {}, ALLOWED_SCOUT },
    { ITEM_PISTOL,            "weapon_pistol",           {}, ALLOWED_SCOUT | ALLOWED_ENGINEER },
    { ITEM_BAT,               "weapon_bat",              {}, ALLOWED_SCOUT },
    { ITEM_ROCKETLAUNCHER,    "weapon_rocketlauncher",   {}, ALLOWED_SOLDIER },
    { ITEM_SHOTGUN,           "weapon_shotgun",          {}, ALLOWED_SOLDIER | ALLOWED_PYRO | ALLOWED_HEAVY | ALLOWED_ENGINEER },
    { ITEM_SHOVEL,            "weapon_shovel",           {}, ALLOWED_SOLDIER },
    { ITEM_FLAMETHROWER,      "weapon_flamethrower",     {}, ALLOWED_PYRO },
    { ITEM_FIREAXE,           "weapon_fireaxe",          {}, ALLOWED_PYRO },
    { ITEM_GRENADELAUNCHER,   "weapon_grenadelauncher",  {}, ALLOWED_DEMOMAN },
    { ITEM_STICKYBOMBLAUNCHER,"weapon_stickylauncher",   {}, ALLOWED_DEMOMAN },
    { ITEM_BOTTLE,            "weapon_bottle",           {}, ALLOWED_DEMOMAN },
    { ITEM_MINIGUN,           "weapon_minigun",          {}, ALLOWED_HEAVY },
    { ITEM_FISTS,             "weapon_fist",             {}, ALLOWED_HEAVY },
    { ITEM_WRENCH,            "weapon_wrench",           {}, ALLOWED_ENGINEER },
    { ITEM_BUILDPDA,          "weapon_buildpda",         {}, ALLOWED_ENGINEER },
    { ITEM_DESTROYPDA,        "weapon_destroypda",       {}, ALLOWED_ENGINEER },
    { ITEM_SYRINGEGUN,        "weapon_syringegun",       {}, ALLOWED_MEDIC },
    { ITEM_MEDIGUN,           "weapon_medigun",          {}, ALLOWED_MEDIC },
    { ITEM_BONESAW,           "weapon_bonesaw",          {}, ALLOWED_MEDIC },
    { ITEM_SNIPERRIFLE,       "weapon_sniperifle",       {}, ALLOWED_SNIPER },
    { ITEM_SMG,               "weapon_smg",              {}, ALLOWED_SNIPER },
    { ITEM_KUKRI,             "weapon_kukri",            {}, ALLOWED_SNIPER },
    { ITEM_REVOLVER,          "weapon_revolver",         {}, ALLOWED_SPY },
    { ITEM_SAPPER,            "weapon_sapper",           {}, ALLOWED_SPY },
    { ITEM_BUTTERFLY,         "weapon_butterfly",        {}, ALLOWED_SPY },
    { ITEM_DISGUISEKIT,       "weapon_disguisekit",      {}, ALLOWED_SPY },
};
static_assert(ARRAYSIZE(g_ItemDefs) == ITEM_COUNT, "g_ItemDefs out of sync with enum");
// this is equal to items_game.txt in tf2
// i WOULD make a txt file, but im too stupid to know how to work that
// only reason nav file reading works is bcuz of valve doing it for me LOLOLOL i suck with file shit
// downside is that this hardcodes all the files. however, this is probably for the best anyway

void InstallEconomyManager(void)
{
    if (TheEconManager == NULL)
    {
        TheEconManager = new CEconAttributeMgr;
        // TheEconManager->AddServerCommands();
        ValidateItemDefs();
    }
}

float CEconAttributeMgr::GetAttributeValue(const CBasePlayerWeapon* pWeapon, int iAttrID, float flDefault)
{
    const ItemDef* pDef = GetItemDef(pWeapon->m_iItemIndex);
    if (!pDef)
        return flDefault;

    int attribLength = ARRAYSIZE(pDef->attrs);
    if (!attribLength)
        return flDefault;

    for (int i = 0; i < attribLength; i++)
    {
        if (pDef->attrs[i].iAttrID == iAttrID)
            return pDef->attrs[i].flValue;
    }
    return flDefault;
}

int CEconAttributeMgr::GetAttributeInt(const CBasePlayerWeapon* pWeapon, int iAttrID, int iDefault)
{
    const ItemDef* pDef = GetItemDef(pWeapon->m_iItemIndex);
    if (!pDef)
        return iDefault;

    int attribLength = ARRAYSIZE(pDef->attrs);
    if (!attribLength)
        return iDefault;

    for (int i = 0; i < attribLength; i++)
    {
        if (pDef->attrs[i].iAttrID == iAttrID)
            return (int)pDef->attrs[i].flValue;
    }
    return iDefault;
}

const ItemDef* CEconAttributeMgr::GetItemDefFor(const CBasePlayerWeapon* pWeapon)
{
	return GetItemDef(pWeapon->m_iItemIndex);
}

const ItemDef* CEconAttributeMgr::GetItemDef(int iIndex)
{
    if (iIndex <= ITEM_NULL || iIndex >= ARRAYSIZE(g_ItemDefs))
        return nullptr;
    const ItemDef* pDef = &g_ItemDefs[iIndex];
    return (pDef->iIndex == iIndex) ? pDef : nullptr;
}