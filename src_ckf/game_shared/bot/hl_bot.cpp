// this WOULD have the rooster fortress bot code, but that does not work here
// lots of bugs.............
// this'll be used eventually!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "client.h"		// ClientConnect()/ClientPutInServer()
#include "hl_bot.h"
#include "hl_bot_manager.h"
#include "hl_bot_locomotion.h"

extern cvar_t cv_bot_debug;

static const float BotCommandInterval = 1.0f / 30.0f;	// send movement commands 30 times per second
static const float BotFullThinkInterval = 1.0f / 10.0f;	// run decision logic 30 times per second

CBot::CBot()
{
	// init for da bots
	
	// if the bot can get away with playing this class, they will
	// this does not mean that it will be the class they always play. sometimes there will be better classes to play at the moment
	m_iFavoriteClass = RANDOM_LONG(CLASS_SCOUT, CLASS_SPY);
	m_flNextBotThink = m_flNextFullBotThink = m_flPreviousCommandTime = m_forwardSpeed = m_strafeSpeed = m_verticalSpeed = 0.0f;
	m_buttonFlags = 0;

	m_path.Invalidate();
	m_repathTimer.Invalidate();
	m_stuckTimer.Start();
	m_hEnemy = NULL;

	m_pathfollower.SetImprov( new CTFBotLocomotion(this) );
	m_pathfollower.SetPath(&m_path);
}

CBot* CBot::CreateBot(const char* name)
{
	if (UTIL_ClientsInGame() >= gpGlobals->maxClients)
	{
		CONSOLE_ECHO("bot_add: Server is full.\n");
		return NULL;
	}

	// pick a default name if none was given (or the given one is taken)
	char botName[64];
	if (name && *name && !UTIL_IsNameTaken(name))
	{
		strncpy(botName, name, sizeof(botName) - 1);
		botName[sizeof(botName) - 1] = '\0';
	}
	else
	{
		int serial = 1;
		do
		{
			sprintf(botName, "Bot%02d", serial++);
		} while (UTIL_IsNameTaken(botName) && serial < 100);
	}

	edict_t* pent = CREATE_FAKE_CLIENT(botName);
	if (FNullEnt(pent))
	{
		CONSOLE_ECHO("bot_add: pfnCreateFakeClient() failed - no free client slots?\n");
		return NULL;
	}

	// run the normal connection path
	char reject[128];
	if (0 == ClientConnect(pent, STRING(pent->v.netname), "127.0.0.1", reject))
	{
		SERVER_COMMAND(UTIL_VarArgs("kick \"%s\"\n", STRING(pent->v.netname)));
		return NULL;
	}

	// give the bot a player model
	//char* infobuffer = GET_USERINFO(pent);
	//SET_CLIENT_KEY_VALUE(ENTINDEX(pent), infobuffer, "model", "gordon");

	FREE_PRIVATE(pent);
	CBot* bot = GetClassPtr((CBot*)VARS(pent));

	ClientPutInServer(pent);

	HandleMenu_ChooseTeam(bot, 0);
	HandleMenu_ChooseClass(bot, 0);

	//bot->m_bShouldClearBuild = true;
	//bot->RoundRespawn();
	//bot->ResetMaxSpeed();

	return bot;
}

void CBot::Spawn(void)
{
	// let the game set everything up
	CBasePlayer::Spawn();

	// make sure everyone knows we are a bot
	pev->flags |= (FL_CLIENT | FL_FAKECLIENT);

	// bots use their own thinking mechanism (BotThink() from the manager)
	SetThink(NULL);
	pev->nextthink = -1;

	m_flNextBotThink = gpGlobals->time + BotCommandInterval;
	m_flNextFullBotThink = gpGlobals->time + BotFullThinkInterval;
	m_flPreviousCommandTime = gpGlobals->time;

	m_path.Invalidate();
	m_pathGoal = pev->origin;
	m_repathTimer.Invalidate();

	m_stuckSpot = pev->origin;
	m_stuckTimer.Start();

	m_hEnemy = NULL;	// no target carries over into a fresh life

	ResetCommand();
}

void CBot::Think(void)
{
	if (gpGlobals->time < m_flNextBotThink)
		return;

	m_flNextBotThink = gpGlobals->time + BotCommandInterval;

	if (m_iTeam != TEAM_BLU && m_iTeam != TEAM_RED)
	{
		HandleMenu_ChooseTeam(this, 0);
		CONSOLE_ECHO("%s is trying to pick a team.\n", STRING(pev->netname));
		// if not on a team, bots automatically pick the team
	}
	if (!(CLASS_SPY >= m_iClass && m_iClass >= CLASS_SCOUT))
	{
		HandleMenu_ChooseClass(this, 0);
		CONSOLE_ECHO("%s is trying to pick a class.\n", STRING(pev->netname));
	}

	ResetCommand();

	if (!IsAlive())
	{
		ExecuteCommand();
		m_path.Invalidate();
		// CONSOLE_ECHO("%s is dead!", STRING(pev->netname));
		return;
	}

	if (gpGlobals->time >= m_flNextFullBotThink)
	{
		m_flNextFullBotThink = gpGlobals->time + BotFullThinkInterval;
		Update();
	}

	// maintains inbetween stuff
	Upkeep();

	ExecuteCommand();
}

void CBot::Update(void)
{
	UpdateStuckMonitor();
	if (m_hEnemy)
	{
		CBaseEntity* e = m_hEnemy;
		if (!e || !e->IsAlive())
			m_hEnemy = NULL;
	}

	if(m_repathTimer.IsElapsed())
	{
		m_path.Invalidate();

		CClosestTFPlayer closest(this, GetEnemyTeam());
		ForEachPlayer(closest);
		
		if (closest.m_closePlayer)
		{
			m_hEnemy = closest.m_closePlayer;

			SetPathToGoal(closest.m_closePlayer);

			CONSOLE_ECHO("%s wants to follow %s\n", STRING(pev->netname), STRING(closest.m_closePlayer->pev->netname));
		}
		
		m_repathTimer.Start(RANDOM_FLOAT(1.0f, 3.0f));
	}
}

static void Bot_LookAt(CBaseEntity* self, const Vector& point)
{
	// this is temporary!!!!!

	Vector to = point - (self->pev->origin + self->pev->view_ofs);

	if (to.Length() < 1.0f)
		return; // point is on the eye, avoid a zero vector

	float dist2D = sqrtf(to.x * to.x + to.y * to.y);

	self->pev->v_angle.x = -(float)(atan2(to.z, dist2D) * 180.0 / M_PI); // pitch
	self->pev->v_angle.y = UTIL_VecToYaw(to);                            // yaw
	self->pev->v_angle.z = 0.0f;
	self->pev->ideal_yaw = self->pev->v_angle.y;
}

void CBot::Upkeep(void)
{
	if (m_hEnemy && m_hEnemy->IsPlayer())
	{
		bool isVis = m_pathfollower.GetImprov()->IsVisible(m_hEnemy->Center(), false);
		if (isVis)
		{
			PressPrimaryAttack();
			Bot_LookAt(this, m_hEnemy->Center());
		}
	}

	m_pathfollower.Debug(cv_bot_debug.value == 1.0);
	m_pathfollower.Update(BotCommandInterval);
}

void CBot::SetPathToGoal(CBasePlayer *goal)
{
	SetPathToGoal(goal->Center());
	// this SHOULD cause no problems
	// gotta learn more about function overloads
}

void CBot::SetPathToGoal(const Vector& goal)
{
	const float goalMovedTolerance = 240.0f;
	// repath if a mobile goal strays this far from the path end

	bool needPath = !m_path.IsValid();

	if (!needPath && m_repathTimer.IsElapsed())
		needPath = true;

	if (!needPath && (goal - m_pathGoal).IsLengthGreaterThan(goalMovedTolerance))
		needPath = true;

	if (needPath)
	{
		ShortestPathCost cost;
		Vector start = pev->origin;

		if (m_path.Compute(&start, &goal, cost))
		{
			m_pathfollower.Reset();
			m_pathGoal = goal;
		}
		else
		{
			m_path.Invalidate();
		}

		// don't repath every think even on failure - it is expensive
		m_repathTimer.Start(RANDOM_FLOAT(1.5f, 2.5f));
	}
}

void CBot::ResetCommand(void)
{
	m_forwardSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
	m_verticalSpeed = 0.0f;
	m_buttonFlags = 0;
}

void CBot::ExecuteCommand(void)
{
	byte adjustedMSec = ThrottledMsec();

	if (cv_bot_stop.value != 0.0f)
	{
		ResetCommand();
	}
	else
	{
		// player model angles are "munged" from view angles
		pev->angles = pev->v_angle;
		pev->angles.x /= -3.0f;
	}

	m_flPreviousCommandTime = gpGlobals->time;

	(*g_engfuncs.pfnRunPlayerMove)(edict(), pev->v_angle, m_forwardSpeed, m_strafeSpeed, m_verticalSpeed,
		m_buttonFlags, 0, adjustedMSec);
}

//-------------------------------------------------------------------------------------------------------------- 
byte CBot::ThrottledMsec(void) const
{
	// estimate msec for this command from the time since the previous one
	int newMsec = (int)((gpGlobals->time - m_flPreviousCommandTime) * 1000.0f);
	if (newMsec > 255)
		newMsec = 255;

	return (byte)newMsec;
}

void CBot::UpdateStuckMonitor(void)
{
	const float movedTolerance = 30.0f;
	const float jumpAfter = 1.0f;
	const float repathAfter = 3.0f;

	if ((pev->origin - m_stuckSpot).IsLengthGreaterThan(movedTolerance))
	{
		// we are moving fine
		m_stuckSpot = pev->origin;
		m_stuckTimer.Start();
		return;
	}

	if (m_stuckTimer.IsGreaterThen(repathAfter))
	{
		m_path.Invalidate();
		m_repathTimer.Invalidate();
		m_stuckSpot = pev->origin;
		m_stuckTimer.Start();
	}
	else if (m_stuckTimer.IsGreaterThen(jumpAfter))
		PressJump();
}

// UTILITY FUNCTIONS!!
//--------------------------------------------------------------------------------------------------------------
float CBot::GetMoveSpeed(void) const
{
	return pev->maxspeed;
}

//--------------------------------------------------------------------------------------------------------------
void CBot::MoveForward(void)
{
	m_forwardSpeed = GetMoveSpeed();
	SetBits(m_buttonFlags, IN_FORWARD);
	ClearBits(m_buttonFlags, IN_BACK);
}

//--------------------------------------------------------------------------------------------------------------
void CBot::MoveBackward(void)
{
	m_forwardSpeed = -GetMoveSpeed();
	SetBits(m_buttonFlags, IN_BACK);
	ClearBits(m_buttonFlags, IN_FORWARD);
}

//--------------------------------------------------------------------------------------------------------------
void CBot::StrafeLeft(void)
{
	m_strafeSpeed = -GetMoveSpeed();
	SetBits(m_buttonFlags, IN_MOVELEFT);
	ClearBits(m_buttonFlags, IN_MOVERIGHT);
}

//--------------------------------------------------------------------------------------------------------------
void CBot::StrafeRight(void)
{
	m_strafeSpeed = GetMoveSpeed();
	SetBits(m_buttonFlags, IN_MOVERIGHT);
	ClearBits(m_buttonFlags, IN_MOVELEFT);
}

//--------------------------------------------------------------------------------------------------------------
bool CBot::PressJump(void)
{
	SetBits(m_buttonFlags, IN_JUMP);
	return true;
}

//--------------------------------------------------------------------------------------------------------------
void CBot::PressDuck(void)
{
	SetBits(m_buttonFlags, IN_DUCK);
}

//--------------------------------------------------------------------------------------------------------------
void CBot::PressPrimaryAttack(void)
{
	SetBits(m_buttonFlags, IN_ATTACK);
}

//--------------------------------------------------------------------------------------------------------------
void CBot::PressSecondaryAttack(void)
{
	SetBits(m_buttonFlags, IN_ATTACK2);
}

//--------------------------------------------------------------------------------------------------------------

void CBot::MoveTowardPos(const Vector& pos)
{
	CONSOLE_ECHO("MoveTowardsPos\n");

	float dx = pos.x - pev->origin.x;
	float dy = pos.y - pev->origin.y;

	if (dx * dx + dy * dy < 1.0f)
		return;		// standing on it - nothing to do

	// flip Y so the rotation below lands in the engine's move space
	// (world yaw turns counter-clockwise, sidemove is positive-right)
	dy = -dy;

	float flRad = pev->v_angle.y * (float)(M_PI / 180.0);
	float s = sin(flRad);
	float c = cos(flRad);

	// rotate: x = along the view (forwardmove), y = across it (sidemove)
	float flForward = c * dx - s * dy;
	float flSide = s * dx + c * dy;

	// normalize so distance to the goal doesn't scale the speed
	float flLen = sqrt(flForward * flForward + flSide * flSide);
	if (flLen < 0.001f)
		return;

	flForward /= flLen;
	flSide /= flLen;

	m_forwardSpeed = flForward * GetMoveSpeed();
	m_strafeSpeed = flSide * GetMoveSpeed();
	// this was initially with the movement keys, but that doesnt work as good. it breaks pathing
	// directly editing this SHOULD be fine

	const float dead = 0.1f;
	if (flForward > dead) SetBits(m_buttonFlags, IN_FORWARD);
	if (flForward < -dead) SetBits(m_buttonFlags, IN_BACK);
	if (flSide > dead) SetBits(m_buttonFlags, IN_MOVERIGHT);
	if (flSide < -dead) SetBits(m_buttonFlags, IN_MOVELEFT);
}