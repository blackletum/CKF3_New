#ifndef ROOSTER_BOT_H
#define ROOSTER_BOT_H

// shoddy bot made to test things out
// port from rooster fortress
// THIS WILL BE REDONE, REPLACED WITH A VERSION SIMILAR TO THE NEXTBOT SYSTEM
// jakulo: dont expect this

#include "nav_path.h"
#include "trigger.h"

class CBot : public CBasePlayer
{
public:
	CBot(void);

	static CBot* CreateBot(const char* name);

	virtual void Spawn(void);

	void Think(void);

	float GetMoveSpeed(void) const;
	void MoveForward(void);
	void MoveBackward(void);
	void StrafeLeft(void);
	void StrafeRight(void);
	bool PressJump(void);
	void PressDuck(void);
	void PressPrimaryAttack(void);
	void PressSecondaryAttack(void);
	void MoveTowardPos(const Vector& pos);

	virtual BOOL IsBot(void) const { return TRUE; }
private:
	// decision logic
	void Update(void);
	void Upkeep(void);

	void ResetCommand(void);
	void ExecuteCommand(void);
	byte ThrottledMsec(void) const;
	void UpdateStuckMonitor(void);

	void SetPathToGoal(const Vector& goal); // recompute goal
	void SetPathToGoal(CBasePlayer* goal);

	int m_iFavoriteClass;

	float m_forwardSpeed;
	float m_strafeSpeed;
	float m_verticalSpeed;
	unsigned short m_buttonFlags;

	float m_flNextBotThink;
	float m_flNextFullBotThink;
	float m_flPreviousCommandTime;

	CNavPath m_path;					// the path itself
	CNavPathFollower m_pathfollower;	// pathfollower + the improv is in here
	Vector m_pathGoal;					// where the current path leads
	CountdownTimer m_repathTimer;		// limits how often we recompute the path

	Vector m_stuckSpot;					// where we were when the stuck check last ran
	IntervalTimer m_stuckTimer;			// how long we have been near m_stuckSpot

	EHANDLE m_hEnemy;					// the target
};

/*

class BotState
{
public:
	virtual void OnEnter(CCSBot *me) {}
	virtual void OnUpdate(CCSBot *me) {}
	virtual void OnExit(CCSBot *me) {}
	virtual const char *GetName() const = 0;
};

todo
grabbed from regamedll_cs
similar to nextbot system, will do eventually

*/

// this is ripped straight from the source sdk LOLOL
class CClosestTFPlayer
{
public:
	CClosestTFPlayer(const Vector& where, int team = -2)
	{
		m_where = where;
		m_closeRangeSq = FLT_MAX;
		m_closePlayer = NULL;
		m_team = team;
	}

	CClosestTFPlayer(CBaseEntity* entity, int team = -2)
	{
		m_where = (entity->pev->absmax + entity->pev->absmin) / 2.0f;
		m_closeRangeSq = FLT_MAX;
		m_closePlayer = NULL;
		m_team = team;
	}

	bool operator() (CBasePlayer* player)
	{
		if (!player->IsAlive())
			return true;

		if (player->m_iTeam != TEAM_RED && player->m_iTeam != TEAM_BLUE)
			return true;

		if (m_team != -2 && player->m_iTeam != m_team)
			return true;
		// player->m_iDisguiseTeam
		// use this for disguise differenciating 
		if ( player->m_iCloak == CLOAK_YES || player->m_iDisguise == DISGUISE_YES )
			return true;

		float rangeSq = (m_where - player->pev->origin).LengthSquared();
		if (rangeSq < m_closeRangeSq)
		{
			m_closeRangeSq = rangeSq;
			m_closePlayer = player;
		}
		return true;
	}

	Vector m_where;
	float m_closeRangeSq;
	CBasePlayer* m_closePlayer;
	int m_team;
};

/*
class CClosestControlPoint
{
public:
	CClosestControlPoint(const Vector& where, int team = -2)
	{
		m_where = where;
		m_closeRangeSq = FLT_MAX;
		m_closePoint = NULL;
		m_team = team;
	}

	CClosestControlPoint(CBaseEntity* entity, int team = -2)
	{
		m_where = entity->pev->origin;
		m_closeRangeSq = FLT_MAX;
		m_closePoint = NULL;
		m_team = team;
	}

	bool operator() (CControlPoint* point)
	{
		if (m_team != -2 && point->pev->team != m_team)
			return true;

		if (m_team == TEAM_RED && !point->m_bCanRedCap)
			return true;

		if (m_team == TEAM_BLUE && !point->m_bCanBluCap)
			return true;

		if (point->m_bLocked || point->m_bDisabled)
			return true;

		float rangeSq = (m_where - point->pev->origin).LengthSquared();
		if (rangeSq < m_closeRangeSq)
		{
			m_closeRangeSq = rangeSq;
			m_closePoint = point;
		}
		return true;
	}

	Vector m_where;
	float m_closeRangeSq;
	CControlPoint* m_closePoint;
	int m_team;
};

template < typename Functor >
bool ForEachControlPoint(Functor& func)
{
	// this might not work

	CControlPoint* point = NULL;
	while ((point = static_cast<CControlPoint*>(UTIL_FindEntityByClassname(point, "func_controlpoint"))) != NULL)
	{
		if (!IsEntityValid(point))
			continue;

		if (!(point->Classify() == CLASS_CONTROLPOINT))
			continue;

		if (func(point) == false)
			return false;
	}
	return true;
}
*/

#endif