//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#include "globals.h"
#include "sampler.h"
#include "stdio.h"

#include "unitconversion.h"
#include "modmatrix.h"
#include "configuration.h"

#include <algorithm>
using std::min;
using std::max;

int keyname_to_keynumber(const char *name)	// using C4 == 60
{
	int key = 0;
	
	if(name[0] < 64)
	{
		return atoi(name);		// first letter is number, must be in numeric format then
	}

	char letter = tolower(name[0]);


	switch(letter)
	{
		case 'c': 
			key = 0;
			break;
		case 'd': 
			key = 2;
			break;
		case 'e': 
			key = 4;
			break;
		case 'f': 
			key = 5;
			break;
		case 'g': 
			key = 7;
			break;
		case 'a': 
			key = 9;
			break;
		case 'b': 
		case 'h': 
			key = 11;
			break;
	}

	const char *c = name+1;
	if(*c == '#')	key++;
	else if (*c == 'b') key--;
	else if (*c == 'B') key--;
	else c--;
	c++;

	bool negative = (*c == '-');
	if (negative) c++;
	int octave = (*c - 48);
	if(negative) octave = -octave;
	octave = max(0,octave + 1);
	
	return 12*octave + key;
}

bool sampler::load_sfz(const char *data, size_t datasize,int *new_g,char channel)
{		
	bool eof=false;
	const char *r = data;
	sample_zone *region=0,*z=0;
	sample_zone empty,groupzone;	

	// create empty zone, copy it to a buffer and then delete it
	int t_id;
	add_zone(0,&t_id,channel,false);
	memcpy(&empty,&zones[t_id],sizeof(sample_zone));
	memcpy(&groupzone,&empty,sizeof(sample_zone));	// init, just in case it's used
	this->free_zone(t_id);

	part_init(channel,true,true);

	int z_id = 0;
	while((r<(data+datasize)) && !eof)
	{
		switch(*r)
		{
		case '0':
			// EOF, 
			eof = true;
			break;
		case '<':		// group/region
			{
				if(strnicmp(r,"<region>",8) == 0)
				{
					r += 8;					
					if (add_zone(0,&z_id,channel,false))	// add an empty zone
					{
						region = &zones[z_id];
						memcpy(region,&groupzone,sizeof(sample_zone));
						z = region;
					}
				}
				else if(strnicmp(r,"<group>",7) == 0)
				{
					r += 7;
					memcpy(&groupzone,&empty,sizeof(sample_zone));
					z = &groupzone;
				}
				else
				{
					// Unknown tag, ignore
					while(*r != '>' && !eof)
					{
						if (*r == 0)
						{
							eof = true;
						}						
						else
						{
							r++;
						}						
					}
					r++;
				}
			}
			break;
		
		case '/':
			while(*r && (*r != 10) && (*r != 13))
			{
				r++;
			}
			// comment, skip ahead until the rest of the line
			break;
		case 9:		// whitespace/CR/LF, ignore
		case 10:
		case 13:
		case 32:
			r++;		
			break;
		
		default:		// neither group/region, nor whitespace, must be an opcode then
			{
				char opcode[64];
				strncpy(opcode,r,64);		// this one doesn't check zero-termination on purpose
				char *end = strchr(opcode,'=');
				*end = 0;
				r += strlen(opcode) + 1;
				char val[256];				
				int i=0; 
				
				/*while((i<254) && *r && (*r != 10) && (*r != 13) && (*r != 32) && (*r != 9))
				{
					val[i] = *r;
					r++;
					i++;
				}
				val[i++] = 0;*/

				// algo
				// 1) store initial position
				// 2) search ahead until '=' or '<' is found
				// 3) if '=' search backwards until whitespace found (pass whitespace as well)
				// 4) the span of the value should now be in the string
				
				
				const char *v_start = r; // 1
				
				while((i<254) && *r && (*r != '<') && (*r != '/') && (*r != '=') && (*r != 10) && (*r != 13))	// 2
				{					
					r++;
					i++;
				}
				
				if (*r == '=')	// 3
				{
					bool stop_after_ws=false;
					while((i>0) && *r && (!stop_after_ws || ((*r == 10) || (*r == 13) || (*r == 32) || (*r == 9))))	
					{											
						r--;
						i--;
						stop_after_ws = stop_after_ws || ((*r == 10) || (*r == 13) || (*r == 32) || (*r == 9));
					}						
				}
				else 
				{					
					while((i>0) && *r && ((*r == 10) || (*r == 13) || (*r == 32) || (*r == 9)))	
					{											
						r--;
						i--;						
					}						
				}
				r++; i++;
				
				strncpy(val,v_start,min(255L,r-v_start));
				val[i] = 0;

				if (!z) break;		// no region/group has been created, cannot continue
				
				if(stricmp(opcode,"sample") == 0)
				{
					char spath[256];
					sprintf(spath,"<relative>\\%s",val);
					replace_zone(z_id,spath);
				} 
				else if(stricmp(opcode,"key") == 0)
				{
					int key = keyname_to_keynumber(val);
					z->key_low = key;
					z->key_root = key;
					z->key_high = key;
				} 
				else if(stricmp(opcode,"lokey") == 0)
				{
					z->key_low = keyname_to_keynumber(val);
				}
				else if(stricmp(opcode,"hikey") == 0)
				{
					z->key_high = keyname_to_keynumber(val);
				}
				else if(stricmp(opcode,"pitch_keycenter") == 0)
				{
					z->key_root = keyname_to_keynumber(val);
				}
				else if(stricmp(opcode,"lovel") == 0)
				{
					z->velocity_low = atol(val);
				}
				else if(stricmp(opcode,"hivel") == 0)
				{
					z->velocity_high = atol(val);
				}
				else if(stricmp(opcode,"pitch_keytrack") == 0)
				{
					z->keytrack = atof(val);
				}
				else if(stricmp(opcode,"transpose") == 0)
				{
					z->transpose = atoi(val);
				}
				else if(stricmp(opcode,"tune") == 0)
				{
					z->finetune = (float) 0.01f * atoi(val);
				}
				else if(stricmp(opcode,"volume") == 0)
				{
					z->aux[0].level = atof(val);
				}
				else if(stricmp(opcode,"pan") == 0)
				{
					z->aux[0].balance = (float) 0.01f * atof(val);
				}
				else if(stricmp(opcode,"ampeg_attack") == 0)
				{
					z->AEG.attack = log2(atof(val));
				}
				else if(stricmp(opcode,"ampeg_hold") == 0)
				{
					z->AEG.hold = log2(atof(val));
				}
				else if(stricmp(opcode,"ampeg_decay") == 0)
				{
					z->AEG.decay = log2(atof(val));
				}
				else if(stricmp(opcode,"ampeg_release") == 0)
				{
					z->AEG.release = log2(atof(val));
				}
				else if(stricmp(opcode,"ampeg_sustain") == 0)
				{
					z->AEG.sustain = 0.01f * atof(val);
				}	
				
				/*// crossfading
				int xfin_lokey=-1,xfin_hikey=-1,xfout_lokey=-1,xfout_hikey=-1;
				int xfin_lovel=-1,xfin_hivel=-1,xfout_lovel=-1,xfout_hivel=-1;
				else if(stricmp(opcode,"xfin_lokey") == 0)
				{					
					xfin_lokey = keyname_to_keynumber(val);
				}
				else if(stricmp(opcode,"xfin_hikey") == 0)
				{
					xfin_hikey = keyname_to_keynumber(val);					
				}
				else if(stricmp(opcode,"xfout_lokey") == 0)
				{					
					xfout_lokey = keyname_to_keynumber(val);
				}
				else if(stricmp(opcode,"xfout_hikey") == 0)
				{
					xfout_hikey = keyname_to_keynumber(val);					
				}
			
				// has to be done per-zone (to be order independent), not per-opcode as it is now 
				if (xfin_lokey>=0)	z->key_low = xfin_lokey + 1;
				if (xfin_hikey>=0)	z->key_low_fade = xfin_hikey - z->key_low;								
				if (xfout_hikey>=0)		z->key_high = xfout_lokey + 1;
				if (xfinout_lokey>=0)	z->key_high_fade = xfout_hikey - z->key_high;*/
			}
			break;
		};
	}		
	return false;
}
