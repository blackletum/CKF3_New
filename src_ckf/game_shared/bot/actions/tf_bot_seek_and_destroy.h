#ifndef TFBOTSEEKANDDESTROY
#define TFBOTSEEKANDDESTROY

#include "bot/hl_bot_action_interface.h"

class CTFBotSeekAndDestroy : public CTFAction
{

public:
	virtual void OnEnter(CBot* me);		// when getting changed/suspended into, OR when the initial action
	virtual void Update(CBot* me);		// CONTINUE status
	virtual void OnExit(CBot* me);		// CHANGE_TO or DONE status
	virtual void OnResume(CBot* me);	// action that was suspended into is now finished, so this action is now the active one
	virtual const char* GetName() const { return "SeekAndDestroy"; };

private:
	CResupplyRoom* GetClosestSpawnRoom(CBot* me, int team = -2);
};

#endif