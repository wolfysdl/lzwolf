/*
** file_audiot.cpp
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

#include "filesys.h"
#include "doomerrors.h"
#include "wl_def.h"
#include "resourcefile.h"
#include "w_wad.h"
#include "lumpremap.h"
#include "zstring.h"

class FAudiot : public FUncompressedFile
{
	public:
		FAudiot(const char* filename, FileReader *file) : FUncompressedFile(filename, file)
		{
			FString path(filename);
			int lastSlash = path.LastIndexOfAny("/\\");
			extension = path.Mid(lastSlash+8);
			path = path.Left(lastSlash+1);

			File directory(path.Len() > 0 ? path : ".");
			FString audiohedFile = path + directory.getInsensitiveFile(FString("audiohed.") + extension, true);

			audiohedReader = new FileReader();
			if(!audiohedReader->Open(audiohedFile))
			{
				delete audiohedReader;
				audiohedReader = NULL;

				FString error;
				error.Format("Could not open audiot since %s is missing.", audiohedFile.GetChars());
				throw CRecoverableError(error);
			}
		}

		FAudiot(const char* filename, FileReader **file) : FUncompressedFile(filename, file[0])
		{
			FString path(filename);
			int lastSlash = path.LastIndexOf(':');
			extension = path.Mid(lastSlash+8);

			audiohedReader = file[1];
		}

		~FAudiot()
		{
			delete audiohedReader;
		}

		bool Open(bool quiet)
		{
			unsigned int segstart[4] = {0};
			unsigned int curseg = 0;

			audiohedReader->Seek(0, SEEK_END);
			NumLumps = (audiohedReader->Tell()/4)-1;
			audiohedReader->Seek(0, SEEK_SET);
			Lumps = new FUncompressedLump[NumLumps];
			DWORD* positions = new DWORD[NumLumps+1];
			audiohedReader->Read(positions, (NumLumps+1)*4);

			positions[0] = LittleLong(positions[0]);
			for(unsigned int i = 0;i < NumLumps;++i)
			{
				positions[i+1] = LittleLong(positions[i+1]);
				DWORD size = positions[i+1] - positions[i];

				char name[9];
				sprintf(name, "AUD%05d", i);
				Lumps[i].Owner = this;
				Lumps[i].LumpNameSetup(name);
				Lumps[i].Position = positions[i];
				Lumps[i].LumpSize = size;
				Lumps[i].Namespace = curseg != 3 ? ns_sounds : ns_music;

				// Try to find !ID! tags
				if(curseg < 3 && size >= 4)
				{
					char tag[4];
					Reader->Seek(positions[i]+size-4, SEEK_SET);
					Reader->Read(tag, 4);
					if(strncmp("!ID!", tag, 4) == 0)
					{
						segstart[++curseg] = i;
						Lumps[i].LumpSize -= 4;
					}
				}
			}

			// If we didn't find enough !ID! tags, lets try another method of
			// counting.  Since digitized chunks were not used, find the first
			// empty chunk.
			if(curseg != 4)
			{
				for(int i = NumLumps-1;i >= 0;i -= 3)
				{
					if(Lumps[i].LumpSize <= 4)
					{
						for(++i;i < static_cast<int>(NumLumps);++i)
							Lumps[i].Namespace = ns_music;
						break;
					}
				}
			}

			delete[] positions;

			if(!quiet)
			{
				Printf(", %d lumps\n", NumLumps);

				// Check that there are the same number of each sound type.
				// Technically the format can contain differing numbers, but
				// the engine may not like that, so issue a warning.
				if(curseg == 4)
				{
					DWORD numsounds = segstart[1] - segstart[0];
					if(segstart[2] - segstart[1] != numsounds || segstart[3] - segstart[2] != numsounds)
						Printf("Warning: AUDIOT doesn't contain the same number of AdLib, PC Speaker, and Digitized sound chunks.\n");
				}
			}

			LumpRemapper::AddFile(extension, this, LumpRemapper::AUDIOT);
			return true;
		}

	private:
		FString		extension;
		FileReader	*audiohedReader;
};

FResourceFile *CheckAudiot(const char *filename, FileReader *file, bool quiet)
{
	FString fname(filename);
	int embeddedSep = fname.LastIndexOf(':');
#ifdef _WIN32
	if(embeddedSep == 1) embeddedSep = -1;
#endif
	int lastSlash = MAX<long>(fname.LastIndexOfAny("/\\"), embeddedSep);
	if(lastSlash != -1)
		fname = fname.Mid(lastSlash+1, 6);
	else
		fname = fname.Left(6);

	if(fname.Len() == 6 && fname.CompareNoCase("audiot") == 0) // file must be audiot.something
	{
		FResourceFile *rf = embeddedSep == -1 ? new FAudiot(filename, file) :
			new FAudiot(filename, reinterpret_cast<FileReader**>(file)); // HACK

		if(rf->Open(quiet)) return rf;
		rf->Reader = NULL; // to avoid destruction of reader
		delete rf;
	}
	return NULL;
}
