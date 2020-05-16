/*
** mapedit.cpp
**
**---------------------------------------------------------------------------
** Copyright 2020 Linux Wolf
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "wl_def.h"
#include "am_map.h"
#include "colormatcher.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "r_data/colormaps.h"
#include "r_sprites.h"
#include "v_font.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"
#include "c_dispatch.h"
#include "v_text.h"
#include "thingdef/thingdef.h"

class GameMapEditor
{
public:
	MapSpot spot;
	fixed armlength;

	GameMapEditor() : spot(NULL), armlength(TILEGLOBAL*2)
	{
	}

	GameMap::Tile *GetTile(unsigned int index) const
	{
		if(index > map->tilePalette.Size())
			return NULL;
		return &map->tilePalette[index];
	}

	size_t GetTileCount() const
	{
		return map->tilePalette.Size();
	}

	std::pair<fixed, fixed> GetCurLoc() const
	{
		fixed x, y;
		x = players[ConsolePlayer].camera->x + FixedMul(armlength,viewcos);
		y = players[ConsolePlayer].camera->y - FixedMul(armlength,viewsin);
		return std::make_pair(x, y);
	}

	MapSpot GetCurSpot()
	{
		int tilex,tiley;
		tilex = GetCurLoc().first >> TILESHIFT;
		tiley = GetCurLoc().second >> TILESHIFT;
		return map->IsValidTileCoordinate(tilex, tiley, 0) ?
			map->GetSpot(tilex, tiley, 0) : NULL;
	}

	static GameMapEditor *GetInstance();
};

GameMapEditor *GameMapEditor::GetInstance()
{
	static GameMapEditor inst;
	return &inst;
}

CCMD(spotinfo)
{
	GameMapEditor *editor = GameMapEditor::GetInstance();

	MapSpot spot = editor->GetCurSpot();

	size_t tileind = map->GetTileIndex(spot->tile);
	size_t sectorind = map->GetSectorIndex(spot->sector);
	if (map->GetTile(tileind) != NULL)
		Printf("tile = %lu\n", tileind);
	if (map->GetSector(sectorind) != NULL)
		Printf("sector = %lu\n", sectorind);
}

CCMD(settile)
{
	GameMapEditor *editor = GameMapEditor::GetInstance();

	if (argv.argc() < 2)
	{
		Printf("Usage: settile <tileind>\n");
		return;
	}

	unsigned tileind = INT_MAX;
	sscanf(argv[1], "%d", &tileind);

	MapTile *tile = editor->GetTile(tileind);
	if (tile == NULL)
	{
		Printf(TEXTCOLOR_RED " Index must be in range 0 - %lu!\n",
			editor->GetTileCount());
		return;
	}

	MapSpot spot = editor->GetCurSpot();
	spot->SetTile(tile);
}

CCMD(spawnactor)
{
	GameMapEditor *editor = GameMapEditor::GetInstance();

	if (argv.argc() < 2)
	{
		Printf("Usage: spawnactor <type>\n");
		return;
	}

	const ClassDef *cls = ClassDef::FindClass(argv[1]);
	if(cls == NULL)
	{
		Printf(TEXTCOLOR_RED " Unknown thing\n");
		return;
	}

	AActor *actor = AActor::Spawn(cls, editor->GetCurLoc().first, editor->GetCurLoc().second, 0, SPAWN_AllowReplacement);
	actor->angle = players[ConsolePlayer].camera->angle;
	actor->dir = nodir;
}
