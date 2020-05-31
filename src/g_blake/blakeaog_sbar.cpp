/*
** blakeaog_sbar.cpp
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

#include <deque>
#include "wl_def.h"
#include "wl_game.h"
#include "a_inventory.h"
#include "a_keys.h"
#include "colormatcher.h"
#include "id_ca.h"
#include "id_us.h"
#include "id_vh.h"
#include "id_sd.h"
#include "language.h"
#include "g_mapinfo.h"
#include "v_font.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_def.h"
#include "wl_play.h"
#include "xs_Float.h"
#include "thingdef/thingdef.h"

enum
{
	STATUSLINES = 48,
	STATUSTOPLINES = 16
};

CVAR (Bool, aog_heartbeatsnd, false, CVAR_ARCHIVE)

class BlakeAOGHealthMonitor
{
public:
	void Draw();

private:
	static inline FTextureID TexID(const char *name)
	{
		return TexMan.GetTexture(name, FTexture::TEX_Any);
	}

	int GetHp() const
	{
		return players[0].health;
	}

	const FTextureID ECG_GRID_PIECE = TexID("ECGGPIEC");
	const FTextureID ECG_HEART_GOOD = TexID("ECGHGOOD");
	const FTextureID ECG_HEART_BAD = TexID("ECGHBAD");

	int ecg_scroll_tics = 0;
	int ecg_next_scroll_tics = 0;
	std::deque<int> ecg_legend = std::deque<int>(6);
	std::deque<int> ecg_segments = std::deque<int>(6);

	FTextureID heart_picture_index = ECG_GRID_PIECE;
	int heart_sign_tics = 0;
	int heart_sign_next_tics = 0;

	static constexpr auto HEALTH_SCROLL_RATE = 7;
	static constexpr auto HEALTH_PULSE = 70;
};

// Draws electrocardiogram (ECG) and the heart sign
void BlakeAOGHealthMonitor::Draw()
{
	//
	// ECG
	//

	// ECG segment indices:
	// 0 - silence
	// 1..8 - shape #1 (health 66%-100%)
	// 9..17 - shape #2 (health 33%-65%)
	// 18..27 - shape #3 (health 0%-32%)

	// ECG segment legend:
	// 0 - silence
	// 1 - signal #1 (health 66%-100%)
	// 2 - signal #2 (health 33%-65%)
	// 3 - signal #3 (health 0%-32%)

	if (ecg_scroll_tics >= ecg_next_scroll_tics)
	{
		ecg_scroll_tics = 0;
		ecg_next_scroll_tics = HEALTH_SCROLL_RATE;

		bool carry = false;

		for (int i = 5; i >= 0; --i)
		{
			if (carry)
			{
				carry = false;
				ecg_legend[i] = ecg_legend[i + 1];
				ecg_segments[i] = ecg_segments[i + 1] - 4;
			}
			else if (ecg_segments[i] != 0)
			{
				ecg_segments[i] += 1;

				bool use_carry = false;

				if (ecg_legend[i] == 1 && ecg_segments[i] == 5)
				{
					use_carry = true;
				}
				else if (ecg_legend[i] == 2 && ecg_segments[i] == 13)
				{
					use_carry = true;
				}
				if (ecg_legend[i] == 3 &&
					(ecg_segments[i] == 22 || ecg_segments[i] == 27))
				{
					use_carry = true;
				}

				if (use_carry)
				{
					carry = true;
				}
				else
				{
					bool skip = false;

					if (ecg_legend[i] == 1 && ecg_segments[i] > 8)
					{
						skip = true;
					}
					else if (ecg_legend[i] == 2 && ecg_segments[i] > 17)
					{
						skip = true;
					}
					if (ecg_legend[i] == 3 && ecg_segments[i] > 27)
					{
						skip = true;
					}

					if (skip)
					{
						ecg_legend[i] = 0;
						ecg_segments[i] = 0;
					}
				}
			}
		}

		if (GetHp() > 0 && ecg_legend[5] == 0)
		{
			if (GetHp() < 33)
			{
				ecg_legend[5] = 3;
				ecg_segments[5] = 18;
			}
			else if (GetHp() >= 66)
			{
				if (ecg_legend[4] == 0 || ecg_legend[4] != 1)
				{
					ecg_legend[5] = 1;
					ecg_segments[5] = 1;
				}
			}
			else
			{
				ecg_legend[5] = 2;
				ecg_segments[5] = 9;
			}
		}
	}
	else
	{
		ecg_scroll_tics += tics;
	}

	auto draw_hudpic = []( FTextureID texid, double hx, double hy ) {
		auto tex = TexMan(texid);

		double x = hx;
		double y = 200 - STATUSLINES + hy;
		double w = tex->GetScaledWidthDouble();
		double h = tex->GetScaledHeightDouble();

		screen->VirtualToRealCoords(x, y, w, h, 320, 200, true, true);

		screen->DrawTexture(tex, x, y,
			DTA_DestWidthF, w,
			DTA_DestHeightF, h,
			TAG_DONE);
	};

	for (int i = 0; i < 6; ++i)
	{
		FString hbstr;
		hbstr.Format("ECGHBE%02d", ecg_segments[i]);
		auto texid = TexID(hbstr.GetChars());
		draw_hudpic(texid, 120 + (i * 8), 8);
	}


	//
	// Heart sign
	//

	bool reset_heart_tics = false;

	if (GetHp() <= 0)
	{
		reset_heart_tics = true;
		heart_picture_index = ECG_GRID_PIECE;
	}
	else if (GetHp() < 40)
	{
		reset_heart_tics = true;
		heart_picture_index = ECG_HEART_BAD;
	}
	else if (heart_sign_tics >= heart_sign_next_tics)
	{
		reset_heart_tics = true;

		if (heart_picture_index == ECG_GRID_PIECE)
		{
			heart_picture_index = ECG_HEART_GOOD;

			if (aog_heartbeatsnd)
			{
				SD_PlaySound("misc/heartbeat");
			}
		}
		else
		{
			heart_picture_index = ECG_GRID_PIECE;
		}
	}

	if (reset_heart_tics)
	{
		heart_sign_tics = 0;
		heart_sign_next_tics = HEALTH_PULSE / 2;
	}
	else
	{
		heart_sign_tics += 1;
	}

	draw_hudpic(heart_picture_index, 120, 32);
}

class BlakeAOGInfoArea
{
public:
};

class BlakeAOGStatusBar : public DBaseStatusBar
{
public:
	BlakeAOGStatusBar() : CurrentScore(0) {}

	void DrawStatusBar();
	unsigned int GetHeight(bool top)
	{
		if(viewsize == 21)
			return 0;
		return top ? STATUSTOPLINES : STATUSLINES;
	}

	void NewGame()
	{
		CurrentScore = players[0].score;
	}

	void Tick();

	void InfoMessage(FString key);

protected:
	void DrawLed(double percent, double x, double y) const;
	void DrawString(FFont *font, const char* string, double x, double y, bool shadow, EColorRange color=CR_UNTRANSLATED, bool center=false) const;
	void LatchNumber (int x, int y, unsigned width, int32_t number, bool zerofill, bool cap);
	void LatchString (int x, int y, unsigned width, const FString &str);

private:
	int CurrentScore;
	BlakeAOGHealthMonitor HealthMonitor;

	static constexpr auto SCORE_ROLL_WAIT = 60 * 10; // Tics
	static constexpr auto MAX_DISPLAY_SCORE = 9999999;
};

DBaseStatusBar *CreateStatusBar_BlakeAOG() { return new BlakeAOGStatusBar(); }

void BlakeAOGStatusBar::DrawLed(double percent, double x, double y) const
{
	static FTextureID LED[2][3] = {
		{TexMan.GetTexture("STLEDDR", FTexture::TEX_Any), TexMan.GetTexture("STLEDDY", FTexture::TEX_Any), TexMan.GetTexture("STLEDDG", FTexture::TEX_Any)},
		{TexMan.GetTexture("STLEDLR", FTexture::TEX_Any), TexMan.GetTexture("STLEDLY", FTexture::TEX_Any), TexMan.GetTexture("STLEDLG", FTexture::TEX_Any)}
	};

	unsigned int which = 0;
	if(percent > 0.71)
		which = 2;
	else if(percent > 0.38)
		which = 1;
	FTexture *dim = TexMan(LED[0][which]);
	FTexture *light = TexMan(LED[1][which]);

	double w = dim->GetScaledWidthDouble();
	double h = dim->GetScaledHeightDouble();

	screen->VirtualToRealCoords(x, y, w, h, 320, 200, true, true);

	int lightclip = xs_ToInt(y + h*(1-percent));
	screen->DrawTexture(dim, x, y,
		DTA_DestWidthF, w,
		DTA_DestHeightF, h,
		DTA_ClipBottom, lightclip,
		TAG_DONE);
	screen->DrawTexture(light, x, y,
		DTA_DestWidthF, w,
		DTA_DestHeightF, h,
		DTA_ClipTop, lightclip,
		TAG_DONE);
}

void BlakeAOGStatusBar::DrawStatusBar()
{
	if(viewsize == 21 && ingame)
		return;

	static FFont *IndexFont = V_GetFont("INDEXFON");
	//static FFont *HealthFont = V_GetFont("BlakeHealthFont");
	static FFont *ScoreFont = V_GetFont("BlakeScoreFont");

	static FTextureID STBar = TexMan.GetTexture("STBAR", FTexture::TEX_Any);
	static FTextureID STBarTop = TexMan.GetTexture("STTOP", FTexture::TEX_Any);

	double stx = 0;
	double sty = 200-STATUSLINES;
	double stw = 320;
	double sth = STATUSLINES;
	screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
	int boty = xs_ToInt(sty);

	screen->DrawTexture(TexMan(STBar), stx, sty,
		DTA_DestWidthF, stw,
		DTA_DestHeightF, sth,
		TAG_DONE);

	stx = 0;
	sty = 0;
	stw = 320;
	sth = STATUSTOPLINES;
	screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
	int topy = xs_ToInt(sth);

	screen->DrawTexture(TexMan(STBarTop), stx, 0.0,
		DTA_DestWidthF, stw,
		DTA_DestHeightF, sth,
		TAG_DONE);

	if(viewsize < 20)
	{
		// Draw outset border
		static byte colors[3] =
		{
			ColorMatcher.Pick(RPART(gameinfo.Border.topcolor), GPART(gameinfo.Border.topcolor), BPART(gameinfo.Border.topcolor)),
			ColorMatcher.Pick(RPART(gameinfo.Border.bottomcolor), GPART(gameinfo.Border.bottomcolor), BPART(gameinfo.Border.bottomcolor)),
			ColorMatcher.Pick(RPART(gameinfo.Border.highlightcolor), GPART(gameinfo.Border.highlightcolor), BPART(gameinfo.Border.highlightcolor))
		};

		VWB_Clear(colors[1], 0, topy, screenWidth-scaleFactorX, topy+scaleFactorY);
		VWB_Clear(colors[1], 0, topy+scaleFactorY, scaleFactorX, boty);
		VWB_Clear(colors[0], scaleFactorX, boty-scaleFactorY, screenWidth, boty);
		VWB_Clear(colors[0], screenWidth-scaleFactorX, topy, screenWidth, static_cast<int>(boty-scaleFactorY));
	}

	// Draw the top information
	FString lives, area;
	// TODO: Don't depend on LevelNumber for this switch
	if(levelInfo->LevelNumber > 20)
		area = "SECRET";
	else
		area.Format("AREA: %d", levelInfo->LevelNumber);
	lives.Format("LIVES: %d", players[0].lives);
	DrawString(IndexFont, area, 18, 5, true, CR_WHITE);
	DrawString(IndexFont, levelInfo->GetName(map), 160, 5, true, CR_WHITE, true);
	DrawString(IndexFont, lives, 267, 5, true, CR_WHITE);

	// Draw bottom information

	static FTextureID ECGGPIEC = TexMan.GetTexture("ECGGPIEC", FTexture::TEX_Any);
	for (int i = 0; i < 3; ++i)
	{
		FTexture *tex = TexMan(ECGGPIEC);

		double stx = 144 + (i * 8);
		double sty = 200-STATUSLINES+32;
		double stw = tex->GetScaledWidthDouble();
		double sth = tex->GetScaledHeightDouble();
		screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);

		screen->DrawTexture(tex, stx, sty,
			DTA_DestWidthF, stw,
			DTA_DestHeightF, sth,
			TAG_DONE);
	}

	FString health;
	health.Format("%3d%%", players[0].health);
	DrawString(IndexFont, health, 149, 200 - STATUSLINES + 34, false, CR_LIGHTBLUE);

	auto draw_score = [this]() {
		if (CurrentScore > MAX_DISPLAY_SCORE)
		{
			if (gamestate.score_roll_wait)
				LatchString(256, 3, 7, " -ROLL-");
			else
				LatchNumber(256, 3, 7, CurrentScore % (MAX_DISPLAY_SCORE + 1), false, false);
		}
		else
		{
			LatchNumber(256, 3, 7, CurrentScore, false, false);
		}
	};
	draw_score();

	if(players[0].ReadyWeapon)
	{
		FTexture *weapon = TexMan(players[0].ReadyWeapon->icon);
		if(weapon)
		{
			stx = 248;
			sty = 176;
			stw = weapon->GetScaledWidthDouble();
			sth = weapon->GetScaledHeightDouble();
			screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
			screen->DrawTexture(weapon, stx, sty,
				DTA_DestWidthF, stw,
				DTA_DestHeightF, sth,
				TAG_DONE);
		}

		// TODO: Fix color
		unsigned int amount = players[0].ReadyWeapon->ammo[AWeapon::PrimaryFire]->amount;
		DrawLed(static_cast<double>(amount)/static_cast<double>(players[0].ReadyWeapon->ammo[AWeapon::PrimaryFire]->maxamount), 243, 155);

		FString ammo;
		ammo.Format("%3d%%", amount);
		DrawString(IndexFont, ammo, 211, 200 - STATUSLINES + 38, false, CR_LIGHTBLUE);
	}

	//if(players[0].mo)
	//{
	//	static const ClassDef * const radarPackCls = ClassDef::FindClass("RadarPack");
	//	AInventory *radarPack = players[0].mo->FindInventory(radarPackCls);
	//	if(radarPack)
	//		DrawLed(static_cast<double>(radarPack->amount)/static_cast<double>(radarPack->maxamount), 235, 155);
	//	else
	//		DrawLed(0, 235, 155);
	//}

	// Find keys in inventory
	//int presentKeys = 0;
	//if(players[0].mo)
	//{
	//	for(AInventory *item = players[0].mo->inventory;item != NULL;item = item->inventory)
	//	{
	//		if(item->IsKindOf(NATIVE_CLASS(Key)))
	//		{
	//			int slot = static_cast<AKey *>(item)->KeyNumber;
	//			if(slot <= 3)
	//				presentKeys |= 1<<(slot-1);
	//			if(presentKeys == 0x7)
	//				break;
	//		}
	//	}
	//}

	//static FTextureID Keys[4] = {
	//	TexMan.GetTexture("STKEYS0", FTexture::TEX_Any),
	//	TexMan.GetTexture("STKEYS1", FTexture::TEX_Any),
	//	TexMan.GetTexture("STKEYS2", FTexture::TEX_Any),
	//	TexMan.GetTexture("STKEYS3", FTexture::TEX_Any)
	//};
	//for(unsigned int i = 0;i < 3;++i)
	//{
	//	FTexture *tex;
	//	if(presentKeys & (1<<i))
	//		tex = TexMan(Keys[i+1]);
	//	else
	//		tex = TexMan(Keys[0]);

	//	stx = 120+16*i;
	//	sty = 179;
	//	stw = tex->GetScaledWidthDouble();
	//	sth = tex->GetScaledHeightDouble();
	//	screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
	//	screen->DrawTexture(tex, stx, sty,
	//		DTA_DestWidthF, stw,
	//		DTA_DestHeightF, sth,
	//		TAG_DONE);
	//}

	HealthMonitor.Draw();
}

void BlakeAOGStatusBar::DrawString(FFont *font, const char* string, double x, double y, bool shadow, EColorRange color, bool center) const
{
	word strWidth, strHeight;
	VW_MeasurePropString(font, string, strWidth, strHeight);

	if(center)
		x -= strWidth/2.0;

	const double startX = x;
	FRemapTable *remap = font->GetColorTranslation(color);

	while(*string != '\0')
	{
		char ch = *string++;
		if(ch == '\n')
		{
			y += font->GetHeight();
			x = startX;
			continue;
		}

		int chWidth;
		FTexture *tex = font->GetChar(ch, &chWidth);
		if(tex)
		{
			double tx, ty, tw, th;

			if(shadow)
			{
				tx = x + 1, ty = y + 1, tw = tex->GetScaledWidthDouble(), th = tex->GetScaledHeightDouble();
				screen->VirtualToRealCoords(tx, ty, tw, th, 320, 200, true, true);
				screen->DrawTexture(tex, tx, ty,
					DTA_DestWidthF, tw,
					DTA_DestHeightF, th,
					DTA_FillColor, GPalette.BlackIndex,
					TAG_DONE);
			}

			tx = x, ty = y, tw = tex->GetScaledWidthDouble(), th = tex->GetScaledHeightDouble();
			screen->VirtualToRealCoords(tx, ty, tw, th, 320, 200, true, true);
			screen->DrawTexture(tex, tx, ty,
				DTA_DestWidthF, tw,
				DTA_DestHeightF, th,
				DTA_Translation, remap,
				TAG_DONE);
		}
		x += chWidth;
	}
}

/*
===============
=
= LatchNumber
=
= right justifies and pads with blanks
=
===============
*/

void BlakeAOGStatusBar::LatchNumber (int x, int y, unsigned width, int32_t number, bool zerofill, bool cap)
{
	FString str;
	if(zerofill)
		str.Format("%0*d", width, number);
	else
		str.Format("%*d", width, number);
	if(str.Len() > width && cap)
	{
		int maxval = width <= 9 ? (int) ceil(pow(10., (int)width))-1 : INT_MAX;
		str.Format("%d", maxval);
	}

	LatchString(x, y, width, str);
}

void BlakeAOGStatusBar::LatchString (int x, int y, unsigned width, const FString &str)
{
	static FFont *HudFont = NULL;
	if(!HudFont)
	{
		HudFont = V_GetFont("HudFont");
	}

	y = 200-(STATUSLINES-y);// + HudFont->GetHeight();

	int cwidth;
	FRemapTable *remap = HudFont->GetColorTranslation(CR_UNTRANSLATED);
	for(unsigned int i = MAX<int>(0, (int)(str.Len()-width));i < str.Len();++i)
	{
		VWB_DrawGraphic(HudFont->GetChar(str[i], &cwidth), x, y, MENU_NONE, remap);
		x += cwidth;
	}
}

void BlakeAOGStatusBar::Tick()
{
	int scoreDelta = players[0].score - CurrentScore;
	if(scoreDelta > 1500)
		CurrentScore += scoreDelta/4;
	else
		CurrentScore += clamp<int>(scoreDelta, 0, 8);

	if (gamestate.score_roll_wait)
		gamestate.score_roll_wait--;
}

void BlakeAOGStatusBar::InfoMessage(FString key)
{
	auto msg = (key[0] == '$') ? language[key.Mid(1)] : key.GetChars();
	printf("%s\n", msg);
}
