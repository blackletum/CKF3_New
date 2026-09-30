// copyright weyouthey and jakulo !!!!!!!!!!!
// lololol

#pragma once

#ifndef _HL_BOT_LOCOMOTION_H_
#define _HL_BOT_LOCOMOTION_H_

#include "nav_area.h"
#include "improv.h"
#include "bot_util.h"

class CBot;

class CTFBotLocomotion : public CImprov
{
public:
	CTFBotLocomotion(CBot* bot);

	CBot* GetBot() const { return m_bot; }

	virtual bool IsAlive(void) const;

	virtual void MoveForward(void);
	virtual void MoveBackward(void);
	virtual void StrafeLeft(void);
	virtual void StrafeRight(void);
	virtual bool Jump(void);
	virtual void Crouch(void);
	virtual void StandUp(void);			// "un-crouch"

	virtual bool IsCrouching(void) const override;
	virtual bool IsUsingLadder(void) const override;

	virtual bool IsJumping(void) const override;
	virtual bool IsOnGround(void) const override;
	virtual bool IsMoving(void) const override;							// if true, improv is walking, crawling, running somewhere

	virtual bool IsRunning(void) const override { return true; };
	virtual bool IsWalking(void)  const override { return false; };
	virtual bool IsStopped(void) const override;

	virtual const Vector& GetFeet(void) const override;					// return position of "feet" - point below centroid of improv at feet level
	virtual const Vector& GetCentroid(void) const override;
	virtual const Vector& GetEyes(void) const override;

	virtual CNavArea* GetLastKnownArea(void) const override;

	void TrackPath(const Vector& pathGoal, float deltaT);

	virtual bool GetSimpleGroundHeightWithFloor(const Vector* pos, float* height, Vector* normal = NULL);
	// find "simple" ground height, treating current nav area as part of the floor

	virtual void StartLadder(const CNavLadder* ladder, NavTraverseType how, const Vector* approachPos, const Vector* departPos);	// invoked when a ladder is encountered while following a path
	virtual bool TraverseLadder(const CNavLadder* ladder, NavTraverseType how, const Vector* approachPos, const Vector* departPos, float deltaT);	// traverse given ladder

	// THESE HAVE NOT BEEN IMPLEMENTED. I JUST ADDED THESE HERE SO THE CLASS DOESNT BREAK
	// THEY WILL BE ADDED.... EVENTUALLY

	virtual void MoveTo(const Vector& goal) {};						///< move improv towards far-away goal (pathfind)
	virtual void LookAt(const Vector& target) {};					///< define desired view target
	virtual void ClearLookAt(void) {};								///< remove view goal
	virtual void FaceTo(const Vector& goal) {};						///< orient body towards goal
	virtual void ClearFaceTo(void) {};								///< remove body orientation goal

	virtual bool IsAtMoveGoal(float error = 20.0f) const override { return false; };			///< return true if improv is standing on its movement goal
	// NOT IMPLEMENTED

	virtual bool HasLookAt(void) const override { return false; };						///< return true if improv has a look at goal
	virtual bool HasFaceTo(void) const override { return false; };						///< return true if improv has a face to goal
	virtual bool IsAtFaceGoal(void) const override { return false; };						///< return true if improv is facing towards its face goal
	virtual bool IsFriendInTheWay(const Vector& goalPos) const override { return false; };	///< return true if a friend is blocking our line to the given goal position
	virtual bool IsFriendInTheWay(CBaseEntity* myFriend, const Vector& goalPos) const override { return false; };	///< return true if the given friend is blocking our line to the given goal position

	virtual void Run(void) {};
	virtual void Walk(void) {};
	virtual void Stop(void) {};

	virtual float GetMoveAngle(void) const override;						///< return direction of movement
	virtual float GetFaceAngle(void) const override;						///< return direction of view

	virtual bool CanRun(void) const override { return true; };
	virtual bool CanCrouch(void) const override { return true; };
	virtual bool CanJump(void) const override { return true; };

#define CHECK_FOV true
	virtual bool IsVisible(const Vector& pos, bool testFOV = CHECK_FOV) const override;	///< return true if improv can see position

	virtual bool IsPlayerLookingAtMe(CBasePlayer* other, float cosTolerance = 0.95f) const { return false;  };	///< return true if 'other' is looking right at me
	virtual CBasePlayer* IsAnyPlayerLookingAtMe(int team = 0, float cosTolerance = 0.95f) const { return NULL; };	///< return player on given team that is looking right at me (team == 0 means any team), NULL otherwise

	virtual CBasePlayer* GetClosestPlayerByTravelDistance(int team = 0, float* range = NULL) const { return NULL; };	///< return actual travel distance to closest player on given team (team == 0 means any team)

	virtual void OnUpdate(float deltaT) {};							///< a less frequent, full update 'tick'
	virtual void OnUpkeep(float deltaT) {};							///< a frequent, lightweight update 'tick'
	virtual void OnReset(void) {};									///< reset improv to initial state
	virtual void OnGameEvent(GameEventType event, CBaseEntity* entity, CBaseEntity* other) {};	///< invoked when an event occurs in the game
	virtual void OnTouch(CBaseEntity* other) {};						///< "other" has touched us

private:
	CBot* m_bot;
	mutable Vector m_feet, m_centroid, m_eyes;

	bool m_isCrouching;
	bool m_attemptingLadder;
};

#endif