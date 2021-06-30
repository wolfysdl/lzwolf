/*
** a_barrier.cpp
**
**---------------------------------------------------------------------------
** BStone: A Source port of
** Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
** 
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

#include <functional>
#include "c_cvars.h"
#include "thingdef/thingdef.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"

#include <climits>

// Blake Stone Barrier ---------------
//
namespace bibendovsky
{
std::uint16_t ScanBarrierTable(
	std::uint8_t x,
	std::uint8_t y);

uint8_t GetBarrierState(uint16_t temp2);
} // namespace bibendovsky

class ABarrier : public AActor
{
	DECLARE_CLASS (ABarrier, AActor)

	using TCoord = std::pair<WORD,WORD>;

	void OnSpawn();

	void OnLink();

	void Transition();

	std::uint16_t temp2;
	bool curstate;
public:
};

IMPLEMENT_CLASS (Barrier)

class ABarrierLinker : public AActor
{
	DECLARE_CLASS (ABarrierLinker, AActor)

	void	Destroy() override;

	static std::map<ABarrier::TCoord, ABarrier*> all_instances;
};

void ABarrierLinker::Destroy()
{
	all_instances.clear();
}

std::map<ABarrier::TCoord, ABarrier*> ABarrierLinker::all_instances;

IMPLEMENT_CLASS (BarrierLinker)

void ABarrier::OnSpawn()
{
	temp2 = bibendovsky::ScanBarrierTable(
			static_cast<std::uint8_t>(tilex),
			static_cast<std::uint8_t>(tiley));
	curstate = true;
	ABarrierLinker::all_instances[std::make_pair(tilex,tiley)] = this;
}

void ABarrier::OnLink()
{
	// this barrier should be linked by another one
	if(temp2 == 0xffff)
		return;

	std::function<void(WORD,WORD)> visit;
	visit = [&visit, temp2=this->temp2](WORD tilex, WORD tiley) {
		auto it = ABarrierLinker::all_instances.find(std::make_pair(tilex,tiley));
		if(it != std::end(ABarrierLinker::all_instances))
		{
			auto barrier = it->second;
			if(barrier->temp2 == 0xffff)
			{
				barrier->temp2 = temp2;
				visit(tilex+1,tiley+0);
				visit(tilex-1,tiley+0);
				visit(tilex+0,tiley+1);
				visit(tilex+0,tiley-1);
			}
		}
	};
	this->temp2 = 0xffff;
	visit(tilex,tiley);
}

void ABarrier::Transition()
{
	//
	// Check for Turn offs
	//
	if (temp2 != 0xffff)
	{
		auto onOff = bibendovsky::GetBarrierState(temp2);
		if(onOff != curstate)
		{
			curstate = onOff;
			auto state = FindState(curstate ? NAME_Active : NAME_Inactive);
			SetState(state);
		}
	}
}

//------------------------------------------------------------------------------
// Smart animation: Blake Stone used these to implement simple animation
// sequences. This would be fine, but they used the random number generator a
// little too much. So the main purpose of this is to hold a random number
// throughout an animation sequence.
//
// Due to technical limitations, this inconsistently takes 70hz tics instead of
// Doom style half tics.

ACTION_FUNCTION(A_InitBarrierAnim)
{
	ACTION_PARAM_INT(delay, 0);
	self->temp1 = delay;
	return true;
}

ACTION_FUNCTION(A_BarrierAnimDelay)
{
	self->ticcount = self->temp1;
	return true;
}

ACTION_FUNCTION(A_BarrierLink)
{
	ABarrier *barrier = reinterpret_cast<ABarrier *>(self);
	barrier->OnLink();
	return true;
}

ACTION_FUNCTION(A_BarrierSpawn)
{
	ABarrier *barrier = reinterpret_cast<ABarrier *>(self);
	barrier->OnSpawn();
	return true;
}

ACTION_FUNCTION(A_BarrierTransition)
{
	ABarrier *barrier = reinterpret_cast<ABarrier *>(self);
	barrier->Transition();
	return true;
}
