/*
** bibendovsky.cpp
**
**---------------------------------------------------------------------------
** Copyright (c) 1992-2013 Apogee Entertainment, LLC
** Copyright (c) 2013-2021 Boris I. Bendovsky (bibendovsky@hotmail.com)
** 
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the
** Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifdef USE_GPL

#include <climits>

#include "doomerrors.h"
#include "id_ca.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "gamemap_common.h"
#include "lnspec.h"
#include "scanner.h"
#include "thingdef/thingdef.h"
#include "tmemory.h"
#include "w_wad.h"
#include "wl_game.h"
#include "wl_shade.h"
#include "m_random.h"

#include <vector>
#include "wl_agent.h"
#include "wl_play.h"
#include "id_sd.h"
#include "a_inventory.h"

#define MAX_CACHE_MSGS (30)
#define MAX_CACHE_MSG_LEN (190)

// ------------------------- BASIC STRUCTURES -----------------------------

// Basic 'message info' structure
//
struct mCacheInfo
{
	// where msg is in 'local' list
	// !!! Used in saved game.
	std::uint8_t local_val;

	// where msg was in 'global' list
	// !!! Used in saved game.
	std::uint8_t global_val;

	// pointer to message
	char* mSeg;
}; // mCacheInfo

// Basic 'message list' structure
//
struct mCacheList
{
	std::int16_t NumMsgs; // number of messages
	mCacheInfo mInfo[MAX_CACHE_MSGS]; // table of message 'info'
}; // mCacheList

// ----------------------- CONCESSION STRUCTURES --------------------------

// Concession 'message info' structure
//
struct con_mCacheInfo
{
	mCacheInfo mInfo;

	// type of concession
	// !!! Used in saved game.
	std::uint8_t type;

	// # of times req'd to operate
	// !!! Used in saved game.
	std::uint8_t operate_cnt;
}; // con_mCacheInfo

// Concession 'message list' structure
//
struct concession_t
{
	// also, num concessions
	// !!! Used in saved game.
	std::int16_t NumMsgs;

	// !!! Used in saved game.
	con_mCacheInfo cmInfo[MAX_CACHE_MSGS];
}; // concession_t

// ------------------------ INFORMANT STRUCTURES --------------------------
//
// Informant 'message info' structure
//
struct sci_mCacheInfo
{
	mCacheInfo mInfo;
	std::uint8_t areanumber; // 'where' msg can be used
}; // sci_mCacheInfo

// Informant 'message list' structure
//
struct scientist_t
{
	std::int16_t NumMsgs;
	sci_mCacheInfo smInfo[MAX_CACHE_MSGS];
}; // scientist_t

#define MAX_INF_AREA_MSGS (6)
#define MAXCONCESSIONS (15) // max number of concession machines

#define CT_FOOD 0x1
#define CT_BEVS 0x2

char* InfAreaMsgs[MAX_INF_AREA_MSGS];
std::uint8_t NumAreaMsgs, LastInfArea;
std::int16_t FirstGenInfMsg, TotalGenInfMsgs;

concession_t ConHintList = {};
scientist_t InfHintList; // Informant messages
scientist_t NiceSciList; // Non-informant, non-pissed messages
scientist_t MeanSciList; // Non-informant, pissed messages

// ==========================================================================
//
//                    'SPECIAL MESSAGE' CACHING SYSTEM
//
// When creating special 'types' of message caching structures, make sure
// all 'special data' is placed at the end of the BASIC message structures.
// In memory, BASIC INFO should appear first. ex:
//
// mCacheList
// ---> NumMsgs
// ---> mCacheInfo
//      ---> local_val
//      ---> global_val
//      ---> mSeg
//
// ... all special data follows ...
//
// ==========================================================================
//
void FreeMsgCache(
	mCacheList* mList,
	std::uint16_t infoSize);

void InitMsgCache(
	mCacheList* mList,
	std::uint16_t listSize,
	std::uint16_t infoSize)
{
	FreeMsgCache(mList, infoSize);

	std::uninitialized_fill_n(
		reinterpret_cast<std::uint8_t*>(mList),
		listSize,
		0
	);
}

void FreeMsgCache(
	mCacheList* mList,
	std::uint16_t infoSize)
{
	mCacheInfo* ci = mList->mInfo;
	char* ch_ptr;

	while (mList->NumMsgs--)
	{
		delete[] ci->mSeg;
		ci->mSeg = nullptr;

		ch_ptr = (char*)ci;
		ch_ptr += infoSize;
		ci = (mCacheInfo*)ch_ptr;
	}
}

// ---------------------------------------------------------------------------
// LoadMsg()
//
// Loads the specific message in FROM a given 'grsegs' block TO the
// the memory address provided.  Memory allocation and handleing prior and
// after this function usage is responsibility of the calling function(s).
//
// PARAMS:  hint_buffer - Destination address to store message
//          block  - GrSeg for messages in VGAGRAPH.BS?
//          MsgNum - Message number to load
//          MaxMsgLen - Max len of cache msg (Len of hint_buffer)
//
// RETURNS : Returns the length of the loaded message
// ---------------------------------------------------------------------------
std::int16_t LoadMsg(
	char* hint_buffer,
	const char *block,
	std::uint16_t MsgNum,
	std::uint16_t MaxMsgLen)
{
	const auto msg_xx = "^XX";

	char* Message, *EndOfMsg;
	std::int16_t pos = 0;

	FWadLump msglump = Wads.OpenLumpName (block);

	auto lumplen = msglump.GetLength();
	char msgdata[lumplen];
	if(msglump.Read(msgdata, lumplen) != lumplen)
	{
		Quit("Failed to read message lump!");
	}

	Message = static_cast<char*>(msgdata);

	// Search for end of MsgNum-1 (Start of our message)
	//
	while (--MsgNum)
	{
		Message = strstr(Message, msg_xx);

		if (!Message)
		{
			Quit("Invalid 'Cached Message' number");
		}

		Message += 3;           // Bump to start of next Message
	}

	// Move past LFs and CRs that follow "^XX"
	//
	while ((*Message == '\n') || (*Message == '\r'))
	{
		Message++;
	}

	// Find the end of the message
	//
	if ((EndOfMsg = strstr(Message, msg_xx)) == nullptr)
	{
		Quit("Invalid 'Cached Message' number");
	}
	EndOfMsg += 3;

	// Copy to a temp buffer
	//
	while (Message != EndOfMsg)
	{
		if (*Message != '\n')
		{
			hint_buffer[pos++] = *Message;
		}

		if (pos >= MaxMsgLen)
		{
			Quit("Cached Hint Message is to Long for allocated space.");
		}

		Message++;
	}

	hint_buffer[pos] = 0; // Null Terminate
	return pos;
}

// ---------------------------------------------------------------------------
// CacheMsg()
//
// Caches the specific message in FROM a given 'grsegs' block TO the
// next available message segment pointer.
// ---------------------------------------------------------------------------
void CacheMsg(
	mCacheInfo* ci,
	const char *block,
	std::uint16_t MsgNum)
{
	ci->mSeg = new char[MAX_CACHE_MSG_LEN];
	LoadMsg(ci->mSeg, block, MsgNum, MAX_CACHE_MSG_LEN);
}

/*
=============================================================================

 CONCESSION MACHINES

=============================================================================
*/

// --------------------------------------------------------------------------
// SpawnConcession()
// --------------------------------------------------------------------------
int GameMap::SpawnConcession(std::uint16_t credits, std::uint16_t machinetype)
{
	con_mCacheInfo* ci = &ConHintList.cmInfo[ConHintList.NumMsgs];

	if (ConHintList.NumMsgs >= MAXCONCESSIONS)
	{
		Quit("Too many concession machines in level.");
	}

	if (credits != PUSHABLETILE)
	{
		switch (credits & 0xff00)
		{
		case 0:
		case 0xFC00: // Food Id
			ci->mInfo.local_val = credits & 0xff;
			ci->operate_cnt = 0;
			ci->type = static_cast<std::uint8_t>(machinetype);
			break;
		}
	}

	// Consider it a solid wall (val != 0)
	//
	if (++ConHintList.NumMsgs > MAX_CACHE_MSGS)
	{
		Quit("(CONCESSIONS) Too many 'cached msgs' loaded.");
	}

	return ConHintList.NumMsgs;
}

void GameMap::OperateConcession(std::uint16_t concession)
{
	con_mCacheInfo* ci;
	bool ok = false;

	auto tokens = []() {
		if(players[0].mo)
		{
			ACoinItem *coins = players[0].mo->FindInventory<ACoinItem>();
			if(coins)
			{
				return coins->amount;
			}
		}
		return (unsigned)0;
	};

	auto use_token = []() {
		if(players[0].mo)
		{
			ACoinItem *coins = players[0].mo->FindInventory<ACoinItem>();
			if(coins && coins->Use())
			{
				--coins->amount;
			}
		}
	};

	auto HealSelf = [](int amount) {
		auto &hp = players[0].health;
		hp = std::min(hp + amount, 100);
	};

	ci = &ConHintList.cmInfo[concession - 1];

	switch (ci->type)
	{
	case CT_FOOD:
	case CT_BEVS:
		if (ci->mInfo.local_val)
		{
			if (players[ConsolePlayer].health == 100)
			{
				StatusBar->InfoMessage("$BLAKE_CANT_EAT");
				SD_PlaySound ("player/usefail");

				return;
			}
			else
			{
				ok = true;
			}
		}
		break;
	}

	if (ok)
	{
		// Whada' ya' want?

		switch (ci->type)
		{
		case CT_FOOD:
		case CT_BEVS:
			// One token please... Thank you.

			if (!tokens())
			{
				StatusBar->InfoMessage("$BLAKE_NO_TOKENS");
				SD_PlaySound ("player/usefail");

				return;
			}
			else
			{
				use_token();
			}

			ci->mInfo.local_val--;
			SD_PlaySound ("misc/coin_pickup");


			switch (ci->type)
			{
			case CT_FOOD:
				StatusBar->InfoMessage("$BLAKE_FOOD1");
				HealSelf(10);
				break;

			case CT_BEVS:
				StatusBar->InfoMessage("$BLAKE_BEVS1");
				HealSelf(7);
				break;
			}
			break;
		}
	}
	else
	{
		StatusBar->InfoMessage("$BLAKE_OUTOFORDER");
		SD_PlaySound ("player/usefail");
	}
}

bool ReuseMsg(
	mCacheInfo* ci,
	std::int16_t count,
	std::int16_t struct_size)
{
	char* scan_ch = (char*)ci;
	mCacheInfo* scan = (mCacheInfo*)(scan_ch - struct_size);

	// Scan through all loaded messages -- see if we're loading one already
	// cached-in.
	//
	while (count--)
	{
		// Is this message already cached in?
		//
		if (scan->global_val == ci->global_val)
		{
			ci->local_val = scan->local_val;
			return true;
		}

		// Funky structure decrement... (structures can be any size...)
		//
		scan_ch = (char*)scan;
		scan_ch -= struct_size;
		scan = (mCacheInfo*)scan_ch;
	}

	return false;
}

void GameMap::ResetHints()
{
	InitMsgCache((mCacheList*)&ConHintList, sizeof(ConHintList),
			sizeof(ConHintList.cmInfo[0]));
	InitMsgCache((mCacheList*)&InfHintList, sizeof(InfHintList),
			sizeof(InfHintList.smInfo[0]));
	InitMsgCache((mCacheList*)&NiceSciList, sizeof(NiceSciList),
			sizeof(InfHintList.smInfo[0]));
	InitMsgCache((mCacheList*)&MeanSciList, sizeof(MeanSciList),
			sizeof(InfHintList.smInfo[0]));
}

void GameMap::InitInformantMessageState()
{
	// Init informant stuff
	//
	auto count = InfHintList.NumMsgs;
	LastInfArea = 0xff;
	FirstGenInfMsg = 0;
	sci_mCacheInfo* ci = InfHintList.smInfo;
	for (; (ci->areanumber != 0xff) && (count--); ci++)
	{
		FirstGenInfMsg++;
	}

	TotalGenInfMsgs = InfHintList.NumMsgs - FirstGenInfMsg;
}

void GameMap::ProcessHintTile(uint8_t tilehi, uint8_t tilelo, uint8_t areanumber)
{
	std::int16_t tile;
	std::uint16_t* start;

	sci_mCacheInfo* ci;
	scientist_t* st = nullptr;
	const char *block;

	switch (tilehi)
	{
	case 0xf1:
		block = "INFOHINT";
		st = &InfHintList;
		break;

	case 0xf2:
		block = "NISCHINT";
		st = &NiceSciList;
		break;

	case 0xf3:
		block = "MESCHINT";
		st = &MeanSciList;
		break;
	}

	ci = &st->smInfo[st->NumMsgs];
	ci->mInfo.local_val = 0xff;
	ci->mInfo.global_val = tilelo;
	if (!ReuseMsg((mCacheInfo*)ci, st->NumMsgs, sizeof(sci_mCacheInfo)))
	{
		CacheMsg((mCacheInfo*)ci, block, ci->mInfo.global_val);
		ci->mInfo.local_val = static_cast<std::uint8_t>(InfHintList.NumMsgs);
	}

	if (++st->NumMsgs > MAX_CACHE_MSGS)
	{
		Quit("(INFORMANTS) Too many \"cached msgs\" loaded.");
	}

	ci->areanumber = areanumber;
}

const char *GameMap::GetInformantMessage(AActor *ob, FRandom &rng)
{
	const char* msgptr = nullptr;

	auto areanumber = [](AActor *ob) {
		auto zone = ob->GetZone();

		int num = 0xff;
		if(zone != nullptr)
		{
			if(zone->hintareanum != -1)
				num = zone->hintareanum;
			else
				num = zone->index;
		}

		return static_cast<uint8_t>(num);
	};

	auto Random = [&rng](auto mod) {
		if(mod == 0)
		{
			Quit("Invalid zero operand to Random!");
		}
		return rng(static_cast<int>(mod));
	};

	// If new areanumber OR no 'area msgs' have been compiled, compile
	// a list of all special messages for this areanumber.
	//
	if ((LastInfArea == 0xFF) || (LastInfArea != areanumber(ob)))
	{
		NumAreaMsgs = 0;

		for (auto i = 0; i < InfHintList.NumMsgs; ++i)
		{
			const auto& ci = InfHintList.smInfo[i];

			if (ci.areanumber == 0xFF)
			{
				break;
			}

			if (ci.areanumber == areanumber(ob))
			{
				InfAreaMsgs[NumAreaMsgs++] = InfHintList.smInfo[ci.mInfo.local_val].mInfo.mSeg;
			}
		}

		LastInfArea = areanumber(ob);
	}

	// Randomly select an informant hint, either: specific to areanumber
	// or general hint...
	//
	if (NumAreaMsgs)
	{
		if (ob->informant.ammo != areanumber(ob))
		{
			ob->informant.s_tilex = 0xFF;
		}

		ob->informant.ammo = areanumber(ob);

		if (ob->informant.s_tilex == 0xFF)
		{
			ob->informant.s_tilex = static_cast<std::uint8_t>(Random(NumAreaMsgs));
		}

		msgptr = InfAreaMsgs[ob->informant.s_tilex];
	}
	else
	{
		if (ob->informant.s_tiley == 0xff)
		{
			ob->informant.s_tiley = static_cast<std::uint8_t>(FirstGenInfMsg + Random(TotalGenInfMsgs));
		}

		msgptr = InfHintList.smInfo[ob->informant.s_tiley].mInfo.mSeg;
	}

	// Still no msgptr? This is a shared message! Use smInfo[local_val]
	// for this message.
	//
	if (!msgptr)
	{
		msgptr = InfHintList.smInfo[InfHintList.smInfo[ob->informant.s_tiley].mInfo.local_val].mInfo.mSeg;
	}

	return msgptr;
}

const char *GameMap::GetScientistMessage(AActor *ob, FRandom &rng)
{
	const char* msgptr = nullptr;

	auto Random = [&rng](auto mod) {
		if(mod == 0)
		{
			Quit("Invalid zero operand to Random!");
		}
		return rng(static_cast<int>(mod));
	};

	// Non-Informant
	//
	scientist_t* st;

	if ((ob->extraflags & FL_MUSTATTACK) != 0 || (rng() < 128))
	{
		// Mean
		//
		ob->extraflags &= ~FL_FRIENDLY; // Make him attack!
		//ob->extraflags |= FL_INTERROGATED; //  "    "     "
		st = &MeanSciList;
	}
	else
	{
		// Nice
		//
		ob->extraflags |= FL_MUSTATTACK; // Make him mean!
		st = &NiceSciList;
	}

	msgptr = st->smInfo[Random(st->NumMsgs)].mInfo.mSeg;
	return msgptr;
}

namespace bibendovsky
{

int AssetsInfo::level() const noexcept
{
	if(levelInfo->LevelNumber > 0)
		return levelInfo->LevelNumber - 1;
	return 0;
}

//
// General Coord (tile) structure
//
struct tilecoord_t
{
	using pair_t = std::pair<int, int>;

	// !!! Used in saved game.
	std::uint8_t tilex;

	// !!! Used in saved game.
	std::uint8_t tiley;
}; // tilecoord_t

// -----------------------------------
//
// barrier coord/table structure
//
// -----------------------------------
struct barrier_type
{
	// !!! Used in saved game.
	tilecoord_t coord;

	// !!! Used in saved game.
	std::uint8_t on;
}; // barrier_type;

using Barriers = std::vector<barrier_type>;

struct gametype
{
	// !!! Used in saved game.
	Barriers barrier_table;
	std::map<tilecoord_t::pair_t, int> switch_codes;

	void initialize();

	void initialize_barriers();

	int get_barrier_group_offset(
		const int level) const;

	int get_barrier_index(
		const int code) const;

	int encode_barrier_index(
		const int level,
		const int index) const;

	void decode_barrier_index(
		const int code,
		int& level,
		int& index) const;
}; // gametype

gametype gamestate;

void gametype::initialize()
{
	const auto& assets_info = AssetsInfo{};

	barrier_table = {};

	const auto switches_per_level = assets_info.get_barrier_switches_per_level();
	const auto levels_per_episode = assets_info.get_levels_per_episode();
	const auto switches_per_episode = switches_per_level * levels_per_episode;

	barrier_table.resize(switches_per_episode);
}

void gametype::initialize_barriers()
{
	for (auto& barrier : barrier_table)
	{
		barrier.coord.tilex = 0xFF;
		barrier.coord.tiley = 0xFF;
		barrier.on = 0xFF;
	}
}

int gametype::get_barrier_group_offset(
	const int level) const
{
	const auto& assets_info = AssetsInfo{};

	if (level < 0 || level >= assets_info.get_levels_per_episode())
	{
		Quit("[BRR_GRP_IDX] Level index out of range.");
	}

	const auto switches_per_level = assets_info.get_barrier_switches_per_level();

	return level * switches_per_level;
}

int gametype::get_barrier_index(
	const int code) const
{
	auto level = 0;
	auto index = 0;
	decode_barrier_index(code, level, index);

	const auto& assets_info = AssetsInfo{};
	const auto max_switches = assets_info.get_barrier_switches_per_level();

	return (level * max_switches) + index;
}

int gametype::encode_barrier_index(
	const int level,
	const int index) const
{
	const auto& assets_info = AssetsInfo{};

	if (index < 0 || index >= assets_info.get_barrier_switches_per_level())
	{
		Quit("[BARR_ENC_IDX] Barrier index out of range.");
	}

	if (level < 0 || level >= assets_info.get_levels_per_episode())
	{
		Quit("[BARR_ENC_IDX] Level index out of range.");
	}

	const auto switch_index_bits = assets_info.get_max_barrier_switches_per_level_bits();
	const auto switch_index_shift = 1 << switch_index_bits;
	const auto switch_index_mask = switch_index_shift - 1;

	return (level * switch_index_shift) | (index & switch_index_mask);
}

void gametype::decode_barrier_index(
	const int code,
	int& level,
	int& index) const
{
	if (code < 0)
	{
		Quit("[BARR_DEC_IDX] Invalid code.");
	}

	const auto& assets_info = AssetsInfo{};

	const auto switch_index_bits = assets_info.get_max_barrier_switches_per_level_bits();
	const auto switch_index_shift = 1 << switch_index_bits;
	const auto switch_index_mask = switch_index_shift - 1;

	level = code / switch_index_shift;
	index = code & switch_index_mask;

	if (level < 0 || level >= assets_info.get_levels_per_episode())
	{
		Quit("[BARR_DEC_IDX] Level index out of range.");
	}

	if (index < 0 || index >= assets_info.get_barrier_switches_per_level())
	{
		Quit("[BARR_DEC_IDX] Barrier index out of range.");
	}
}

void newgame_initialize()
{
	gamestate.initialize();

	gamestate.initialize_barriers();
}

void level_initialize()
{
	gamestate.switch_codes.clear();
}

// --------------------------------------------------------------------------
// UpdateBarrierTable(x,y,level) - Finds/Inserts arc entry in arc list
//
// RETURNS: Offset into barrier_table[] for a particular arc
//
// --------------------------------------------------------------------------

std::uint16_t UpdateBarrierTable(
	const int level,
	const int x,
	const int y,
	const bool on_off)
{
	const auto& assets_info = AssetsInfo{};

	if (level >= assets_info.get_levels_per_episode())
	{
		Quit("[BARR_UPD_TBL] Level index out of range.");
	}

	if (x <= 0 || static_cast<unsigned int>(x) >= (map->GetHeader().width - 1) ||
		y <= 0 || static_cast<unsigned int>(y) >= (map->GetHeader().height - 1))
	{
		Quit("[BARR_UPD_TBL] Coordinates out of range.");
	}


	//
	// Scan Table...
	//
	const auto group_offset = gamestate.get_barrier_group_offset(level);
	const auto max_switches = assets_info.get_barrier_switches_per_level();

	for (int i = 0; i < max_switches; ++i)
	{
		auto& barrier = gamestate.barrier_table[group_offset + i];

		if (barrier.coord.tilex == x && barrier.coord.tiley == y)
		{
			return static_cast<std::uint16_t>(gamestate.encode_barrier_index(level, i));
		}
		else
		{
			if (barrier.on == 0xFF)
			{
				// Empty?
				// We have hit end of list - Add

				barrier.coord.tilex = static_cast<std::uint8_t>(x);
				barrier.coord.tiley = static_cast<std::uint8_t>(y);
				barrier.on = static_cast<std::uint8_t>(on_off);

				return static_cast<std::uint16_t>(gamestate.encode_barrier_index(level, i));
			}
		}
	}

	Quit("[BARR_UPD_TBL] Too many barrier switches.");
}

std::uint16_t UpdateBarrierTable(
	const int level,
	const int x,
	const int y)
{
	const auto& assets_info = AssetsInfo{};
	const auto new_level = (level == 0xFF ?
			assets_info.level() : level);

	return UpdateBarrierTable(new_level, x, y, true);
}

void UpdateSwitchCodes(
	const int x,
	const int y,
	int barrier_code)
{
	gamestate.switch_codes[std::make_pair(x,y)] = barrier_code;
}

// --------------------------------------------------------------------------
// ScanBarrierTable(x,y) - Scans a switch table for a arc in this level
//
// RETURNS :
//      0xFFFF - Not found in table
//      barrier - barrier_table of the barrier for [num]
//
// --------------------------------------------------------------------------
std::uint16_t ScanBarrierTable(
	std::uint8_t x,
	std::uint8_t y)
{
	const auto& assets_info = AssetsInfo{};
	const auto max_switches = assets_info.get_barrier_switches_per_level();

	const auto level = assets_info.level();
	const auto barrier_group_index = gamestate.get_barrier_group_offset(level);

	for (int i = 0; i < max_switches; ++i)
	{
		const auto& barrier = gamestate.barrier_table[barrier_group_index + i];

		if (barrier.coord.tilex == x && barrier.coord.tiley == y)
		{
			const auto code = gamestate.encode_barrier_index(level, i);

			// Found switch...
			return static_cast<std::uint16_t>(code);
		}
	}

	return static_cast<std::uint16_t>(0xFFFF); // Mark as EMPTY
}

uint8_t GetBarrierState(uint16_t temp2)
{
	const auto barrier_index = gamestate.get_barrier_index(temp2);
	return gamestate.barrier_table[barrier_index].on;
}

// --------------------------------------------------------------------------
// DisplaySwitchOperateMsg() - Displays the Operating Barrier Switch message
//      for a particular level across the InfoArea.
// --------------------------------------------------------------------------
void DisplaySwitchOperateMsg(
	int coords)
{
	auto level = 0;
	auto index = 0;
	gamestate.decode_barrier_index(coords, level, index);
	const auto group_offset = gamestate.get_barrier_group_offset(level);

	const auto barrier_index = group_offset + index;
	auto barrier = &gamestate.barrier_table[barrier_index];

	static std::string message;

	if (barrier->on != 0)
	{
		message = "\r\r  ACTIVATING BARRIER";
	}
	else
	{
		message = "\r\r DEACTIVATING BARRIER";
	}

	const auto& assets_info = AssetsInfo{};

	if (assets_info.is_secret_level(level))
	{
		const auto secret_level = assets_info.secret_floor_get_index(level) + 1;

		message += "\r  ON SECRET FLOOR " + std::to_string(secret_level);
	}
	else
	{
		message += "\r      ON FLOOR " + std::to_string(level);
	}

	StatusBar->InfoMessage(message.c_str());
}

void ActivateWallSwitch(int num)
{
	//
	// Update tile map & Display switch'd message
	//
	auto level = 0;
	auto index = 0;
	gamestate.decode_barrier_index(num, level, index);
	const auto group_offset = gamestate.get_barrier_group_offset(level);

	const auto barrier_index = group_offset + index;
	auto& barrier = gamestate.barrier_table[barrier_index];

	barrier.on ^= 1;

	DisplaySwitchOperateMsg(num);
	SD_PlaySound ("switches/normbutn");

	for(const auto &p : gamestate.switch_codes)
	{
		if(p.second == num)
		{
			int x, y;
			std::tie(x, y) = p.first;
			auto spot = map->GetSpot(x, y, 0);
			if(spot && spot->tile && spot->tile->switchDestTile != nullptr)
			{
				spot->SetTile(spot->tile->switchDestTile);
			}
		}
	}
}

} // namespace bibendovsky

// --------------------------------------------------------------------------
// SpawnWallSwitch()
// --------------------------------------------------------------------------
int GameMap::SpawnWallSwitch(std::uint16_t oldnum, std::uint16_t oldnum2, int x, int y)
{
	const auto level = oldnum & 0xFF;

	const auto switch_x = (oldnum2 >> 8) & 0xFF;
	const auto switch_y = oldnum2 & 0xFF;

	const auto barrier_code = bibendovsky::UpdateBarrierTable(level, switch_x, switch_y);
	bibendovsky::UpdateSwitchCodes(x, y, barrier_code);
	return barrier_code;
}

// --------------------------------------------------------------------------
// ActivateWallSwitch() - Updates the Map, Actors, and Tables for wall switchs
// --------------------------------------------------------------------------
void GameMap::ActivateWallSwitch(int barrier_code)
{
	bibendovsky::ActivateWallSwitch(barrier_code);
}

#endif
