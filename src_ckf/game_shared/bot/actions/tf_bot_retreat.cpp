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

// collect nearby areas that provide cover from threats
class CSearchForCover
{
public:
	CSearchForCover(CBot* me)
	{
		// THIS ASSUMES THAT WE ARE HIDING FROM ENEMIES
		m_me = me;

		FOR_EACH_VEC(me->m_knownEntityVector, iter)
		{
			CKnownEntity& known = me->m_knownEntityVector.Element(iter);

			if (known.GetTeam() != m_me->m_iTeam && known.GetEntity() && known.GetEntity()->IsPlayer())
			{
				CBasePlayer* enemy = (CBasePlayer*)known.GetEntity();
				if (enemy && enemy->IsAlive())
				{
					if (enemy->IsPlayerDisguised() && enemy->m_iDisguiseTeam == m_me->m_iTeam)
						continue;

					if (enemy->IsPlayerCloaked())
						continue;

					m_hideFromTheseEntities.AddToTail(enemy);
				}
			}
		}

		m_viability = NOT_VIABLE;

		m_numFound = 0;
	}
	enum { MAX_SPOTS = 256 };

	enum Viability
	{
		NOT_VIABLE = 0,		// enemy can see this point
		KINDA_VIABLE = 1,	// enemy can technically see this point but it is outside their fov
		VIABLE = 2,			// enemy cannot see this point
		VERY_VIABLE = 3,	// enemy cannot see this point and it is far away
	};

	bool operator() (CNavArea* area)
	{
		if (m_hideFromTheseEntities.Count() == 0 || ++m_numFound > MAX_SPOTS)
			return false;
		// STOP COUNTING

		Viability areaTotalViability = VERY_VIABLE;
		FOR_EACH_VEC(m_hideFromTheseEntities, iter)
		{
			CBasePlayer* enemy = m_hideFromTheseEntities.Element(iter);
			Vector point = *area->GetCenter();
			point.z += HalfHumanHeight;

			bool enemyVisible = enemy->FVisible(point);
			bool enemyInFov = enemy->FInViewCone(&point);

			int v = (!enemyVisible ? 2 : 0) + (!enemyInFov ? 1 : 0);
			Viability newViability = (Viability)((((v) < (2)) ? (v) : (2)));

			if (newViability == VIABLE)
			{
				if ((enemy->pev->origin - point).IsLengthGreaterThan(1024.0f))
					newViability = VERY_VIABLE;
			}

			if (newViability < areaTotalViability)
				areaTotalViability = newViability;
			// this area is considered as bad as its worse viable area
			// eg: 9/10 players cant see it, making it viable. however, one player can see it, making it not_viable
			// since someone can see it, it is considered a not_viable area
		}

		if (areaTotalViability > m_viability)
		{
			// this new area is considered more viable than the set of old ones, so start a new set at this level
			m_viability = areaTotalViability;
			m_coverAreaVector.RemoveAll();
		}

		if (areaTotalViability == m_viability) // adds the new area to it. ignores worse ones LOLOL
			m_coverAreaVector.AddToTail(area);

		return true;
	}

	CBot* m_me;
	CUtlVector< CBasePlayer* > m_hideFromTheseEntities;
	CUtlVector< CNavArea* > m_coverAreaVector;
	Viability m_viability;
	int m_numFound;
};

void CTFBotRetreat::OnEnter(CBot* me)
{
	m_cover = NULL;
	m_recomputeRetreatSpot.Invalidate();
	m_recomputeCoverArea.Invalidate();
}

void CTFBotRetreat::Update(CBot* me)
{
	if (!me || !me->m_lastNavArea)
		return;

	bool needCover = false;

	if (m_recomputeRetreatSpot.IsElapsed())
	{
		m_recomputeRetreatSpot.Start(RANDOM_FLOAT(5.0f, 10.0f));

		Vector center = me->Center();
		const Vector* retreat = FindNearbyRetreatSpot(me, &center, me->m_lastNavArea, 1024.0f, me->GetEnemyTeam(), true);
		if (retreat)
		{
			m_cover = NULL;
			Vector goal = *retreat;
			me->SetPathToGoal(goal);
		}
		else
			needCover = true;
	}
	else if (m_cover && me->m_lastNavArea == m_cover && m_recomputeCoverArea.IsElapsed())
	{
		needCover = true;
	}

	if (needCover)
	{
		m_recomputeCoverArea.Start(RANDOM_FLOAT(5.0f, 10.0f));
		m_cover = NULL;

		CSearchForCover cover(me);
		const Vector* pos = &me->pev->origin;
		SearchSurroundingAreas(me->m_lastNavArea, pos, cover, 1024.0f);

		if (cover.m_viability > CSearchForCover::NOT_VIABLE && cover.m_coverAreaVector.Count() > 0)
		{
			CNavArea* best = NULL;
			float bestDist = FLT_MAX;
			FOR_EACH_VEC(cover.m_coverAreaVector, i)
			{
				float d = (*cover.m_coverAreaVector[i]->GetCenter() - me->pev->origin).LengthSquared();
				if (d < bestDist)
				{
					bestDist = d;
					best = cover.m_coverAreaVector[i];
				}
			}

			m_cover = best;

			if (m_cover) // juuuuuuuuuust in case
			{
				Vector goal = *m_cover->GetCenter();
				me->SetPathToGoal(goal);
			}
		}
		// TODO: find spawn room if no options
	}

	Continue();
}

void CTFBotRetreat::OnExit(CBot* me)
{
}

void CTFBotRetreat::OnResume(CBot* me)
{
}