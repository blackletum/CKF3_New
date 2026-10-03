// go defend the points

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "bot/hl_bot.h"
#include "bot/actions/tf_bot_control_point_defend.h"
#include "bot/actions/tf_bot_seek_and_destroy.h"

void CTFBotControlPointDefend::OnEnter(CBot* me)
{
	m_seekTimer.Invalidate();
}

void CTFBotControlPointDefend::Update(CBot* me)
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
		Vector point = controlpoint->Center();
		point.x = RANDOM_FLOAT(controlpoint->pev->absmax.x, controlpoint->pev->absmin.x);
		point.y = RANDOM_FLOAT(controlpoint->pev->absmax.y, controlpoint->pev->absmin.y);
		point.z = controlpoint->Center().z;
		m_seekTimer.Invalidate();
		me->SetPathToGoal(point);

		if ((me->Center() - controlpoint->Center()).IsLengthLessThan( (controlpoint->pev->absmax - controlpoint->pev->absmin).Length() / 2.0f ))
		{
			// inside point area already

			if (me->m_iClass == CLASS_ENGINEER && !(me->m_pBuildable[BUILDABLE_SENTRY - 1]))// no sentry
			{
				m_seekTimer.Invalidate();

				CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)me->m_pActiveItem;

				if (!pWeapon)
				{
					return;
				}
				// this should never happen

				if (pWeapon->m_iId == WEAPON_BUILDPDA && me->m_iCarryBluePrint)
				{
					// holding blueprint, fire it to build
					me->PressPrimaryAttack();
				}
				else
				{
					if (pWeapon->m_iId == WEAPON_BUILDPDA)
						me->Build_Start(BUILDABLE_SENTRY);
					else
						me->SwitchSlotWeapon(WEAPON_SLOT_PDA);
				}
			}
			else
			{
				if (!m_seekTimer.HasStarted())
				{
					m_seekTimer.Start(RANDOM_FLOAT(1.0f, 10.0f));
				}
				else if (m_seekTimer.IsElapsed())
				{
					Suspend_For(new CTFBotSeekAndDestroy(RANDOM_FLOAT(10.0f, 30.0f)), "Bored. Looking for kills!");
					return;
				}
			}
		}
	}
	else
	{
		Suspend_For(new CTFBotSeekAndDestroy(), "No control points to attack!");
		return;
	}

	Continue();
}

void CTFBotControlPointDefend::OnExit(CBot* me)
{
	m_seekTimer.Invalidate();
}

void CTFBotControlPointDefend::OnResume(CBot* me)
{
	m_seekTimer.Invalidate();
}

CControlPoint* CTFBotControlPointDefend::GetClosestControlPoint(CBot* me)
{
	CControlPoint* pBest = NULL;
	float bestDist = FLT_MAX;
	bool sameteam = false;
	for (int i = 0; i < g_pGameRules->m_ControlPoints.Count(); i++)
	{
		if (!g_pGameRules->m_ControlPoints.IsValidIndex(i))
			break; // idk if this matters kjnegjthjbgrkjklwrgwjwfkjlwfjnlwjfejwkefjlfe

		CControlPoint* pPoint = (CControlPoint*)CBaseEntity::Instance(g_pGameRules->m_ControlPoints.Element(i));

		if (!pPoint)
			continue;

		if (pPoint->m_bLocked || pPoint->m_bDisabled)
			continue;

		if (sameteam && (pPoint->pev->team != me->m_iTeam) ) // if we have found a defendable cap point that's owned by us, then filter all others that do not ( blue/neutral )
			continue;

		if ((me->m_iTeam == TEAM_RED && !pPoint->m_bCanBluCap) || (me->m_iTeam == TEAM_BLUE && !pPoint->m_bCanRedCap))
			continue;

		float dist = (pPoint->pev->origin - me->pev->origin).LengthSquared();
		if (dist < bestDist)
		{
			bestDist = dist;
			pBest = pPoint;
			sameteam = (pPoint->pev->team == me->m_iTeam);
		}
	}

	return pBest;
}