#ifndef CKFECONMANAGER
#define CKFECONMANAGER

// economy shit

class CBasePlayerWeapon;

enum
{
    ITEM_NULL, // dont use this
    ITEM_SCATTERGUN,
    ITEM_PISTOL,
    ITEM_BAT,
    ITEM_ROCKETLAUNCHER,
    ITEM_SHOTGUN,
    ITEM_SHOVEL,
    ITEM_FLAMETHROWER,
    ITEM_FIREAXE,
    ITEM_GRENADELAUNCHER,
    ITEM_STICKYBOMBLAUNCHER,
    ITEM_BOTTLE,
    ITEM_MINIGUN,
    ITEM_FISTS,
    ITEM_WRENCH,
    ITEM_BUILDPDA,
    ITEM_DESTROYPDA,
    ITEM_SYRINGEGUN,
    ITEM_MEDIGUN,
    ITEM_BONESAW,
    ITEM_SNIPERRIFLE,
    ITEM_SMG,
    ITEM_KUKRI,
    ITEM_REVOLVER,
    ITEM_SAPPER,
    ITEM_BUTTERFLY,
    ITEM_DISGUISEKIT,

    ITEM_COUNT
};

enum AllowedClasses
{
    ALLOWED_SCOUT       = (1 << 0),
    ALLOWED_SOLDIER     = (1 << 1),
    ALLOWED_PYRO        = (1 << 2),
    ALLOWED_DEMOMAN     = (1 << 3),
    ALLOWED_HEAVY       = (1 << 4),
    ALLOWED_ENGINEER    = (1 << 5),
    ALLOWED_MEDIC       = (1 << 6),
    ALLOWED_SNIPER      = (1 << 7),
    ALLOWED_SPY         = (1 << 8),
};

inline int ClassToAllowedBit(int iClass)
{
    switch (iClass)
    {
    case CLASS_SCOUT:    return ALLOWED_SCOUT;
    case CLASS_SOLDIER:  return ALLOWED_SOLDIER;
    case CLASS_PYRO:     return ALLOWED_PYRO;
    case CLASS_DEMOMAN:  return ALLOWED_DEMOMAN;
    case CLASS_HEAVY:    return ALLOWED_HEAVY;
    case CLASS_ENGINEER: return ALLOWED_ENGINEER;
    case CLASS_MEDIC:    return ALLOWED_MEDIC;
    case CLASS_SNIPER:   return ALLOWED_SNIPER;
    case CLASS_SPY:      return ALLOWED_SPY;
    }
    return 0;
}

enum
{
    ATK_NULL = 0,
    ATK_CLIP,
    ATK_DAMAGE,
    ATK_FIRE_RATE,

    CUSTOM_MODEL, // dont do this yet
};

struct WeaponAttr
{
    int iAttrID;
    float flValue;
};

struct ItemDef
{
    int iIndex;                 // unique id. i will eventually make another way of doing this
    const char* szBaseClass;    // base class that the weapon is based on. eg: "weapon_rocketlauncher" for rocket launcher unlocks  
    // const char* szViewmodel;    // custom viewmodel. Not implemented yet
    WeaponAttr attrs[8];        // eg: {ATK_DAMAGE, 1.2}, {ATK_CLIP, -2}
    int iAllowedClasses;
};

extern ItemDef   g_ItemDefs[ITEM_COUNT];
extern const int g_StockLoadout[CLASS_SPY + 1][MAX_WEAPON_SLOTS];

class CEconAttributeMgr
{
public:
    float GetAttributeValue(const CBasePlayerWeapon* pWeapon, int iAttrID, float flDefault);
    int GetAttributeInt(const CBasePlayerWeapon* pWeapon, int iAttrID, int iDefault);
    const ItemDef* GetItemDefFor(const CBasePlayerWeapon* pWeapon);
    const ItemDef* GetItemDef(int iIndex);
};

/*
// in a virtual or where stats are consumed
float GetDamage()     { return g_EconMgr.GetAttributeValue(this, ATK_DAMAGE, m_flBaseDamage); }
int   iMaxClip()      { return g_EconMgr.GetAttributeInt(this, ATK_CLIP, m_iMaxClip); }
float GetFireRate()   { return g_EconMgr.GetAttributeValue(this, ATK_FIRE_RATE, 1.0f); }
*/

void InstallEconomyManager(void);

extern CEconAttributeMgr* TheEconManager;

#endif // !CKFECONMANAGER
