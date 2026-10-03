// go attack the points

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "bot/hl_bot.h"
#include "bot/actions/tf_bot_control_point_attack.h"
#include "bot/actions/tf_bot_seek_and_destroy.h"

void CTFBotControlPointAttack::OnEnter(CBot* me)
{
}

void CTFBotControlPointAttack::Update(CBot* me)
{
	if (g_pGameRules->m_iRoundStatus == ROUND_END)
	{
		if (g_pGameRules->m_iRoundWinStatus != me->m_iTeam)
		{
			Suspend_For(new CTFBotSeekAndDestroy, "We lost!");
		}
		else
		{
			// winners
			Suspend_For(new CTFBotSeekAndDestroy, "Look for losers to kill!!");
		}
		return;
	}

	CControlPoint* controlpoint = GetClosestControlPoint(me);

	if (controlpoint)
	{
		if ( !(me->Center() < controlpoint->pev->absmax && me->Center() > controlpoint->pev->absmin) ) // if not already inside the point area
		{
			Vector point = controlpoint->Center();
			point.x = RANDOM_FLOAT(controlpoint->pev->absmax.x, controlpoint->pev->absmin.x);
			point.y = RANDOM_FLOAT(controlpoint->pev->absmax.y, controlpoint->pev->absmin.y);
			point.z = controlpoint->Center().z;

			me->SetPathToGoal(point);
		}
	}
	else
	{
		Suspend_For(new CTFBotSeekAndDestroy, "No control points to attack!");
		return;
	}

	Continue();
}

void CTFBotControlPointAttack::OnExit(CBot* me)
{
}

void CTFBotControlPointAttack::OnResume(CBot* me)
{
}

CControlPoint* CTFBotControlPointAttack::GetClosestControlPoint(CBot* me)
{
	CControlPoint* pBest = NULL;
	float bestDist = FLT_MAX;
	for (int i = 0; i < g_pGameRules->m_ControlPoints.Count(); i++)
	{
		if (!g_pGameRules->m_ControlPoints.IsValidIndex(i))
			break; // idk if this matters kjnegjthjbgrkjklwrgwjwfkjlwfjnlwjfejwkefjlfe

		CControlPoint* pPoint = (CControlPoint*)CBaseEntity::Instance(g_pGameRules->m_ControlPoints.Element(i));

		if (!pPoint)
			continue;

		if (pPoint->m_bLocked || pPoint->m_bDisabled)
			continue;

		if (pPoint->pev->team == me->m_iTeam)
			continue;
		// we already own this point

		if ((me->m_iTeam == TEAM_RED && !pPoint->m_bCanRedCap) || (me->m_iTeam == TEAM_BLUE && !pPoint->m_bCanBluCap))
			continue;

		float dist = (pPoint->pev->origin - me->pev->origin).LengthSquared();
		if (dist < bestDist)
		{
			bestDist = dist;
			pBest = pPoint;
		}
	}

	return pBest;
}