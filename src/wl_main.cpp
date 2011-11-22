// WL_MAIN.C

#ifdef _WIN32
//	#include <io.h>
#else
	#include <unistd.h>
#endif

#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_pm.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#pragma hdrstop
#include "wl_atmos.h"
#include "m_classes.h"
#include "m_random.h"
#include "config.hpp"
#include "w_wad.h"
#include "language.h"
#include "textures/textures.h"
#include "c_cvars.h"
#include "thingdef/thingdef.h"
#include "v_palette.h"
#include "v_video.h"
#include "r_data/colormaps.h"
#include "wl_agent.h"
#include "doomerrors.h"
#include "lumpremap.h"

// Wad Code Stuff
TArray<FString> wadfiles;
void WL_AddFile(const char *file)
{
	wadfiles.Push(file);
/*	wadlist_t *wad = (wadlist_t *)malloc(sizeof(*wad) + strlen(file));

	*wadtail = wad;
	wad->next = NULL;
	strcpy(wad->name, file);
	wadtail = &wad->next;*/
}


/*
=============================================================================

							WOLFENSTEIN 3-D

						An Id Software production

							by John Carmack

=============================================================================
*/

extern byte signon[];

/*
=============================================================================

							LOCAL CONSTANTS

=============================================================================
*/


#define FOCALLENGTH     (0x5700l)               // in global coordinates

#define VIEWWIDTH       256                     // size of view window
#define VIEWHEIGHT      144

/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

char    str[80];
int     dirangle[9] = {0,ANGLES/8,2*ANGLES/8,3*ANGLES/8,4*ANGLES/8,
					5*ANGLES/8,6*ANGLES/8,7*ANGLES/8,ANGLES};

//
// proejection variables
//
fixed    focallength;
unsigned screenofs;
int      viewscreenx, viewscreeny;
int      viewwidth;
int      viewheight;
short    centerx;
int      shootdelta;           // pixels away from centerx a target can be
fixed    scale;
int32_t  heightnumerator;


void    Quit (const char *error,...);

boolean startgame;
boolean loadedgame;
int     mouseadjustment;

//
// Command line parameter variables
//
boolean param_nowait = false;
int     param_difficulty = 1;           // default is "normal"
int     param_tedlevel = -1;            // default is not to start a level
int     param_joystickindex = 0;

#if defined(_arch_dreamcast)
int     param_joystickhat = 0;
int     param_samplerate = 11025;       // higher samplerates result in "out of memory"
int     param_audiobuffer = 4096 / (44100 / param_samplerate);
#elif defined(GP2X)
int     param_joystickhat = -1;
int     param_samplerate = 11025;       // higher samplerates result in "out of memory"
int     param_audiobuffer = 128;
#else
int     param_joystickhat = -1;
int     param_samplerate = 44100;
int     param_audiobuffer = 2048 / (44100 / param_samplerate);
#endif

int     param_mission = 1;
boolean param_goodtimes = false;
boolean param_ignorenumchunks = false;

/*
=============================================================================

							LOCAL VARIABLES

=============================================================================
*/





//===========================================================================

/*
=====================
=
= NewGame
=
= Set up new game to start from the beginning
=
=====================
*/

void NewGame (int difficulty,int episode)
{
	memset (&gamestate,0,sizeof(gamestate));
	gamestate.difficulty = difficulty;
	gamestate.weapon = gamestate.bestweapon
			= gamestate.chosenweapon = wp_pistol;
	gamestate.ammo = STARTAMMO;
	gamestate.episode=episode;

	players[0].state = player_t::PST_ENTER;

	startgame = true;
}

//===========================================================================

void DiskFlopAnim(int x,int y)
{
	static int8_t which=0;
	if (!x && !y)
		return;
	VWB_DrawPic(x,y,which == 0 ? "M_LDING1" : "M_LDING2");
	VW_UpdateScreen();
	which^=1;
}


int32_t DoChecksum(byte *source,unsigned size,int32_t checksum)
{
	unsigned i;

	for (i=0;i<size-1;i++)
	checksum += source[i]^source[i+1];

	return checksum;
}


/*
==================
=
= SaveTheGame
=
==================
*/

extern statetype s_grdstand;
extern statetype s_player;

boolean SaveTheGame(FILE *file,int x,int y)
{
	return false;
}

//===========================================================================

/*
==================
=
= LoadTheGame
=
==================
*/

boolean LoadTheGame(FILE *file,int x,int y)
{
	return false;
}

//===========================================================================

/*
==========================
=
= ShutdownId
=
= Shuts down all ID_?? managers
=
==========================
*/

void ShutdownId (void)
{
	US_Shutdown ();         // This line is completely useless...
	SD_Shutdown ();
	PM_Shutdown ();
	IN_Shutdown ();
	VW_Shutdown ();
#if defined(GP2X)
	GP2X_Shutdown();
#endif
}


//===========================================================================

/*
==================
=
= BuildTables
=
= Calculates:
=
= scale                 projection constant
= sintable/costable     overlapping fractional tables
=
==================
*/

const float radtoint = (float)(FINEANGLES/2/PI);

void BuildTables (void)
{
	//
	// calculate fine tangents
	//

	int i;
	for(i=0;i<FINEANGLES/8;i++)
	{
		double tang=tan((i+0.5)/radtoint);
		finetangent[i]=(int32_t)(tang*GLOBAL1);
		finetangent[FINEANGLES/4-1-i]=(int32_t)((1/tang)*GLOBAL1);
	}

	//
	// costable overlays sintable with a quarter phase shift
	// ANGLES is assumed to be divisable by four
	//

	float angle=0;
	float anglestep=(float)(PI/2/ANGLEQUAD);
	for(i=0; i<ANGLEQUAD; i++)
	{
		fixed value=(int32_t)(GLOBAL1*sin(angle));
		sintable[i]=sintable[i+ANGLES]=sintable[ANGLES/2-i]=value;
		sintable[ANGLES-i]=sintable[ANGLES/2+i]=-value;
		angle+=anglestep;
	}
	angle = 0;
	anglestep = (float)(PI/2/ANG90);
	for(i=0; i<FINEANGLES; i++)
	{
		finesine[i]=fixed(GLOBAL1*sin(angle));
		angle+=anglestep;
	}
	sintable[ANGLEQUAD] = 65536;
	sintable[3*ANGLEQUAD] = -65536;

#if defined(USE_STARSKY) || defined(USE_RAIN) || defined(USE_SNOW)
	Init3DPoints();
#endif
}

//===========================================================================


/*
====================
=
= CalcProjection
=
= Uses focallength
=
====================
*/

void CalcProjection (int32_t focal)
{
	int     i;
	int    intang;
	float   angle;
	double  tang;
	int     halfview;
	double  facedist;

	focallength = focal;
	facedist = focal+MINDIST;
	halfview = viewwidth/2;                                 // half view in pixels

	//
	// calculate scale value for vertical height calculations
	// and sprite x calculations
	//
	scale = (fixed) (halfview*facedist/(AspectCorrection[vid_aspect].viewGlobal/2));

	//
	// divide heightnumerator by a posts distance to get the posts height for
	// the heightbuffer.  The pixel height is height>>2
	//
	heightnumerator = CorrectHeightFactor((TILEGLOBAL*scale)>>6);

	//
	// calculate the angle offset from view angle of each pixel's ray
	//

	for (i=0;i<halfview;i++)
	{
		// start 1/2 pixel over, so viewangle bisects two middle pixels
		tang = (int32_t)i*AspectCorrection[vid_aspect].viewGlobal/viewwidth/facedist;
		angle = (float) atan(tang);
		intang = (int) (angle*radtoint);
		pixelangle[halfview-1-i] = intang;
		pixelangle[halfview+i] = -intang;
	}
}

//===========================================================================

/*
==========================
=
= SignonScreen
=
==========================
*/

void SignonScreen (void)                        // VGA version
{
	VL_SetVGAPlaneMode ();
	CA_CacheScreen("WLFSGNON");
}


/*
==========================
=
= FinishSignon
=
==========================
*/

void FinishSignon (void)
{
#ifndef SPEAR
	VW_Bar (0,189,300,11,VL_GetPixel(0,0));
	WindowX = 0;
	WindowW = 320;
	PrintY = 190;

	#ifndef JAPAN
	SETFONTCOLOR(14,4);

	#ifdef SPANISH
	US_CPrint ("Oprima una tecla");
	#else
	US_CPrint ("Press a key");
	#endif

	#endif

	VH_UpdateScreen();

	if (!param_nowait)
		IN_Ack ();

	#ifndef JAPAN
	VW_Bar (0,189,300,11,VL_GetPixel(0,0));

	PrintY = 190;
	SETFONTCOLOR(10,4);

	#ifdef SPANISH
	US_CPrint ("pensando...");
	#else
	US_CPrint ("Working...");
	#endif

	VH_UpdateScreen();
	#endif

	SETFONTCOLOR(0,15);
#else
	VH_UpdateScreen();

	if (!param_nowait)
		VW_WaitVBL(3*70);
#endif
}

//===========================================================================

Menu musicMenu(CTL_X, CTL_Y-6, 280, 32);
static TArray<FString> songList;

MENU_LISTENER(ChangeMusic)
{
	StartCPMusic(songList[which]);
	for(unsigned int i = 0;i < songList.Size();++i)
		musicMenu[i]->setHighlighted(i == which);
	musicMenu.draw();
	return true;
}

void DoJukebox(void)
{
	IN_ClearKeysDown();
	if (!AdLibPresent && !SoundBlasterPresent)
		return;

	MenuFadeOut();

	fontnumber=1;
	ClearMScreen ();
	musicMenu.setHeadText(language["ROBSJUKEBOX"], true);
	for(unsigned int i = 0;i < Wads.GetNumLumps();++i)
	{
		if(Wads.GetLumpNamespace(i) != ns_music)
			continue;

		FString langString;
		langString.Format("MUS_%s", Wads.GetLumpFullName(i));
		const char* trackName = language[langString];
		if(trackName == langString.GetChars())
			musicMenu.addItem(new MenuItem(Wads.GetLumpFullName(i), ChangeMusic));
		else
			musicMenu.addItem(new MenuItem(language[langString], ChangeMusic));
		songList.Push(Wads.GetLumpFullName(i));
		
	}
	musicMenu.show();
	return;
}

/*
==========================
=
= InitGame
=
= Load a few things right away
=
==========================
*/

#ifdef _WIN32
void SetupWM();
#endif

static void InitGame()
{
	// initialize SDL
#if defined _WIN32
	putenv("SDL_VIDEODRIVER=directx");
#endif
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		printf("Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atterm(SDL_Quit);

	int numJoysticks = SDL_NumJoysticks();
	if(param_joystickindex && (param_joystickindex < -1 || param_joystickindex >= numJoysticks))
	{
		if(!numJoysticks)
			printf("No joysticks are available to SDL!\n");
		else
			printf("The joystick index must be between -1 and %i!\n", numJoysticks - 1);
		exit(1);
	}

	//
	// Init texture manager
	//

	TexMan.Init();

	SignonScreen ();

#if defined _WIN32
	if(!fullscreen)
		SetupWM();
#endif
	VW_UpdateScreen();

	GenerateLookupTables();
	VH_Startup ();
	IN_Startup ();
	PM_Startup ();
	SD_Startup ();
	printf("US_Startup: Starting the User Manager.\n");
	US_Startup ();

	// TODO: Will any memory checking be needed someday??
#ifdef NOTYET
#ifndef SPEAR
	if (mminfo.mainmem < 235000L)
#else
	if (mminfo.mainmem < 257000L && !MS_CheckParm("debugmode"))
#endif
	{
		byte *screen;

		CA_CacheGrChunk (ERRORSCREEN);
		screen = grsegs[ERRORSCREEN];
		ShutdownId();
/*        memcpy((byte *)0xb8000,screen+7+7*160,17*160);
		gotoxy (1,23);*/
		exit(1);
	}
#endif


//
// build some tables
//

	printf("SetupSaveGames: Looking for saves.\n");
	SetupSaveGames();

//
// Load Actors
//

	ClassDef::LoadActors();

//
// draw intro screen stuff
//
	IntroScreen ();

//
// Finish with setting up through the config file.
//
	FinalReadConfig();

//
// load in and lock down some basic chunks
//

	LoadLatchMem ();
	BuildTables ();          // trig tables

	NewViewSize (viewsize);

//
// initialize variables
//
	InitRedShifts ();
	FinishSignon();

//
// HOLDING DOWN 'M' KEY?
//
	IN_ProcessEvents();

	if (Keyboard[sc_M])
		DoJukebox();

#ifdef NOTYET
	vdisp = (byte *) (0xa0000+PAGE1START);
	vbuf = (byte *) (0xa0000+PAGE2START);
#endif
}

//===========================================================================

/*
==========================
=
= SetViewSize
=
==========================
*/

boolean SetViewSize (unsigned width, unsigned height)
{
	viewwidth = width&~15;                  // must be divisable by 16
	viewheight = height&~1;                 // must be even
	centerx = viewwidth/2-1;
	shootdelta = viewwidth/10;
	if((unsigned) viewheight == screenHeight)
		viewscreenx = viewscreeny = screenofs = 0;
	else
	{
		viewscreenx = (screenWidth-viewwidth) / 2;
		viewscreeny = (screenHeight-scaleFactor*STATUSLINES-viewheight)/2;
		screenofs = viewscreeny*screenWidth+viewscreenx;
	}

//
// calculate trace angles and projection constants
//
	CalcProjection (FOCALLENGTH);

	return true;
}


void ShowViewSize (int width)
{
	int oldwidth,oldheight;

	oldwidth = viewwidth;
	oldheight = viewheight;

	if(width == 21)
	{
		viewwidth = screenWidth;
		viewheight = screenHeight;
		VWB_BarScaledCoord (0, 0, screenWidth, screenHeight, 0);
	}
	else if(width == 20)
	{
		viewwidth = screenWidth;
		viewheight = screenHeight - scaleFactor*STATUSLINES;
		DrawPlayBorder ();
	}
	else
	{
		viewwidth = width*16*screenWidth/320;
		viewheight = (int) (width*16*HEIGHTRATIO*screenHeight/200);
		DrawPlayBorder ();
	}

	viewwidth = oldwidth;
	viewheight = oldheight;
}


void NewViewSize (int width)
{
	if(width < 4 || width > 21)
		return;

	viewsize = width;
	if(viewsize == 21)
		SetViewSize(screenWidth, screenHeight);
	else if(viewsize == 20)
		SetViewSize(screenWidth, screenHeight - scaleFactor * STATUSLINES);
	else
		SetViewSize(width*16*screenWidth/320, (unsigned) (width*16*HEIGHTRATIO*screenHeight/200));
}



//===========================================================================

/*
==========================
=
= Quit
=
==========================
*/

void Quit (const char *errorStr, ...)
{
#ifdef NOTYET
	byte *screen;
#endif
	char error[256];
	if(errorStr != NULL)
	{
		va_list vlist;
		va_start(vlist, errorStr);
		vsprintf(error, errorStr, vlist);
		va_end(vlist);
	}
	else error[0] = 0;

/*    if (!pictable)  // don't try to display the red box before it's loaded
	{
		ShutdownId();
		if (error && *error)
		{
#ifdef NOTYET
			SetTextCursor(0,0);
#endif
			puts(error);
#ifdef NOTYET
			SetTextCursor(0,2);
#endif
			VW_WaitVBL(100);
		}
		exit(1);
	}*/

	if (error[0] == 0)
	{
#ifdef NOTYET
		#ifndef JAPAN
		CA_CacheGrChunk (ORDERSCREEN);
		screen = grsegs[ORDERSCREEN];
		#endif
#endif

		WriteConfig ();
	}
#ifdef NOTYET
	else
	{
		CA_CacheGrChunk (ERRORSCREEN);
		screen = grsegs[ERRORSCREEN];
	}
#endif

	ShutdownId ();

	if (error[0] != 0)
	{
#ifdef NOTYET
		memcpy((byte *)0xb8000,screen+7,7*160);
		SetTextCursor(9,3);
#endif
		puts(error);
#ifdef NOTYET
		SetTextCursor(0,7);
#endif
		VW_WaitVBL(200);
		exit(1);
	}
	else
	if (error[0] == 0)
	{
#ifdef NOTYET
		#ifndef JAPAN
		memcpy((byte *)0xb8000,screen+7,24*160); // 24 for SPEAR/UPLOAD compatibility
		#endif
		SetTextCursor(0,23);
#endif
	}

	exit(0);
}

//===========================================================================



/*
=====================
=
= DemoLoop
=
=====================
*/


static void DemoLoop()
{
	int LastDemo = 0;

//
// check for launch from ted
//
	if (param_tedlevel != -1)
	{
		param_nowait = true;
		NewGame(param_difficulty,0);

#ifndef SPEAR
		gamestate.episode = param_tedlevel/10;
		gamestate.mapon = param_tedlevel%10;
#else
		gamestate.episode = 0;
		gamestate.mapon = param_tedlevel;
#endif
		GameLoop();
		Quit (NULL);
	}


//
// main game cycle
//

#ifndef DEMOTEST

	#ifndef UPLOAD

		#ifndef GOODTIMES
		#ifndef SPEAR
		#ifndef JAPAN
		if (!param_nowait)
			NonShareware();
		#endif
		#endif
		#endif
	#endif

	StartCPMusic(INTROSONG);

#ifndef JAPAN
	if (!param_nowait)
		PG13 ();
#endif

#endif

	while (1)
	{
		while (!param_nowait)
		{
//
// title page
//
#ifndef DEMOTEST

#ifdef SPEAR
			SDL_Color pal[256];
			VL_ConvertPalette("TITLEPAL", pal, 256);

			VWB_DrawPic (0,0,"TITLE1");

			VWB_DrawPic (0,80,"TITLE2");
			VW_UpdateScreen ();
			VL_FadeIn(0,255,pal,30);
#else
			CA_CacheScreen ("TITLEPIC");
			VW_UpdateScreen ();
			VW_FadeIn();
#endif
			if (IN_UserInput(TickBase*15))
				break;
			VW_FadeOut();
//
// credits page
//
			CA_CacheScreen ("CREDITS");
			VW_UpdateScreen();
			VW_FadeIn ();
			if (IN_UserInput(TickBase*10))
				break;
			VW_FadeOut ();
//
// high scores
//
			DrawHighScores ();
			VW_UpdateScreen ();
			VW_FadeIn ();

			if (IN_UserInput(TickBase*10))
				break;
#endif
//
// demo
//

			bool demoPlayed = false;
			do // This basically loops twice at most.  If the lump exists it plays the demo if not it goes to DEMO0.
			{  // PlayDemo will actually play the demo picked if it exists otherwise it will immediately return.
				char demoName[9];
				sprintf(demoName, "DEMO%d", LastDemo);
				if(Wads.CheckNumForName(demoName) == -1)
				{
					if(LastDemo == 0)
						break;
					else
						LastDemo = 0;
					continue;
				}
				else
				{
					demoPlayed = true;
					PlayDemo(LastDemo++);
					break;
				}
			}
			while(true);

			if (playstate == ex_abort)
				break;
			VW_FadeOut();
			if(screenHeight % 200 != 0)
				VL_ClearScreen(0);
			if(demoPlayed)
				StartCPMusic(INTROSONG);
		}

		VW_FadeOut ();

		if (Keyboard[sc_Tab])
			RecordDemo ();
		else
			US_ControlPanel (0);

		if (startgame || loadedgame)
		{
			GameLoop ();
			if(!param_nowait)
			{
				VW_FadeOut();
				StartCPMusic(INTROSONG);
			}
		}
	}
}


//===========================================================================

// CheckRatio -- From ZDoom
//
// Tries to guess the physical dimensions of the screen based on the
// screen's pixel dimensions.
Aspect CheckRatio (int width, int height)//, int *trueratio)
{
	//int fakeratio = -1;
	Aspect ratio;

	/*if ((vid_aspect >=1) && (vid_aspect <=4))
	{
		// [SP] User wants to force aspect ratio; let them.
		fakeratio = vid_aspect == 3? 0: int(vid_aspect);
	}*/
	/*if (vid_nowidescreen)
	{
		if (!vid_tft)
		{
			fakeratio = 0;
		}
		else
		{
			fakeratio = (height * 5/4 == width) ? 4 : 0;
		}
	}*/
	// If the size is approximately 16:9, consider it so.
	if (abs (height * 16/9 - width) < 10)
	{
		ratio = ASPECT_16_9;
	}
	// 16:10 has more variance in the pixel dimensions. Grr.
	else if (abs (height * 16/10 - width) < 60)
	{
		// 320x200 and 640x400 are always 4:3, not 16:10
		if ((width == 320 && height == 200) || (width == 640 && height == 400))
		{
			ratio = ASPECT_NONE;
		}
		else
		{
			ratio = ASPECT_16_10;
		}
	}
	// Unless vid_tft is set, 1280x1024 is 4:3, not 5:4.
	else if (height * 5/4 == width)// && vid_tft)
	{
		ratio = ASPECT_5_4;
	}
	// Assume anything else is 4:3.
	else
	{
		ratio = ASPECT_4_3;
	}

	/*if (trueratio != NULL)
	{
		*trueratio = ratio;
	}*/
	return /*(fakeratio >= 0) ? fakeratio :*/ ratio;
}

#define IFARG(str) if(!strcmp(arg, (str)))

void CheckParameters(int argc, char *argv[])
{
	bool hasError = false, showHelp = false;
	bool sampleRateGiven = false, audioBufferGiven = false;
	int defaultSampleRate = param_samplerate;
	bool needRatio = true;

	fullscreen = vid_fullscreen;

	for(int i = 1; i < argc; i++)
	{
		char *arg = argv[i];
		IFARG("--baby")
			param_difficulty = 0;
		else IFARG("--easy")
			param_difficulty = 1;
		else IFARG("--normal")
			param_difficulty = 2;
		else IFARG("--hard")
			param_difficulty = 3;
		else IFARG("--nowait")
			param_nowait = true;
		else IFARG("--tedlevel")
		{
			if(++i >= argc)
			{
				printf("The tedlevel option is missing the level argument!\n");
				hasError = true;
			}
			else param_tedlevel = atoi(argv[i]);
		}
		else IFARG("--fullscreen")
			fullscreen = true;
		else IFARG("--res")
		{
			if(i + 2 >= argc)
			{
				printf("The res option needs the width and/or the height argument!\n");
				hasError = true;
			}
			else
			{
				screenWidth = atoi(argv[++i]);
				screenHeight = atoi(argv[++i]);
				unsigned factor = screenWidth / 320;
				if(screenWidth % 320 || screenHeight != 200 * factor && screenHeight != 240 * factor)
					printf("Screen size must be a multiple of 320x200 or 320x240!\n"), hasError = true;
			}
		}
		else IFARG("--resf")
		{
			if(i + 2 >= argc)
			{
				printf("The resf option needs the width and/or the height argument!\n");
				hasError = true;
			}
			else
			{
				screenWidth = atoi(argv[++i]);
				screenHeight = atoi(argv[++i]);
				if(screenWidth < 320)
					printf("Screen width must be at least 320!\n"), hasError = true;
				if(screenHeight < 200)
					printf("Screen height must be at least 200!\n"), hasError = true;
			}
		}
		else IFARG("--aspect")
		{
			const char* ratio = argv[++i];
			if(strcmp(ratio, "4:3") == 0)
				vid_aspect = ASPECT_4_3;
			else if(strcmp(ratio, "16:10") == 0)
				vid_aspect = ASPECT_16_10;
			else if(strcmp(ratio, "16:9") == 0)
				vid_aspect = ASPECT_16_9;
			else if(strcmp(ratio, "5:4") == 0)
				vid_aspect = ASPECT_5_4;
			else
			{
				printf("Unknown aspect ratio %s!\n", ratio);
				hasError = true;
			}
			needRatio = false;
		}
		else IFARG("--bits")
		{
			if(++i >= argc)
			{
				printf("The bits option is missing the color depth argument!\n");
				hasError = true;
			}
			else
			{
				screenBits = atoi(argv[i]);
				switch(screenBits)
				{
					case 8:
					case 16:
					case 24:
					case 32:
						break;

					default:
						printf("Screen color depth must be 8, 16, 24, or 32!\n");
						hasError = true;
						break;
				}
			}
		}
		else IFARG("--noadaptive")
			noadaptive = true;
		else IFARG("--nodblbuf")
			usedoublebuffering = false;
		else IFARG("--extravbls")
		{
			if(++i >= argc)
			{
				printf("The extravbls option is missing the vbls argument!\n");
				hasError = true;
			}
			else
			{
				extravbls = atoi(argv[i]);
				if(extravbls < 0)
				{
					printf("Extravbls must be positive!\n");
					hasError = true;
				}
			}
		}
		else IFARG("--joystick")
		{
			if(++i >= argc)
			{
				printf("The joystick option is missing the index argument!\n");
				hasError = true;
			}
			else param_joystickindex = atoi(argv[i]);   // index is checked in InitGame
		}
		else IFARG("--joystickhat")
		{
			if(++i >= argc)
			{
				printf("The joystickhat option is missing the index argument!\n");
				hasError = true;
			}
			else param_joystickhat = atoi(argv[i]);
		}
		else IFARG("--samplerate")
		{
			if(++i >= argc)
			{
				printf("The samplerate option is missing the rate argument!\n");
				hasError = true;
			}
			else param_samplerate = atoi(argv[i]);
			sampleRateGiven = true;
		}
		else IFARG("--audiobuffer")
		{
			if(++i >= argc)
			{
				printf("The audiobuffer option is missing the size argument!\n");
				hasError = true;
			}
			else param_audiobuffer = atoi(argv[i]);
			audioBufferGiven = true;
		}
		else IFARG("--mission")
		{
			if(++i >= argc)
			{
				printf("The mission option is missing the mission argument!\n");
				hasError = true;
			}
			else param_mission = atoi(argv[i]);
		}
		else IFARG("--goodtimes")
			param_goodtimes = true;
		else IFARG("--ignorenumchunks")
			param_ignorenumchunks = true;
		else IFARG("--help")
			showHelp = true;
		else
			WL_AddFile(argv[i]);
	}
	if(hasError || showHelp)
	{
		if(hasError) printf("\n");
		printf(
			"Wolf4SDL v1.7\n"
			"Ported by Chaos-Software (http://www.chaos-software.de.vu)\n"
			"Original Wolfenstein 3D by id Software\n\n"
			"Usage: Wolf4SDL [options]\n"
			"Options:\n"
			" --help                 This help page\n"
			" --tedlevel <level>     Starts the game in the given level\n"
			" --baby                 Sets the difficulty to baby for tedlevel\n"
			" --easy                 Sets the difficulty to easy for tedlevel\n"
			" --normal               Sets the difficulty to normal for tedlevel\n"
			" --hard                 Sets the difficulty to hard for tedlevel\n"
			" --nowait               Skips intro screens\n"
			" --fullscreen           Starts the game in fullscreen mode\n"
			" --res <width> <height> Sets the screen resolution\n"
			"                        (must be multiple of 320x200 or 320x240)\n"
			" --resf <w> <h>         Sets any screen resolution >= 320x200\n"
			"                        (which may result in graphic errors)\n"
			" --aspect <aspect>      Sets the aspect ratio.\n"
			" --noadaptive           Disables adaptive tics.\n"
			" --bits <b>             Sets the screen color depth\n"
			"                        (use this when you have palette/fading problems\n"
			"                        allowed: 8, 16, 24, 32, default: \"best\" depth)\n"
			" --nodblbuf             Don't use SDL's double buffering\n"
			" --extravbls <vbls>     Sets a delay after each frame, which may help to\n"
			"                        reduce flickering (unit is currently 8 ms, default: 0)\n"
			" --joystick <index>     Use the index-th joystick if available\n"
			"                        (-1 to disable joystick, default: 0)\n"
			" --joystickhat <index>  Enables movement with the given coolie hat\n"
			" --samplerate <rate>    Sets the sound sample rate (given in Hz, default: %i)\n"
			" --audiobuffer <size>   Sets the size of the audio buffer (-> sound latency)\n"
#ifdef _arch_dreamcast
			"                        (given in bytes, default: 4096 / (44100 / samplerate))\n"
#else
			"                        (given in bytes, default: 2048 / (44100 / samplerate))\n"
#endif
			" --ignorenumchunks      Ignores the number of chunks in VGAHEAD.*\n"
			"                        (may be useful for some broken mods)\n"
#if defined(SPEAR) && !defined(SPEARDEMO)
			" --mission <mission>    Mission number to play (1-3)\n"
			" --goodtimes            Disable copy protection quiz\n"
#endif
			, defaultSampleRate
		);
		exit(1);
	}

	if(needRatio)
	{
		vid_aspect = CheckRatio(screenWidth, screenHeight);
	}

	if(sampleRateGiven && !audioBufferGiven)
#ifdef _arch_dreamcast
		param_audiobuffer = 4096 / (44100 / param_samplerate);
#else
		param_audiobuffer = 2048 / (44100 / param_samplerate);
#endif
}

#ifndef _WIN32
// I_MakeRNGSeed is from ZDoom
#include <time.h>

// Return a random seed, preferably one with lots of entropy.
unsigned int I_MakeRNGSeed()
{
	unsigned int seed;
	int file;

	// Try reading from /dev/urandom first, then /dev/random, then
	// if all else fails, use a crappy seed from time().
	seed = time(NULL);
	file = open("/dev/urandom", O_RDONLY);
	if (file < 0)
	{
		file = open("/dev/random", O_RDONLY);
	}
	if (file >= 0)
	{
		read(file, &seed, sizeof(seed));
		close(file);
	}
	return seed;
}
#else
unsigned int I_MakeRNGSeed();
#endif

/*
==========================
=
= main
=
==========================
*/

void InitThinkerList();
void ScannerMessageHandler(Scanner::MessageLevel level, const char *error, va_list list)
{
	FString errorMessage;
	errorMessage.VFormat(error, list);

	if(level == Scanner::ERROR)
		throw CRecoverableError(errorMessage);
	else
		vfprintf(stderr, error, list);
}

// Basically from ZDoom
// We are definting an atterm function so that we can control the exit behavior.
static const unsigned int MAX_TERMS = 32;
static void (*TermFuncs[MAX_TERMS])(void);
static unsigned int NumTerms;
void atterm(void (*func)(void))
{
	for(unsigned int i = 0;i < NumTerms;++i)
	{
		if(TermFuncs[i] == func)
			return;
	}

	if(NumTerms < MAX_TERMS)
		TermFuncs[NumTerms++] = func;
	else
		fprintf(stderr, "Failed to register atterm function!\n");
}
void CallTerminateFunctions()
{
	while(NumTerms > 0)
		TermFuncs[--NumTerms]();
}

int main (int argc, char *argv[])
{
	Scanner::SetMessageHandler(ScannerMessageHandler);
	atexit(CallTerminateFunctions);

	try
	{
		printf("ReadConfig: Reading the Configuration.\n");
		config->LocateConfigFile(argc, argv);
		ReadConfig();

		WL_AddFile("ecwolf.pk3");

		WL_AddFile("audiot.wl6");
		WL_AddFile("gamemaps.wl6");
		WL_AddFile("vgagraph.wl6");
		WL_AddFile("vswap.wl6");

#if defined(_arch_dreamcast)
		DC_Main();
		DC_CheckParameters();
#elif defined(GP2X)
		GP2X_Init();
#else
		CheckParameters(argc, argv);
#endif

		CheckForEpisodes();

		printf("W_Init: Init WADfiles.\n");
		Wads.InitMultipleFiles(wadfiles);
		language.SetupStrings();
		LumpRemapper::RemapAll();

		InitThinkerList();

		printf("VL_ReadPalette: Setting up the Palette...\n");
		VL_ReadPalette();
		InitPalette("WOLFPAL");
		R_InitColormaps();
		atterm(R_DeinitColormaps);
		printf("InitGame: Setting up the game...\n");
		InitGame();
		printf("CreateMenus: Preparing the menu system...\n");
		CreateMenus();

		rngseed = I_MakeRNGSeed();
		FRandom::StaticClearRandom();

		printf("DemoLoop: Starting the game loop...\n");
		DemoLoop();

		Quit("Demo loop exited???");
	}
	catch(class CDoomError &error)
	{
		if(error.GetMessage())
			fprintf(stderr, "%s\n", error.GetMessage());
		exit(-1);
	}
	return 1;
}
