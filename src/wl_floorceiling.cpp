#include <vector>
#include <algorithm>
#include "textures/textures.h"
#include "c_cvars.h"
#include "id_ca.h"
#include "gamemap.h"
#include "wl_def.h"
#include "wl_draw.h"
#include "wl_main.h"
#include "wl_shade.h"
#include "r_data/colormaps.h"
#include "g_mapinfo.h"

#include <climits>

extern fixed viewshift;
extern fixed viewz;

namespace Shading
{
	class Span
	{
	public:
		int len;
		int light;
		const byte *shades;

		explicit Span(int len_, int light_) : len(len_), light(light_), shades(0)
		{
		}
	};

	class YLine
	{
	public:
		fixed gu,gv,du,dv;
		int tz;
	};

	int halfheight;
	fixed planeheight;
	std::vector<Span> spans;
	Span *curspan;
	YLine yline;

	void PrepareConstants (int halfheight_, fixed planeheight_)
	{
		halfheight = halfheight_;
		planeheight = planeheight_;
	}

	void HitSpans (int curx, int cury)
	{
		spans.clear();
		spans.push_back(Span(viewwidth, 0));

		// ray
		// x = S + Vt
		// where V = E-S

		// halos
		// H(i): ||x - C(i)|| <= R(i)

		// f(t) = ||S + Vt - C||
		// need (f(t))^2 <= R^2
		// f(t) = ||Vt + (S-C)||
		// (f(t))^2 = V.Vt^2 +2V.(S-C)t + (S-C).(S-C)
	}

	void NextY (int y, fixed gu, fixed gv, fixed du, fixed dv)
	{
		// Depth fog
		yline.tz = FixedMul(FixedDiv(r_depthvisibility, abs(planeheight)), abs(((halfheight)<<16) - ((halfheight-y)<<16)));

		yline.gu = gu;
		yline.gv = gv;
		yline.du = du;
		yline.dv = dv;
	}

	void HitTile (int curx, int cury)
	{
		HitSpans (curx, cury);

		for (std::vector<Span>::size_type i = 0; i < spans.size(); i++)
		{
			Span &span = spans[i];
			const int shade = LIGHT2SHADE(gLevelLight + r_extralight + span.light);
			span.shades = &NormalLight.Maps[GETPALOOKUP(yline.tz, shade)<<8];
		}

		curspan = &spans[0];
	}

	const byte *ShadeForPix ()
	{
		const byte *curshades = curspan->shades;
		curspan->len--;
		if (!curspan->len)
			curspan++;
		return curshades;
	}
}

static inline bool R_PixIsTrans(byte col, const std::pair<bool, byte> &trans)
{
	return trans.first && col == trans.second;
}

static void R_DrawPlane(byte *vbuf, unsigned vbufPitch, int min_wallheight, int halfheight, fixed planeheight, std::pair<bool, byte> trans = std::make_pair(false, 0x00))
{
	fixed dist;                                // distance to row projection
	fixed tex_step;                            // global step per one screen pixel
	fixed gu, gv, du, dv;                      // global texture coordinates
	const byte *tex = NULL;
	int texwidth, texheight;
	fixed texxscale, texyscale;
	FTextureID lasttex;
	byte *tex_offset;
	bool useOptimized = false;

	if(planeheight == 0) // Eye level
		return;
	
	Shading::PrepareConstants (halfheight, planeheight);

	const fixed heightFactor = abs(planeheight)>>8;
	int y0 = ((min_wallheight*heightFactor)>>FRACBITS) - abs(viewshift);
	if(y0 > halfheight)
		return; // view obscured by walls
	if(y0 <= 0) y0 = 1; // don't let division by zero

	lasttex.SetInvalid();

	const unsigned int mapwidth = map->GetHeader().width;
	const unsigned int mapheight = map->GetHeader().height;

	fixed planenumerator = FixedMul(heightnumerator, planeheight);
	const bool floor = planenumerator < 0;
	int tex_offsetPitch;
	if(floor)
	{
		tex_offset = vbuf + (signed)vbufPitch * (halfheight + y0);
		tex_offsetPitch = vbufPitch-viewwidth;
		planenumerator *= -1;
	}
	else
	{
		tex_offset = vbuf + (signed)vbufPitch * (halfheight - y0 - 1);
		tex_offsetPitch = -viewwidth-vbufPitch;
	}

	unsigned int oldmapx = INT_MAX, oldmapy = INT_MAX;
	const byte* curshades = NormalLight.Maps;
	// draw horizontal lines
	for(int y = y0;floor ? y+halfheight < viewheight : y < halfheight; ++y, tex_offset += tex_offsetPitch)
	{
		if(floor ? (y+halfheight < 0) : (y < halfheight - viewheight))
		{
			tex_offset += viewwidth;
			continue;
		}

		// Shift in some extra bits so that we don't get spectacular round off.
		dist = (planenumerator / (y + 1))<<8;
		gu =  (viewx<<8) + FixedMul(dist, viewcos);
		gv = -(viewy<<8) + FixedMul(dist, viewsin);
		tex_step = dist / scale;
		du =  FixedMul(tex_step, viewsin);
		dv = -FixedMul(tex_step, viewcos);
		gu -= (viewwidth >> 1) * du;
		gv -= (viewwidth >> 1) * dv; // starting point (leftmost)

		curshades = NormalLight.Maps;
		Shading::NextY (y, gu, gv, du, dv);

		for(unsigned int x = 0;x < (unsigned)viewwidth; ++x, ++tex_offset)
		{
			if(((wallheight[x]*heightFactor)>>FRACBITS) <= y)
			{
				unsigned int curx = (gu >> (TILESHIFT+8));
				unsigned int cury = (-(gv >> (TILESHIFT+8)) - 1);

				if(curx != oldmapx || cury != oldmapy)
				{
					Shading::HitTile (curx, cury);

					oldmapx = curx;
					oldmapy = cury;
					const MapSpot spot = map->GetSpot(oldmapx%mapwidth, oldmapy%mapheight, 0);

					if(spot->sector)
					{
						FTextureID curtex = spot->sector->texture[floor ? MapSector::Floor : MapSector::Ceiling];
						if (curtex != lasttex && curtex.isValid())
						{
							FTexture * const texture = TexMan(curtex);
							lasttex = curtex;
							tex = texture->GetPixels();
							texwidth = texture->GetWidth();
							texheight = texture->GetHeight();
							texxscale = texture->xScale>>10;
							texyscale = -texture->yScale>>10;

							useOptimized = texwidth == 64 && texheight == 64 && texxscale == FRACUNIT>>10 && texyscale == -FRACUNIT>>10;
						}
					}
					else
						tex = NULL;
				}

				curshades = Shading::ShadeForPix ();

				if(tex)
				{
					if(useOptimized)
					{
						const int u = (gu>>18) & 63;
						const int v = (-gv>>18) & 63;
						const unsigned texoffs = (u * 64) + v;
						if (!R_PixIsTrans(tex[texoffs], trans))
							*tex_offset = curshades[tex[texoffs]];
					}
					else
					{
						const int u = (FixedMul((gu>>8)-512, texxscale)) & (texwidth-1);
						const int v = (FixedMul((gv>>8)+512, texyscale)) & (texheight-1);
						const unsigned texoffs = (u * texheight) + v;
						if (!R_PixIsTrans(tex[texoffs], trans))
							*tex_offset = curshades[tex[texoffs]];
					}
				}
			}
			else
			{
				Shading::ShadeForPix ();
			}
			gu += du;
			gv += dv;
		}
	}
}

// Textured Floor and Ceiling by DarkOne
// With multi-textured floors and ceilings stored in lower and upper bytes of
// according tile in third mapplane, respectively.
void DrawFloorAndCeiling(byte *vbuf, unsigned vbufPitch, int min_wallheight)
{
	const int halfheight = (viewheight >> 1) - viewshift;

	const byte skyceilcol = (gameinfo.parallaxskyceilcolor > 256 ?
		(byte)(gameinfo.parallaxskyceilcolor&0xff) : 0xff);

	const int numParallax = levelInfo->ParallaxSky.Size();
	std::pair<bool, byte> ceiltrans(numParallax > 0, skyceilcol);

	R_DrawPlane(vbuf, vbufPitch, min_wallheight, halfheight, viewz);
	R_DrawPlane(vbuf, vbufPitch, min_wallheight, halfheight, viewz+(map->GetPlane(0).depth<<FRACBITS), ceiltrans);
}
