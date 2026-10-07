#ifndef ROOSTER_BOT_H
#define ROOSTER_BOT_H

// shoddy bot made to test things out
// port from rooster fortress
// THIS WILL BE REDONE, REPLACED WITH A VERSION SIMILAR TO THE NEXTBOT SYSTEM
// jakulo: dont expect this

#include "nav_path.h"
#include "trigger.h"
#include "known_entity.h"
#include "hl_bot_action_interface.h"

#define FOR_EACH_VEC( vecName, iteratorName ) \
	for ( int iteratorName = 0; iteratorName < vecName.Count(); iteratorName++ )

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

	bool HasPath(void) { return m_path.IsValid(); }
private:
	// decision logic
	void Update(void);
	void Upkeep(void);

	void ResetCommand(void);
	void ExecuteCommand(void);
	byte ThrottledMsec(void) const;
	void UpdateStuckMonitor(void);

	CTFAction* DesiredAction(void);

	int ChooseGoodClass(void);

	void UpdateKnownEntities(void);
	const CKnownEntity* GetPrimaryKnownThreat(bool onlyVisibleThreats = true);

	float m_forwardSpeed;
	float m_strafeSpeed;
	float m_verticalSpeed;
	unsigned short m_buttonFlags;

	float m_flNextBotThink;
	float m_flNextFullBotThink;
	float m_flPreviousCommandTime;

	CUtlVector< CKnownEntity > m_knownEntityVector;		// the set of enemies/friends we are aware of
	float m_lastVisionUpdateTimestamp;
	IntervalTimer m_notVisibleTimer[3];		// for tracking interval since last saw a member of the given team

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

class CCountClassMembers
{
public:
	CCountClassMembers(const CBot* me, int teamID)
	{
		m_me = me;
		m_myTeam = teamID;
		m_teamSize = 0;

		for (int i = 0; i < 9; ++i)
			m_count[i] = 0;
	}

	bool operator() (CBasePlayer* basePlayer)
	{
		if (basePlayer->m_iTeam != m_myTeam)
			return true;

		++m_teamSize;

		if (m_me == basePlayer)
			return true;

		++m_count[basePlayer->m_iClass];

		return true;
	}

	const CBot* m_me;
	int m_myTeam;
	int m_count[10];
	int m_teamSize;
};

class PopulateVisibleVector
{
public:
	PopulateVisibleVector(CUtlVector< CBaseEntity* >* potentiallyVisible)
	{
		m_potentiallyVisible = potentiallyVisible;
	}

	bool operator() (CBaseEntity* actor)
	{
		m_potentiallyVisible->AddToTail(actor);
		return true;
	}

	CUtlVector< CBaseEntity* >* m_potentiallyVisible;
};

class CollectVisible
{
public:
	CollectVisible(CBot* me)
	{
		m_me = me;
	}

	bool operator() (CBaseEntity* entity)
	{
		if (entity &&
			entity->IsAlive() &&
			m_me->FVisible(entity))
		{
			if (entity->IsPlayer())
			{
				CBasePlayer* player = (CBasePlayer*)entity;
				if (player->IsPlayerCloaked() || player->IsPlayerDisguised())
				{
					return true;
					// skips players that are disguised or cloaked
					// TODO: disguise logic also counts it if the spy is disguised as their teammate, which would look weird
					// eg: blue bot ignores red spy disguised as red player due to them being disguised regardless
				}
			}

			m_recognized.AddToTail(entity);
		}

		return true;
	}

	bool Contains(CBaseEntity* entity) const
	{
		for (int i = 0; i < m_recognized.Count(); ++i)
		{
			if (entity->entindex() == m_recognized[i]->entindex())
			{
				return true;
			}
		}
		return false;
	}
	CBot* m_me;
	CUtlVector< CBaseEntity* > m_recognized;
};

inline void CollectPotentiallyVisibleEntities(CUtlVector< CBaseEntity* >* potentiallyVisible)
{
	potentiallyVisible->RemoveAll();

	// for now only consider players as potentially visible
	// i will iterate through buildings eventually
	PopulateVisibleVector populate(potentiallyVisible);
	ForEachPlayer(populate);
}

#endif