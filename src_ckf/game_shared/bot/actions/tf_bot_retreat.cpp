// tf_bot_retreat

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "bot/hl_bot.h"
#include "bot/actions/tf_bot_retreat.h"

inline int FNullEnt(CBaseEntity* ent) { return ((!ent) || FNullEnt(ent->edict())); }

void CTFBotRetreat::OnEnter(CBot* me)
{
}

void CTFBotRetreat::Update(CBot* me)
{
	if (!me)
		return;
	
	Vector center = me->Center();
	const Vector* retreat = FindNearbyRetreatSpot(me, &center, me->m_lastNavArea, 1024.0f, me->GetEnemyTeam(), true);

	if (retreat)
	{
		Vector goal = *retreat;
		me->SetPathToGoal(goal);
	}
	else
	{
		// fallback
		// todo: let the bot go back to spawn room if possible
		me->SetPathToGoal(center);
	}

	Continue();
}

void CTFBotRetreat::OnExit(CBot* me)
{
}

void CTFBotRetreat::OnResume(CBot* me)
{
}