#include "version.h"

#include "wl_def.h"
#include "wl_agent.h"
#include "wl_play.h"
#include "wl_main.h"
#include "wl_draw.h"
#include "id_ca.h"
#include "g_mapinfo.h"

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

	startpage += numParallax - 1;

	for(int x = 0; x < viewwidth; x++)
	{
		int curang = pixelangle[x] + midangle;
		if(curang < 0) curang += FINEANGLES;
		else if(curang >= FINEANGLES) curang -= FINEANGLES;
		int xtex = curang * numParallax * TEXTURESIZE / FINEANGLES;
		int newtex = xtex >> TEXTURESHIFT;
		if(newtex != curtex)
		{
			curtex = newtex;
			FTexture *source = TexMan(levelInfo->ParallaxSky[startpage-curtex]);
			skytex = source->GetPixels();
			//skytex = PM_GetTexture(startpage - curtex);
		}
		int texoffs = TEXTUREMASK - ((xtex & (TEXTURESIZE - 1)) << TEXTURESHIFT);
		int yend = skyheight - (wallheight[x] >> 3);
		if(yend <= 0) continue;

		for(int y = 0, offs = x; y < yend; y++, offs += vbufPitch)
			vbuf[offs] = skytex[texoffs + (y * TEXTURESIZE) / skyheight];
	}
}
