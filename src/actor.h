/*
** actor.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#ifndef __ACTOR_H__
#define __ACTOR_H__

#include "wl_def.h"
#include "actordef.h"
#include "gamemap.h"
#include "linkedlist.h"
#include "name.h"
#include "dobject.h"
#include "tflags.h"
#include "thinker.h"

enum
{
	AMETA_BASE = 0x12000,

	AMETA_Damage,
	AMETA_DropItems,
	AMETA_SecretDeathSound,
	AMETA_GibHealth,
	AMETA_DefaultHealth1,
	AMETA_DefaultHealth2,
	AMETA_DefaultHealth3,
	AMETA_DefaultHealth4,
	AMETA_DefaultHealth5,
	AMETA_DefaultHealth6,
	AMETA_DefaultHealth7,
	AMETA_DefaultHealth8,
	AMETA_DefaultHealth9,
	AMETA_ConversationID,
	AMETA_DamageResistances,
	AMETA_EnemyFactions,
};

enum
{
	SPAWN_AllowReplacement = 1,
	SPAWN_Patrol = 2
};

typedef TFlags<ActorFlag> ActorFlags;
DEFINE_TFLAGS_OPERATORS (ActorFlags)

class player_t;
class ClassDef;
class AInventory;
namespace Dialog { struct Page; }
class AActor : public Thinker,
	public EmbeddedList<AActor>::Node
{
	typedef EmbeddedList<AActor>::Node ActorLink;

	DECLARE_CLASS(AActor, Thinker)
	HAS_OBJECT_POINTERS

	public:
		struct DropItem
		{
			public:
				FName			className;
				unsigned int	amount;
				uint8_t			probability;
		};
		typedef LinkedList<DropItem> DropList;

		struct DamageResistance
		{
			public:
				const ClassDef  *damagetype;
				unsigned int	percent;
		};
		typedef LinkedList<DamageResistance> DamageResistanceList;

		struct EnemyFaction
		{
			public:
				const ClassDef  *faction;
		};
		typedef LinkedList<EnemyFaction> EnemyFactionList;

		void			AddInventory(AInventory *item);
		virtual void	BeginPlay() {}
		void			ClearCounters();
		void			ClearInventory();
		virtual void	Destroy();
		virtual void	Die();
		void			EnterZone(const MapZone *zone);
		AInventory		*FindInventory(const ClassDef *cls);
		template<class T> T *FindInventory ()
		{
			return static_cast<T *> (FindInventory (RUNTIME_CLASS(T)));
		}
		const Frame		*FindState(const FName &name) const;
		static void		FinishSpawningActors();
		int				GetDamage();
		const AActor	*GetDefault() const;
		DropList		*GetDropList() const;
		DamageResistanceList		*GetDamageResistanceList() const;
		EnemyFactionList		*GetEnemyFactionList() const;
		const MapZone	*GetZone() const { return soundZone; }
		bool			GiveInventory(const ClassDef *cls, int amount=0, bool allowreplacement=true);
		bool			InStateSequence(const Frame *basestate) const;
		bool			IsFast() const;
		virtual void	PostBeginPlay() {}
		void			RemoveFromWorld();
		virtual void	RemoveInventory(AInventory *item);
		void			Serialize(FArchive &arc);
		void			SetState(const Frame *state, bool norun=false);
		void			SpawnFog();
		static AActor	*Spawn(const ClassDef *type, fixed x, fixed y, fixed z, int flags);
		int32_t			SpawnHealth() const;
		bool			Teleport(fixed x, fixed y, angle_t angle, bool nofog=false);
		virtual void	Tick();
		virtual void	Touch(AActor *toucher) {}

		void PrintInventory();

		static PointerIndexTable<ExpressionNode> damageExpressions;
		static PointerIndexTable<DropList> dropItems;
		static PointerIndexTable<DamageResistanceList> damageResistances;
		static PointerIndexTable<EnemyFactionList> enemyFactions;

		// Spawned actor ID
		unsigned int spawnid;

		// Basic properties from objtype
		ActorFlags flags;

		int32_t	distance; // if negative, wait for that door to open
		dirtype	dir;

#pragma pack(push, 1)
// MSVC and older versions of GCC don't support constant union parts
// We do this instead of just using a regular word since writing to tilex/y
// indicates an error.
#if !defined(_MSC_VER) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 6)
#define COORD_PART const word
#else
#define COORD_PART word
#endif
		union
		{
			fixed x;
#ifdef __BIG_ENDIAN__
			struct { COORD_PART tilex; COORD_PART fracx; };
#else
			struct { COORD_PART fracx; COORD_PART tilex; };
#endif
		};
		union
		{
			fixed y;
#ifdef __BIG_ENDIAN__
			struct { COORD_PART tiley; COORD_PART fracy; };
#else
			struct { COORD_PART fracy; COORD_PART tiley; };
#endif
		};
#pragma pack(pop)
		fixed z;
		unsigned int viewplanenum;
		fixed	velx, vely;

		angle_t	angle;
		angle_t pitch;
		int32_t	health;
		int32_t	speed, runspeed;
		int		points;
		fixed	radius;
		fixed	projectilepassheight;

		const Frame		*state;
		unsigned int	sprite;
		fixed			scaleX, scaleY;
		short			ticcount;

		short       viewx;
		word        viewheight;
		fixed       transx,transy;      // in global coord

		FTextureID	overheadIcon;

		uint16_t	sighttime;
		uint8_t		sightrandom;
		fixed		missilefrequency;
		uint16_t	minmissilechance;
		short		movecount; // Emulation of Doom's movecount
		fixed		meleerange;
		uint16_t	painchance;
		FNameNoInit	activesound, attacksound, deathsound, painsound, seesound;
		FNameNoInit	actionns;

		const Frame *SpawnState, *SeeState, *PathState, *PainState, *MeleeState, *MissileState, *RadiusWakeState;
		short       temp1,hidden;
		fixed		killerx,killery; // For deathcam
		const ClassDef  *killerdamagetype;
		int         picX, picY;
		const ClassDef  *faction;

		TObjPtr<AActor> target;
		player_t	*player;	// Only valid with APlayerPawn

		TObjPtr<AActor> missileParent;

		TObjPtr<AInventory>	inventory;

		const Dialog::Page *conversation;

		static EmbeddedList<AActor>::List actors;
		typedef EmbeddedList<AActor>::Iterator Iterator;
		static Iterator GetIterator() { return Iterator(actors); }
	protected:
		void	Init();

		const MapZone	*soundZone;
};

// This could easily be a bool but then it'd be much harder to find later. ;)
enum replace_t
{
	NO_REPLACE = 0,
	ALLOW_REPLACE = 1
};

template<class T>
inline T *Spawn (fixed x, fixed y, fixed z, replace_t replace)
{
	return static_cast<T *>(AActor::Spawn (RUNTIME_CLASS(T), x, y, z, replace));
}

// Old save compatibility
// FIXME: Remove for 1.4
class AActorProxy : public Thinker
{
	DECLARE_CLASS(AActorProxy, Thinker)

public:
	void Tick() {}

	void Serialize(FArchive &arc);

	TObjPtr<AActor> actualObject;
};

namespace ActorSpawnID
{
	extern std::map<unsigned int, AActor *> Actors;

	void Serialize(FArchive &arc);
}

#endif
