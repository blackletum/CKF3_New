// tf_bot_get_health

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "bot/hl_bot.h"
#include "bot/actions/tf_bot_get_ammo.h"

inline int FNullEnt(CBaseEntity* ent) { return ((!ent) || FNullEnt(ent->edict())); }

void CTFBotGetAmmo::OnEnter(CBot* me)
{
}

void CTFBotGetAmmo::Update(CBot* me)
{
	if (!me)
		return;

	CBaseEntity* pSpot = NULL;
	while (pSpot = UTIL_FindEntityByClassname(pSpot, "item_ammobox"))
	{
		if (!FNullEnt(pSpot))
		{
			me->SetPathToGoal(pSpot->pev->origin);
			break;
		}
	}
	if (FNullEnt(pSpot))
	{
		while (pSpot = UTIL_FindEntityByClassname(pSpot, "func_resupplycase"))
		{
			if (!FNullEnt(pSpot))
			{
				me->SetPathToGoal(pSpot->pev->origin);
				break;
			}
		}
	}

	Continue();
}

void CTFBotGetAmmo::OnExit(CBot* me)
{
}

void CTFBotGetAmmo::OnResume(CBot* me)
{
}