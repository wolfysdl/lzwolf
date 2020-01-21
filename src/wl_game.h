#ifndef __WL_GAME_H__
#define __WL_GAME_H__

#include <map>
#include <string>
#include "textures/textures.h"
#include "farchive.h"

/*
=============================================================================

						WL_GAME DEFINITIONS

=============================================================================
*/

//---------------
//
// Hub World structure
//
//---------------

struct HubWorld
{
	HubWorld()
	{
	}

	bool hasMap(const std::string& mapname) const
	{
		return mapdata.find(mapname) != mapdata.end();
	}

	bool thingKilled(const std::string& mapname, int thingNum) const
	{
		return (mapdata.find(mapname) != mapdata.end() ?
			mapdata.find(mapname)->second.thingKilled(thingNum) : false);
	}

	void setThingKilled(const std::string& mapname, int thingNum)
	{
		mapdata[mapname].thingskilled[thingNum] = true;
	}

	struct MapData
	{
		MapData() :
			secretcount(0),
			treasurecount(0),
			killcount(0)
		{
		}

		bool thingKilled(int thingNum) const
		{
			return thingskilled.find(thingNum) != thingskilled.end();
		}

		short       secretcount;
		short       treasurecount;
		short       killcount;
		std::map< int, bool >    thingskilled;
	};

	std::map< std::string, MapData >    mapdata;
};

inline FArchive &operator<< (FArchive &arc, HubWorld::MapData &mapdata)
{
	arc << mapdata.secretcount
		<< mapdata.treasurecount
		<< mapdata.killcount
		<< mapdata.thingskilled;
	return arc;
}

inline FArchive &operator<< (FArchive &arc, HubWorld &hubworld)
{
	arc << hubworld.mapdata;
	return arc;
}

//---------------
//
// gamestate structure
//
//---------------

extern struct gametype
{
	char		mapname[9];
	const class SkillInfo *difficulty;
	const class ClassDef *playerClass;

	FTextureID  faceframe;

	short       secretcount,treasurecount,killcount,
				secrettotal,treasuretotal,killtotal;
	int32_t     TimeCount;
	bool        victoryflag;            // set during victory animations
	bool		fullmap;

	HubWorld*   phubworld;
} gamestate;

extern  char            demoname[13];

void    SetupGameLevel (void);
bool    GameLoop (void);
void    DrawPlayScreen (bool noborder=false);
void    DrawPlayBorderSides (void);

void    PlayDemo (int demonumber);
void    RecordDemo (void);

enum
{
	NEWMAP_KEEPFACING = 1,
	NEWMAP_KEEPPOSITION = 2
};
extern struct NewMap_t
{
	fixed x;
	fixed y;
	angle_t angle;
	int newmap;
	int flags;
} NewMap;

// JAB
#define PlaySoundLocMapSpot(s,spot)     PlaySoundLocGlobal(s,(((int32_t)spot->GetX() << TILESHIFT) + (1L << (TILESHIFT - 1))),(((int32_t)spot->GetY() << TILESHIFT) + (1L << (TILESHIFT - 1))),SD_GENERIC)
#define PlaySoundLocTile(s,tx,ty)       PlaySoundLocGlobal(s,(((int32_t)(tx) << TILESHIFT) + (1L << (TILESHIFT - 1))),(((int32_t)ty << TILESHIFT) + (1L << (TILESHIFT - 1))),SD_GENERIC)
#define PlaySoundLocActor(s,ob)         PlaySoundLocGlobal(s,(ob)->x,(ob)->y,SD_GENERIC)
#define PlaySoundLocActorBoss(s,ob)     PlaySoundLocGlobal(s,(ob)->x,(ob)->y,SD_BOSSWEAPONS)
void    PlaySoundLocGlobal(const char* s,fixed gx,fixed gy,int chan,unsigned int objId=0,bool looped=false,double attenuation=0.0,double volume=1.0);
void UpdateSoundLoc(void);

class SoundIndex;

namespace LoopedAudio
{
	typedef unsigned int ObjId;
	typedef int SndChannel;

	bool has (ObjId objId);

	void add (ObjId objId, SndChannel channel, const SoundIndex &sound, double attenuation, double volume);

	bool claimed (SndChannel channel);

	void finished (SndChannel channel);

	void updateSoundPos (void);

	void stopSoundFrom (ObjId objId);
}

#endif
