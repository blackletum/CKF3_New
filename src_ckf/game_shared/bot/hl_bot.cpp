// koroz bot code
// i name it something as if its my own bot, but a good chunk of this is just repurposed nextbot code
// known entity, pathfollower (NOT THE CTFBOTLOCOMOTION, I MADE THAT), and other stuff
// AKA I STOLE VALVE CODE FROM SOURCE SDK 2013
// sorry...
// the action interface is mostly mine though #swag
// although, that is basically a bootleg nextbot action interface............

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

#include "bot/actions/tf_bot_idle.h"
#include "bot/actions/tf_bot_seek_and_destroy.h"
#include "bot/actions/tf_bot_control_point_attack.h"
#include "bot/actions/tf_bot_control_point_defend.h"

extern cvar_t cv_bot_debug;

static const float BotCommandInterval = 1.0f / 30.0f;	// send movement commands 30 times per second
static const float BotFullThinkInterval = 1.0f / 10.0f;	// run decision logic 30 times per second

CBot::CBot()
	: m_actionInterface( new CTFBotIdle )
{
	// init for da bots
	
	m_flNextBotThink = m_flNextFullBotThink = m_flPreviousCommandTime = m_forwardSpeed = m_strafeSpeed = m_verticalSpeed = 0.0f;
	m_buttonFlags = 0;

	m_path.Invalidate();
	m_repathTimer.Invalidate();
	m_stuckTimer.Start();
	m_hEnemy = NULL;

	m_pathfollower.SetImprov( new CTFBotLocomotion(this) );
	m_pathfollower.SetPath(&m_path);
	m_actionInterface.SetBot(this);
}

CTFAction* CBot::DesiredAction(void)
{
	if (g_pGameRules->CPExist())
	{
	//	if (m_iTeam == TEAM_BLUE)
			return (new CTFBotControlPointAttack);
	//	else
		//	return (new CTFBotControlPointDefend);
	// no reason for the defend point thing to exist...
	// yet...
	// i will do this with koth eventually but koth itself is buggy as fuuuuuuck rn
	}

	return (new CTFBotSeekAndDestroy);
}

int CBot::ChooseGoodClass(void)
{
	int newclass = 0;

	static int offenseRoster[] =
	{
		CLASS_ENGINEER,
		CLASS_SOLDIER,
		CLASS_MEDIC,
		CLASS_HEAVY,
		CLASS_DEMOMAN,
		CLASS_SCOUT,

		CLASS_PYRO,
		CLASS_SOLDIER,
		CLASS_DEMOMAN,
		CLASS_SNIPER,
		CLASS_MEDIC,
		CLASS_SPY,
	};

	/*
	static int defenseRoster[] =
	{
		CLASS_MEDIC,
		CLASS_ENGINEER,
		CLASS_ENGINEER,
		CLASS_HEAVY,
		CLASS_MEDIC,
		CLASS_DEMOMAN,

		CLASS_SOLDIER,
		CLASS_SNIPER,
		CLASS_SOLDIER,
		CLASS_PYRO,
		CLASS_SCOUT,
		CLASS_SPY,
	};

	not needed
	*/

	// count classes in use by my team, not including me
	CCountClassMembers currentRoster(this, m_iTeam);
	ForEachPlayer(currentRoster);

	int classCount[9] = { 0,0,0,0,0,0,0,0,0 };
	for (int i = 0; i < 12; ++i)
	{
		if (currentRoster.m_count[offenseRoster[i]] > classCount[offenseRoster[i]])
		{
			// if we have enough of this class, skip it
			classCount[offenseRoster[i]]++;
		}
		else
		{
			return offenseRoster[i];
		}
	}

	return 0;
}

void CBot::UpdateKnownEntities(void)
{
	// construct set of potentially visible objects
	CUtlVector< CBaseEntity* > potentiallyVisible;
	CollectPotentiallyVisibleEntities(&potentiallyVisible);

	// collect set of visible and recognized entities at this moment
	CollectVisible visibleNow(this);
	FOR_EACH_VEC(potentiallyVisible, pit)
	{
		if (visibleNow(potentiallyVisible[pit]) == false)
			break;
	};

	// update known set with new data
	{
		int i;
		for (i = 0; i < m_knownEntityVector.Count(); ++i)
		{
			CKnownEntity& known = m_knownEntityVector[i];

			// clear out obsolete knowledge
			if (known.GetEntity() == NULL || known.IsObsolete())
			{
				m_knownEntityVector.Remove(i);
				--i;
				continue;
			}

			if (visibleNow.Contains(known.GetEntity()))
			{
				// this visible entity was already known (but perhaps not visible until now)
				known.UpdatePosition();
				known.UpdateVisibilityStatus(true);

				// has our reaction time just elapsed?
				/*
				if (gpGlobals->time - known.GetTimeWhenBecameVisible() >= 0.0f &&
					m_lastVisionUpdateTimestamp - known.GetTimeWhenBecameVisible() < 0.0f)
				{
					// OnSight(known.GetEntity());
					// this will probably be with the action interface
				}
				*/

				// restart 'not seen' timer
				
				int team = known.GetTeam();

				if (team != -1 && (team == TEAM_RED || team == TEAM_BLUE))
					m_notVisibleTimer[team].Start();
			}
			else // known entity is not currently visible
			{
				if (known.IsVisibleInFOVNow())
				{
					// previously known and visible entity is now no longer visible
					known.UpdateVisibilityStatus(false);

					// lost sight of this entity
					// OnLostSight(known.GetEntity());
					// again, probably with the action interface
				}

				if (!known.HasLastKnownPositionBeenSeen())
				{
					// can we see the entity's last know position?
					if (m_pathfollower.GetImprov()->IsVisible(known.GetLastKnownPosition()))
					{
						known.MarkLastKnownPositionAsSeen();
					}
				}
			}
		}
	}

	// check for new recognizes that were not in the known set
	int i, j;
	for (i = 0; i < visibleNow.m_recognized.Count(); ++i)
	{
		for (j = 0; j < m_knownEntityVector.Count(); ++j)
		{
			if (visibleNow.m_recognized[i] == m_knownEntityVector[j].GetEntity())
			{
				break;
			}
		}

		if (j == m_knownEntityVector.Count())
		{
			// recognized a previously unknown entity (emit OnSight() event after reaction time has passed)
			CKnownEntity known(visibleNow.m_recognized[i]);
			known.UpdatePosition();
			known.UpdateVisibilityStatus(true);
			m_knownEntityVector.AddToTail(known);
		}
	}
}

const CKnownEntity* CBot::GetPrimaryKnownThreat(bool onlyVisibleThreats)
{
	if (m_knownEntityVector.Count() == 0)
	{
		m_hEnemy = NULL;
		return NULL;
	}

	const CKnownEntity* threat = NULL;
	int i;

	// find the first valid entity
	for (i = 0; i < m_knownEntityVector.Count(); ++i)
	{
		const CKnownEntity& firstThreat = m_knownEntityVector[i];
		int team = firstThreat.GetTeam();
		bool enemy = team != -1 && team != m_iTeam;

		// check in case status changes between updates
		if ( (firstThreat.GetTimeSinceBecameKnown() >= 0.0f) && !firstThreat.IsObsolete() && enemy )
		{
			if (!onlyVisibleThreats || firstThreat.IsVisibleRecently())
			{
				threat = &firstThreat;
				break;
			}
		}
	}

	if (threat == NULL)
	{
		m_hEnemy = NULL;
		return NULL;
	}

	for (++i; i < m_knownEntityVector.Count(); ++i)
	{
		const CKnownEntity& newThreat = m_knownEntityVector[i];
		int team = newThreat.GetTeam();
		bool enemy = team != -1 && team != m_iTeam;

		// check in case status changes between updates
		if (newThreat.GetTimeSinceBecameKnown() >= 0.0f && !newThreat.IsObsolete() && enemy)
		{
			if (!onlyVisibleThreats || newThreat.IsVisibleRecently())
			{
				threat = &newThreat;
				// SelectMoreDangerousThreat has not been implemented yet...
			}
		}
	}

	// cache off threat
	m_hEnemy = threat ? threat->GetEntity() : NULL;

	return threat;
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

	/*
	bool joinblue = false;
	int onBlue = g_pGameRules->m_iNumCT;
	int onRed = g_pGameRules->m_iNumTerrorist;
	bool blueIsWinning = (g_pGameRules->m_iNumCTWins > g_pGameRules->m_iNumTerroristWins);

	no longer needed
	*/
	HandleMenu_ChooseTeam(bot, 0);
	HandleMenu_ChooseClass(bot, bot->ChooseGoodClass());

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
	m_actionInterface.Reset(DesiredAction());
}

void CBot::Think(void)
{
	if (gpGlobals->time < m_flNextBotThink)
		return;

	m_flNextBotThink = gpGlobals->time + BotCommandInterval;

	if (m_iTeam != TEAM_BLU && m_iTeam != TEAM_RED)
	{
		HandleMenu_ChooseTeam(this, 0);
		//CONSOLE_ECHO("%s is trying to pick a team.\n", STRING(pev->netname));
		// if not on a team, bots automatically pick the team
	}
	if (!(CLASS_SPY >= m_iClass && m_iClass >= CLASS_SCOUT))
	{
		HandleMenu_ChooseClass(this, 0);
		//CONSOLE_ECHO("%s is trying to pick a class.\n", STRING(pev->netname));
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

		// this shouldnt need to happen now that we ported the known entity system
	}

	if(m_hEnemyRecalculateTimer.IsElapsed())
	{
		UpdateKnownEntities();
		GetPrimaryKnownThreat(); // caches m_hEnemy in here

		/*
		CClosestTFPlayer closest(this, GetEnemyTeam());
		ForEachPlayer(closest);

		if (closest.m_closePlayer)
			m_hEnemy = closest.m_closePlayer;

			simple look, do not use this anymore
		*/

		m_hEnemyRecalculateTimer.Start(RANDOM_FLOAT(0.2f, 0.6f));
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
	m_actionInterface.Update();

	if (pev->waterlevel >= 3)
	{
		// if underwater, get up
		PressJump();
	}

	Vector m_aimPosition = pev->origin;
	if (GetPrimaryKnownThreat() && GetPrimaryKnownThreat()->GetEntity()->IsPlayer())
	{
		CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)m_pActiveItem;
		m_aimPosition = GetPrimaryKnownThreat()->GetLastKnownPosition();
		bool isVis = m_pathfollower.GetImprov()->IsVisible(m_aimPosition);

		if (m_iClass == CLASS_SNIPER)
		{
			if (m_pathfollower.GetImprov()->IsVisible(m_hEnemy->EyePosition()))
			{
				m_aimPosition = m_hEnemy->EyePosition();
				isVis = true;
			}
			// this only replaced isVis if the head is visible
			// IT IS SPECIFICALLY A IF STATEMENT SO IF THE HEAD IS NOT VISIBLE THEN THE CENTER IS USED AS THE ISVIS CHECKER
		}

		if (isVis)
		{
			if (m_iClass == CLASS_SNIPER)
			{
				if (pWeapon && m_pActiveItem->m_iId == WEAPON_SNIPERIFLE)
				{
					CSniperifle* pSniperRifle = (CSniperifle*)pWeapon;

					if (!(pWeapon->m_iWeaponState & WEAPONSTATE_CHARGING))
					{
						PressSecondaryAttack();
					}
					else if (pSniperRifle && pSniperRifle->m_fCharge > 10)
					{
						PressPrimaryAttack();
					}
				}
			}
			else
				PressPrimaryAttack();
		}
	}
	else
	{
	//	Vector point = pev->origin;
	//	m_pathfollower.FindPathPoint(512.0f, &point, NULL); // use this mayb

		const float minMoveSpeed = 1.0f;
		// below this we count as standing still
		// super small so that it keeps the vel stuff until we are REALLY still

		Vector point = pev->origin;
		Vector eyePos = EyePosition();
		Vector horizVel = pev->velocity;
		horizVel.z = 0.0f;

		Vector lookDir;
		if (horizVel.Length() > minMoveSpeed)
		{
			// face the direction we're travelling
			// on paths this SHOULD produce a natural look ahead without needing to compute it from the path
			// this might look weird something though...
			lookDir = horizVel.Normalize();
		}
		else
		{
			// standing still
			// just face where ur already looking but flat in the horizon
			float yawRad = pev->v_angle.y * (float)(M_PI / 180.0);
			lookDir = Vector(cos(yawRad), sin(yawRad), 0.0f);
		}

		// target point is level with the eyes, so Bot_LookAt computes a pitch of 0
		point = eyePos + lookDir * 128.0f;
		point.z = eyePos.z;

		m_aimPosition = point;
	}

	Bot_LookAt(this, m_aimPosition);

	m_pathfollower.Debug(cv_bot_debug.value == 1.0);
	m_pathfollower.Update(BotCommandInterval);
}

void CBot::SetPathToGoal(CBasePlayer *goal)
{
	if (!goal)
		return;

	SetPathToGoal(goal->Center());
	// this SHOULD cause no problems
	// gotta learn more about function overloads
}

void CBot::SetPathToGoal(CBaseEntity* goal)
{
	if (!goal)
		return;

	SetPathToGoal(goal->Center());
}

void CBot::SetPathToGoal(const Vector& goal)
{
	const float goalMovedTolerance = 240.0f;
	// repath if a mobile goal strays this far from the path end

	bool needPath = !m_path.IsValid();

	if (!needPath && m_repathTimer.IsElapsed())
		needPath = true;
	// prevents spam

	if (!needPath && (goal - m_pathGoal).IsLengthGreaterThan(goalMovedTolerance))
		needPath = true;
	// if the origin has moved too much, then change this
	// this allows for path to remain the same if the new goal is close to the old one, to save fps

	if (needPath)
	{
		ShortestPathCost cost(this);
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

		// don't repath every think even on failure
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
	//CONSOLE_ECHO("MoveTowardsPos\n");

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