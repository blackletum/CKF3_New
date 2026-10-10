// tf_bot_get_health

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "bot/hl_bot.h"
#include "bot/actions/tf_bot_get_health.h"

inline int FNullEnt(CBaseEntity* ent) { return ((!ent) || FNullEnt(ent->edict())); }

void CTFBotGetHealth::OnEnter(CBot* me)
{
}

void CTFBotGetHealth::Update(CBot* me)
{
	if (!me)
		return;

	CBaseEntity* pSpot = UTIL_FindEntityByClassname(NULL, "item_healthbox");
	if (!FNullEnt(pSpot))
	{
		me->SetPathToGoal(pSpot->Center());
	}

	Continue();
}

void CTFBotGetHealth::OnExit(CBot* me)
{
}

void CTFBotGetHealth::OnResume(CBot* me)
{
}