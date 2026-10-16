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
extern cvar_t cv_bot_debug;
/*
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
*/

#define RetreatDangerDensity 1000.0f
#define RetreatMaxThreatRange 500.0f
#define RetreatClimbCost 10.0f
#define RetreatMinRepathInterval 0.5f
#define RetreatDebugLife 2.0f

static Vector ClosestPointOnSegment(const Vector& p, const Vector& a, const Vector& b)
{
	Vector ab = b - a;
	float lenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;

	if (lenSq < 0.0001f)
		return a;

	Vector ap = p - a;
	float t = (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / lenSq;

	if (t < 0.0f)
		t = 0.0f;
	else if (t > 1.0f)
		t = 1.0f;

	return a + ab * t;
}

// grabbed from the source sdk's RetreatPathBuilder
class RetreatPathBuilder
{
public:
	RetreatPathBuilder(CBot* me, CBaseEntity* threat, float retreatRange = 500.0f)
	{
		m_me = me;
		// m_mover = me->GetLocomotionInterface();
		m_threat = threat;
		m_retreatRange = retreatRange;

		m_endposition = Vector(0, 0, 0);
		m_endArea = NULL;
		m_foundValidPosition = false;
		m_adjAreaIndex = 0;

		m_foundValidPosition = ComputePath();
	}

	bool ComputePath(void)
	{
		m_endArea = NULL;

		if (!m_me || !m_threat)
			return false;

		CNavArea* startArea = m_me->m_lastNavArea;

		if (!startArea)
			return false;

		CNavArea* retreatFromArea = TheNavAreaGrid.GetNearestNavArea(&m_threat->pev->origin);

		if (retreatFromArea == NULL)
			return false;

		startArea->SetParent(NULL);

		CNavArea::ClearSearchLists();

		float initCost = Cost(startArea, NULL, NULL);
		if (initCost < 0.0f)
			return false;

		startArea->SetTotalCost(initCost);
		startArea->SetCostSoFar(initCost);	// the sdk never sets this, but Cost() reads it from the start area's neighbors

		startArea->AddToOpenList();

		// keep track of the area farthest away from the threat
		CNavArea* farthestArea = NULL;
		float farthestRange = 0.0f;

		//
		// Dijkstra's algorithm (since we don't know our goal).
		// Build a path as far away from the retreat area as possible.
		// Minimize total path length and danger.
		// Maximize distance to threat of end of path.
		//
		while (!CNavArea::IsOpenListEmpty())
		{
			// get next area to check
			CNavArea* area = CNavArea::PopOpenList();

			area->AddToClosedList();

			// don't consider blocked areas
			// if (area->IsBlocked(team))
				// continue;
			// not implemented lolz

			// build adjacent area array
			CollectAdjacentAreas(area);

			// search adjacent areas
			for (int i = 0; i < m_adjAreaIndex; ++i)
			{
				CNavArea* newArea = m_adjAreaVector[i].area;

				// only visit each area once
				if (newArea->IsClosed())
					continue;

				// don't use this area if it is out of range
				if ((*newArea->GetCenter() - m_me->pev->origin).IsLengthGreaterThan(m_retreatRange))
					continue;

				// determine cost of traversing this area
				float newCost = Cost(newArea, area, m_adjAreaVector[i].ladder);

				// don't use adjacent area if cost says it is a dead-end
				if (newCost < 0.0f)
					continue;

				if (newArea->IsOpen() && newArea->GetTotalCost() <= newCost)
				{
					// we have already visited this area, and it has a better path
					continue;
				}

				// whether this area has been visited or not, we now have a better path
				newArea->SetParent(area, m_adjAreaVector[i].how);
				newArea->SetTotalCost(newCost);

				// use 'cost so far' to hold cumulative cost
				newArea->SetCostSoFar(newCost);

				if (newArea->IsOpen())
				{
					newArea->UpdateOnOpenList();
				}
				else
				{
					newArea->AddToOpenList();
				}

				// keep track of area farthest from threat
				float threatRange = (*newArea->GetCenter() - m_threat->pev->origin).Length();
				if (threatRange > farthestRange)
				{
					farthestArea = newArea;
					farthestRange = threatRange;
				}
			}
		}

		if (!farthestArea)
			return false;

		m_endArea = farthestArea;
		m_endposition = *farthestArea->GetCenter();

		return true;
	}

	float Cost(CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder)
	{
		float cost;

		if (fromArea == NULL)
		{
			cost = 0.0f;

			if (area->Contains(&m_threat->pev->origin))
			{
				cost += 10.0f * RetreatDangerDensity;

				if (cv_bot_debug.value != 0.0)
					area->Draw(255, 0, 0, RetreatDebugLife);
			}
			else
			{
				// danger proportional to range to us
				float rangeToThreat = (m_threat->pev->origin - m_me->pev->origin).Length();

				if (rangeToThreat < RetreatMaxThreatRange)
				{
					cost += RetreatDangerDensity * (1.0f - (rangeToThreat / RetreatMaxThreatRange));

					if (cv_bot_debug.value != 0.0)
						UTIL_DrawBeamPoints(m_me->pev->origin, m_threat->pev->origin, RetreatDebugLife, 255, 0, 0);
				}
			}
		}
		else
		{
			// compute distance traveled along path so far
			float dist;

			if (ladder)
			{
				dist = 100.0f * ladder->m_length;
			}
			else
			{
				Vector to = *area->GetCenter() - *fromArea->GetCenter();

				dist = to.Length();

				// check for vertical discontinuities
				Vector closeFrom, closeTo;
				area->GetClosestPointOnArea(fromArea->GetCenter(), &closeTo);
				fromArea->GetClosestPointOnArea(area->GetCenter(), &closeFrom);

				float deltaZ = closeTo.z - closeFrom.z;

				if (deltaZ > JumpCrouchHeight)
				{
					// too high to jump
					return -1.0f;
				}
				else if (-deltaZ > DeathDrop)
				{
					// too far down to drop
					return -1.0f;
				}

				// prefer to maintain our level
				dist += RetreatClimbCost * fabs(deltaZ);
			}

			cost = dist + fromArea->GetTotalCost();

			// add in danger cost due to threat
			// assume straight line between areas

			// path danger is CUMULATIVE
			float dangerCost = fromArea->GetCostSoFar();

			Vector close = ClosestPointOnSegment(m_threat->pev->origin, *area->GetCenter(), *fromArea->GetCenter());

			float rangeToThreat = (m_threat->pev->origin - close).Length();

			if (rangeToThreat < RetreatMaxThreatRange)
			{
				float dangerFactor = 1.0f - (rangeToThreat / RetreatMaxThreatRange);
				dangerCost = RetreatDangerDensity * dangerFactor;

				if (cv_bot_debug.value != 0.0)
				{
					UTIL_DrawBeamPoints(*fromArea->GetCenter(), *area->GetCenter(), RetreatDebugLife, (int)(255 * dangerFactor), 0, 0);

					Vector away = close - m_threat->pev->origin;
					away.NormalizeInPlace();

					UTIL_DrawBeamPoints(close, close - 50.0f * away, RetreatDebugLife, 255, 0, 0);
				}
			}

			cost += dangerCost;
		}

		return cost;
	}

	CBot* m_me;
	float m_retreatRange;
	CBaseEntity* m_threat;

	Vector m_endposition;
	bool m_foundValidPosition;
	CNavArea* m_endArea;

private:
	enum { MAX_ADJ_AREAS = 64 };

	struct AdjInfo
	{
		CNavArea* area;
		const CNavLadder* ladder;
		NavTraverseType how;
	};

	AdjInfo m_adjAreaVector[MAX_ADJ_AREAS];
	int m_adjAreaIndex;

	void AddAdjacentArea(CNavArea* area, NavTraverseType how, const CNavLadder* ladder)
	{
		if (!area || m_adjAreaIndex >= MAX_ADJ_AREAS)
			return;

		m_adjAreaVector[m_adjAreaIndex].area = area;
		m_adjAreaVector[m_adjAreaIndex].how = how;
		m_adjAreaVector[m_adjAreaIndex].ladder = ladder;
		++m_adjAreaIndex;
	}

	void CollectAdjacentAreas(CNavArea* area)
	{
		m_adjAreaIndex = 0;

		// areas we can walk (or drop) to
		for (int dir = 0; dir < NUM_DIRECTIONS; ++dir)
		{
			const NavConnectList* list = area->GetAdjacentList((NavDirType)dir);

			for (NavConnectList::const_iterator iter = list->begin(); iter != list->end(); ++iter)
				AddAdjacentArea((*iter).area, (NavTraverseType)dir, NULL);
		}

		// areas at the top of ladders going up
		const NavLadderList* upList = area->GetLadderList(LADDER_UP);
		for (NavLadderList::const_iterator iter = upList->begin(); iter != upList->end(); ++iter)
		{
			const CNavLadder* ladder = *iter;

			// cannot use this ladder if the ladder bottom is hanging above our head
			if (ladder->m_isDangling)
				continue;

			// do not use BEHIND connection, as its very hard to get to when going up a ladder
			AddAdjacentArea(ladder->m_topForwardArea, GO_LADDER_UP, ladder);
			AddAdjacentArea(ladder->m_topLeftArea, GO_LADDER_UP, ladder);
			AddAdjacentArea(ladder->m_topRightArea, GO_LADDER_UP, ladder);
		}

		// areas at the bottom of ladders going down
		const NavLadderList* downList = area->GetLadderList(LADDER_DOWN);
		for (NavLadderList::const_iterator iter = downList->begin(); iter != downList->end(); ++iter)
		{
			const CNavLadder* ladder = *iter;

			AddAdjacentArea(ladder->m_bottomArea, GO_LADDER_DOWN, ladder);
		}
	}
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

	// bool needCover = false;

	if (m_recomputeRetreatSpot.IsElapsed())
	{
		m_recomputeRetreatSpot.Start(RANDOM_FLOAT(0.5f, 2.0f));

		float bestDist = FLT_MAX;
		CBaseEntity* best = NULL;

		// run away from our enemy if there is somewhere to run to
		bool retreating = false;
		CBaseEntity* threat = me->GetEnemy();
		if (threat)
		{
			RetreatPathBuilder retreat(me, threat, 500.0f);
			if (retreat.m_foundValidPosition && me->MakePath(retreat.m_endArea))
				retreating = true;
			else
			{
				m_recomputeRetreatSpot.Invalidate();
				me->StopPath();
			}
		}
		// nothing to run from (or nowhere to run to) so head for our nearest resupply instead
		if (!retreating)
		{
			while (best = UTIL_FindEntityByClassname(best, "func_resupplycase"))
			{
				if (best->pev->team == TEAM_UNASSIGNED || best->pev->team == me->m_iTeam)
				{
					me->SetPathToGoal(best);
					break;
				}
			}
		}

		/*
		for (int i = 0; i < g_pGameRules->m_RespawnRooms.Count(); ++i)
		{
			CBaseEntity* pRoom = CBaseEntity::Instance(g_pGameRules->m_RespawnRooms.Element(i));
			if (!pRoom)
				continue;

			if (pRoom->pev->team != me->m_iTeam)
				continue;

			float d = (pRoom->pev->origin - me->pev->origin).LengthSquared();
			if (d < bestDist)
			{
				bestDist = d;
				best = pRoom;
			}
		}
		*/

		//if (best)
			//me->SetPathToGoal(best);

		/*
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
		*/
	}
	/* else if (m_cover && me->m_lastNavArea == m_cover && m_recomputeCoverArea.IsElapsed())
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

			if (m_cover)
			{
				Vector goal = *m_cover->GetCenter();
				me->SetPathToGoal(goal);
			}
		}
		else // look for nearest team spawn room if there are no other options
		{
			float bestDist = FLT_MAX;
			CBaseEntity* best = NULL;
			for (int i = 0; i < g_pGameRules->m_RespawnRooms.Count(); ++i)
			{
				CBaseEntity* pRoom = CBaseEntity::Instance(g_pGameRules->m_RespawnRooms.Element(i));
				if (!pRoom)
					continue;

				if (pRoom->pev->team != me->m_iTeam)
					continue;

				float d = (pRoom->pev->origin - me->pev->origin).LengthSquared();
				if (d < bestDist)
				{
					bestDist = d;
					best = pRoom;
				}
			}

			if (best)
				me->SetPathToGoal(best);
		}
	} */

	Continue();
}

void CTFBotRetreat::OnExit(CBot* me)
{
}

void CTFBotRetreat::OnResume(CBot* me)
{
}