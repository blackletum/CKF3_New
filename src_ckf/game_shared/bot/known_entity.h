#ifndef TF_BOT_KNOWN_ENTITY_H
#define TF_BOT_KNOWN_ENTITY_H

// STOLEN FROM THE SOURCE SDK LOOOOOOOOOOOOOOL
// IF IT WORKS IT WORKS

class CKnownEntity
{
public:
	// constructing assumes we currently know about this entity
	CKnownEntity(CBaseEntity* who)
	{
		m_who = who;
		m_whenLastSeen = -1.0f;
		m_whenLastBecameVisible = -1.0f;
		m_isVisible = false;
		m_whenBecameKnown = gpGlobals->time;
		m_hasLastKnownPositionBeenSeen = false;
		UpdatePosition();
	}

	virtual ~CKnownEntity() {}

	virtual void Destroy(void)
	{
		m_who = NULL;
		m_isVisible = false;
	}

	virtual void UpdatePosition(void)		// could be seen or heard, but now the entity's position is known
	{
		if (m_who)
		{
			m_lastKnownPostion = m_who->pev->origin;
			m_lastKnownArea = TheNavAreaGrid.GetNearestNavArea(&m_who->pev->origin);
			m_whenLastKnown = gpGlobals->time;
		}
	}

	virtual CBaseEntity* GetEntity(void) const
	{
		return m_who;
	}

	virtual const Vector& GetLastKnownPosition(void) const
	{
		return m_lastKnownPostion;
	}

	// Have we had a clear view of the last known position of this entity?
	// This encapsulates the idea of "I just saw a guy right over *there* a few seconds ago, but I don't know where he is now"
	virtual bool HasLastKnownPositionBeenSeen(void) const
	{
		return m_hasLastKnownPositionBeenSeen;
	}

	virtual void MarkLastKnownPositionAsSeen(void)
	{
		m_hasLastKnownPositionBeenSeen = true;
	}

	virtual const CNavArea* GetLastKnownArea(void) const
	{
		return m_lastKnownArea;
	}

	virtual float GetTimeSinceLastKnown(void) const
	{
		return gpGlobals->time - m_whenLastKnown;
	}

	virtual float GetTimeSinceBecameKnown(void) const
	{
		return gpGlobals->time - m_whenBecameKnown;
	}

	virtual void UpdateVisibilityStatus(bool visible)
	{
		if (visible)
		{
			if (!m_isVisible)
			{
				// just became visible
				m_whenLastBecameVisible = gpGlobals->time;
			}

			m_whenLastSeen = gpGlobals->time;
		}

		m_isVisible = visible;
	}

	virtual bool IsVisibleInFOVNow(void) const	// return true if this entity is currently visible and in my field of view
	{
		return m_isVisible;
	}

	virtual bool IsVisibleRecently(void) const	// return true if this entity is visible or was very recently visible
	{
		if (m_isVisible)
			return true;

		if (WasEverVisible() && GetTimeSinceLastSeen() < 3.0f)
			return true;

		return false;
	}

	virtual float GetTimeSinceBecameVisible(void) const
	{
		return gpGlobals->time - m_whenLastBecameVisible;
	}

	virtual float GetTimeWhenBecameVisible(void) const
	{
		return m_whenLastBecameVisible;
	}

	virtual float GetTimeSinceLastSeen(void) const
	{
		return gpGlobals->time - m_whenLastSeen;
	}

	virtual bool WasEverVisible(void) const
	{
		return m_whenLastSeen > 0.0f;
	}

	// has our knowledge of this entity become obsolete?
	virtual bool IsObsolete(void) const
	{
		return GetEntity() == NULL || !m_who->IsAlive() || GetTimeSinceLastKnown() > 10.0f;
	}

	virtual bool operator==(const CKnownEntity& other) const
	{
		if (GetEntity() == NULL || other.GetEntity() == NULL)
			return false;

		return (GetEntity() == other.GetEntity());
	}

	virtual bool Is(CBaseEntity* who) const
	{
		if (GetEntity() == NULL || who == NULL)
			return false;

		return (GetEntity() == who);
	}

	// custom
	// returns the player's team, or -1 if the entity isn't a player
	// mainly for bot enemy checking
	virtual int GetTeam() const
	{
		CBaseEntity* ent = GetEntity();
		if (!ent || !ent->IsPlayer())
			return -1;

		return ((CBasePlayer*)ent)->m_iTeam;
	}

private:
	CBaseEntity* m_who;
	Vector m_lastKnownPostion;
	bool m_hasLastKnownPositionBeenSeen;
	CNavArea* m_lastKnownArea;
	float m_whenLastSeen;
	float m_whenLastBecameVisible;
	float m_whenLastKnown;			// last seen or heard, confirming its existance
	float m_whenBecameKnown;
	bool m_isVisible;				// flagged by IVision update as visible or not
};
#endif