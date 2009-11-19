#ifndef __ID_CA__
#define __ID_CA__

//===========================================================================

#define NUMMAPS         60
#ifdef USE_FLOORCEILINGTEX
    #define MAPPLANES       3
#else
    #define MAPPLANES       2
#endif

#define UNCACHEAUDIOCHUNK(chunk) {if(audiosegs[chunk]) {free(audiosegs[chunk]); audiosegs[chunk]=NULL;}}

//===========================================================================

typedef struct
{
    int32_t planestart[3];
    word    planelength[3];
    word    width,height;
    char    name[16];
} maptype;

//===========================================================================

extern  int   mapon;

extern  word *mapsegs[MAPPLANES];

extern  char  extension[5];
extern  char  audioext[5];

//===========================================================================

boolean CA_LoadFile (const char *filename, memptr *ptr);
boolean CA_WriteFile (const char *filename, void *ptr, int32_t length);

int32_t CA_RLEWCompress (word *source, int32_t length, word *dest, word rlewtag);

void CA_RLEWexpand (word *source, word *dest, int32_t length, word rlewtag);

void CA_Startup (void);
void CA_Shutdown (void);

void CA_CacheMap (int mapnum);

void CA_CacheScreen (const char* chunk);

void CA_CannotOpen(const char *name);

#endif
