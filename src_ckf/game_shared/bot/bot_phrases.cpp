// See bot_phrases.h for why this exists
// THIS IS PROBABLY NOT NEEDED ANYMORE CONSIDERING ROOSTER FORTRESS DOES NOT NEED THIS
// WYT: JAKULO SAID HE WANTS TO USE IT BUT THAT FUCKER IS LAZY!!!!!!!!!!!!!!!!!!!!!

#include "extdll.h"
#include "util.h"

#include "shared_util.h"	// CloneString()
#include "bot_phrases.h"

static BotPhraseManager s_botPhrases;
BotPhraseManager *TheBotPhrases = &s_botPhrases;

//--------------------------------------------------------------------------------------------------------------
BotPhraseManager::~BotPhraseManager()
{
	Reset();
}

//--------------------------------------------------------------------------------------------------------------

void BotPhraseManager::Reset( void )
{
	for ( std::vector<char *>::iterator it = m_placeNames.begin(); it != m_placeNames.end(); ++it )
		delete [] *it;

	m_placeNames.clear();
}

//--------------------------------------------------------------------------------------------------------------

Place BotPhraseManager::NameToID( const char *name )
{
	if (name == NULL || *name == '\0')
		return UNDEFINED_PLACE;

	for ( unsigned int i = 0; i < m_placeNames.size(); ++i )
	{
		if (!stricmp( m_placeNames[i], name ))
			return (Place)(i + 1);
	}

	// new place - register it
	m_placeNames.push_back( CloneString( name ) );
	return (Place)m_placeNames.size();
}

//--------------------------------------------------------------------------------------------------------------

const char *BotPhraseManager::IDToName( Place place ) const
{
	if (place == UNDEFINED_PLACE || place > m_placeNames.size())
		return NULL;

	return m_placeNames[ place - 1 ];
}
