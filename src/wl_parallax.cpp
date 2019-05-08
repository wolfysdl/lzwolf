#include "version.h"

#include "wl_def.h"
#include "wl_agent.h"
#include "wl_play.h"
#include "wl_main.h"
#include "wl_draw.h"
#include "id_ca.h"
#include "g_mapinfo.h"

extern fixed viewz;

#ifdef USE_FEATUREFLAGS

// The lower left tile of every map determines the start texture of the parallax sky.
static int GetParallaxStartTexture()
{
	int startTex = ffDataBottomLeft;
	assert(startTex >= 0 && startTex < PMSpriteStart);
	return startTex;
}

#else

static int GetParallaxStartTexture()
{
	int startTex = 0;
	//switch(gamestate.episode * 10 + mapon)
	//{
	//	case  0: startTex = 20; break;
	//	default: startTex =  0; break;
	//}
	//assert(startTex >= 0 && startTex < PMSpriteStart);
	return startTex;
}

#endif

void DrawParallax(byte *vbuf, unsigned vbufPitch)
{
	int startpage = GetParallaxStartTexture();
	int midangle = (players[ConsolePlayer].camera->angle)>>ANGLETOFINESHIFT;
	int skyheight = viewheight >> 1;
	int curtex = -1;
	const byte *skytex;
	const int numParallax = levelInfo->ParallaxSky.Size();
	int i;
	int wbits = 0, hbits = 0;

	// make sure all tiles are equal width and height
	for (i = 0; i < numParallax; i++)
	{
		FTexture *source = TexMan(levelInfo->ParallaxSky[i]);
		wbits = (wbits == 0 ? source->WidthBits : wbits);
		if (source->WidthBits != wbits)
			return;
		hbits = (hbits == 0 ? source->HeightBits : hbits);
		if (source->HeightBits != hbits)
			return;
	}

	const int w = (1 << wbits);
	const int h = (1 << hbits);

	const int tmask = h*(w-1);

	fixed planeheight = viewz+(map->GetPlane(0).depth<<FRACBITS);
	const fixed heightFactor = abs(planeheight)>>8;

	startpage += numParallax - 1;

	for(int x = 0; x < viewwidth; x++)
	{
		int curang = pixelangle[x] + midangle;
		if(curang < 0) curang += FINEANGLES;
		else if(curang >= FINEANGLES) curang -= FINEANGLES;
		int xtex = curang * numParallax * w / FINEANGLES;
		int newtex = xtex >> wbits;
		if(newtex != curtex)
		{
			curtex = newtex;
			FTexture *source = TexMan(levelInfo->ParallaxSky[startpage-curtex]);
			skytex = source->GetPixels();
			//skytex = PM_GetTexture(startpage - curtex);
		}
		int texoffs = tmask - ((xtex & (w - 1)) << hbits);
		int yend = skyheight - ((wallheight[x]*heightFactor)>>FRACBITS);
		if(yend <= 0) continue;

		for(int y = 0, offs = x; y < yend; y++, offs += vbufPitch)
			vbuf[offs] = skytex[texoffs + (y * h) / skyheight];
	}
}
