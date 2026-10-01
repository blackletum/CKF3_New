#ifndef ROOSTER_BOT_H
#define ROOSTER_BOT_H

// shoddy bot made to test things out
// port from rooster fortress
// THIS WILL BE REDONE, REPLACED WITH A VERSION SIMILAR TO THE NEXTBOT SYSTEM
// jakulo: dont expect this

#include "nav_path.h"
#include "trigger.h"
#include "hl_bot_action_interface.h"

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

	void SetPathToGoal(const Vector& goal); // recompute goal
	void SetPathToGoal(CBasePlayer* goal);
	void SetPathToGoal(CBaseEntity* goal);

	bool HasEnemy() { return m_hEnemy && m_hEnemy->IsPlayer(); }
	CBaseEntity* GetEnemy()
	{
		CBaseEntity* e = m_hEnemy;

		return e;
	}

	virtual BOOL IsBot(void) const { return TRUE; }
private:
	// decision logic
	void Update(void);
	void Upkeep(void);

	void ResetCommand(void);
	void ExecuteCommand(void);
	byte ThrottledMsec(void) const;
	void UpdateStuckMonitor(void);

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

	CTFBotActionInterface m_actionInterface;	// Korozbot action interface

	EHANDLE m_hEnemy;					// the target
	CountdownTimer m_hEnemyRecalculateTimer;		// limits how often we change our target
};

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
#endif