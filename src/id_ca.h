#ifndef __ID_CA__
#define __ID_CA__

//===========================================================================

namespace MapEdit
{
	class GameMapEditor;
}

extern  class GameMap *map;
extern	class LevelInfo *levelInfo;
extern  class MapEdit::GameMapEditor *mapeditor;

//===========================================================================

void CA_CacheMap (const class FString &mapname, bool loading);

void CA_CacheScreen (class FTexture *tex, bool noaspect=false);

#endif
