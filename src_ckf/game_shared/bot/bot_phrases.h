// bot_phrases.h
// leftover from cs:cz, this just puts names to areas
// pointless really

#ifndef BOT_PHRASES_H
#define BOT_PHRASES_H

#include <vector>
#include "nav.h"		// for the Place typedef and UNDEFINED_PLACE

class BotPhraseManager
{
public:
	~BotPhraseManager();

	void Reset( void );							///< forget all known places (invoked on map change)

	Place NameToID( const char *name );			///< return ID for a place name, registering it if new
	const char *IDToName( Place place ) const;	///< return name for a place ID, or NULL if unknown

private:
	std::vector<char *> m_placeNames;			///< index + 1 == Place ID
};

extern BotPhraseManager *TheBotPhrases;

#endif // BOT_PHRASES_H
