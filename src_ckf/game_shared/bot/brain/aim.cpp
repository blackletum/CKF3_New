// aim.cpp
// A human-motor aiming model for HL1 bots, converted from the TF2 aim.inc.
// See aim.h for the public interface.
//
// WHAT THIS IS
// -------------
// How a bot decides WHERE to aim, and HOW its view actually gets there. A
// perfect-information bot reads the enemy's exact current position/velocity
// every think and eases the camera straight at it - a thing no human can do,
// and critically, a thing that can never be juked. Real players aim at where
// they THINK the enemy is (a slightly old mental snapshot), extrapolate that
// snapshot forward using the velocity they last saw, and chase the guess
// with real muscle: a fast flick to get close, then slower correction to
// settle. None of those steps are exact.
//
// This models that pipeline in three stages:
//
//   A. PERCEIVE  ( Aim_PerceiveTarget )
//        Look at the target as it was ~reactionDelay ago, not this instant.
//        A small per-target ring buffer (below) is filled lazily - every
//        perceive call records the target's current pos/vel - so the "old
//        snapshot" costs nothing extra to keep around. For the length of the
//        delay the bot literally cannot see that the enemy turned; that's
//        the whole source of juking, and it falls out of this for free.
//
//   B. PREDICT   ( Bot_ComputeAimPoint )
//        Extrapolate that old snapshot forward along the velocity it saw:
//        estimate = seenPos + seenVel * (reactionDelay * predictGain).
//        predictGain is how hard the bot leads its tracking - higher tracks
//        straight-line movers well but commits harder to the wrong place
//        when the target reverses.
//
//   C. MOVE      ( Aim_DriveToward, via Bot_AimAtEnemy / Bot_AimAtPoint )
//        The view is driven to that estimate by a second-order spring-
//        damper (ballistic swing toward the goal, then a settling
//        correction - the classic two-component reach), capped by a maximum
//        turn speed (you cannot flick infinitely fast), with motor noise
//        that scales with how fast the crosshair is moving (panic flicks
//        spray, slow holds are tight - the speed-accuracy tradeoff falls out
//        for free, and it fades to zero as the aim settles).
//
// WHAT GOT DROPPED FROM THE ORIGINAL
// -----------------------------------
// The original lived in a TF2 plugin and carried a lot that doesn't apply
// here: per-class shading (soldier/sniper/scout/spy/heavy overrides), a
// zero-latency instant-turn carve-out for spy melee, projectile ballistic
// arc solving keyed off TF2 weapon classnames/item-defindexes, and a
// cvar-driven bank of test presets ("vs lovelybots" / "vs x64" / "vs
// legacy") used to A/B against specific TF2 bot plugins. All of that is
// gone. What's left is the general, engine-agnostic part of the pipeline,
// fixed to a single profile (see Bot_GetAimProfile) instead of switching on
// a cvar or a player class - there's only one kind of bot here, so there's
// only one aim profile. No cvars are read anywhere in this file; it works
// with a vanilla server config.
//
// All persistent state (per-bot motor velocity, per-target perceive
// history) lives in file-scope tables here, keyed off entindex(), so this
// file is a drop-in - it adds no members to CHLBot and needs no changes to
// hl_bot.h. Everything but the three functions declared in aim.h has
// internal linkage, so it's safe even if some other translation unit needed
// its own aim.cpp-alike someday.
//
// USAGE
// -----
//   Combat (e.g. CHLBot::UpdateAttack), tracking a live enemy - runs all
//   three stages, so the lead/lag juke behavior described above applies:
//
//       Bot_AimAtEnemy(this, enemy);              // body shot
//       Bot_AimAtEnemy(this, enemy, true);         // headshot line
//
//   Everything else that just wants a human-ish turn toward a fixed point
//   (path waypoints, ladders, item pickups, idle looks) - stage C only, no
//   perceive/predict, since there's no target to juke:
//
//       Bot_AimAtPoint(this, m_viewTarget);
//
//   Both share the same spring-damper motor and its "just spawned / long
//   gap since last aim" snap path, so switching CHLBot::AimAt() over to
//   call Bot_AimAtPoint(this, spot) makes m_flLastAimTime and the old
//   AimStepToward() helper in hl_bot.cpp redundant, if you want to remove
//   them.
// ============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

#include "aim.h"	// pulls in the public declarations so the compiler checks them against these definitions

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//------------------------------------------------------------------------------------------------------
// Aim profile - every human-bounded knob that shapes the bot's aim, in one
// place. The original picked one of these per class/cvar-preset; there's
// only one kind of bot here, so there's only one profile: the values below
// are the old "realistic / well-defended reference" preset (the one that
// was written to resist juking rather than to demonstrate it), carried over
// verbatim. If the current feel is too snappy for a given weapon or
// situation, tune these numbers rather than reaching for a bigger rewrite.
//------------------------------------------------------------------------------------------------------
struct AimProfile
{
	float reactionDelay;	///< s      - how old the bot's snapshot of the target is (perception+motor latency)
	float predictGain;		///< 0..1   - how hard it leads its tracking (higher = better vs straight movers, worse vs reversals)
	float stiffness;		///< x      - multiplier on spring frequency (snappiness of the flick)
	float zeta;				///< x      - damping ratio (<1 overshoots then settles, ~1 is critical)
	float slewMax;			///< deg/s  - hard cap on turn speed (the flick ceiling)
	float noiseGain;		///< x      - scales signal-dependent motor scatter
};

/// The single active aim profile. See the struct comment above for where these numbers come from.
static const AimProfile& Bot_GetAimProfile(void)
{
	static const AimProfile profile =
	{
		0.11f,		// reactionDelay
		0.45f,		// predictGain
		1.10f,		// stiffness
		0.65f,		// zeta
		1100.0f,	// slewMax
		0.025f,		// noiseGain
	};

	return profile;
}

//------------------------------------------------------------------------------------------------------
// Shared small helpers
//------------------------------------------------------------------------------------------------------

/// Wrap an angle into (-180, 180].
static float Aim_AngleNormalize(float angle)
{
	angle = fmodf(angle, 360.0f);
	if (angle > 180.0f)
		angle -= 360.0f;
	if (angle < -180.0f)
		angle += 360.0f;

	return angle;
}

static float Aim_Clamp(float value, float lo, float hi)
{
	if (value < lo)
		return lo;
	if (value > hi)
		return hi;

	return value;
}

/// Zero-mean, ~unit-ish scatter from summing three uniforms (central limit
/// theorem gets us most of the way to gaussian for a fraction of the cost
/// of a proper Box-Muller draw - plenty for motor tremor).
static float Aim_GaussianNoise(void)
{
	return (RANDOM_FLOAT(0.0f, 1.0f) + RANDOM_FLOAT(0.0f, 1.0f) + RANDOM_FLOAT(0.0f, 1.0f)) - 1.5f;
}

// Both tables below are sized for a standard GoldSrc client count (1..32)
// and indexed directly by entindex(); slot 0 is an unused scratch slot for
// any stray non-client entindex so a bad lookup can't corrupt player 1's
// state instead of just wasting a slot.
enum { AIM_MAX_ENTITIES = 33 };

//------------------------------------------------------------------------------------------------------
// Stage A state: a short per-target history of (time, position, velocity),
// filled lazily by whichever bot(s) are currently perceiving that target.
// This stands in for the original's shared ring buffer of every client's
// pos/vel, without needing a separate per-frame recorder hooked into
// player movement - it costs nothing when nobody is looking at you.
//------------------------------------------------------------------------------------------------------
struct AimPerceiveSample
{
	float time;
	Vector pos;
	Vector vel;
};

enum { AIM_HISTORY_SIZE = 16 };	// ~0.5s of history at the bot's ~30Hz think rate - comfortably more than our ~0.11s reaction delay needs

struct AimTargetHistory
{
	AimPerceiveSample samples[AIM_HISTORY_SIZE];
	int head;	///< next write slot
	int count;	///< how many valid samples we have (caps at AIM_HISTORY_SIZE)
};

static AimTargetHistory s_aimHistory[AIM_MAX_ENTITIES];

/// Record the target's current pos/vel, unless something already recorded
/// this exact tick (several bots can perceive the same target on the same
/// server frame; only the first should push a sample).
static void Aim_RecordSample(CBaseEntity* target)
{
	int idx = target->entindex();
	if (idx <= 0 || idx >= AIM_MAX_ENTITIES)
		idx = 0;

	AimTargetHistory& hist = s_aimHistory[idx];

	if (hist.count > 0)
	{
		int last = (hist.head - 1 + AIM_HISTORY_SIZE) % AIM_HISTORY_SIZE;
		if (hist.samples[last].time == gpGlobals->time)
			return;
	}

	AimPerceiveSample& sample = hist.samples[hist.head];
	sample.time = gpGlobals->time;
	sample.pos = target->pev->origin;
	sample.vel = target->pev->velocity;

	hist.head = (hist.head + 1) % AIM_HISTORY_SIZE;
	if (hist.count < AIM_HISTORY_SIZE)
		++hist.count;
}

/// Find the two samples straddling lookupTime and linearly blend between
/// them. Returns false if the history doesn't reach back that far yet (the
/// bot only just spotted this target) - caller should fall back to live data.
static bool Aim_FindBracket(const AimTargetHistory& hist, float lookupTime, Vector* outPos, Vector* outVel)
{
	if (hist.count < 2)
		return false;

	for (int i = 0; i < hist.count - 1; ++i)
	{
		int loSlot = (hist.head - hist.count + i + AIM_HISTORY_SIZE * 2) % AIM_HISTORY_SIZE;
		int hiSlot = (loSlot + 1) % AIM_HISTORY_SIZE;

		const AimPerceiveSample& lo = hist.samples[loSlot];
		const AimPerceiveSample& hi = hist.samples[hiSlot];

		if (lookupTime >= lo.time && lookupTime <= hi.time)
		{
			float span = hi.time - lo.time;
			float frac = (span > 0.0001f) ? (lookupTime - lo.time) / span : 0.0f;

			*outPos = lo.pos + (hi.pos - lo.pos) * frac;
			*outVel = lo.vel + (hi.vel - lo.vel) * frac;
			return true;
		}
	}

	return false;
}

/// STAGE A: fills outPos/outVel with the target as this bot currently
/// BELIEVES it to be - position and velocity sampled ~reactionDelay in the
/// past. The vertical aim offset (head vs body) is taken from the target's
/// CURRENT pose so crouch height still tracks, but the horizontal position -
/// the plane juking happens in - is intentionally old. That lag is the whole point.
static void Aim_PerceiveTarget(CBasePlayer* target, const AimProfile& profile, bool headAim, Vector* outPos, Vector* outVel)
{
	Aim_RecordSample(target);

	int idx = target->entindex();
	const AimTargetHistory& hist = (idx > 0 && idx < AIM_MAX_ENTITIES) ? s_aimHistory[idx] : s_aimHistory[0];

	float lookupTime = gpGlobals->time - profile.reactionDelay;

	if (!Aim_FindBracket(hist, lookupTime, outPos, outVel))
	{
		// buffer doesn't reach back far enough yet - fall back to the live
		// state so we never aim at a stale/zero snapshot
		*outPos = target->pev->origin;
		*outVel = target->pev->velocity;
	}

	// Vertical aim offset from the target's CURRENT pose. One bounding-box
	// lookup, applied on top of the delayed horizontal position.
	float aimZOffset = headAim
		? (target->EyePosition().z - target->pev->origin.z)
		: (target->Center().z - target->pev->origin.z);

	outPos->z += aimZOffset;
}

//------------------------------------------------------------------------------------------------------
// Stage B: PERCEIVE + PREDICT combined. Everything here runs on the
// PERCEIVED (delayed) snapshot, never on live truth, so the lead is wrong
// in exactly the way a human's lead is wrong.
//------------------------------------------------------------------------------------------------------
void Bot_ComputeAimPoint(CBasePlayer* target, bool headAim, Vector* outAimPoint)
{
	if (!target || !target->IsAlive())
	{
		if (target)
			*outAimPoint = target->Center();
		return;
	}

	const AimProfile& profile = Bot_GetAimProfile();

	Vector seenPos, seenVel;
	Aim_PerceiveTarget(target, profile, headAim, &seenPos, &seenVel);

	// Extrapolate the old snapshot forward along the velocity it saw. For a
	// straight mover this lands dead on target; for one that just reversed,
	// it confidently points where the target ISN'T - the natural source of
	// the juke window.
	float leadAhead = profile.reactionDelay * profile.predictGain;
	*outAimPoint = seenPos + seenVel * leadAhead;
}

//------------------------------------------------------------------------------------------------------
// Stage C: the motor. Drives self's view toward a world point with human
// hand dynamics - a second-order spring-damper (fast ballistic swing, then
// a settling correction; underdamped, so big corrections overshoot and
// reversals lag), a maximum turn speed, and motor noise that scales with
// how fast the crosshair is moving (fast flicks scatter, slow holds are
// tight, and it fades to zero as the aim settles so there's no idle drift).
//------------------------------------------------------------------------------------------------------
struct AimMotorState
{
	float yawVel;
	float pitchVel;
	float lastTime;	///< gpGlobals->time this entity's aim last moved; <= 0 means "never" (forces a snap)
};

static AimMotorState s_aimMotor[AIM_MAX_ENTITIES];

/// responsiveness mirrors the old per-call "flAimSpeed": most callers should
/// just use the default (a normal human tracking response); pass a value
/// >= 0.99 for an instant, noise-free snap (useful right after spawning a
/// bot facing some fixed direction, or any other case that wants the old
/// exact-set behavior with no easing).
static void Aim_DriveToward(CBaseEntity* self, const Vector& worldPoint, float responsiveness, const AimProfile& profile)
{
	int idx = self->entindex();
	if (idx <= 0 || idx >= AIM_MAX_ENTITIES)
		idx = 0;

	AimMotorState& state = s_aimMotor[idx];

	Vector eyePos = self->pev->origin + self->pev->view_ofs;
	Vector to = worldPoint - eyePos;
	if (to.Length() < 1.0f)
		return;	// degenerate (goal sitting on the eye) - avoid feeding a zero vector into VecToYaw/atan2

	float idealYaw = UTIL_VecToYaw(to);

	// pitch computed directly - VecToAngles' pitch convention (positive =
	// up) is the opposite of view angles (negative = up)
	float dist2D = sqrtf(to.x * to.x + to.y * to.y);
	float idealPitch = -(float)(atan2(to.z, dist2D) * 180.0 / M_PI);
	idealPitch = Aim_Clamp(idealPitch, -89.0f, 89.0f);

	float now = gpGlobals->time;
	bool firstRun = (state.lastTime <= 0.0f);
	float dt = now - state.lastTime;
	state.lastTime = now;

	// Snap path: an instant-response caller, a first-ever aim, or a long
	// gap since the last one (nothing meaningful to smooth from) - set the
	// angle directly and clear stored momentum so the next eased aim starts clean.
	if (responsiveness >= 0.99f || firstRun || dt <= 0.0f || dt > 0.25f)
	{
		state.yawVel = 0.0f;
		state.pitchVel = 0.0f;
		self->pev->v_angle.x = idealPitch;
		self->pev->v_angle.y = idealYaw;
		self->pev->v_angle.z = 0.0f;
		self->pev->ideal_yaw = idealYaw;
		return;
	}

	// Spring frequency: responsiveness sets the base snappiness, profile.stiffness shades it.
	float omega = (6.0f + responsiveness * 26.0f) * profile.stiffness;
	float zeta = profile.zeta;

	// --- yaw ---
	float errYaw = Aim_AngleNormalize(idealYaw - self->pev->v_angle.y);
	float accYaw = omega * omega * errYaw - 2.0f * zeta * omega * state.yawVel;
	state.yawVel += accYaw * dt;
	state.yawVel = Aim_Clamp(state.yawVel, -profile.slewMax, profile.slewMax);

	// --- pitch ---
	float errPitch = Aim_AngleNormalize(idealPitch - self->pev->v_angle.x);
	float accPitch = omega * omega * errPitch - 2.0f * zeta * omega * state.pitchVel;
	state.pitchVel += accPitch * dt;
	state.pitchVel = Aim_Clamp(state.pitchVel, -profile.slewMax, profile.slewMax);

	// Signal-dependent motor noise: zero-mean, std proportional to current
	// turn speed, applied to the output (not integrated into velocity) so
	// it can't random-walk the aim off target.
	float noiseYaw = Aim_GaussianNoise() * fabsf(state.yawVel) * profile.noiseGain * dt;
	float noisePitch = Aim_GaussianNoise() * fabsf(state.pitchVel) * profile.noiseGain * dt;

	float newYaw = Aim_AngleNormalize(self->pev->v_angle.y + state.yawVel * dt + noiseYaw);
	float newPitch = Aim_AngleNormalize(self->pev->v_angle.x + state.pitchVel * dt + noisePitch);
	newPitch = Aim_Clamp(newPitch, -89.0f, 89.0f);

	self->pev->v_angle.x = newPitch;
	self->pev->v_angle.y = newYaw;
	self->pev->v_angle.z = 0.0f;
	self->pev->ideal_yaw = newYaw;
}

//------------------------------------------------------------------------------------------------------
// Public entry points (declared in aim.h)
//------------------------------------------------------------------------------------------------------
void Bot_AimAtEnemy(CBaseEntity* self, CBasePlayer* target, bool headAim, float responsiveness)
{
	if (!self || !target)
		return;

	Vector aimPoint;
	Bot_ComputeAimPoint(target, headAim, &aimPoint);
	Aim_DriveToward(self, aimPoint, responsiveness, Bot_GetAimProfile());
}

void Bot_AimAtPoint(CBaseEntity* self, const Vector& point, float responsiveness)
{
	if (!self)
		return;

	Aim_DriveToward(self, point, responsiveness, Bot_GetAimProfile());
}