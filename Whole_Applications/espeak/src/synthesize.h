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

#define N_PHONEME_LIST  700    // enough for source[] full of text, else it will truncate

#define MAX_HARMONIC  400           // 400 * 50Hz = 20 kHz, more than enough
#define N_SEQ_FRAMES   25           // max frames in a spectrum sequence (real max is ablut 8)

#define    PITCHfall   0
#define    PITCHrise   1

// flags set for frames within a spectrum sequence
#define FRFLAG_VOWEL_CENTRE    0x02   // centre point of vowel
#define FRFLAG_LEN_MOD         0x04   // reduce effect of length adjustment
#define FRFLAG_BREAK_LF        0x08   // but keep f3 upwards
#define FRFLAG_BREAK           0x10   // don't merge with next frame
#define FRFLAG_FORMANT_RATE    0x20   // Flag5 allow increased rate of change of formant freq
#define FRFLAG_MODULATE        0x40   // Flag6 modulate amplitude of some cycles to give trill

#define SFLAG_SEQCONTINUE      0x01
#define SFLAG_EMBEDDED         0x02   // there are embedded commands before this phoneme
#define SFLAG_SYLLABLE         0x04   // vowel or syllabic consonant
#define SFLAG_LENGTHEN         0x08   // lengthen symbol : included after this phoneme
#define SFLAG_DICTIONARY       0x10   // the pronunciation of this word was listed in the xx_list dictionary


// embedded command numbers
#define EMBED_P     1   // pitch
#define EMBED_S     2   // speed (used in setlengths)
#define EMBED_A     3   // amplitude/volume
#define EMBED_R     4   // pitch range/expression
#define EMBED_H     5   // echo/reverberation
#define EMBED_T     6   // different tone
#define EMBED_I     7   // sound icon
#define EMBED_S2    8   // speed (used in synthesize)
#define EMBED_Y     9   // say-as commands
#define EMBED_M    10   // mark name
#define EMBED_U    11   // audio uri
#define EMBED_B    12   // break
#define EMBED_F    13   // emphasis

#define N_EMBEDDED_VALUES    14
extern int embedded_value[N_EMBEDDED_VALUES];
extern int embedded_default[N_EMBEDDED_VALUES];


// formant data used by wavegen
typedef struct {
	int freq;     // Hz<<16
	int height;   // height<<15
	int left;     // Hz<<16
	int right;    // Hz<<16
	DOUBLEX freq1; // floating point versions of the above
	DOUBLEX height1;
	DOUBLEX left1;
	DOUBLEX right1;
	DOUBLEX freq_inc;    // increment by this every 64 samples
	DOUBLEX height_inc;
	DOUBLEX left_inc;
	DOUBLEX right_inc;
}  wavegen_peaks_t;

typedef struct {
   short length;
   unsigned char  n_frames;
   unsigned char  flags;
   frame_t  frame[N_SEQ_FRAMES];     // max. frames in a spectrum sequence
} SPECT_SEQ;

typedef struct {
	short length;
	short flags;
	frame_t *frame;
} frameref_t;


typedef struct {
	PHONEME_TAB *ph;
	unsigned char env;    // pitch envelope number
	unsigned char tone;
	unsigned char type;
	unsigned char prepause;
	unsigned char amp;
	unsigned char tone_ph;   // tone phoneme to use with this vowel
	unsigned char newword;   // bit 0=start of word, bit 1=end of clause, bit 2=start of sentence
	unsigned char synthflags;
	short length;  // length_mod
	short pitch1;  // pitch, 0-4095 within the Voice's pitch range
	short pitch2;
	unsigned short sourceix;  // ix into the original source text string, only set at the start of a word
} PHONEME_LIST;


typedef struct {
	int name;
	int length;
	char *data;
	char *filename;
} SOUND_ICON;

// phoneme table
extern PHONEME_TAB *phoneme_tab[N_PHONEME_TAB];

// list of phonemes in a clause
extern int n_phoneme_list;
extern PHONEME_LIST phoneme_list[N_PHONEME_LIST];
extern unsigned int embedded_list[];

extern unsigned char env_fall[128];
extern unsigned char env_rise[128];
extern unsigned char env_frise[128];


// queue of commands for wavegen
#define WCMD_AMPLITUDE 1
#define WCMD_PITCH	2
#define WCMD_SPECT	3
#define WCMD_SPECT2	4
#define WCMD_PAUSE	5
#define WCMD_WAVE    6
#define WCMD_WAVE2   7
#define WCMD_MARKER	8
#define WCMD_VOICE   9
#define WCMD_EMBEDDED 10


#define N_WCMDQ   200
#define MIN_WCMDQ  20   // need this many free entries before adding new phoneme

extern long wcmdq[N_WCMDQ][4];
extern int wcmdq_head;
extern int wcmdq_tail;

// from Wavegen file
int  WcmdqFree();
void WcmdqStop();
int  WcmdqUsed();
void WcmdqInc();
int  WavegenOpenSound();
int  WavegenCloseSound();
int  WavegenInitSound();
void WavegenInit(int rate, int wavemult_fact);
int  OpenWaveFile(const char *path, int rate);
void CloseWaveFile(int rate);
float polint(float xa[],float ya[],int n,float x);
int  WavegenFile(void);
int WavegenFill(int fill_zeros);
void MarkerEvent(int type, unsigned int char_position, int value, unsigned char *out_ptr);
extern unsigned char *out_ptr;
extern unsigned char *out_end;


extern unsigned char *wavefile_data;
extern int samplerate;

extern int wavefile_ix;
extern int wavefile_amp;
extern int vowel_transition[4];
extern int vowel_transition0, vowel_transition1;

// from synthdata file
unsigned int LookupSound(PHONEME_TAB *ph1, PHONEME_TAB *ph2, int which, int *match_level, int control);
frameref_t *LookupSpect(PHONEME_TAB *ph1, PHONEME_TAB *prev_ph, PHONEME_TAB *next_ph, int which, int *match_level, int *n_frames, PHONEME_LIST *plist);

unsigned char *LookupEnvelope(int ix);
int LoadPhData();

void SynthesizeInit(void);
int  Generate(PHONEME_LIST *phoneme_list, int n_phoneme_list, int resume);
void MakeWave2(PHONEME_LIST *p, int n_ph);
int  SynthOnTimer(void);
int  SpeakNextClause(FILE *f_text, const void *text_in, int control);
int  SynthStatus(void);
void SetSpeed(int control);
void SetEmbedded(int control, int value);
void SelectPhonemeTable(int number);
int  SelectPhonemeTableName(const char *name);

extern unsigned char *envelope_data[16];
extern int formant_rate[];         // max rate of change of each formant

#define N_SOUNDICON_TAB  100
extern int n_soundicon_tab;
extern SOUND_ICON soundicon_tab[N_SOUNDICON_TAB];
