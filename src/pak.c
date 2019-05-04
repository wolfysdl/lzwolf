/***************************************************************************
                          pak.c  -  description
                             -------------------
    begin                : Fre Feb 21 17:42:09 CET 2003
    copyright            : (C) 2003-2004 by Dominik Jall
    email                : hackebeil20@web.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "pak.h"

#define VERSION 0.3

int checkPak(pakfile *fp)
{
  if( fp->header.head[0] != 'P' || fp->header.head[1] != 'A' || fp->header.head[2] != 'C' || fp->header.head[3] != 'K')
    return -1;  //ident corrupt

  if( fp->header.dir_offset < (sizeof(fp->header.head) + 1) || fp->header.dir_length < 1)
    return -2;  //header corrupt

    
  return 0;
}


pakfile *openPak(const char* filename)
{
    pakfile *mypak;
    FILE *fp;
    
    fp = fopen(filename, "rb");
    
    mypak = (pakfile *)malloc(sizeof(pakfile));
    mypak->handle = fp;
  
    if(mypak->handle == NULL)
    {
        free(mypak);
        return NULL;
    }

    fread(&mypak->header, sizeof(pakheader), 1, mypak->handle);
    
    return mypak;
}


int closePak(pakfile *fp)
{
    if (fp)
    {
        fclose(fp->handle);
        free(fp);
    }
    return 0;
}

dirsection *inPak(const char *file, pakfile *fp)
{
    int x =0;

    dirsection *files = loadDirSection(fp, &x);
            
    //if (strcmp(file, "sounds/ATKKAR98SND_LGY.wav") == 0)
    //    printf("*%d\n", x);
    //now check if the file exists
    int i;
    for( i = 0; i < x; i++)
    {
        char *name = files[i].filename;
        unsigned int position = files[i].file_position;
        unsigned int length = files[i].file_length;
    
        //if (strcmp(file, "sounds/ATKKAR98SND_LGY.wav") == 0)
        //    printf("[%d] %s\n", i, name);
        if( strcmp(name, file) == 0 )
        {
            dirsection *sect = (dirsection *)malloc(
                sizeof(dirsection));
            memset(sect, 0, sizeof(dirsection));
            strcpy(sect->filename, name);
            sect->file_position = position;
            sect->file_length = length;
            free(files);
            return sect;
        }
    }

    free(files);
    return NULL; // does not exist
}


dirsection *loadDirSection(pakfile *fp, int *x)
{
    dirsection *files;
         
    //see how many files are in the pak
    *x = fp->header.dir_length / 64;  //guaranteed to be even

    //allocate space for all the files including one new (for possible insertion)
    files = (dirsection *)malloc(*x * sizeof(dirsection));

    //read all the dirsections into 'files'
    fseek(fp->handle, fp->header.dir_offset, SEEK_SET);  //go to directory offset

    fread(files, sizeof(dirsection), *x, fp->handle);

    return files;
}
  
#ifdef USE_SDL
SDL_Surface *loadBMP(char* file, pakfile *fp)
{
	unsigned int size;
	SDL_RWops *rw;
	SDL_Surface *plane;
	// this function uses the undocumented SDL_RWops-feature that allows
	// loading images from almost any source, like memorypointers, pipes etc.

	//first, read the file within the pak into a buffer
	byte *buffer = extractMem(file, fp, &size);
	
	if(buffer == NULL)
		return NULL;
	
	//now that we have the file as memory, we need to make a SDL_RWop-struct from it
	rw = SDL_RWFromMem(buffer, size);
	
	//now load the BMP in an ordinary way
	plane = SDL_LoadBMP_RW(rw, 0);
		
	//at last, free all unnecessary stuff
	free(buffer);
	SDL_FreeRW(rw);
	
	return plane;
}

Mix_Chunk *loadWAV(char* file, pakfile *fp)
{
	unsigned int size;
	SDL_RWops *rw;
	Mix_Chunk *chunk;
	// this function uses the undocumented SDL_RWops-feature that allows
	// loading chunks from almost any source, like memorypointers, pipes etc.

	//first, read the file within the pak into a buffer
	byte *buffer = extractMem(file, fp, &size);
	
	if(buffer == NULL)
    {
        //printf("failed wav %s\n", file);
		return NULL;
    }
	
	//now that we have the file as memory, we need to make a SDL_RWop-struct from it
	rw = SDL_RWFromMem(buffer, size);
	
	//now load the WAV in an ordinary way
	chunk = Mix_LoadWAV_RW(rw, 0);
		
	//at last, free all unnecessary stuff
	free(buffer);
	SDL_FreeRW(rw);
	
	return chunk;
}

byte *extractMem(char *file, pakfile* fp, unsigned int *size)
{
    if(fp == NULL)
        return NULL;

    //test if the file exists in the pak
    dirsection *mysect = inPak(file, fp);

    if( mysect == NULL )
        return NULL;   //not found

    //now go to the address in the pak
    fseek(fp->handle, mysect->file_position, SEEK_SET);
        
    byte *buffer = (byte *)malloc(mysect->file_length * sizeof(byte));
    
    *size = mysect->file_length;
    
    //now read the data into the buffer
    fread(buffer, sizeof(byte), mysect->file_length, fp->handle);

    free(mysect);

    return buffer;
}        
#endif

FILE *pakSeekTo(const char *file, pakfile *fp)
{
    //test if the file exists in the pak
    dirsection *mysect = inPak(file, fp);

    if( mysect == NULL )
        return NULL;   //not found
  
    //now go to the address in the pak
    fseek(fp->handle, mysect->file_position, SEEK_SET);

    free(mysect);

    return fp->handle;
}
