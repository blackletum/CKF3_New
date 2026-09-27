#ifndef ROOSTER_BOT_H
#define ROOSTER_BOT_H

#include "nav_path.h"

class CBot : public CBasePlayer
{
public:
	CBot(void);

	static CHLBot* CreateBot(const char* name);

	virtual void Spawn(void);

	void Think(void);

private:
	// decision logic
	void Update(void);
	void Upkeep(void);
};

#endif