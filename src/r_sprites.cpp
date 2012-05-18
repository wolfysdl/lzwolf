/*
** r_sprites.cpp
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

#include "textures/textures.h"
#include "c_cvars.h"
#include "r_sprites.h"
#include "linkedlist.h"
#include "tarray.h"
#include "templates.h"
#include "actor.h"
#include "thingdef/thingdef.h"
#include "v_palette.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_main.h"
#include "wl_play.h"
#include "wl_shade.h"
#include "zstring.h"
#include "r_data/colormaps.h"
#include "a_inventory.h"

struct SpriteInfo
{
	union
	{
		char 		name[5];
		uint32_t	iname;
	};
	unsigned int	frames;
};

struct Sprite
{
	static const uint8_t NO_FRAMES = 255; // If rotations == NO_FRAMES

	FTextureID	texture[8];
	uint8_t		rotations;
	uint16_t	mirror; // Mirroring bitfield
};

TArray<Sprite> spriteFrames;
TArray<SpriteInfo> loadedSprites;

bool R_CheckSpriteValid(unsigned int spr)
{
	if(spr < NUM_SPECIAL_SPRITES)
		return true;

	SpriteInfo &sprite = loadedSprites[spr];
	if(sprite.frames == 0)
		return false;
	return true;
}

// Cache sprite name lookups
unsigned int R_GetSprite(const char* spr)
{
	static unsigned int mid = 0;

	union
	{
		char 		name[4];
		uint32_t	iname;
	} tmp;
	memcpy(tmp.name, spr, 4);

	if(tmp.iname == loadedSprites[mid].iname)
		return mid;

	for(mid = 0;mid < NUM_SPECIAL_SPRITES;++mid)
	{
		if(tmp.iname == loadedSprites[mid].iname)
			return mid;
	}

	unsigned int max = loadedSprites.Size()-1;
	unsigned int min = NUM_SPECIAL_SPRITES;
	mid = (min+max)/2;
	do
	{
		if(tmp.iname == loadedSprites[mid].iname)
			return mid;

		if(tmp.iname < loadedSprites[mid].iname)
			max = mid-1;
		else if(tmp.iname > loadedSprites[mid].iname)
			min = mid+1;
		mid = (min+max)/2;
	}
	while(max >= min);

	// I don't think this should ever happen, but if it does return no sprite.
	return 0;
}

void R_InstallSprite(Sprite &frame, FTexture *tex, int dir, bool mirror)
{
	if(dir < -1 || dir >= 8)
	{
		printf("Invalid frame data for '%s' %d.\n", tex->Name, dir);
		return;
	}

	if(dir == -1)
	{
		frame.rotations = 0;
		dir = 0;
	}
	else
		frame.rotations = 8;

	frame.texture[dir] = tex->GetID();
	frame.mirror |= 1<<dir;
}

int SpriteCompare(const void *s1, const void *s2)
{
	uint32_t n1 = static_cast<const SpriteInfo *>(s1)->iname;
	uint32_t n2 = static_cast<const SpriteInfo *>(s2)->iname;
	if(n1 < n2)
		return -1;
	else if(n1 > n2)
		return 1;
	return 0;
}

void R_InitSprites()
{
	static const uint8_t MAX_SPRITE_FRAMES = 29; // A-Z, [, \, ]

	// First sort the loaded sprites list
	qsort(&loadedSprites[NUM_SPECIAL_SPRITES], loadedSprites.Size()-NUM_SPECIAL_SPRITES, sizeof(loadedSprites[0]), SpriteCompare);

	typedef LinkedList<FTexture*> SpritesList;
	typedef TMap<uint32_t, SpritesList> SpritesMap;

	SpritesMap spritesMap;

	// Collect potential sprite list (linked list of sprites by name)
	for(unsigned int i = 0;i < TexMan.NumTextures();++i)
	{
		FTexture *tex = TexMan.ByIndex(i);
		if(tex->UseType == FTexture::TEX_Sprite && strlen(tex->Name) >= 6)
		{
			SpritesList &list = spritesMap[tex->dwName];
			list.Push(tex);
		}
	}

	// Now process the sprites if we need to load them
	for(unsigned int i = NUM_SPECIAL_SPRITES;i < loadedSprites.Size();++i)
	{
		SpritesList &list = spritesMap[loadedSprites[i].iname];
		if(list.Size() == 0)
			continue;
		loadedSprites[i].frames = spriteFrames.Size();

		Sprite frames[MAX_SPRITE_FRAMES];
		uint8_t maxframes = 0;
		for(unsigned int j = 0;j < MAX_SPRITE_FRAMES;++j)
		{
			frames[j].rotations = Sprite::NO_FRAMES;
			frames[j].mirror = 0;
		}

		for(SpritesList::Node *iter = list.Head();iter;iter = iter->Next())
		{
			FTexture *tex = iter->Item();
			char frame = tex->Name[4] - 'A';
			if(frame < MAX_SPRITE_FRAMES)
			{
				if(frame > maxframes)
					maxframes = frame;
				R_InstallSprite(frames[frame], tex, tex->Name[5] - '1', false);

				if(strlen(tex->Name) == 8)
				{
					frame = tex->Name[6] - 'A';
					if(frame < MAX_SPRITE_FRAMES)
					{
						if(frame > maxframes)
							maxframes = frame;
						R_InstallSprite(frames[frame], tex, tex->Name[7] - '1', true);
					}
				}
			}
		}

		++maxframes;
		for(unsigned int j = 0;j < maxframes;++j)
		{
			// Check rotations
			if(frames[j].rotations == 8)
			{
				for(unsigned int r = 0;r < 8;++r)
				{
					if(!frames[j].texture[r].isValid())
					{
						printf("Sprite %s is missing rotations for frame %c.\n", loadedSprites[i].name, j);
						break;
					}
				}
			}

			spriteFrames.Push(frames[j]);
		}
	}
}

void R_LoadSprite(const FString &name)
{
	if(loadedSprites.Size() == 0)
	{
		// Make sure the special sprites are loaded
		SpriteInfo sprInf;
		sprInf.frames = 0;
		strcpy(sprInf.name, "TNT1");
		loadedSprites.Push(sprInf);
	}

	if(name.Len() != 4)
	{
		printf("Sprite name invalid.\n");
		return;
	}

	static uint32_t lastSprite = 0;
	SpriteInfo sprInf;
	sprInf.frames = 0;

	strcpy(sprInf.name, name.GetChars());
	if(loadedSprites.Size() > 0)
	{
		if(sprInf.iname == lastSprite)
			return;

		for(unsigned int i = 0;i < loadedSprites.Size();++i)
		{
			if(loadedSprites[i].iname == sprInf.iname)
			{
				sprInf = loadedSprites[i];
				lastSprite = sprInf.iname;
				return;
			}
		}
	}
	lastSprite = sprInf.iname;

	loadedSprites.Push(sprInf);
}

////////////////////////////////////////////////////////////////////////////////

// From wl_draw.cpp
int CalcRotate(AActor *ob);
extern byte* vbuf;
extern unsigned vbufPitch;

void ScaleSprite(AActor *actor, int xcenter, const Frame *frame, unsigned height)
{
	if(actor->sprite == SPR_NONE)
		return;

	const Sprite &spr = spriteFrames[loadedSprites[actor->sprite].frames+frame->frame];
	FTexture *tex;
	if(spr.rotations == 0)
		tex = TexMan[spr.texture[0]];
	else
		tex = TexMan[spr.texture[(CalcRotate(actor)+4)%8]];
	if(tex == NULL)
		return;

	const BYTE *colormap;
	if(!r_depthfog || (actor->flags & FL_BRIGHT) || frame->fullbright)
		colormap = NormalLight.Maps;
	else
		colormap = &NormalLight.Maps[256*GetShade(height)];

	const unsigned int scale = height>>3; // Integer part of the height
	if(scale == 0)
		return;

	const double dyScale = height/256.0;
	const double dxScale = (height/256.0)/(yaspect/65536.);

	const int actx = xcenter - tex->GetScaledLeftOffsetDouble()*dxScale;
	const int upperedge = (viewheight/2)+scale - tex->GetScaledTopOffsetDouble()*dyScale;

	const unsigned int startX = -MIN(actx, 0);
	const unsigned int startY = -MIN(upperedge, 0);
	const fixed xStep = (1/dxScale)*FRACUNIT;
	const fixed yStep = (1/dyScale)*FRACUNIT;

	const BYTE *src;
	byte *dest;
	unsigned int i, j;
	fixed x, y;
	for(i = startX, x = startX*xStep;x < tex->GetWidth()<<FRACBITS;x += xStep, ++i)
	{
		src = tex->GetColumn(x>>FRACBITS, NULL);
		dest = vbuf+actx+i;
		if(actx+i >= viewwidth)
			break;
		else if(wallheight[actx+i] > height)
			continue;
		if(upperedge > 0)
			dest += vbufPitch*upperedge;

		for(j = startY, y = startY*yStep;y < tex->GetHeight()<<FRACBITS;y += yStep, ++j)
		{
			if(upperedge+j >= viewheight)
				break;
			if(src[y>>FRACBITS] != 0)
				*dest = colormap[src[y>>FRACBITS]];
			dest += vbufPitch;
		}
	}
}

void R_DrawPlayerSprite(AActor *actor, const Frame *frame, fixed offsetX, fixed offsetY)
{
	if(frame->spriteInf == SPR_NONE)
		return;

	const Sprite &spr = spriteFrames[loadedSprites[frame->spriteInf].frames+frame->frame];
	FTexture *tex;
	if(spr.rotations == 0)
		tex = TexMan[spr.texture[0]];
	else
		tex = TexMan[spr.texture[(CalcRotate(actor)+4)%8]];
	if(tex == NULL)
		return;

	const BYTE *colormap = NormalLight.Maps;

	const fixed scale = viewheight<<(FRACBITS-1);

	const fixed centeringOffset = (centerx - 2*centerxwide)<<FRACBITS;
	const fixed leftedge = FixedMul((160<<FRACBITS) - fixed(tex->GetScaledLeftOffsetDouble()*FRACUNIT) + offsetX, pspritexscale) + centeringOffset;
	fixed upperedge = ((100-32)<<FRACBITS) + fixed(tex->GetScaledTopOffsetDouble()*FRACUNIT) - offsetY - AspectCorrection[vid_aspect].tallscreen;
	if(viewsize == 21 && players[0].ReadyWeapon)
	{
		upperedge -= players[0].ReadyWeapon->yadjust;
	}
	upperedge = scale - FixedMul(upperedge, pspriteyscale);

	// startX and startY indicate where the sprite becomes visible, we only
	// need to calculate the start since the end will be determined when we hit
	// the view during drawing.
	const unsigned int startX = -MIN(leftedge>>FRACBITS, 0);
	const unsigned int startY = -MIN(upperedge>>FRACBITS, 0);
	const fixed xStep = FixedDiv(tex->xScale, pspritexscale);
	const fixed yStep = FixedDiv(tex->yScale, pspriteyscale);

	const int x1 = leftedge>>FRACBITS;
	const int y1 = upperedge>>FRACBITS;
	const BYTE *src;
	byte *dest;
	unsigned int i, j;
	fixed x, y;
	for(i = startX, x = startX*xStep;x < tex->GetWidth()<<FRACBITS;x += xStep, ++i)
	{
		src = tex->GetColumn(x>>FRACBITS, NULL);
		dest = vbuf+x1+i;
		if(x1+i >= viewwidth)
			break;
		if(y1 > 0)
			dest += vbufPitch*y1;

		for(j = startY, y = startY*yStep;y < tex->GetHeight()<<FRACBITS;y += yStep, ++j)
		{
			if(y1+j >= viewheight)
				break;
			if(src[y>>FRACBITS] != 0)
				*dest = colormap[src[y>>FRACBITS]];
			dest += vbufPitch;
		}
	}
}