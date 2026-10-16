#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "bot/hl_bot.h"
#include "bot/actions/tf_bot_medic_heal.h"

void CTFBotMedicHeal::OnEnter(CBot* me)
{
}

void CTFBotMedicHeal::Update(CBot* me)
{
	if (!me)
		return;

	const CKnownEntity* teammate = me->GetPrimaryKnownTeammate();
	if(teammate)
		me->SetPathToGoal(teammate->GetLastKnownPosition());

	Continue();
}

void CTFBotMedicHeal::OnExit(CBot* me)
{
}

void CTFBotMedicHeal::OnResume(CBot* me)
{
}