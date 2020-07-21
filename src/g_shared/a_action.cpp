/*
** a_action.cpp
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

#include "c_cvars.h"
#include "thingdef/thingdef.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"

#include <climits>

// SwitchableDecoration: Activate and Deactivate change state ---------------

class ASwitchableDecoration : public AActor
{
	DECLARE_CLASS (ASwitchableDecoration, AActor)
public:
	void Activate (AActor *activator);
	void Deactivate (AActor *activator);
};

IMPLEMENT_CLASS (SwitchableDecoration)

void ASwitchableDecoration::Activate (AActor *activator)
{
	SetState (FindState(NAME_Active));
}

void ASwitchableDecoration::Deactivate (AActor *activator)
{
	SetState (FindState(NAME_Inactive));
}

// SwitchingDecoration: Only Activate changes state -------------------------

class ASwitchingDecoration : public ASwitchableDecoration
{
	DECLARE_CLASS (ASwitchingDecoration, ASwitchableDecoration)
public:
	void Deactivate (AActor *activator) {}
};

IMPLEMENT_CLASS (SwitchingDecoration)

// Blake Stone Barrier ---------------

class ABarrier : public AActor
{
	DECLARE_CLASS (ABarrier, AActor)

	std::uint16_t temp2;
public:
};

IMPLEMENT_CLASS (Barrier)

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

namespace bibendovsky
{
std::uint16_t ScanBarrierTable(
	std::uint8_t x,
	std::uint8_t y);

bool GetBarrierState(uint16_t temp2);
} // namespace bibendovsky

ACTION_FUNCTION(A_BarrierSpawn)
{
	ABarrier *barrier = reinterpret_cast<ABarrier *>(self);
	barrier->temp2 = bibendovsky::ScanBarrierTable(
			static_cast<std::uint8_t>(self->tilex),
			static_cast<std::uint8_t>(self->tiley));
	return true;
}

ACTION_FUNCTION(A_BarrierTransition)
{
	ABarrier *barrier = reinterpret_cast<ABarrier *>(self);
	//
	// Check for Turn offs
	//
	if (barrier->temp2 != 0xffff)
	{
		auto onOff = bibendovsky::GetBarrierState(barrier->temp2);
		if (!onOff)
		{
			printf("foo %p\n", barrier);
			//ToggleBarrier(obj);
		}
	}
	return true;
}
