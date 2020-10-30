#ifndef __WL_DRAW_H__
#define __WL_DRAW_H__

#include "tmemory.h"
#include <array>

/*
=============================================================================

							WL_DRAW DEFINITIONS

=============================================================================
*/

using TWallHeight = std::array<int,3>;

//
// math tables
//
extern  TUniquePtr<short[]> pixelangle;
extern  fixed finetangent[FINEANGLES/2 + ANG180];
extern	fixed finesine[FINEANGLES+FINEANGLES/4];
extern	fixed* finecosine;
extern  TUniquePtr<TWallHeight[]> wallheight;
extern  TUniquePtr<TWallHeight[]> skywallheight;
extern  word horizwall[],vertwall[];
extern  int32_t    frameon;
extern  int32_t    moveobj_frameon;
extern  int32_t    projectile_frameon;
extern	int r_extralight;

extern  unsigned screenloc[3];

extern  bool fizzlein, fpscounter;

extern  fixed   viewx,viewy;                    // the focal point
extern  fixed   viewsin,viewcos;

void    ThreeDRefresh (void);
int     WallMidY (int ywcount, int bot);
int     InvWallMidY(int y, int bot);

typedef struct
{
	word leftpix,rightpix;
	word dataofs[64];
// table data after dataofs[rightpix-leftpix+1]
} t_compshape;

extern bool UseWolf4SDL3DSpriteScaler;

#endif
