/*
** a_playerpawn.cpp
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

#include "a_inventory.h"
#include "a_playerpawn.h"
#include "c_cvars.h"
#include "thingdef/thingdef.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"
#include "c_console.h"
#include "wl_menu.h"

#include <climits>

extern	fixed finesine[FINEANGLES+FINEANGLES/4];

IMPLEMENT_CLASS(PlayerPawn)

PointerIndexTable<AActor::DropList> APlayerPawn::startInventory;

AWeapon *APlayerPawn::BestWeapon(const ClassDef *ammo)
{
	AWeapon *best = NULL;
	int order = INT_MAX;

	for(AInventory *item = inventory;item != NULL;item = item->inventory)
	{
		if(!item->IsKindOf(NATIVE_CLASS(Weapon)))
			continue;

		const int thisOrder = item->GetClass()->Meta.GetMetaInt(AWMETA_SelectionOrder);
		if(thisOrder > order)
			continue;

		AWeapon *weapon = static_cast<AWeapon *>(item);
		if(ammo && (weapon->ammo[0] == NULL || weapon->ammo[0]->GetClass() != ammo))
			continue;
		if(!weapon->CheckAmmo(AWeapon::PrimaryFire, false))
			continue;

		order = thisOrder;
		best = weapon;
	}

	return best;
}

void APlayerPawn::CheckWeaponSwitch(const ClassDef *ammo)
{
	if(player->PendingWeapon != WP_NOCHANGE)
		return;

	AWeapon *weapon = BestWeapon(ammo);
	if(!weapon)
		return;

	const int selectionOrder = weapon->GetClass()->Meta.GetMetaInt(AWMETA_SelectionOrder);
	const int currentOrder = player->ReadyWeapon ? player->ReadyWeapon->GetClass()->Meta.GetMetaInt(AWMETA_SelectionOrder) : 0;
	if(selectionOrder < currentOrder)
		player->PendingWeapon = weapon;
}

void APlayerPawn::Die()
{
	player->state = player_t::PST_DEAD;

	if(player)
	{
		player->extralight = 0;
		player->PendingWeapon = WP_NOCHANGE;
		if(player->ReadyWeapon)
			player->SetPSprite(player->ReadyWeapon->GetDownState(), player_t::ps_weapon);
		std::fill(std::begin(player->weaponSlotStates),
				std::end(player->weaponSlotStates), player_t::WeaponSlotState{});
	}
	Super::Die();
}

AActor::DropList *APlayerPawn::GetStartInventory()
{
	int index = GetClass()->Meta.GetMetaInt(APMETA_StartInventory);
	if(index >= 0)
		return startInventory[index];
	return NULL;
}

void APlayerPawn::GiveStartingInventory()
{
	if(!GetStartInventory())
		return;

	DropList::Iterator item = GetStartInventory()->Head();
	do
	{
		DropItem &inv = item;
		const ClassDef *cls = ClassDef::FindClass(inv.className);
		if(!cls || !cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
			continue;

		AInventory *invItem = (AInventory *)AActor::Spawn(cls, 0, 0, 0, 0);
		invItem->RemoveFromWorld();
		invItem->amount = inv.amount;
		if(cls->IsDescendantOf(NATIVE_CLASS(Weapon)))
		{
			player->PendingWeapon = (AWeapon *)invItem;

			// Empty weapon.
			((AWeapon *)invItem)->ammogive[0] = ((AWeapon *)invItem)->ammogive[1] = 0;
		}
		if(!invItem->CallTryPickup(this))
			invItem->Destroy();
	}
	while(item.Next());

	SetupWeaponSlots();

#if 0
	AInventory *inv = inventory;
	while(inv)
	{
		Printf("%s %d/%d\n", inv->GetClass()->GetName().GetChars(), inv->amount, inv->maxamount);
		inv = inv->inventory;
	}
#endif
}

AWeapon *APlayerPawn::PickNewWeapon()
{
	AWeapon *best = BestWeapon();

	if(best)
	{
		player->PendingWeapon = best;
		if(player->ReadyWeapon)
			player->SetPSprite(player->ReadyWeapon->GetDownState(), player_t::ps_weapon);
	}

	return best;
}

void APlayerPawn::RemoveInventory(AInventory *item)
{
	bool pickWeap = false;
	if(item == player->PendingWeapon)
		player->PendingWeapon = WP_NOCHANGE;
	else if(item == player->ReadyWeapon)
	{
		if(player->PendingWeapon == WP_NOCHANGE)
			pickWeap = true;
	}

#ifdef USE_LASTWEAPON
	std::size_t i;
	for (i = 0; i < player->weaponSlotStates.size(); i++)
	{
		player_t::WeaponSlotState &slot = player->weaponSlotStates[i];
		if (slot.LastWeapon == item)
			slot.LastWeapon = NULL;
	}
#endif

	Super::RemoveInventory(item);

	if(pickWeap)
		PickNewWeapon();
}

void APlayerPawn::Serialize(FArchive &arc)
{
	arc << maxhealth;

	Super::Serialize(arc);
}

void APlayerPawn::SetupWeaponSlots()
{
	player->weapons.StandardSetup(GetClass());
}

void APlayerPawn::Tick()
{
	Super::Tick();
	if(!player)
		return;

	TickPSprites();
	TickHeightAnim();

	if(player - players == ConsolePlayer)
	{
		// [RH] Smooth transitions between bobbing and not-bobbing frames.
		// This also fixes the bug where you can "stick" a weapon off-center by
		// shooting it when it's at the peak of its swing.
		static fixed curbob = 0;

		if(movebob)
		{
			static const fixed MAXBOB = 0x100000;
			fixed bobtarget = gamestate.victoryflag ? 0 : FixedMul(player->thrustspeed << 8, movebob);
			if(bobtarget > MAXBOB)
				bobtarget = MAXBOB;

			if (curbob != bobtarget)
			{
				if (abs (bobtarget - curbob) <= 1*FRACUNIT)
				{
					curbob = bobtarget;
				}
				else
				{
					fixed_t zoom = MAX<fixed_t> (1*FRACUNIT, abs (curbob - bobtarget) / 40);
					if (curbob > bobtarget)
					{
						curbob -= zoom;
					}
					else
					{
						curbob += zoom;
					}
				}
			}
		}
		else
			curbob = 0;

		player->bob = curbob;
	}

	player->AdjustFOV();

	// Watching BJ
	if(gamestate.victoryflag)
		return;

	if((player - players) == ConsolePlayer)
		StatusBar->UpdateFace();
	CheckWeaponChange(this);

	TicCmd_t &cmd = control[player - players];

	if(cmd.buttonstate[bt_use])
		Cmd_Use();

	if(cmd.buttonstate[bt_toggleconsole] && !cmd.buttonheld[bt_toggleconsole])
		C_ToggleConsole();

	if((player->flags & (player_t::PF_WEAPONREADY|player_t::PF_WEAPONREADYALT)))
	{
		// Determine primary or alternate attack
		Button fireButton = bt_nobutton;
		if(cmd.buttonstate[bt_attack] && (player->flags & player_t::PF_WEAPONREADY))
		{
			fireButton = bt_attack;
			player->ReadyWeapon->mode = AWeapon::PrimaryFire;
		}
		else if(cmd.buttonstate[bt_altattack] && (player->flags & player_t::PF_WEAPONREADYALT))
		{
			fireButton = bt_altattack;
			player->ReadyWeapon->mode = AWeapon::AltFire;
		}

		// Try to fire
		if(fireButton != bt_nobutton && player->ReadyWeapon->CheckAmmo(player->ReadyWeapon->mode, true))
		{
			if(!cmd.buttonheld[fireButton])
				player->attackheld = false;
			if(!(player->ReadyWeapon->weaponFlags & WF_NOAUTOFIRE) || !player->attackheld)
			{
				player->attackheld = true;
				if(MissileState)
					SetState(MissileState);
				player->SetPSprite(player->ReadyWeapon->GetAtkState(player->ReadyWeapon->mode, false), player_t::ps_weapon);
			}
		}
		else if(player->PendingWeapon != WP_NOCHANGE && (player->flags & player_t::PF_WEAPONSWITCHOK))
		{
			player->SetPSprite(player->ReadyWeapon->GetDownState(), player_t::ps_weapon);
		}
	}
	else if(player->attackheld)
		player->attackheld = cmd.buttonstate[bt_attack]|cmd.buttonstate[bt_altattack];

	// Reload
	if((player->flags & player_t::PF_WEAPONRELOADOK) && cmd.buttonstate[bt_reload])
	{
		const Frame *reload = player->ReadyWeapon->GetReloadState();
		if(reload)
			player->SetPSprite(reload, player_t::ps_weapon);
	}
	// Zoom
	if((player->flags & player_t::PF_WEAPONZOOMOK) && cmd.buttonstate[bt_zoom])
	{
		const Frame *zoom = player->ReadyWeapon->GetZoomState();
		if(zoom)
			player->SetPSprite(zoom, player_t::ps_weapon);
	}

	if(sighttime) // Player is frozen
		--sighttime;
	else
		ControlMovement(this);
}

void APlayerPawn::TickPSprites()
{
	for(unsigned int layer = 0;layer < player_t::NUM_PSPRITES;++layer)
	{
		if(!player->psprite[layer].frame)
			return;

		if(player->psprite[layer].ticcount > 0)
			--player->psprite[layer].ticcount;

		if(player->psprite[layer].frame && player->psprite[layer].ticcount == 0)
			player->SetPSprite(player->psprite[layer].frame->next, static_cast<player_t::PSprite>(layer));

		if(player->psprite[layer].frame)
			player->psprite[layer].frame->thinker(this, player->ReadyWeapon, player->psprite[layer].frame);
	}
}

void APlayerPawn::TickHeightAnim()
{
	player_t::HeightAnim &ha = player->heightanim;

	if (ha.ticcount > 0)
	{
		ha.ticcount--;

		int ticspassed = ha.period - ha.ticcount;
		const fixed t = FRACUNIT - ((finesine[ANG90 + ((ticspassed * ANG180) / ha.period)] + FRACUNIT) / 2);

		viewheight = FixedMul(ha.startpos, FRACUNIT - t) + FixedMul(ha.endpos, t);
	}
}

void APlayerPawn::ObituaryMessage(AActor *attacker)
{
	ADamage *killerDamage = static_cast<ADamage *>(FindInventory(killerdamagetype));
	const bool killed_by_melee = (killerDamage && killerDamage->melee);

	const char *message;
	if(killed_by_melee)
	{
		message = attacker->GetClass()->Meta.GetMetaString (AMETA_HitObituary);
		if (message == NULL)
		{
			message = attacker->GetClass()->Meta.GetMetaString (AMETA_Obituary);
		}
	}
	else
	{
		message = attacker->GetClass()->Meta.GetMetaString (AMETA_Obituary);
	}

	if(message != NULL && strlen(message) > 0)
	{
		Printf (PRINT_MEDIUM, "%s\n", message);
		GameMessage (message);
	}
}
