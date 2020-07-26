/*
** a_barrier.cpp
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
