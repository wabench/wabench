/***************************************************************************
 *   Copyright (C) 2005, 2006 by Jonathan Duddington                       *
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
#include "StdAfx.h"

// this version keeps wavemult window as a constant fraction
// of the cycle length - but that spreads out the HF peaks too much

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


#include "speech.h"
#include "voice.h"
#include "phoneme.h"
#include "synthesize.h"

#ifdef USE_PORTAUDIO
#include "portaudio.h"
#undef USE_PORTAUDIO
// determine portaudio version by looking for a #define which is not in V18
#ifdef paNeverDropInput
#define USE_PORTAUDIO   19
#else
#define USE_PORTAUDIO   18
#endif
#endif

#define N_SINTAB  2048
#include "sintab.h"



#define PI2 6.283185307
#define STEPSIZE  64                // 2.9mS at 22 kHz sample rate
#define N_WAV_BUF   15

static voice_t *wvoice;

FILE *f_log = NULL;
int option_waveout = 0;
int option_harmonic1 = 11;   // 10
static int flutter_amp = 64;

static int general_amplitude = 60;
static int consonant_amp = 26;   // 24

int embedded_value[N_EMBEDDED_VALUES];

static int PHASE_INC_FACTOR;
int samplerate = 0;       // this is set by Wavegeninit()

static wavegen_peaks_t peaks[N_PEAKS];
static int peak_harmonic[N_PEAKS];
static int peak_height[N_PEAKS];

#define N_ECHO_BUF 5500   // max of 250mS at 22050 Hz
static int echo_head;
static int echo_tail;
static int echo_amp = 0;
static short echo_buf[N_ECHO_BUF];

#define N_TONE_ADJUST  1250
static unsigned char tone_adjust[N_TONE_ADJUST];   //  8Hz steps * 1250 = 10kHz
static int harm_sqrt_n = 0;


#define N_LOWHARM  30
static int harm_inc[N_LOWHARM];    // only for these harmonics do we interpolate amplitude between steps
static int *harmspect;
static int hswitch=0;
static int hspect[2][MAX_HARMONIC];         // 2 copies, we interpolate between then
static int max_hval=0;

static int nsamples=0;       // number to do
static int amplitude = 32;
static int amplitude_v = 0;
static int modulation_type = 0;
static int glottal_flag = 0;
static int glottal_reduce = 0;

static unsigned char *mix_wavefile = NULL;  // wave file to be added to synthesis
static int n_mix_wavefile = 0;       // length in bytes
static int mix_wave_scale = 0;         // 0=2 byte samples
static int mix_wave_amp = 32;
static int mix_wavefile_ix = 0;

static int pitch;          // pitch Hz*256
static int pitch_ix;       // index into pitch envelope (*256)
static int pitch_inc;      // increment to pitch_ix
static unsigned char *pitch_env;
static int pitch_base;     // Hz*256 low, before modified by envelope
static int pitch_range;    // Hz*256 range of envelope
static int amp_ix;
static int amp_inc;
static unsigned char *amplitude_env = NULL;

static int samplecount=0;    // number done
static int samplecount_start=0;  // count at start of this segment
static int end_wave=0;      // continue to end of wave cycle
static int wavephase;
static int phaseinc;
static int cycle_samples;         // number of samples in a cycle at current pitch
static int cbytes;
static int hf_factor;


unsigned char *out_ptr;
unsigned char *out_end;
static int out_channels=1;
static unsigned char outbuf[1024];  // used when writing to file

// the queue of operations passed to wavegen from sythesize
long wcmdq[N_WCMDQ][4];
int wcmdq_head=0;
int wcmdq_tail=0;

// pitch,speed,
int embedded_default[N_EMBEDDED_VALUES]        = {0,50,165,100,50, 0,50, 0,165,0,0,0,0,0};
static int embedded_max[N_EMBEDDED_VALUES]     = {0,99,320,300,99,99,99, 0,320,0,0,0,0,4};

#define N_CALLBACK_IX N_WAV_BUF-2   // adjust this delay to match display with the currently spoken word
int current_source_index=0;

FILE *f_wave = NULL;

#if (USE_PORTAUDIO == 18)
static PortAudioStream *pa_stream=NULL;
#endif
#if (USE_PORTAUDIO == 19)
static PaStream *pa_stream=NULL;
#endif

/* default pitch envelope, a steady fall */
#define ENV_LEN  128


/*
unsigned char Pitch_env0[ENV_LEN] = {
    255,253,251,249,247,245,243,241,239,237,235,233,231,229,227,225,
    223,221,219,217,215,213,211,209,207,205,203,201,199,197,195,193,
    191,189,187,185,183,181,179,177,175,173,171,169,167,165,163,161,
    159,157,155,153,151,149,147,145,143,141,139,137,135,133,131,129,
    127,125,123,121,119,117,115,113,111,109,107,105,103,101, 99, 97,
     95, 93, 91, 89, 87, 85, 83, 81, 79, 77, 75, 73, 71, 69, 67, 65,
     63, 61, 59, 57, 55, 53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 33,
     31, 29, 27, 25, 23, 21, 19, 17, 15, 13, 11,  9,  7,  5,  3,  1
};
*/

/*
unsigned char Pitch_long[ENV_LEN] = {
	254,249,250,251,252,253,254,254, 255,255,255,255,254,254,253,252,
	251,250,249,247,244,242,238,234, 230,225,221,217,213,209,206,203,
	199,195,191,187,183,179,175,172, 168,165,162,159,156,153,150,148,
	145,143,140,138,136,134,132,130, 128,126,123,120,117,114,111,107,
	104,100,96,91, 86,82,77,73, 70,66,63,60, 58,55,53,51,
	49,47,46,45, 43,42,40,38, 36,34,31,28, 26,24,22,20,
	18,16,14,12, 11,10,9,8, 8,8,8,8, 9,8,8,8,
	8,8,7,7, 6,6,6,5, 4,4,3,3, 2,1,1,0
};
*/

// 1st index=roughness
// 2nd index=modulation_type
// value: bits 0-3  amplitude (16ths), bits 4-7 every n cycles
#define N_ROUGHNESS 8
static unsigned char modulation_tab[N_ROUGHNESS][8] = {
	{0, 0x00, 0x00, 0x00, 0, 0x35, 0xf2, 0x29},
	{0, 0x2f, 0x00, 0x2f, 0, 0x34, 0xf2, 0x29},
	{0, 0x2f, 0x00, 0x2e, 0, 0x33, 0xf2, 0x28},
	{0, 0x2e, 0x00, 0x2d, 0, 0x33, 0xf2, 0x28},
	{0, 0x2d, 0x2d, 0x2c, 0, 0x33, 0xf2, 0x28},
	{0, 0x2b, 0x2b, 0x2b, 0, 0x33, 0xf2, 0x28},
	{0, 0x2a, 0x2a, 0x2a, 0, 0x33, 0xf2, 0x28},
	{0, 0x29, 0x29, 0x29, 0, 0x33, 0xf2, 0x28},
};

// Flutter table, to add natural variations to the pitch
#define N_FLUTTER  0x170
static int Flutter_inc;
static const unsigned char Flutter_tab[N_FLUTTER] = {
   0x80, 0x9b, 0xb5, 0xcb, 0xdc, 0xe8, 0xed, 0xec,
   0xe6, 0xdc, 0xce, 0xbf, 0xb0, 0xa3, 0x98, 0x90,
   0x8c, 0x8b, 0x8c, 0x8f, 0x92, 0x94, 0x95, 0x92,
   0x8c, 0x83, 0x78, 0x69, 0x59, 0x49, 0x3c, 0x31,
   0x2a, 0x29, 0x2d, 0x36, 0x44, 0x56, 0x69, 0x7d,
   0x8f, 0x9f, 0xaa, 0xb1, 0xb2, 0xad, 0xa4, 0x96,
   0x87, 0x78, 0x69, 0x5c, 0x53, 0x4f, 0x4f, 0x55,
   0x5e, 0x6b, 0x7a, 0x88, 0x96, 0xa2, 0xab, 0xb0,

   0xb1, 0xae, 0xa8, 0xa0, 0x98, 0x91, 0x8b, 0x88,
   0x89, 0x8d, 0x94, 0x9d, 0xa8, 0xb2, 0xbb, 0xc0,
   0xc1, 0xbd, 0xb4, 0xa5, 0x92, 0x7c, 0x63, 0x4a,
   0x32, 0x1e, 0x0e, 0x05, 0x02, 0x05, 0x0f, 0x1e,
   0x30, 0x44, 0x59, 0x6d, 0x7f, 0x8c, 0x96, 0x9c,
   0x9f, 0x9f, 0x9d, 0x9b, 0x99, 0x99, 0x9c, 0xa1,
   0xa9, 0xb3, 0xbf, 0xca, 0xd5, 0xdc, 0xe0, 0xde,
   0xd8, 0xcc, 0xbb, 0xa6, 0x8f, 0x77, 0x60, 0x4b,

   0x3a, 0x2e, 0x28, 0x29, 0x2f, 0x3a, 0x48, 0x59,
   0x6a, 0x7a, 0x86, 0x90, 0x94, 0x95, 0x91, 0x89,
   0x80, 0x75, 0x6b, 0x62, 0x5c, 0x5a, 0x5c, 0x61,
   0x69, 0x74, 0x80, 0x8a, 0x94, 0x9a, 0x9e, 0x9d,
   0x98, 0x90, 0x86, 0x7c, 0x71, 0x68, 0x62, 0x60,
   0x63, 0x6b, 0x78, 0x88, 0x9b, 0xaf, 0xc2, 0xd2,
   0xdf, 0xe6, 0xe7, 0xe2, 0xd7, 0xc6, 0xb2, 0x9c,
   0x84, 0x6f, 0x5b, 0x4b, 0x40, 0x39, 0x37, 0x38,

   0x3d, 0x43, 0x4a, 0x50, 0x54, 0x56, 0x55, 0x52,
   0x4d, 0x48, 0x42, 0x3f, 0x3e, 0x41, 0x49, 0x56,
   0x67, 0x7c, 0x93, 0xab, 0xc3, 0xd9, 0xea, 0xf6,
   0xfc, 0xfb, 0xf4, 0xe7, 0xd5, 0xc0, 0xaa, 0x94,
   0x80, 0x71, 0x64, 0x5d, 0x5a, 0x5c, 0x61, 0x68,
   0x70, 0x77, 0x7d, 0x7f, 0x7f, 0x7b, 0x74, 0x6b,
   0x61, 0x57, 0x4e, 0x48, 0x46, 0x48, 0x4e, 0x59,
   0x66, 0x75, 0x84, 0x93, 0x9f, 0xa7, 0xab, 0xaa,

   0xa4, 0x99, 0x8b, 0x7b, 0x6a, 0x5b, 0x4e, 0x46,
   0x43, 0x45, 0x4d, 0x5a, 0x6b, 0x7f, 0x92, 0xa6,
   0xb8, 0xc5, 0xcf, 0xd3, 0xd2, 0xcd, 0xc4, 0xb9,
   0xad, 0xa1, 0x96, 0x8e, 0x89, 0x87, 0x87, 0x8a,
   0x8d, 0x91, 0x92, 0x91, 0x8c, 0x84, 0x78, 0x68,
   0x55, 0x41, 0x2e, 0x1c, 0x0e, 0x05, 0x01, 0x05,
   0x0f, 0x1f, 0x34, 0x4d, 0x68, 0x81, 0x9a, 0xb0,
   0xc1, 0xcd, 0xd3, 0xd3, 0xd0, 0xc8, 0xbf, 0xb5,

   0xab, 0xa4, 0x9f, 0x9c, 0x9d, 0xa0, 0xa5, 0xaa,
   0xae, 0xb1, 0xb0, 0xab, 0xa3, 0x96, 0x87, 0x76,
   0x63, 0x51, 0x42, 0x36, 0x2f, 0x2d, 0x31, 0x3a,
   0x48, 0x59, 0x6b, 0x7e, 0x8e, 0x9c, 0xa6, 0xaa,
   0xa9, 0xa3, 0x98, 0x8a, 0x7b, 0x6c, 0x5d, 0x52,
   0x4a, 0x48, 0x4a, 0x50, 0x5a, 0x67, 0x75, 0x82
};

// waveform shape table for HF peaks, formants 6,7,8
#define N_WAVEMULT 512
static int wavemult_offset=0;
static int wavemult_max=0;

// the presets are for 22050 Hz sample rate.
// A different rate will need to recalculate the presets in WavegenInit()
static unsigned char wavemult[N_WAVEMULT] = {
  0,  0,  0,  2,  3,  5,  8, 11, 14, 18, 22, 27, 32, 37, 43, 49,
    55, 62, 69, 76, 83, 90, 98,105,113,121,128,136,144,152,159,166,
   174,181,188,194,201,207,213,218,224,228,233,237,240,244,246,249,
   251,252,253,253,253,253,252,251,249,246,244,240,237,233,228,224,
   218,213,207,201,194,188,181,174,166,159,152,144,136,128,121,113,
   105, 98, 90, 83, 76, 69, 62, 55, 49, 43, 37, 32, 27, 22, 18, 14,
    11,  8,  5,  3,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 };
   

// set from y = pow(2,x) * 128,  x=-1 to 1
unsigned char pitch_adjust_tab[100] = {
    64, 65, 66, 67, 68, 69, 70, 71,
    72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 86, 87, 88,
    89, 91, 92, 93, 94, 96, 97, 98,
   100,101,103,104,105,107,108,110,
   111,113,115,116,118,119,121,123,
   124,126,128,130,132,133,135,137,
   139,141,143,145,147,149,151,153,
   155,158,160,162,164,167,169,171,
   174,176,179,181,184,186,189,191,
   194,197,199,202,205,208,211,214,
   217,220,223,226,229,232,236,239,
   242,246,249,252 };

int WavegenFill(int fill_zeros);


void WcmdqStop()
{//=============
	wcmdq_head = 0;
	wcmdq_tail = 0;
#ifdef USE_PORTAUDIO
	Pa_AbortStream(pa_stream);
#endif
}


int WcmdqFree()
{//============
	int i;
	i = wcmdq_head - wcmdq_tail;
	if(i <= 0) i += N_WCMDQ;
	return(i);
}

int WcmdqUsed()
{//============
   return(N_WCMDQ - WcmdqFree());
}


void WcmdqInc()
{//============
	wcmdq_tail++;
	if(wcmdq_tail >= N_WCMDQ) wcmdq_tail=0;
}

static void WcmdqIncHead()
{//=======================
	wcmdq_head++;
	if(wcmdq_head >= N_WCMDQ) wcmdq_head=0;
}



// data points from which to make the presets for pk_shape1 and pk_shape2
#define PEAKSHAPEW 256
static const float pk_shape_x[2][8] = {
	{0,-0.6, 0.0, 0.6, 1.4, 2.5, 4.5, 5.5},
	{0,-0.6, 0.0, 0.6, 1.4, 2.0, 4.5, 5.5 }};
static const float pk_shape_y[2][8] = {
	{0,  67,  81,  67,  31,  14,   0,  -6} ,
	{0,  77,  81,  77,  31,   7,   0,  -6 }};

unsigned char pk_shape1[PEAKSHAPEW+1] = {
   255,254,254,254,254,254,253,253,252,251,251,250,249,248,247,246,
   245,244,242,241,239,238,236,234,233,231,229,227,225,223,220,218,
   216,213,211,209,207,205,203,201,199,197,195,193,191,189,187,185,
   183,180,178,176,173,171,169,166,164,161,159,156,154,151,148,146,
   143,140,138,135,132,129,126,123,120,118,115,112,108,105,102, 99,
    96, 95, 93, 91, 90, 88, 86, 85, 83, 82, 80, 79, 77, 76, 74, 73,
    72, 70, 69, 68, 67, 66, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55,
    55, 54, 53, 52, 52, 51, 50, 50, 49, 48, 48, 47, 47, 46, 46, 46,
    45, 45, 45, 44, 44, 44, 44, 44, 44, 44, 43, 43, 43, 43, 44, 43,
    42, 42, 41, 40, 40, 39, 38, 38, 37, 36, 36, 35, 35, 34, 33, 33,
    32, 32, 31, 30, 30, 29, 29, 28, 28, 27, 26, 26, 25, 25, 24, 24,
    23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 18, 17, 17, 16,
    16, 15, 15, 15, 14, 14, 13, 13, 13, 12, 12, 11, 11, 11, 10, 10,
    10,  9,  9,  9,  8,  8,  8,  7,  7,  7,  7,  6,  6,  6,  5,  5,
     5,  5,  4,  4,  4,  4,  4,  3,  3,  3,  3,  2,  2,  2,  2,  2,
     2,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0 };

static unsigned char pk_shape2[PEAKSHAPEW+1] = {
   255,254,254,254,254,254,254,254,254,254,253,253,253,253,252,252,
   252,251,251,251,250,250,249,249,248,248,247,247,246,245,245,244,
   243,243,242,241,239,237,235,233,231,229,227,225,223,221,218,216,
   213,211,208,205,203,200,197,194,191,187,184,181,178,174,171,167,
   163,160,156,152,148,144,140,136,132,127,123,119,114,110,105,100,
    96, 94, 91, 88, 86, 83, 81, 78, 76, 74, 71, 69, 66, 64, 62, 60,
    57, 55, 53, 51, 49, 47, 44, 42, 40, 38, 36, 34, 32, 30, 29, 27,
    25, 23, 21, 19, 18, 16, 14, 12, 11,  9,  7,  6,  4,  3,  1,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0 };

static unsigned char *pk_shape;


static void WavegenInitPkData(int which)
{//=====================================
// this is only needed to set up the presets for pk_shape1 and pk_shape2
// These have already been pre-calculated and preset
#ifdef deleted
	int ix;
	int p;
	float x;
	float y[PEAKSHAPEW];
	float maxy=0;

	if(which==0)
		pk_shape = pk_shape1;
	else
		pk_shape = pk_shape2;

	p = 0;
	for(ix=0;ix<PEAKSHAPEW;ix++)
	{
		x = (4.5*ix)/PEAKSHAPEW;
		if(x >= pk_shape_x[which][p+3]) p++;
		y[ix] = polint(&pk_shape_x[which][p],&pk_shape_y[which][p],3,x);
		if(y[ix] > maxy) maxy = y[ix];
	}
	for(ix=0;ix<PEAKSHAPEW;ix++)
	{
		p = (int)(y[ix]*255/maxy);
      pk_shape[ix] = (p >= 0) ? p : 0;
	}
	pk_shape[PEAKSHAPEW]=0;
#endif
}  //  end of WavegenInitPkData



#ifdef USE_PORTAUDIO
// PortAudio interface

static int userdata[4];
static PaError pa_init_err=0;

#if USE_PORTAUDIO == 18
static int WaveCallback(void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer, PaTimestamp outTime, void *userData )
#else
static int WaveCallback(const void *inputBuffer, void *outputBuffer,
		long unsigned int framesPerBuffer, const PaStreamCallbackTimeInfo *outTime,
		PaStreamCallbackFlags flags, void *userData )
#endif
{
	int ix;
	int result;
	unsigned char *p;

	out_ptr = (unsigned char *)outputBuffer;
	out_end = out_ptr + framesPerBuffer*2;

	result = WavegenFill(1);

	if(out_channels == 2)
	{
		// sound output can only do stereo, not mono.  Duplicate each sound sample to
		// produce 2 channels.
		out_ptr = (unsigned char *)outputBuffer;
		for(ix=framesPerBuffer-1; ix>=0; ix--)
		{
			p = &out_ptr[ix*4];
			p[3] = p[1] = out_ptr[ix*2 + 1];
			p[2] = p[0] = out_ptr[ix*2];
		}
	}

#if USE_PORTAUDIO == 18
	if(result != 0)
	{
		static int end_timer = 0;
		if(end_timer == 0)
			end_timer = 4;
		if(end_timer > 0)
		{
			end_timer--;
			if(end_timer == 0)
				return(1);
		}
	}
	return(0);
#else
	return(result);
#endif

}  //  end of WaveCallBack


int WavegenOpenSound()
{//===================
	PaError err, err2;
	PaError active;

	if(option_waveout)
	{
		// writing to WAV file, not to portaudio
		return(0);
	}

#if USE_PORTAUDIO == 18
	active = Pa_StreamActive(pa_stream);
#else
	active = Pa_IsStreamActive(pa_stream);
#endif

	if(active == 1)
		return(0);
	if(active < 0)
	{
		out_channels = 1;

#if USE_PORTAUDIO == 18
		err2 = Pa_OpenDefaultStream(&pa_stream,0,1,paInt16,samplerate,512,N_WAV_BUF,WaveCallback,(void *)userdata);

		if(err2 == paInvalidChannelCount)
		{
			// failed to open with mono, try stereo
			out_channels=2;
			err2 = Pa_OpenDefaultStream(&pa_stream,0,2,paInt16,samplerate,512,N_WAV_BUF,WaveCallback,(void *)userdata);
		}
#else
		err2 = Pa_OpenDefaultStream(&pa_stream,0,1,paInt16,(double)samplerate,512,WaveCallback,(void *)userdata);

		if(err2 == paInvalidChannelCount)
		{
			// failed to open with mono, try stereo
			out_channels=2;
			err2 = Pa_OpenDefaultStream(&pa_stream,0,2,paInt16,(double)samplerate,512,WaveCallback,(void *)userdata);
		}
#endif
	}
	err = Pa_StartStream(pa_stream);

#if USE_PORTAUDIO == 19
	if(err == paStreamIsNotStopped)
	{
		// not sure why we need this, but PA v19 seems to need it
		err = Pa_StopStream(pa_stream);
		err = Pa_StartStream(pa_stream);
	}
#endif

	if(err != paNoError)
	{
		// exit speak if we can't open the sound device - this is OK if speak is being run for each utterance
		exit(2);
	}

	return(0);
}


int WavegenCloseSound()
{//====================
	PaError active;

	// check whether speaking has finished, and close the stream
	if(pa_stream != NULL)
	{
#if USE_PORTAUDIO == 18
		active = Pa_StreamActive(pa_stream);
#else
		active = Pa_IsStreamActive(pa_stream);
#endif
		if(active == 0)
		{
			Pa_CloseStream(pa_stream);
			pa_stream = NULL;
			return(1);
		}
	}
	return(0);
}


int WavegenInitSound()
{//===================
	PaError err;

	// PortAudio sound output library
	err = Pa_Initialize();
	pa_init_err = err;
	if(err != paNoError)
	{
		fprintf(stderr,"Failed to initialise the PortAudio sound\n");
		return(1);
	}
	return(0);
}
#else
int WavegenOpenSound()
{//===================
	return(0);
}
int WavegenCloseSound()
{//====================
	return(0);
}
int WavegenInitSound()
{//===================
	return(0);
}
#endif


void WavegenInit(int rate, int wavemult_fact)
{//==========================================
	int  ix;
	double x;

	if(wavemult_fact == 0)
		wavemult_fact=60;  // default

	samplerate = rate;
	PHASE_INC_FACTOR = 0x8000000 / samplerate;   // assumes pitch is Hz*32
	Flutter_inc = (64 * samplerate)/rate;
	samplecount = 0;
	nsamples = 0;
	wavephase = 0x7fffffff;
	max_hval = 0;

	for(ix=0; ix<N_EMBEDDED_VALUES; ix++)
		embedded_value[ix] = embedded_default[ix];

	if(rate <= 11000)
	{
		// adjust some parameters for telephony with low sample rates
		consonant_amp = 50;
		option_harmonic1 = 6;
	}

	// set up window to generate a spread of harmonics from a
	// single peak for HF peaks
	wavemult_max = (samplerate * wavemult_fact)/(256 * 50);
	if(wavemult_max > N_WAVEMULT) wavemult_max = N_WAVEMULT;

	wavemult_offset = wavemult_max/2;

	if(samplerate != 22050)
	{
		// wavemult table has preset values for 22050 Hz, we only need to
		// recalculate them if we have a different sample rate
		for(ix=0; ix<wavemult_max; ix++)
		{
			x = 127*(1.0 - cos(PI2*ix/wavemult_max));
			wavemult[ix] = (int)x;
		}
	}

	// This table provides the opportunity for tone control.
	// Adjustment of harmonic amplitudes, steps of 8Hz
	// value of 128 means no change
	for(ix=0; ix<N_TONE_ADJUST; ix++)
	{
		tone_adjust[ix] = 128;
	}

	WavegenInitPkData(1);
	WavegenInitPkData(0);
	pk_shape = pk_shape2;         // ph_shape2

#ifdef LOG_FRAMES
remove("log");
#endif
}  // end of WavegenInit


static int GetAmplitude(void)
{//==========================
	int amp;

	// normal, none, reduced, moderate, strong
	static const unsigned char amp_emphasis[5] = {16, 16, 8, 24, 32};

	amp = (embedded_value[EMBED_A])*60/100;
	return(amp * amp_emphasis[embedded_value[EMBED_F]] / 16);
}


static void WavegenSetEcho(void)
{//=============================
	int delay;
	int amp;

	delay = wvoice->echo_delay;
	amp = wvoice->echo_amp;

	if(delay >= N_ECHO_BUF)
		delay = N_ECHO_BUF-1;
	if(amp > 100)
		amp = 100;

	memset(echo_buf,0,sizeof(echo_buf));
	echo_tail = 0;

	if(embedded_value[EMBED_H] > 0)
	{
		// set echo from an embedded command in the text
		amp = embedded_value[EMBED_H];
		delay = 130;
	}
	echo_head = (delay * samplerate)/1000;
	echo_amp = amp;
	// compensate (partially) for increase in amplitude due to echo
	general_amplitude = GetAmplitude();
	general_amplitude = ((general_amplitude * (512-amp))/512);
}



int PeaksToHarmspect(wavegen_peaks_t *peaks, int pitch, int *htab, int control)
{//============================================================================
// Calculate the amplitude of each  harmonics from the formants
// Only for formants 0 to 5

   // pitch and freqs are Hz<<16

	int f;
	wavegen_peaks_t *p;
	int fp;   // centre freq of peak
	int fhi;  // high freq of peak
	int h;    // harmonic number
	int pk;
	int hmax;
	int hmax_samplerate;      // highest harmonic allowed for the samplerate
	int x;
	int h1;

#ifdef SPECT_EDITOR
	if(harm_sqrt_n > 0)
		return(HarmToHarmspect(pitch,htab));
#endif

	// initialise as much of *out as we will need
	if(wvoice == NULL)
		return(1);
	hmax = (peaks[wvoice->n_harmonic_peaks].freq + peaks[wvoice->n_harmonic_peaks].right)/pitch;

	// restrict highest harmonic to half the samplerate
	hmax_samplerate = (((samplerate * 19)/40) << 16)/pitch;   // only 95% of Nyquist freq
//	hmax_samplerate = (samplerate << 16)/(pitch*2);

	if(hmax > hmax_samplerate)
		hmax = hmax_samplerate;

	for(h=0;h<=hmax;h++)
		htab[h]=0;

	h=0;
	for(pk=0; pk<=wvoice->n_harmonic_peaks; pk++)
	{
		p = &peaks[pk];
		if((p->height == 0) || (fp = p->freq)==0)
			continue;

		fhi = p->freq + p->right;
		h = ((p->freq - p->left) / pitch) + 1;
		if(h <= 0) h = 1;

		for(f=pitch*h; f < fp; f+=pitch)
		{
			htab[h++] += pk_shape[(fp-f)/(p->left>>8)] * p->height;
		}
		for(;f < fhi; f+=pitch)
		{
			htab[h++] += pk_shape[(f-fp)/(p->right>>8)] * p->height;
		}
	}

	// find the nearest harmonic for HF peaks where we don't use shape
	for(; pk<N_PEAKS; pk++)
	{
		peak_harmonic[pk] = peaks[pk].freq / pitch;
		x = peaks[pk].height >> 14;
		peak_height[pk] = x * x * 5;

		// only use harmonics up to half the samplerate
		if(peak_harmonic[pk] >= hmax_samplerate)
			peak_height[pk] = 0;
	}

	// convert from the square-rooted values
	f = 0;
	for(h=0; h<=hmax; h++, f+=pitch)
	{
		x = htab[h] >> 15;
		htab[h] = (x * x) >> 7;

		htab[h] = (htab[h] * tone_adjust[f >> 19]) >> 13;  // index tone_adjust with Hz/8
	}

	// adjust the amplitude of the first harmonic, affects tonal quality
	h1 = htab[1] * option_harmonic1;
	htab[1] = h1/8;


	// calc intermediate increments of LF harmonics
	if(control & 1)
	{
		for(h=1; h<N_LOWHARM; h++)
		{
			harm_inc[h] = (htab[h] - harmspect[h]) >> 3;
		}
	}

	return(hmax);  // highest harmonic number
}  // end of PeaksToHarmspect



static void AdvanceParameters()
{//============================
// Called every 64 samples to increment the formant freq, height, and widths

	int x;
	int ix;
	static int Flutter_ix = 0;

	// advance the pitch
	pitch_ix += pitch_inc;
	if((ix = pitch_ix>>8) > 127) ix = 127;
	x = pitch_env[ix] * pitch_range;
	pitch = (x>>8) + pitch_base;

	amp_ix += amp_inc;

	/* add pitch flutter */
	if(Flutter_ix >= (N_FLUTTER*64))
		Flutter_ix = 0;
	x = ((int)(Flutter_tab[Flutter_ix >> 6])-0x80) * flutter_amp;
	Flutter_ix += Flutter_inc;
	pitch += x;


	if(samplecount == samplecount_start)
		return;

	for(ix=0; ix <= wvoice->n_harmonic_peaks; ix++)
	{
		peaks[ix].freq1 += peaks[ix].freq_inc;
		peaks[ix].freq = int(peaks[ix].freq1);
		peaks[ix].height1 += peaks[ix].height_inc;
		if((peaks[ix].height = int(peaks[ix].height1)) < 0)
			peaks[ix].height = 0;
		peaks[ix].left1 += peaks[ix].left_inc;
		peaks[ix].left = int(peaks[ix].left1);
		peaks[ix].right1 += peaks[ix].right_inc;
		peaks[ix].right = int(peaks[ix].right1);
	}
	for(;ix < N_PEAKS; ix++)
	{
		// formants 6,7,8 don't have a width parameter
		peaks[ix].freq1 += peaks[ix].freq_inc;
		peaks[ix].freq = int(peaks[ix].freq1);
		peaks[ix].height1 += peaks[ix].height_inc;
		if((peaks[ix].height = int(peaks[ix].height1)) < 0)
			peaks[ix].height = 0;
	}

#ifdef SPECT_EDITOR
	if(harm_sqrt_n != 0)
	{
		// We are generating from a harmonic spectrum at a given pitch, not from formant peaks
		for(ix=0; ix<harm_sqrt_n; ix++)
			harm_sqrt[ix] += harm_sqrt_inc[ix];
	}
#endif
}  //  end of AdvanceParameters



static int Wavegen()
{//=================
	unsigned short waveph;
	unsigned short theta;
	int total;
	int h;
	int ix;
	int z, z1, z2;
	int ov;
	static int maxh, maxh2;
	int pk;
	signed char c;
	int sample;
	int amp;
	int modn_amp, modn_period;
	static int agc = 256;
	static int h_switch_sign = 0;
	static int cycle_count = 0;
	static int amplitude2 = 0;   // adjusted for pitch

	// continue until the output buffer is full, or
	// the required number of samples have been produced

	for(;;)
	{
		if((end_wave==0) && (samplecount==nsamples))
			return(0);

		if((samplecount & 0x3f) == 0)
		{
			// every 64 samples, adjust the parameters
			if(samplecount == 0)
			{
				hswitch = 0;
				harmspect = hspect[0];
				maxh2 = PeaksToHarmspect(peaks,pitch<<4,hspect[0],0);
				amplitude2 = (amplitude * pitch)/(100 << 12);

            // switch sign of harmonics above about 900Hz, to reduce max peak amplitude
				h_switch_sign = 890 / (pitch >> 12);
			}
			else
				AdvanceParameters();

			// pitch is Hz<<12
			phaseinc = (pitch>>7) * PHASE_INC_FACTOR;
			cycle_samples = samplerate/(pitch >> 12);  // sr/(pitch*2)
			hf_factor = pitch >> 11;

			maxh = maxh2;
			harmspect = hspect[hswitch];
			hswitch ^= 1;
			maxh2 = PeaksToHarmspect(peaks,pitch<<4,hspect[hswitch],1);

		}
		else
		if((samplecount & 0x07) == 0)
		{
			for(h=1; h<N_LOWHARM && h<=maxh2 && h<=maxh; h++)
			{
				harmspect[h] += harm_inc[h];
			}

			// bring automctic gain control back towards unity
			if(agc < 256) agc++;
		}

		samplecount++;

		if(wavephase > 0)
		{
			wavephase += phaseinc;
			if(wavephase < 0)
			{
				// sign has changed, reached a quiet point in the waveform
				cbytes = wavemult_offset - (cycle_samples)/2;
				if(samplecount > nsamples)
					return(0);

				cycle_count++;

				// adjust amplitude to compensate for fewer harmonics at higher pitch
				amplitude2 = (amplitude * pitch)/(100 << 12);

				if(glottal_flag > 0)
				{
					if(glottal_flag == 3)
					{
						if((nsamples-samplecount) < (cycle_samples*2))
						{
							// Vowel before glottal-stop.
							// This is the start of the penultimate cycle, reduce its amplitude
							glottal_flag = 2;
							amplitude2 = (amplitude2 *  glottal_reduce)/256;
						}
					}
					else
					if(glottal_flag == 4)
					{
						// Vowel following a glottal-stop.
						// This is the start of the second cycle, reduce its amplitude
						glottal_flag = 2;
						amplitude2 = (amplitude2 * glottal_reduce)/256;
					}
					else
					{
						glottal_flag--;
					}
				}

				if(amplitude_env != NULL)
				{
					// amplitude envelope is only used for creaky voice effect on certain vowels/tones
					if((ix = amp_ix>>8) > 127) ix = 127;
					amp = amplitude_env[ix];
					amplitude2 = (amplitude2 * amp)/255;
					if(amp < 255)
						modulation_type = 7;
				}

				// introduce roughness into the sound by reducing the amplitude of
				modn_period = 0;
				if(voice->roughness < N_ROUGHNESS)
				{
					modn_period = modulation_tab[voice->roughness][modulation_type];
					modn_amp = modn_period & 0xf;
					modn_period = modn_period >> 4;
				}

				if(modn_period != 0)
				{
					if(modn_period==0xf)
					{
						// just once */
						amplitude2 = (amplitude2 * modn_amp)/16;
						modulation_type = 0;
					}
					else
					{
						// reduce amplitude every [modn_period} cycles
						if((cycle_count % modn_period)==0)
							amplitude2 = (amplitude2 * modn_amp)/16;
					}
				}
			}
		}
		else
		{
			wavephase += phaseinc;
		}
		waveph = (unsigned short)(wavephase >> 16);
		total = 0;

		// apply HF peaks, formants 6,7,8
		// add a single harmonic and then spread this my multiplying by a
		// window.  This is to reduce the processing power needed to add the
		// higher frequence harmonics.
		cbytes++;
		if(cbytes >=0 && cbytes<wavemult_max)
		{
			for(pk=wvoice->n_harmonic_peaks+1; pk<N_PEAKS; pk++)
			{
				theta = peak_harmonic[pk] * waveph;
				total += (long)sin_tab[theta >> 5] * peak_height[pk];
			}

			// spread the peaks by multiplying by a window
			total = (long)(total / hf_factor) * wavemult[cbytes];
		}

		// apply main peaks, formants 0 to 5
#ifdef USE_ASSEMBLER_1
		// use an optimised routine for this loop, if available
		total += AddSineWaves(waveph, h_switch_sign, maxh, harmspect);  // call an assembler code routine
#else
		theta = waveph;

		for(h=1; h<=h_switch_sign; h++)
		{
			total += (int(sin_tab[theta >> 5]) * harmspect[h]);
			theta += waveph;
		}
		while(h<=maxh)
		{
			total -= (int(sin_tab[theta >> 5]) * harmspect[h]);
			theta += waveph;
			h++;
		}
#endif
		// mix with sampled wave if required
		z2 = 0;
		if(mix_wavefile_ix < n_mix_wavefile)
		{
			if(mix_wave_scale == 0)
			{
				// a 16 bit sample
				c = mix_wavefile[mix_wavefile_ix+1];
				sample = mix_wavefile[mix_wavefile_ix] + (c * 256);
				mix_wavefile_ix += 2;
			}
			else
			{
				// a 8 bit sample, scaled
				sample = (signed char)mix_wavefile[mix_wavefile_ix++] * mix_wave_scale;
			}
			z2 = (sample * amplitude_v) >> 10;
			z2 = (z2 * mix_wave_amp)/32;
		}

		z1 = z2 + (((total>>7) * amplitude2) >> 14);
		z = (z1 * agc) >> 8;

		z += ((echo_buf[echo_tail++] * echo_amp) >> 8);
		if(echo_tail >= N_ECHO_BUF)
			echo_tail=0;

		// check for overflow, 16bit signed samples
		if(z >= 32768)
		{
			ov = 8388608/z1 - 1;      // 8388608 is 2^23, i.e. max value * 256
			if(ov < agc) agc = ov;    // set agc to number of 1/256ths to multiply the sample by
			z = (z1 * agc) >> 8;      // reduce sample by agc value to prevent overflow
		}
		else
		if(z <= -32768)
		{
			ov = -8388608/z1 - 1;
			if(ov < agc) agc = ov;
			z = (z1 * agc) >> 8;
		}
		*out_ptr++ = z;
		*out_ptr++ = z >> 8;

		echo_buf[echo_head++] = z;
		if(echo_head >= N_ECHO_BUF)
			echo_head = 0;

		if(out_ptr >= out_end)
			return(1);
	}
	return(0);
}  //  end of Wavegen


static int PlaySilence(int length, int resume)
{//===========================================
	static int n_samples;
	int value;

	nsamples = 0;
	samplecount = 0;

	if(resume==0)
		n_samples = length;

	while(n_samples-- > 0)
	{
		value = (echo_buf[echo_tail++] * echo_amp) >> 8;

		if(echo_tail >= N_ECHO_BUF)
			echo_tail = 0;

		*out_ptr++ = value;
		*out_ptr++ = value >> 8;

		echo_buf[echo_head++] = value;
		if(echo_head >= N_ECHO_BUF)
			echo_head = 0;

		if(out_ptr >= out_end)
			return(1);
	}
	return(0);
}  // end of PlaySilence



static int PlayWave(int length, int resume, unsigned char *data, int scale, int amp)
{//=================================================================================
	static int n_samples;
	static int ix=0;
	int value;
	signed char c;

	if(resume==0)
	{
		n_samples = length;
		ix = 0;
	}

	nsamples = 0;
	samplecount = 0;

	while(n_samples-- > 0)
	{
		if(scale == 0)
		{
			// 16 bits data
			c = data[ix+1];
			value = data[ix] + (c * 256);
			ix+=2;
		}
		else
		{
			// 8 bit data, shift by the specified scale factor
			value = (signed char)data[ix++] * scale;
		}
		value *= (consonant_amp * general_amplitude);   // reduce strength of consonant
		value = value >> 10;
		value = (value * amp)/32;

		value += ((echo_buf[echo_tail++] * echo_amp) >> 8);

		if(value > 32767)
			value = 32768;
		else
		if(value < -32768)
			value = -32768;

		if(echo_tail >= N_ECHO_BUF)
			echo_tail = 0;

		out_ptr[0] = value;
		out_ptr[1] = value >> 8;
		out_ptr+=2;

		echo_buf[echo_head++] = (value*3)/4;
		if(echo_head >= N_ECHO_BUF)
			echo_head = 0;

		if(out_ptr >= out_end)
			return(1);
	}
	return(0);
}


static int SetWithRange0(int value, int max)
{//=========================================
	if(value < 0)
		return(0);
	if(value > max)
		return(max);
	return(value);
}


void SetEmbedded(int control, int value)
{//=====================================
	// there was an embedded command in the text at this point
	int sign=0;
	int command;
	int ix;
	int factor;

	command = control & 0x1f;
	if((control & 0x60) == 0x60)
		sign = -1;
	else
	if((control & 0x60) == 0x40)
		sign = 1;

	if(command < N_EMBEDDED_VALUES)
	{
		if(sign == 0)
			embedded_value[command] = value;
		else
			embedded_value[command] += (value * sign);
		embedded_value[command] = SetWithRange0(embedded_value[command],embedded_max[command]);
	}

	switch(command)
	{
	case EMBED_P:
	case EMBED_T:
		// adjust formants to give better results for a different voice pitch
		factor = 256 + (25 * (embedded_value[EMBED_P] - 50))/50;
		for(ix=0; ix<=5; ix++)
		{
			wvoice->freq[ix] = (wvoice->freq2[ix] * factor)/256;
		}
		factor = (embedded_value[EMBED_T] - 50)*2;
		wvoice->height[0] = (wvoice->height2[0] * (256 - factor*2))/256;
		wvoice->height[1] = (wvoice->height2[1] * (256 - factor))/256;
		break;

	case EMBED_A:  // amplitude
		general_amplitude = GetAmplitude();
		break;

	case EMBED_F:   // emphasiis
		general_amplitude = GetAmplitude();
		break;

	case EMBED_H:
		WavegenSetEcho();
		break;
	}
}


void WavegenSetVoice(voice_t *v)
{//=============================
	wvoice = v;

	if(v->peak_shape==0)
		pk_shape = pk_shape1;
	else
		pk_shape = pk_shape2;

	WavegenSetEcho();
}


static void SetAmplitude(int length, unsigned char *amp_env, int value)
{//====================================================================
	amp_ix = 0;
	if(length==0)
		amp_inc = 0;
	else
		amp_inc = (256 * ENV_LEN * STEPSIZE)/length;

	amplitude = (value * general_amplitude)/16;
	amplitude_v = amplitude * 15;           // for wave mixed with voiced sounds

	amplitude_env = amp_env;
}


void SetPitch(int length, unsigned char *env, int pitch1, int pitch2)
{//==================================================================
// length in samples
	int x;
	int base;
	int range;

#ifdef LOG_FRAMES
f_log=fopen("log","a");
if(f_log != NULL)
{
fprintf(f_log,"	  %3d %3d\n",pitch1,pitch2);
fclose(f_log);
f_log=NULL;
}
#endif
	if((pitch_env = env)==NULL)
		pitch_env = env_fall;  // default

	pitch_ix = 0;
	if(length==0)
		pitch_inc = 0;
	else
		pitch_inc = (256 * ENV_LEN * STEPSIZE)/length;

	if(pitch1 > pitch2)
	{
		x = pitch1;   // swap values
		pitch1 = pitch2;
		pitch2 = x;
	}

	base = (wvoice->pitch_base * pitch_adjust_tab[embedded_value[EMBED_P]])/128;
	range =  (wvoice->pitch_range * embedded_value[EMBED_R])/50;

	// compensate for change in pitch when the range is narrowed or widened
	base -= (range - wvoice->pitch_range)*20;

	pitch_base = base + (pitch1 * range);
	pitch_range = base + (pitch2 * range) - pitch_base;

	// set initial pitch
	pitch = ((pitch_env[0]*pitch_range)>>8) + pitch_base;   // Hz << 12

	flutter_amp = wvoice->flutter;

}  // end of SetPitch





void SetSynth(int length, int modn, frame_t *fr1, frame_t *fr2)
{//============================================================
	int ix;
	DOUBLEX next;
	int length2;
	int length4;
	int qix;
	int cmd;
	voice_t *v;
	static int glottal_reduce_tab1[4] = {0x30, 0x30, 0x40, 0x50};  // vowel before [?], amp * 1/256
//	static int glottal_reduce_tab1[4] = {0x30, 0x40, 0x50, 0x60};  // vowel before [?], amp * 1/256
	static int glottal_reduce_tab2[4] = {0x90, 0xa0, 0xb0, 0xc0};  // vowel after [?], amp * 1/256

#ifdef LOG_FRAMES
f_log=fopen("log","a");
if(f_log != NULL)
{
fprintf(f_log,"%3d   %4d %4d %4d   %4d %4d %4d\n",length*1000/samplerate,
	fr1->ffreq[1], fr1->ffreq[2], fr1->ffreq[3],
	fr2->ffreq[1], fr2->ffreq[2], fr2->ffreq[3] );
fclose(f_log);
f_log=NULL;
}
#endif

	harm_sqrt_n = 0;
	end_wave = 1;

	// any additional information in the param1 ?
	modulation_type = modn & 0xff;

	glottal_flag = 0;
	if(modn & 0x400)
	{
		glottal_flag = 3;  // before a glottal stop
		glottal_reduce = glottal_reduce_tab1[(modn >> 8) & 3];
	}
	if(modn & 0x800)
	{
		glottal_flag = 4;  // after a glottal stop
		glottal_reduce = glottal_reduce_tab2[(modn >> 8) & 3];
	}

	for(qix=wcmdq_head+1;;qix++)
	{
		if(qix >= N_WCMDQ) qix = 0;
		if(qix == wcmdq_tail) break;

		cmd = wcmdq[qix][0];
		if(cmd==WCMD_SPECT)
		{
			end_wave = 0;  // next wave generation is from another spectrum
			break;
		}
		if((cmd==WCMD_WAVE) || (cmd==WCMD_PAUSE))
			break;   // next is not from spectrum, so continue until end of wave cycle
	}

	v = wvoice;

	// round the length to a multiple of the stepsize
	length2 = (length + STEPSIZE/2) & ~0x3f;
	if(length2 == 0)
		length2 = STEPSIZE;

	// add this length to any left over from the previous synth
	samplecount_start = samplecount;
	nsamples += length2;

	length4 = length2/4;
	for(ix=0; ix<N_PEAKS; ix++)
	{
		peaks[ix].freq1 = (fr1->ffreq[ix] * v->freq[ix]) << 8;
		peaks[ix].freq = int(peaks[ix].freq1);
		next = (fr2->ffreq[ix] * v->freq[ix]) << 8;
		peaks[ix].freq_inc =  ((next - peaks[ix].freq1) * (STEPSIZE/4)) / length4;  // lower headroom for fixed point math

		peaks[ix].height1 = (fr1->fheight[ix] * v->height[ix]) << 6;
		peaks[ix].height = int(peaks[ix].height1);
		next = (fr2->fheight[ix] * v->height[ix]) << 6;
		peaks[ix].height_inc =  ((next - peaks[ix].height1) * STEPSIZE) / length2;

		if(ix <= wvoice->n_harmonic_peaks)
		{
			peaks[ix].left1 = (fr1->fwidth[ix] * v->width[ix]) << 10;
			peaks[ix].left = int(peaks[ix].left1);
			next = (fr2->fwidth[ix] * v->width[ix]) << 10;
			peaks[ix].left_inc =  ((next - peaks[ix].left1) * STEPSIZE) / length2;

			peaks[ix].right1 = (fr1->fright[ix] * v->width[ix]) << 10;
			peaks[ix].right = int(peaks[ix].right1);
			next = (fr2->fright[ix] * v->width[ix]) << 10;
			peaks[ix].right_inc = ((next - peaks[ix].right1) * STEPSIZE) / length2;
		}
	}
}  // end of SetSynth


static int Wavegen2(int length, int modulation, int resume, frame_t *fr1, frame_t *fr2)
{//====================================================================================
	if(resume==0)
		SetSynth(length,modulation,fr1,fr2);

	return(Wavegen());
}



int OpenWaveFile(const char *path, int rate)
/******************************************/
{
	int *p;

	static unsigned char wave_hdr[44] = {
		'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',
		0x10,0,0,0,1,0,1,0,  9,0x3d,0,0,0x12,0x7a,0,0,
		2,0,0x10,0,'d','a','t','a',  0,0,0,0 };


	if(path == NULL)
		return(2);

	wavephase = 0x7fffffff;

	// set the sample rate in the header
	p = (int *)(&wave_hdr[24]);
	p[0] = rate;
	p[1] = rate * 2;

	if(strcmp(path,"stdout")==0)
		f_wave = stdout;
	else
		f_wave = fopen(path,"wb");

	if(f_wave != NULL)
	{
		fwrite(wave_hdr,1,sizeof(wave_hdr),f_wave);
		return(0);
	}
	return(1);
}   //  end of OpenWaveFile




void CloseWaveFile(int rate)
/******************/
{
   unsigned int pos;
   static int value;

   if(f_log != NULL)
	{
		fclose(f_log);
		f_log = NULL;
	}

   if((f_wave == NULL) || (f_wave == stdout))
      return;

   fflush(f_wave);
   pos = ftell(f_wave);

   value = pos - 8;
   fseek(f_wave,4,SEEK_SET);
   fwrite(&value,4,1,f_wave);

	value = rate;
	fseek(f_wave,24,SEEK_SET);
	fwrite(&value,4,1,f_wave);

	value = rate*2;
	fseek(f_wave,28,SEEK_SET);
	fwrite(&value,4,1,f_wave);

   value = pos - 44;
   fseek(f_wave,40,SEEK_SET);
   fwrite(&value,4,1,f_wave);

   fclose(f_wave);
   f_wave = NULL;

} // end of CloseWaveFile



void MakeWaveFile()
{//================
	int result=1;

	while(result != 0)
	{
		out_ptr = outbuf;
		out_end = &outbuf[sizeof(outbuf)];
		result = Wavegen();
		if(f_wave != NULL)
			fwrite(outbuf,1,out_ptr-outbuf,f_wave);
	}
}  // end of MakeWaveFile





int WavegenFill(int fill_zeros)
{//============================
// Pick up next wavegen commands from the queue
// return: 0  output buffer has been filled
// return: 1  input command queue is now empty

	long *q;
	int length;
	int result;
	static int resume=0;
//	static unsigned char speed_adjust_tab[10] = {223,199,179,160,143,128,115,103,92,82};


	while(out_ptr < out_end)
	{
		if(WcmdqUsed() <= 0)
		{
			if(fill_zeros)
			{
				while(out_ptr < out_end)
					*out_ptr++ = 0;
			}
			return(1);              // queue empty, close sound channel
		}

		result = 0;
		q = wcmdq[wcmdq_head];
		length = q[1];

		switch(q[0])
		{
		case WCMD_PITCH:
			SetPitch(length,(unsigned char *)q[2],q[3] >> 16,q[3] & 0xffff);
			break;

		case WCMD_PAUSE:
			n_mix_wavefile = 0;
			if(length==0) break;
			result = PlaySilence(length,resume);
			break;

		case WCMD_WAVE:
			n_mix_wavefile = 0;
			result = PlayWave(length,resume,(unsigned char*)q[2], q[3] & 0xff, q[3] >> 8);
			break;

		case WCMD_WAVE2:
			// wave file to be played at the same time as synthesis
			mix_wave_amp = q[3] >> 8;
			mix_wave_scale = q[3] & 0xff;
			if(mix_wave_scale == 0)
				n_mix_wavefile = length*2;
			else
				n_mix_wavefile = length;
			mix_wavefile_ix = 0;
			mix_wavefile = (unsigned char *)q[2];
			break;

		case WCMD_SPECT2:   // as WCMD_SPECT but stop any concurrent wave file
			n_mix_wavefile = 0;   // ... and drop through to WCMD_SPECT case
		case WCMD_SPECT:
			result = Wavegen2(length & 0xffff,q[1] >> 16,resume,(frame_t *)q[2],(frame_t *)q[3]);
			break;

		case WCMD_MARKER:
			MarkerEvent(q[1],q[2],q[3],out_ptr);

			if(q[1] == 1)
			{
				current_source_index = q[2] & 0xffffff;
			}
			break;

		case WCMD_AMPLITUDE:
			SetAmplitude(length,(unsigned char *)q[2],q[3]);
			break;

		case WCMD_VOICE:
			WavegenSetVoice((voice_t *)q[1]);
			break;

		case WCMD_EMBEDDED:
			SetEmbedded(q[1],q[2]);
			break;
		}

		if(result==0)
		{
			WcmdqIncHead();
			resume=0;
		}
		else
		{
			resume=1;
		}
	}

	return(0);
}  // end of WavegenFill



int WavegenFile(void)
{//==================
	int finished;

	out_ptr = outbuf;
	out_end = outbuf + sizeof(outbuf);

	finished = WavegenFill(0);

	if(f_wave != NULL)
	{
		fwrite(outbuf,1,out_ptr-outbuf,f_wave);
	}
	return(finished);
}  // end of WavegenFile

