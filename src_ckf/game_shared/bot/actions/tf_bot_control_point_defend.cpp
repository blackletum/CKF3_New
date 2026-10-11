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
				if (me->m_iClass == CLASS_HEAVY)
				{
					CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)me->m_pActiveItem;
					
					if(pWeapon->m_iId == WEAPON_MINIGUN)
						me->PressSecondaryAttack();
				}

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
		Suspend_For(new CTFBotSeekAndDestroy(), "No control points to defend!");
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
	float bestDist = -1.0f;

	// ALERT(at_console, "CP count: %d\n", g_pGameRules->m_ControlPoints.Count());
	for (int i = 0; i < g_pGameRules->m_ControlPoints.Count(); ++i)
	{
		//if (!g_pGameRules->m_ControlPoints.IsValidIndex(i))
			//break;

		CControlPoint* pPoint = (CControlPoint*)CBaseEntity::Instance(g_pGameRules->m_ControlPoints.Element(i));

		if (!pPoint)
			continue;

		if (pPoint->m_bLocked || pPoint->m_bDisabled)
			continue;

		if (pPoint->pev->team != me->m_iTeam && pPoint->pev->team != TEAM_UNASSIGNED)
			continue;
		// we do not own this point, so we cant defend it

		if (me->m_iTeam == TEAM_RED && !pPoint->m_bCanBluCap)
			continue;
		if (me->m_iTeam == TEAM_BLU && !pPoint->m_bCanRedCap)
			continue;
		// if the enemy cannot capture this point, then we dont need to defend it

		float dist = (pPoint->pev->origin - me->pev->origin).LengthSquared();
		if (dist < bestDist || bestDist == -1.0f)
		{
			bestDist = dist;
			pBest = pPoint;
		}
	}

	return pBest;
}