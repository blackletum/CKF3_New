#ifndef BOT_UTIL_H
#define BOT_UTIL_H

#include "eiface.h"
#include "player.h"
#include "shared_util.h"

//--------------------------------------------------------------------------------------------------------------
// Bot/nav cvars
//--------------------------------------------------------------------------------------------------------------
extern cvar_t cv_bot_traceview;
extern cvar_t cv_bot_stop;
extern cvar_t cv_bot_show_nav;
extern cvar_t cv_bot_show_danger;
extern cvar_t cv_bot_nav_edit;
extern cvar_t cv_bot_nav_zdraw;
extern cvar_t cv_bot_debug;
extern cvar_t cv_bot_quicksave;

#define SIGN( num )		(((num) < 0) ? -1 : 1)
#define ABS( num )		(SIGN(num) * (num))

#define CREATE_FAKE_CLIENT		( *g_engfuncs.pfnCreateFakeClient )
#define GET_USERINFO			( *g_engfuncs.pfnGetInfoKeyBuffer )
#define SET_CLIENT_KEY_VALUE	( *g_engfuncs.pfnSetClientKeyValue )

//--------------------------------------------------------------------------------------------------------------
extern void BotPrecache( void );

extern int UTIL_ClientsInGame( void );
extern bool UTIL_IsNameTaken( const char *name );	///< return true if given name is already in use by another player

extern void UTIL_DrawBeamPoints( Vector vecStart, Vector vecEnd, int iLifetime, byte bRed, byte bGreen, byte bBlue );
extern CBasePlayer *UTIL_GetClosestPlayer( const Vector *pos, float *distance = NULL );
extern CBasePlayer *UTIL_GetClosestPlayer( const Vector *pos, int team, float *distance = NULL );	///< closest alive player on the given team
extern CBaseEntity *UTIL_GetLocalPlayer( void );	///< return the listen server host (client 1)

extern int BotTeamID( CBasePlayer *player );

extern void CONSOLE_ECHO( const char *pszMsg, ... );
extern void CONSOLE_ECHO_LOGGED( const char *pszMsg, ... );

extern void HintMessageToAllPlayers( const char *message );

// fast table-based trig, used by the danger-drawing code
extern void InitBotTrig( void );
extern float BotCOS( float angle );
extern float BotSIN( float angle );

//--------------------------------------------------------------------------------------------------------------

class IntervalTimer
{
public:
	IntervalTimer( void )
	{
		m_timestamp = -1.0f;
	}

	void Reset( void )
	{
		m_timestamp = gpGlobals->time;
	}

	void Start( void )
	{
		m_timestamp = gpGlobals->time;
	}

	void Invalidate( void )
	{
		m_timestamp = -1.0f;
	}

	bool HasStarted( void ) const
	{
		return (m_timestamp > 0.0f);
	}

	/// if not started, elapsed time is very large
	float GetElapsedTime( void ) const
	{
		return (HasStarted()) ? (gpGlobals->time - m_timestamp) : 99999.9f;
	}

	bool IsLessThen( float duration ) const
	{
		return (gpGlobals->time - m_timestamp < duration) ? true : false;
	}

	bool IsGreaterThen( float duration ) const
	{
		return (gpGlobals->time - m_timestamp > duration) ? true : false;
	}

private:
	float m_timestamp;
};

class CountdownTimer
{
public:
	CountdownTimer( void )
	{
		m_timestamp = -1.0f;
		m_duration = 0.0f;
	}

	void Reset( void )
	{
		m_timestamp = gpGlobals->time + m_duration;
	}

	void Start( float duration )
	{
		m_timestamp = gpGlobals->time + duration;
		m_duration = duration;
	}

	void Invalidate( void )
	{
		m_timestamp = -1.0f;
	}

	bool HasStarted( void ) const
	{
		return (m_timestamp > 0.0f);
	}

	bool IsElapsed( void ) const
	{
		return (gpGlobals->time > m_timestamp);
	}

private:
	float m_duration;
	float m_timestamp;
};

inline bool IsEntityValid( CBaseEntity *entity )
{
	if (entity == NULL)
		return false;

	if (FNullEnt( entity->pev ))
		return false;

	if (FStrEq( STRING( entity->pev->netname ), "" ))
		return false;

	if (entity->pev->flags & FL_DORMANT)
		return false;

	return true;
}

inline bool IsIntersecting2D( const Vector &startA, const Vector &endA,
															const Vector &startB, const Vector &endB,
															Vector *result = NULL )
{
	float denom = (endA.x - startA.x) * (endB.y - startB.y) - (endA.y - startA.y) * (endB.x - startB.x);
	if (denom == 0.0f)
	{
		// parallel
		return false;
	}

	float numS = (startA.y - startB.y) * (endB.x - startB.x) - (startA.x - startB.x) * (endB.y - startB.y);
	if (numS == 0.0f)
	{
		// coincident
		return true;
	}

	float numT = (startA.y - startB.y) * (endA.x - startA.x) - (startA.x - startB.x) * (endA.y - startA.y);

	float s = numS / denom;
	if (s < 0.0f || s > 1.0f)
	{
		// intersection is not within line segment of startA to endA
		return false;
	}

	float t = numT / denom;
	if (t < 0.0f || t > 1.0f)
	{
		// intersection is not within line segment of startB to endB
		return false;
	}

	// compute intesection point
	if (result)
		*result = startA + s * (endA - startA);

	return true;
}

template < typename Functor >
bool ForEachPlayer( Functor &func )
{
	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( UTIL_PlayerByIndex( i ) );

		if (!IsEntityValid( player ))
			continue;

		if (!player->IsPlayer())
			continue;

		if (func( player ) == false)
			return false;
	}

	return true;
}

#endif // BOT_UTIL_H
