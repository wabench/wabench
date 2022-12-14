/***************************************************************************
 *   Copyright (C) 2005,2006 by Jonathan Duddington                        *
 *   jsd@clara.co.uk                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// conditional compilation options

#ifdef __WIN32__
#define PATHSEP '\\'
#else
#define PLATFORM_POSIX
#define __cdecl               // define as null, needed for Borland compiler ?
#define PATHSEP  '/'          // unix style file paths
//#define USE_PORTAUDIO
#define USE_NANOSLEEP
#endif

// don't use portaudio in the library version
#ifdef LIBRARY
#undef USE_PORTAUDIO
#endif

// will look for espeak_data directory here, and also in user's home directory
#ifndef PATH_ESPEAK_DATA
   #define PATH_ESPEAK_DATA  "/usr/share/espeak-data"
#endif

typedef unsigned short USHORT;
typedef unsigned char  UCHAR;
typedef double DOUBLEX;



#define N_PEAKS   9
#define N_MARKERS 7


typedef struct {
   const char *mnem;
   int  value;
} MNEM_TAB;
int LookupMnem(MNEM_TAB *table, char *string);


typedef struct {
   short pkfreq;
   short pkheight;
   short pkwidth;
   short pkright;
}  peak_t;

typedef struct {
	short flags;
	unsigned char length;
	unsigned char rms;
	short ffreq[9];
	unsigned char fheight[9];
	unsigned char fwidth[6];          // width/4
	unsigned char fright[6];          // width/4
} frame_t;


int  GetFileLength(const char *filename);
char *Alloc(int size);
void Free(void *ptr);


extern void strncpy0(char *to,const char *from, int size);
extern char path_home[];
