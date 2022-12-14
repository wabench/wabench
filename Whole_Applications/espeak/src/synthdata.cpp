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
#include "StdAfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <wctype.h>
#include <string.h>

#include "speech.h"
#include "voice.h"
#include "phoneme.h"
#include "synthesize.h"
#include "translate.h"

extern const char *WordToString(unsigned int word);

// copy the current phoneme table into here
int n_phoneme_tab;
PHONEME_TAB *phoneme_tab[N_PHONEME_TAB];


unsigned int *phoneme_index=NULL;
char *spects_data=NULL;
unsigned char *wavefile_data=NULL;
static unsigned char *phoneme_tab_data = NULL;

int n_phoneme_tables;
PHONEME_TAB_LIST phoneme_tab_list[N_PHONEME_TABS];
static int phoneme_tab_number = 0;

int wavefile_ix;              // a wavefile to play along with the synthesis
int wavefile_amp;
static int seq_len_adjust;
int vowel_transition[4];
int vowel_transition0;
int vowel_transition1;

void FormantTransitions(frameref_t *seq, int &n_frames, PHONEME_TAB *this_ph, PHONEME_TAB *other_ph, int which);
int FormantTransition2(frameref_t *seq, int &n_frames, unsigned int data1, unsigned int data2, PHONEME_TAB *other_ph, int which);


const char *PhonemeTabName(void)
{//=============================
	return(phoneme_tab_list[phoneme_tab_number].name);
}


static int ReadPhFile(char **ptr, const char *fname)
{//=================================================
	FILE *f_in;
	char *p;
	unsigned int  length;
	char buf[200];

	sprintf(buf,"%s%c%s",path_home,PATHSEP,fname);
	length = GetFileLength(buf);
	
	if((f_in = fopen(buf,"rb")) == NULL)
	{
		fprintf(stderr,"Can't read data file: '%s'\n",buf);
		return(1);
	}

	if(*ptr != NULL)
		Free(*ptr);
		
	if((p = Alloc(length)) == NULL)
	{
		fclose(f_in);
		return(-1);
	}
	if(fread(p,1,length,f_in) != length)
	{
		fclose(f_in);
		return(-1);
	}
	*ptr = p;
	fclose(f_in);
	return(0);
}  //  end of ReadPhFile


int LoadPhData()
{//=============
	int ix;
	int n_phonemes;
	unsigned char *p;

	if(ReadPhFile((char **)(&phoneme_tab_data),"phontab") != 0)
		return(-1);
	if(ReadPhFile((char **)(&phoneme_index),"phonindex") != 0)
		return(-1);
	if(ReadPhFile((char **)(&spects_data),"phondata") != 0)
		return(-1);
   wavefile_data = (unsigned char *)spects_data;

	// set up phoneme tables
	p = phoneme_tab_data;
	n_phoneme_tables = p[0];
	p+=4;

	for(ix=0; ix<n_phoneme_tables; ix++)
	{
		n_phonemes = p[0];
		phoneme_tab_list[ix].n_phonemes = p[0];
		phoneme_tab_list[ix].includes = p[1];
		p += 4;
		memcpy(phoneme_tab_list[ix].name,p,N_PHONEME_TAB_NAME);
		p += N_PHONEME_TAB_NAME;
		phoneme_tab_list[ix].phoneme_tab_ptr = (PHONEME_TAB *)p;
		p += (n_phonemes * sizeof(PHONEME_TAB));
	}

	if(phoneme_tab_number >= n_phoneme_tables)
		phoneme_tab_number = 0;

	return(0);
}  //  end of LoadPhData

#ifdef PLATFORM_RISCOS
void FreePhData(void)
{//==================
	Free(phoneme_tab_data);
	Free(phoneme_index);
	Free(spects_data);
}
#endif

static unsigned int LookupSound2(int index, unsigned int other_phcode, int control)
{//================================================================================
// control=1  get formant transition data only

	int code;
	unsigned int value, value2;
	
	while((value = phoneme_index[index++]) != 0)
	{
		if((code = (value & 0xff)) == other_phcode)
		{
			while(((value2 = phoneme_index[index]) != 0) && ((value2 & 0xff) < 8))
			{
				switch(value2 & 0xff)
				{
				case 0:
					// next entry is a wavefile to be played along with the synthesis
					if(control==0)
						wavefile_ix = value2 >> 8;
					break;
				case 1:
					if(control==0)
						seq_len_adjust = value2 >> 8;
					break;
				case 2:
					if(control==0)
						seq_len_adjust = -(value2 >> 8);
					break;
				case 3:
					if(control==0)
						wavefile_amp = value2 >> 8;
					break;
				case 4:
					// formant transition data, 2 words
					vowel_transition[0] = value2 >> 8;
					vowel_transition[1] = phoneme_index[index++ + 1];
					break;
				case 5:
					// formant transition data, 2 words
					vowel_transition[2] = value2 >> 8;
					vowel_transition[3] = phoneme_index[index++ + 1];
					break;
				}
				index++;
			}
			return(value >> 8);
		}
		else
		if((code == 4) || (code == 5))
		{
			// formant transition data, ignore next word of data
			index++;
		}
	}
	return(3);   // not found
}  //  end of LookupSound2


unsigned int LookupSound(PHONEME_TAB *this_ph, PHONEME_TAB *other_ph, int which, int *match_level, int control)
{//============================================================================================================
	// follows,  1 other_ph preceeds this_ph,   2 other_ph follows this_ph
   // control:  1= get formant transition data only
	int spect_list;
	int spect_list2;
	int s_list;
	unsigned char virtual_ph;
	int  result;
	int  level=0;
	unsigned int  other_code;
	unsigned int  other_virtual;
	
	if(control==0)
	{
		wavefile_ix = 0;
		wavefile_amp = 32;
		seq_len_adjust = 0;
	}
	memset(vowel_transition,0,sizeof(vowel_transition));
	
	other_code = other_ph->code;
	if(phoneme_tab[other_code]->type == phPAUSE)
		other_code = phonPAUSE_SHORT;       // use this version of Pause for matching

	if(which==1)
	{
		spect_list = this_ph->after;
		virtual_ph = this_ph->start_type;
		spect_list2 = phoneme_tab[virtual_ph]->after;
		other_virtual = other_ph->end_type;
	}
	else
	{
		spect_list = this_ph->before;
		virtual_ph = this_ph->end_type;
		spect_list2 = phoneme_tab[virtual_ph]->before;
		other_virtual = other_ph->start_type;
	}

	result = 3;
	// look for ph1-ph2 combination
	if((s_list = spect_list) != 0)
	{
		if((result = LookupSound2(s_list,other_code,control)) != 3)
		{
			level = 2;
		}
		else
		if(other_virtual != 0)
		{
			if((result = LookupSound2(spect_list,other_virtual,control)) != 3)
			{
				level = 1;
			}
		}
	}
	// not found, look in a virtual phoneme if one is given for this phoneme
	if((result==3) && (virtual_ph != 0) && ((s_list = spect_list2) != 0))
	{
		if((result = LookupSound2(s_list,other_code,control)) != 3)
		{
			level = 1;
		}
		else
		if(other_virtual != 0)
		{
			if((result = LookupSound2(spect_list2,other_virtual,control)) != 3)
			{
				level = 1;
			}
		}
	}

	if(match_level != NULL)
		*match_level = level;
	
	if(result==0)
		return(0);   // NULL was given in the phoneme source

	// note: values = 1 indicates use the default for this phoneme, even though we found a match
	// which set a secondary reference 
	if(result >= 4)
	{
		// values 1-3 can be used for special codes
		// 1 = DFT from the phoneme source file
		return(result);
	}
	
	// no match found for other_ph, return the default
	return(LookupSound2(this_ph->spect,phonPAUSE,control));

}  //  end of LookupSound



frameref_t *LookupSpect(PHONEME_TAB *this_ph, PHONEME_TAB *prev_ph, PHONEME_TAB *next_ph,
			int which, int *match_level, int *n_frames, PHONEME_LIST *plist)
{//=========================================================================================================
	int  ix;
	int  nf;
	int  nf1;
	int  seq_break;
	frameref_t *frames;
	int  length1;
	int  length_std;
	int  length_factor;
	SPECT_SEQ *seq;
	SPECT_SEQ *seq2;
	PHONEME_TAB *next2_ph;
	static frameref_t frames_buf[N_SEQ_FRAMES];
	
	PHONEME_TAB *other_ph;
	if(which == 1)
		other_ph = prev_ph;
	else
		other_ph = next_ph;

	if((ix = LookupSound(this_ph,other_ph,which,match_level,0)) == 0)
		return(NULL);
	seq = (SPECT_SEQ *)(&spects_data[ix]);
	nf = seq->n_frames;

	seq_break = 0;
	length1 = 0;
	for(ix=0; ix<nf; ix++)
	{
		frames_buf[ix].frame = &seq->frame[ix];
		frames_buf[ix].flags = seq->frame[ix].flags;
		frames_buf[ix].length = seq->frame[ix].length;
		if(seq->frame[ix].flags & FRFLAG_VOWEL_CENTRE)
			seq_break = ix;
	}
	
	frames = &frames_buf[0];
	if(seq_break > 0)
	{
		if(which==1)
		{
			nf = seq_break + 1;
		}
		else
		{
			frames = &frames_buf[seq_break];  // body of vowel, skip past initial frames
			nf -= seq_break;
		}
	}
	
	// do we need to modify a frame for blending with a consonant?
	if(this_ph->type == phVOWEL)
	{
		if(which==2)
		{
			// lookup formant transition for the following phoneme

			if(*match_level == 0)
			{
				LookupSound(next_ph,this_ph,1,NULL,1);
				seq_len_adjust += FormantTransition2(frames,nf,vowel_transition[2],vowel_transition[3],next_ph,which);
			}
			else
			if(next_ph->phflags == phVOWEL2)
			{
				// not really a consonant, rather a coloured vowel
				if(LookupSound(next_ph,this_ph,1,NULL,1) == 0)
				{
					next2_ph = plist[2].ph;
					LookupSound(next2_ph,next_ph,1,NULL,1);
					seq_len_adjust += FormantTransition2(frames,nf,vowel_transition[2],vowel_transition[3],next2_ph,which);
				}
			}
		}
		else
		{
			if(*match_level == 0)
				seq_len_adjust = FormantTransition2(frames,nf,vowel_transition0,vowel_transition1,prev_ph,which);
		}
//		FormantTransitions(frames,nf,this_ph,other_ph,which);
	}

	nf1 = nf - 1;
	for(ix=0; ix<nf1; ix++)
		length1 += frames[ix].length;


	if((wavefile_ix != 0) && ((wavefile_ix & 0x800000)==0))
	{
		// a secondary reference has been returned, which is not a wavefile
		// add these spectra to the main sequence
		seq2 = (SPECT_SEQ *)(&spects_data[wavefile_ix]);
	
		// first frame of the addition just sets the length of the last frame of the main seq
		nf--;
		for(ix=0; ix<seq2->n_frames; ix++)
		{
			frames[nf].length = seq2->frame[ix].length;
			if(ix > 0)
				frames[nf].frame = &seq2->frame[ix];
			nf++;
		}
		wavefile_ix = 0;
	}
	
	if((this_ph->type == phVOWEL) && (length1 > 0))
	{
		if(which==2)
		{
			// adjust the length of the main part to match the standard length specified for the vowel
			//   less the front part of the vowel and any added suffix
	
			length_std = this_ph->std_length + seq_len_adjust - 45;
			if(plist->synthflags & SFLAG_LENGTHEN)
				length_std += phoneme_tab[phonLENGTHEN]->std_length;  // phoneme was followed by an extra : symbol

// can adjust vowel length for stressed syllables here


			length_factor = (length_std * 256)/ length1;
			
			for(ix=0; ix<nf1; ix++)
			{
				frames[ix].length = (frames[ix].length * length_factor)/256;
			}
		}
		else
		{
			// front of a vowel
			if(*match_level == 0)
			{
				// allow very short vowels to have shorter front parts
				if(this_ph->std_length < 130)
					frames[0].length = (frames[0].length * this_ph->std_length)/130;
			}

			if(seq_len_adjust != 0)
			{
				length_std = 0;
				for(ix=0; ix<nf1; ix++)
				{
					length_std += frames[ix].length;
				}
				length_factor = ((length_std + seq_len_adjust) * 256)/length_std;
				for(ix=0; ix<nf1; ix++)
				{
					frames[ix].length = (frames[ix].length * length_factor)/256;
				}
			}
		}
	}
	
	*n_frames = nf;
	return(frames);
}  //  end of LookupSpect


unsigned char *LookupEnvelope(int ix)
{//================================
	if(ix==0)
		return(NULL);
	return((unsigned char *)&spects_data[phoneme_index[ix]]);
}


static void SetUpPhonemeTable(int number)
{//======================================
	int ix;
	int includes;
	int ph_code;
	PHONEME_TAB *phtab;

	if((includes = phoneme_tab_list[number].includes) > 0)
	{
		// recursively include base phoneme tables
		SetUpPhonemeTable(includes-1);
	}

	// now add the phonemes from this table
	phtab = phoneme_tab_list[number].phoneme_tab_ptr;
	for(ix=0; ix<phoneme_tab_list[number].n_phonemes; ix++)
	{
		ph_code = phtab[ix].code;
		phoneme_tab[ph_code] = &phtab[ix];
		if(ph_code > n_phoneme_tab)
			n_phoneme_tab = ph_code;
	}
}  // end of SetUpPhonemeTable


void SelectPhonemeTable(int number)
{//===============================
	n_phoneme_tab = 0;
	SetUpPhonemeTable(number);  // recursively for included phoneme tables
	n_phoneme_tab++;
}  //  end of SelectPhonemeTable


int SelectPhonemeTableName(const char *name)
{//=========================================
// Look up a phoneme set by name, and select it if it exists
// Returns the phoneme table number
	int ix;

	for(ix=0; ix<n_phoneme_tables; ix++)
	{
		if(strcmp(name,phoneme_tab_list[ix].name)==0)
		{
			phoneme_tab_number = ix;
			break;
		}
	}
	if(ix == n_phoneme_tables)
		return(-1);

	SelectPhonemeTable(ix);
	return(ix);
}  //  end of DelectPhonemeTableName

