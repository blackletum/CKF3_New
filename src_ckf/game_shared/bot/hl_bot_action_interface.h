// Korozbot action interface

#ifndef _HL_BOT_ACTION_INTERFACE_
#define _HL_BOT_ACTION_INTERFACE_

#include <tier1\UtlVector.h>

class CBot;

enum ActionResult
{
	DONE,
	CONTINUE,
	CHANGE_TO,
	SUSPEND_FOR,
};

class CTFAction
{
	// OVERRIDE THIS!!!
public:
	CTFAction() : m_Status(CONTINUE), debug("Continue"), new_action(NULL) {}
	virtual ~CTFAction() {}

	virtual void OnEnter(CBot* me) = 0;		// when getting changed/suspended into, OR when the initial action
	virtual void Update(CBot* me) = 0;		// CONTINUE status
	virtual void OnExit(CBot* me) = 0;		// CHANGE_TO or DONE status
	virtual void OnResume(CBot* me) = 0;	// action that was suspended into is now finished, so this action is now the active one
	virtual const char* GetName() const = 0;
	virtual bool ShouldHurry(CBot* me) = 0;

	// do not change or inherit these
	ActionResult Status() { return m_Status; }
	CTFAction* GetNewAction() { return new_action; };
	const char* GetNewActionName()
	{
		if (!new_action)
			return "NULL ACTION";

		return new_action->GetName();
	}
	void Done(const char* reason)							{ m_Status = DONE; debug = reason; new_action = NULL; }
	void Change_To(CTFAction* action, const char* reason)	{ m_Status = CHANGE_TO; debug = reason; new_action = action; }
	void Suspend_For(CTFAction* action, const char* reason)	{ m_Status = SUSPEND_FOR; debug = reason; new_action = action; }
	void Continue()											{ m_Status = CONTINUE; debug = "Continue"; new_action = NULL; };
private:
	// dont edit these yourself, otherwise Shit Might Get Weird..
	// use the helper functions above
	ActionResult m_Status;
	const char* debug;
	CTFAction* new_action;
};

enum ScenarioMoniterCheckResult
{
	RESULT_HEALTH				= (1 << 0),
	RESULT_RETREAT				= (1 << 1),
	RESULT_AMMO					= (1 << 2),
};

class CTFBotScenarioMoniterInterface
{
	// handles universal things that would normally stop a player
	// eg get health or get ammo
public:
	CTFBotScenarioMoniterInterface()
	{
		m_hpAction = NULL;
		m_retreatAction = NULL;
		m_bot = NULL;
	}

	void SetBot(CBot* me) { m_bot = me; }
	void SetHealthAction(CTFAction* action) { m_hpAction = action; }
	void SetRetreatAction(CTFAction* action) { m_retreatAction = action; }

	int Check();
	void Update(int attrib = 0);
private:
	CBot* m_bot;
	CTFAction* m_hpAction;
	CTFAction* m_retreatAction;
	// CTFAction* m_ammoAction;
};

class CTFBotActionInterface
{
public:
	CTFBotActionInterface(CTFAction* m_baseAction);

	CBot* GetBot() const { return m_bot; }
	void SetBot(CBot* me) { m_bot = me; m_scenarioMoniterInterface.SetBot(me); }

	virtual void Update();
	
	CUtlVector<CTFAction*> m_Actions;
	CTFAction* m_bAction;
	CTFBotScenarioMoniterInterface m_scenarioMoniterInterface;	// Scenario moniter interface
	void Reset(CTFAction* first = NULL);

private:
	CBot* m_bot;
	char debug_info[512];
	void DestroyAction(CTFAction* action);
};

static bool IsInside(const Vector& p, CBaseEntity* e)
{
	return p.x > e->pev->absmin.x && p.x < e->pev->absmax.x
		&& p.y > e->pev->absmin.y && p.y < e->pev->absmax.y
		&& p.z > e->pev->absmin.z && p.z < e->pev->absmax.z;
}
// this was for control points but i might as well just make it a universal thing

#endif