#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "bot/hl_bot.h"
#include "bot/actions/tf_bot_idle.h"

void CTFBotIdle::OnEnter(CBot* me)
{
}

void CTFBotIdle::Update(CBot* me)
{
	Continue();
	// intentionally empty!!!
}

void CTFBotIdle::OnExit(CBot* me)
{
}

void CTFBotIdle::OnResume(CBot* me)
{
}