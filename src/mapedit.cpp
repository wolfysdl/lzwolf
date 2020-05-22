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
#include "id_ca.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "mapedit.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_play.h"
#include "c_dispatch.h"
#include "v_text.h"
#include "thingdef/thingdef.h"
#include "am_map.h"
#include "uwmfdoc.h"
using namespace MapEdit;

CVAR(Bool, me_marker, false, CVAR_ARCHIVE)

GameMapEditor::GameMapEditor() : spot(NULL), armlength(TILEGLOBAL*2)
{
	InitMarkedSector();
}

size_t GameMapEditor::GetTileCount() const
{
	return map->tilePalette.Size();
}

size_t GameMapEditor::GetSectorCount() const
{
	return map->sectorPalette.Size();
}

std::pair<fixed, fixed> GameMapEditor::GetCurLoc() const
{
	fixed x, y;
	if(automap == AMA_Normal)
	{
		x = viewx;
		y = viewy;
	}
	else
	{
		x = viewx + FixedMul(armlength,viewcos);
		y = viewy - FixedMul(armlength,viewsin);
	}
	x = (x & ~(TILEGLOBAL-1)) + (TILEGLOBAL/2);
	y = (y & ~(TILEGLOBAL-1)) + (TILEGLOBAL/2);
	return std::make_pair(x, y);
}

MapSpot GameMapEditor::GetCurSpot()
{
	int tilex,tiley;
	tilex = GetCurLoc().first >> TILESHIFT;
	tiley = GetCurLoc().second >> TILESHIFT;
	return map->IsValidTileCoordinate(tilex, tiley, 0) ?
		map->GetSpot(tilex, tiley, 0) : NULL;
}

void GameMapEditor::InitMarkedSector()
{
	FTextureID defaultMarked = TexMan.GetTexture("#ffff00", FTexture::TEX_Flat);
	markedSector.texture[MapSector::Floor] = defaultMarked;
	markedSector.texture[MapSector::Ceiling] = defaultMarked;
	markedSector.overhead = defaultMarked;
}

void GameMapEditor::ConvertToDoc(const GameMap &map, UwmfDoc::Document &doc)
{
	doc.globProp.ns = "Wolf3D";
	//doc.globProp.name;
	doc.globProp.tileSize = map.header.tileSize;
	doc.globProp.width = map.header.width;
	doc.globProp.height = map.header.height;

	for (unsigned i = 0; i < map.tilePalette.Size(); i++)
	{
		const MapTile &mapTile = map.tilePalette[i];

		UwmfDoc::Tile tile;
		FTexture *tex;

		tex = TexMan(mapTile.texture[MapTile::East]);
		if (tex != NULL)
			tile.textureEast = tex->Name;

		tex = TexMan(mapTile.texture[MapTile::West]);
		if (tex != NULL)
			tile.textureWest = tex->Name;

		tex = TexMan(mapTile.texture[MapTile::South]);
		if (tex != NULL)
			tile.textureSouth = tex->Name;

		tex = TexMan(mapTile.texture[MapTile::North]);
		if (tex != NULL)
			tile.textureNorth = tex->Name;

		tile.blockingEast = mapTile.sideSolid[MapTile::East];
		tile.blockingWest = mapTile.sideSolid[MapTile::West];
		tile.blockingSouth = mapTile.sideSolid[MapTile::South];
		tile.blockingNorth = mapTile.sideSolid[MapTile::North];
		tile.offsetVertical = mapTile.offsetVertical;
		tile.offsetHorizontal = mapTile.offsetHorizontal;
		tile.dontOverlay = mapTile.dontOverlay;
		tile.mapped.val() = mapTile.mapped;

		if (mapTile.soundSequence.IsValidName())
			tile.soundSequence.val() = mapTile.soundSequence.GetChars();

		tex = TexMan(mapTile.overhead);
		if (tex != NULL)
			tile.textureOverhead.val() = tex->Name;

		doc.tiles.push_back(tile);
	}

	for (unsigned i = 0; i < map.sectorPalette.Size(); i++)
	{
		MapSector &mapSector = map.sectorPalette[i];

		UwmfDoc::Sector sector;
		FTexture *tex;

		tex = TexMan(mapSector.texture[MapSector::Floor]);
		if (tex != NULL)
			sector.textureFloor = tex->Name;

		tex = TexMan(mapSector.texture[MapSector::Ceiling]);
		if (tex != NULL)
			sector.textureCeiling = tex->Name;

		doc.sectors.push_back(sector);
	}

	for (unsigned i = 0; i < map.zonePalette.Size(); i++)
	{
		MapZone &mapZone = map.zonePalette[i];

		UwmfDoc::Zone zone;

		doc.zones.push_back(zone);
	}

	for (unsigned z = 0; z < map.planes.Size(); z++)
	{
		MapPlane &mapPlane = map.planes[z];

		UwmfDoc::Plane plane;
		plane.depth = mapPlane.depth;

		doc.planes.push_back(plane);

		doc.planemaps.push_back(UwmfDoc::Planemap());
		UwmfDoc::Planemap &planemap = doc.planemaps.back();

		unsigned int x,y;
		for (y = 0; y < map.header.height; y++)
		{
			for (x = 0; x < map.header.width; x++)
			{
				MapSpot mapSpot = map.GetSpot(x,y,z);

				UwmfDoc::Planemap::Spot spot;
				if (mapSpot->tile != NULL)
					spot.tileind = mapSpot->tile - &map.tilePalette[0];
				if (mapSpot->sector != NULL)
					spot.sectorind = mapSpot->sector - &map.sectorPalette[0];
				if (mapSpot->zone != NULL)
					spot.zoneind = mapSpot->zone - &map.zonePalette[0];
				spot.tag = mapSpot->tag;

				planemap.spots.push_back(spot);
			}
		}
	}
}

AdjustGameMap::AdjustGameMap() : spot(NULL), tile(NULL), sector(NULL)
{
	if (me_marker)
	{
		spot = mapeditor->GetCurSpot();
		if (spot != NULL)
		{
			tile = spot->tile;
			sector = spot->sector;
			spot->SetTile(NULL);
			spot->sector = &mapeditor->markedSector;
		}
	}
}

AdjustGameMap::~AdjustGameMap()
{
	if (spot)
	{
		spot->SetTile(tile);
		spot->sector = sector;
	}
}

CCMD(spotinfo)
{
	MapSpot spot = mapeditor->GetCurSpot();

	size_t tileind = map->GetTileIndex(spot->tile);
	size_t sectorind = map->GetSectorIndex(spot->sector);
	if (map->GetTile(tileind) != NULL)
		Printf("tile = %lu\n", tileind);
	if (map->GetSector(sectorind) != NULL)
		Printf("sector = %lu\n", sectorind);
}

CCMD(spawnactor)
{
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

	AActor *actor = AActor::Spawn(cls, mapeditor->GetCurLoc().first, mapeditor->GetCurLoc().second, 0, SPAWN_AllowReplacement);
	actor->angle = players[ConsolePlayer].camera->angle;
	actor->dir = nodir;
}

CCMD(savemap)
{
	if (argv.argc() < 3)
	{
		Printf("Usage: savemap <mapname> <wadpath>\n");
		return;
	}

	const char *mapname = argv[1];
	const char *wadpath = argv[2];

	UwmfDoc::Document uwmfdoc;
	mapeditor->ConvertToDoc(*map, uwmfdoc);
	uwmfdoc.globProp.name = mapname;

	UwmfToWadWriter::Write(uwmfdoc, mapname, wadpath);
}

// ==================================
//          CVAR me_tile
// ==================================

DYNAMIC_CVAR_GETTER(Int, me_tile)
{
	MapSpot spot = mapeditor->GetCurSpot();
	return (spot != NULL && spot->tile != NULL ?
		map->GetTileIndex(spot->tile) : -1);
}

DYNAMIC_CVAR_SETTER(Int, me_tile)
{
	const MapTile *tile = map->GetTile(value);

	MapSpot spot = mapeditor->GetCurSpot();
	if (spot == NULL)
	{
		Printf(TEXTCOLOR_RED " Invalid spot!\n");
		return false;
	}

	spot->SetTile(tile);
	return true;
}

DYNAMIC_CVAR(Int, me_tile, 0, CVAR_NOFLAGS)


// ==================================
//          CVAR me_sector
// ==================================

DYNAMIC_CVAR_GETTER(Int, me_sector)
{
	MapSpot spot = mapeditor->GetCurSpot();
	return (spot != NULL && spot->sector != NULL ?
		map->GetSectorIndex(spot->sector) : -1);
}

DYNAMIC_CVAR_SETTER(Int, me_sector)
{
	const MapSector *sector = map->GetSector(value);

	MapSpot spot = mapeditor->GetCurSpot();
	if (spot == NULL)
	{
		Printf(TEXTCOLOR_RED " Invalid spot!\n");
		return false;
	}

	spot->sector = sector;
	return true;
}

DYNAMIC_CVAR(Int, me_sector, 0, CVAR_NOFLAGS)
