#ifndef HL_BOT_MANAGER_H
#define HL_BOT_MANAGER_H

#include <vector>

#include "nav.h"
#include "nav_node.h"
#include "nav_area.h"	// NavEditCmdType

class CBasePlayer;

unsigned int GetNavPlace( void );
void SetNavPlace( unsigned int place );

//--------------------------------------------------------------------------------------------------------------
class CHLBotManager
{
public:
	CHLBotManager();

	void ServerActivate( void );				// new map has spawned - load its nav mesh
	void ServerDeactivate( void );				// map is changing - free nav data
	void StartFrame( void );					// invoked once per server frame
	void ClientDisconnect( CBasePlayer *player );

	void ServerCommand( const char *pcmd );		// handle the "bot_*" server commands
	void AddServerCommands( void );

	bool IsNavMeshLoaded( void ) const			{ return m_navLoaded; }
	bool IsNavMeshGenerating(void) const { return m_isGenerating; };

	const char *GetNavMapFilename( void ) const;	// return the filename for this map's .nav file

private:
	bool m_navLoaded;							// true if a nav mesh is loaded for the current map
	NavEditCmdType m_editCmd;					// queued nav edit command, consumed each frame

	bool m_isGenerating;						// true while incrementally sampling walkable space
	CNavNode *m_currentNode;					// sampling frontier node
	NavDirType m_generationDir;					// direction being sampled this step
	std::vector<Vector> m_walkableSeeds;		// seed spots (player spawns) to sample from
	int m_seedIndex;							// next seed to use

	void BeginNavGeneration( void );
	void UpdateNavGeneration( void );
	bool SampleStep( void );
	CNavNode *AddSampledNode( const Vector *destPos, const Vector *normal, NavDirType dir, CNavNode *source );
	CNavNode *GetNextWalkableSeedNode( void );

	void AddServerCommand( const char *cmd );
};

extern CHLBotManager *TheHLBots;

// for game.cpp
void Bot_RegisterCvars( void );
void InstallBotControl( void );

#endif // HL_BOT_MANAGER_H
