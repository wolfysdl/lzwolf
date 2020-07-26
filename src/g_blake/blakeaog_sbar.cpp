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

#include <algorithm>
#include <deque>
#include <vector>
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
#include "r_data/colormaps.h"
#include "wl_agent.h"
#include "wl_def.h"
#include "wl_play.h"
#include "xs_Float.h"
#include "thingdef/thingdef.h"

#define TP_CNVT_CODE(c1, c2) ((c1) | (c2 << 8))

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
	using TTexIDs = std::vector<FTextureID>;
	struct TActiveMessage
	{
		FString str;
		TTexIDs texids;
	};

	void Push(const std::pair<std::string, TTexIDs> &msg);

	void Clear();

	void Tick();

	TActiveMessage GetActiveMessage() const;

	std::int16_t x, y;
	std::int16_t left_margin;
	enum EColorRange fontcolor;
	enum EColorRange text_color;
	enum EColorRange backgr_color;
	FTextureID texid;

private:
	static constexpr auto DISPLAY_MSG_TIME = 5*60;

	struct TPendingMsg
	{
		std::string str;
		int ticsleft;
		TTexIDs texids;
	};
	std::deque<TPendingMsg> pending_msgs;
};

void BlakeAOGInfoArea::Push(const std::pair<std::string, TTexIDs> &msg)
{
	pending_msgs.clear();
	pending_msgs.push_front(TPendingMsg{msg.first, DISPLAY_MSG_TIME, msg.second});
}

void BlakeAOGInfoArea::Clear()
{
	pending_msgs.clear();
}

void BlakeAOGInfoArea::Tick()
{
	// tick the currently displayed message
	if(pending_msgs.size() > 0)
	{
		auto &am = pending_msgs.front();
		am.ticsleft--;
		if(am.ticsleft < 0)
			pending_msgs.pop_front();
	}
}

auto BlakeAOGInfoArea::GetActiveMessage() const -> TActiveMessage
{
	if(pending_msgs.size() > 0)
	{
		auto &pm = pending_msgs.front();
		return TActiveMessage{pm.str.c_str(), pm.texids};
	}

	FString tokens_str;
	tokens_str.Format("%s", language["BLAKE_FOOD_TOKENS"]);
	return TActiveMessage{tokens_str, TTexIDs{}};
}

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

	void InfoMessage(FString key, const std::vector<FTextureID> &texids) override;

	void ClearInfoMessages();

protected:
	void DrawLed(double percent, double x, double y) const;
	void DrawString(FFont *font, const char* string, double x, double y, bool shadow, EColorRange color=CR_UNTRANSLATED, bool center=false, double *pEndX = nullptr) const;
	void LatchNumber (int x, int y, unsigned width, int32_t number, bool zerofill, bool cap);
	void LatchString (int x, int y, unsigned width, const FString &str);

	void DrawInfoArea();
	char* HandleControlCodes(char* first_ch);
	std::uint16_t TP_VALUE(const char* ptr, std::int8_t num_nybbles);
	void ScaleSprite2D(FTexture *tex, int xcenter, int topoffset, unsigned height);

	enum EColorRange BlakeToColorRange(int16_t blakecolor) const;

private:
	int CurrentScore;
	BlakeAOGHealthMonitor HealthMonitor;
	BlakeAOGInfoArea InfoArea;
	double LastDrawX;

	static constexpr auto SCORE_ROLL_WAIT = 60 * 10; // Tics
	static constexpr auto MAX_DISPLAY_SCORE = 9999999;

	static constexpr auto INFOAREA_X = 3;
	static constexpr auto INFOAREA_Y = 200 - STATUSLINES + 3;
	static constexpr auto INFOAREA_W = 109;
	static constexpr auto INFOAREA_H = 37;

	static constexpr auto INFOAREA_BCOLOR = 0x01;
	static constexpr auto INFOAREA_CCOLOR = 0x1A;
	static constexpr auto INFOAREA_TCOLOR = 0xA6;

	static constexpr auto TP_RETURN_CHAR = '\r';
	static constexpr auto TP_CONTROL_CHAR = '^';
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
			stx = 176;
			sty = 152;
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
	int presentKeys = 0;
	if(players[0].mo)
	{
		for(AInventory *item = players[0].mo->inventory;item != NULL;item = item->inventory)
		{
			if(item->IsKindOf(NATIVE_CLASS(Key)))
			{
				int slot = static_cast<AKey *>(item)->KeyNumber;
				if(slot <= 5)
					presentKeys |= 1<<(slot-1);
				if(presentKeys == 0x1f)
					break;
			}
		}
	}

	auto draw_keys = [](int keys) {
		static constexpr auto NUMKEYS = 5;
		static const int indices[NUMKEYS] = {
			0, 1, 3, 2, 4,
		}; // indices

		static const std::uint8_t off_colors[NUMKEYS] = {
			0x11, 0x31, 0x91, 0x51, 0x21,
		}; // off_colors

		static const std::uint8_t on_colors[NUMKEYS] = {
			0xC9, 0xB9, 0x9C, 0x5B, 0x2B,
		}; // on_colors

		for (auto i = 0; i < 5; ++i)
		{
			int index = indices[i];
			std::uint8_t color = 0;

			if (keys & (1<<i))
			{
				color = on_colors[index];
			}
			else
			{
				color = off_colors[index];
			}

			double stx = 257 + (i * 8);
			double sty = 200 - STATUSLINES + 25;
			double stw = 7;
			double sth = 7;
			screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);

			VWB_Clear(color, stx, sty, stx+stw, sty+sth);
		}
	};
	draw_keys(presentKeys);

	HealthMonitor.Draw();

	InfoArea.left_margin = INFOAREA_X;
	InfoArea.text_color = BlakeToColorRange(INFOAREA_TCOLOR);
	InfoArea.backgr_color = BlakeToColorRange(INFOAREA_BCOLOR);
	InfoArea.x = InfoArea.left_margin;
	InfoArea.y = INFOAREA_Y;

	DrawInfoArea();
}

void BlakeAOGStatusBar::DrawString(FFont *font, const char* string, double x, double y, bool shadow, EColorRange color, bool center, double *pEndX) const
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

	if(pEndX != nullptr)
	{
		*pEndX = x;
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

enum EColorRange BlakeAOGStatusBar::BlakeToColorRange(int16_t blakecolor) const
{
	static std::map<int16_t, enum EColorRange> m;
	if(m.empty())
	{
		m[0x01] = CR_DARKGRAY;
		m[0xA6] = CR_GRAY;
	}
	auto it = m.find(blakecolor);
	return (it != std::end(m) ? it->second : CR_WHITE);
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
	
	InfoArea.Tick();
}

void BlakeAOGStatusBar::InfoMessage(FString key, const std::vector<FTextureID>& texids)
{
	auto msg = (key[0] == '$') ? language[key.Mid(1)] : key.GetChars();
	InfoArea.Push({msg, texids});
}

void BlakeAOGStatusBar::ClearInfoMessages()
{
	InfoArea.Clear();
}

// --------------------------------------------------------------------------
// DrawInfoArea()
//
//
// Active control codes:
//
//  ^ANnn                       - define animation
//  ^FCnn                       - set font color
//  ^LMnnn                      - set left margin (if 'nnn' == "fff" uses current x)
//  ^EP                         - end of page (waits for 'M' to read MORE)
//  ^PXnnn                      - move x to coordinate 'n'
//  ^PYnnn                      - move y to coordinate 'n'
//  ^SHnnn                      - display shape 'n' at current x,y
//  ^BGn                                - set background color
//  ^DM                         - Default Margins
//
// Other info:
//
// All 'n' values are hex numbers (0 - f), case insensitive.
// The number of N's listed is the number of digits REQUIRED by that control
// code. (IE: ^LMnnn MUST have 3 values! --> 003, 1a2, 01f, etc...)
//
// If a line consists only of control codes, the cursor is NOT advanced
// to the next line (the ending <CR><LF> is skipped). If just ONE non-control
// code is added, the number "8" for example, then the "8" is displayed
// and the cursor is advanced to the next line.
//
// The text presenter now handles sprites, but they are NOT masked! Also,
// sprite animations will be difficult to implement unless all frames are
// of the same dimensions.
//
// --------------------------------------------------------------------------

void BlakeAOGStatusBar::DrawInfoArea()
{
	int px = 0;
	const std::int16_t IA_FONT_HEIGHT = 6;

	char* first_ch;
	char* scan_ch, temp;

	auto info_msg = InfoArea.GetActiveMessage();
	auto msg = info_msg.str.GetChars();
	const auto &texids = info_msg.texids;

	unsigned int millis = gamestate.TimeCount*1000/70;
	const auto texid = (texids.size() < 1 ? FTextureID() :
			texids[(millis / 300) % texids.size()]);

	if (!*msg)
	{
		return;
	}

	std::vector<char> buffer(
		msg,
		msg + std::string::traits_type::length(msg) + 1);

	first_ch = &buffer[0];

	static FFont *IndexFont = V_GetFont("INDEXFON");
	InfoArea.fontcolor = InfoArea.text_color;
	InfoArea.texid = texid;

	while (first_ch && *first_ch)
	{

		if (*first_ch != TP_CONTROL_CHAR)
		{
			scan_ch = first_ch;

			while ((*scan_ch) && (*scan_ch != '\n') && (*scan_ch != TP_RETURN_CHAR) && (*scan_ch != TP_CONTROL_CHAR))
			{
				scan_ch++;
			}

			// print current line
			//

			temp = *scan_ch;
			*scan_ch = 0;

			if (*first_ch != TP_RETURN_CHAR)
			{
				px = InfoArea.x;

				double endX = px;
				DrawString(IndexFont, first_ch, InfoArea.x, InfoArea.y,
					true, InfoArea.fontcolor, false, &endX);
				px = static_cast<int>(endX);
			}

			*scan_ch = temp;
			first_ch = scan_ch;

			// skip SPACES / RETURNS at end of line
			//

			if ((*first_ch == ' ') || (*first_ch == TP_RETURN_CHAR))
			{
				first_ch++;
			}

			// TP_CONTROL_CHARs don't advance to next character line
			//

			if (*scan_ch != TP_CONTROL_CHAR)
			{
				InfoArea.x = InfoArea.left_margin;
				InfoArea.y += IA_FONT_HEIGHT;
			}
			else
			{
				InfoArea.x = px;
			}
		}
		else
		{
			first_ch = HandleControlCodes(first_ch);
		}
	}
}

char* BlakeAOGStatusBar::HandleControlCodes(char* first_ch)
{
	//piShapeInfo* shape;
	//piAnimInfo* anim;
	//std::uint16_t shapenum;
	std::uint16_t left_margin;

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

	first_ch++;

#ifndef TP_CASE_SENSITIVE
	*first_ch = toupper(*first_ch);
	*(first_ch + 1) = toupper(*(first_ch + 1));
#endif

	std::uint16_t code = *reinterpret_cast<const std::uint16_t*>(first_ch);
	first_ch += 2;

	auto draw_shape = [this](FTextureID texid) {
		auto tex = TexMan(texid);

		double stx = InfoArea.x + 0.5;
		double sty = InfoArea.y + 0.5;
		double stw = 37 - 0.5;
		double sth = 37 - 0.5;
		screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);

		VWB_Clear(0, stx, sty, stx+stw, sty+sth);
		ScaleSprite2D(tex, static_cast<int>(stx + (stw/2)), FLOAT2FIXED(sty+sth/2)>>13, FLOAT2FIXED(sth)>>14);

		InfoArea.x += 37;
	};

	switch (code)
	{

		// INIT ANIMATION ---------------------------------------------------
		//
	case TP_CNVT_CODE('A', 'N'):
		draw_shape(InfoArea.texid);
		first_ch += 2; // do not use the 2-digit animnum

		InfoArea.left_margin = InfoArea.x;
		break;

		// DRAW SHAPE -------------------------------------------------------
		//
	case TP_CNVT_CODE('S', 'H'):

		// NOTE : This needs to handle the left margin....

		draw_shape(InfoArea.texid);
		first_ch += 3; // do not use the 3-digit shapenum

		InfoArea.left_margin = InfoArea.x;
		break;

		// FONT COLOR -------------------------------------------------------
		//
	case TP_CNVT_CODE('F', 'C'):
		InfoArea.text_color = BlakeToColorRange(TP_VALUE(first_ch, 2));
		InfoArea.fontcolor = InfoArea.text_color;
		first_ch += 2;
		break;

		// BACKGROUND COLOR -------------------------------------------------
		//
	case TP_CNVT_CODE('B', 'G'):
		InfoArea.backgr_color = BlakeToColorRange(TP_VALUE(first_ch, 2));
		first_ch += 2;
		break;

		// DEFAULT MARGINS -------------------------------------------------
		//
	case TP_CNVT_CODE('D', 'M'):
		InfoArea.left_margin = INFOAREA_X;
		break;

		// LEFT MARGIN ------------------------------------------------------
		//
	case TP_CNVT_CODE('L', 'M'):
		left_margin = TP_VALUE(first_ch, 3);
		first_ch += 3;
		if (left_margin == 0xfff)
		{
			InfoArea.left_margin = InfoArea.x;
		}
		else
		{
			InfoArea.left_margin = left_margin;
		}
		break;

	case TP_CNVT_CODE('F', 'T'):
		{
			FString tokens_str;
			tokens_str.Format("%2d", tokens());
			first_ch[0] = tokens_str[0];
			first_ch[1] = tokens_str[1];
		}
		break;
	}

	return first_ch;

}

std::uint16_t BlakeAOGStatusBar::TP_VALUE(const char* ptr, std::int8_t num_nybbles)
{
	char ch;
	std::int8_t nybble;
	std::int8_t shift;
	std::uint16_t value = 0;

	for (nybble = 0; nybble < num_nybbles; nybble++)
	{
		shift = 4 * (num_nybbles - nybble - 1);

		ch = *ptr++;
		if (isxdigit(ch))
		{
			if (isalpha(ch))
			{
				value |= (toupper(ch) - 'A' + 10) << shift;
			}
			else
			{
				value |= (ch - '0') << shift;
			}
		}
	}

	return value;
}

void BlakeAOGStatusBar::ScaleSprite2D(FTexture *tex, int xcenter, int topoffset, unsigned height)
{
	auto vbuf = screen->GetBuffer();
	auto vbufPitch = screen->GetPitch();

	// height is a 13.3 fixed point number indicating the number of screen
	const double dyScale = (height/256.0);
	const int upperedge = topoffset + height - static_cast<int>((tex->GetScaledTopOffsetDouble())*dyScale*8);

	const double dxScale = (height/256.0)*FIXED2FLOAT(FixedDiv(FRACUNIT, yaspect));
	const int actx = static_cast<int>(xcenter - tex->GetScaledLeftOffsetDouble()*dxScale);

	const unsigned int startX = -MIN(actx, 0);
	const unsigned int startY = -MIN(upperedge>>3, 0);
	const fixed xStep = static_cast<fixed>(tex->xScale/dxScale);
	const fixed yStep = static_cast<fixed>(tex->yScale/dyScale);
	const fixed xRun = tex->GetWidth()<<FRACBITS;
	const fixed yRun = tex->GetHeight()<<FRACBITS;

	const BYTE *src;
	byte *destBase = vbuf + actx + startX + vbufPitch*(upperedge>>3);
	byte *dest = destBase;
	unsigned int i;
	fixed x, y;
	for(i = actx+startX, x = startX*xStep;x < xRun;x += xStep, ++i, dest = ++destBase)
	{
		src = tex->GetColumn((x>>FRACBITS), NULL);

		for(y = startY*yStep;y < yRun;y += yStep)
		{
			if(src[y>>FRACBITS])
				*dest = src[y>>FRACBITS];
			dest += vbufPitch;
		}
	}
}
