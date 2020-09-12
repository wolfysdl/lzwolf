/*
** a_ambient.cpp
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

#include "a_ambient.h"
#include "id_sd.h"
#include "templates.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "m_random.h"
#include "wl_def.h"
#include "wl_agent.h"
#include "id_ca.h"

IMPLEMENT_POINTY_CLASS(Ambient)
END_POINTERS

//===========================================================================
//
// AAmbient :: JumpState
//
// Jump to play state if player zone index matches the zoneindex property.
//
//===========================================================================

void AAmbient::JumpState(const Frame *frame, bool enter)
{
	auto get_zoneind = [](AActor *self) {
		auto spot = map->GetSpot(self->tilex, self->tiley, 0);
		const auto ind = (spot && spot->zone ? spot->zone->index : 0);
		return ind;
	};

	auto player = players[0].mo;
	if ((get_zoneind(player) == get_zoneind(this)) == enter)
	{
		SetState(frame);
	}
}
