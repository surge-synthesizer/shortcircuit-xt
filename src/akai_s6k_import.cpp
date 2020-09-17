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
#include "logfile.h"
#include "unitconversion.h"
#include <assert.h>
#include <vt_dsp/basic_dsp.h>

#pragma pack(push, 1)

struct akai_s6k_akp_prg
{
	int8 unused0,unused1;
	int8 n_keygroups;		
	int8 unused2,unused3,KGXFade;
};

struct akai_s6k_akp_kloc
{
	int8 unused0[4];
	int8 key_low;
	int8 key_high;
	int8 semitone_tune;
	int8 fine_tune;
	int8 override_fx;
	int8 fx_send_level;
	int8 pitch_mod[2];
	int8 amp_mod;
	int8 zone_xfade;
	int8 mute_group;
	int8 unused1;
};

struct akai_s6k_akp_env
{
	int8 unused0;
	int8 attack;
	int8 unused1;
	int8 decay;
	int8 release;
	int8 unused2[2];
	int8 sustain;
	int8 unused3[2];
	int8 velo2attack;
	int8 unused4;
	int8 keyscale;
	int8 unused5;
	int8 onvel2rel;
	int8 offvel2rel;
	int8 unused6;
	int8 unused7;
};

/*
	00C6:	01	.

	00C7:	00	Attack (0) 0 -> 100

	00C8:	00	.

	00C9:	32	Decay (50) 0 -> 100

	00CA:	0F	Release (15) 0 -> 100

	00CB:	00	.

	00CC:	00	.

	00CD:	64	Sustain (100) 0 -> 100

	00CE:	00	.

	00CF:	00	.

	00D0:	00	Velo->Attack (0) -100 -> 100

	00D1:	00	.

	00D2:	00	Keyscale (0) -100 -> 100

	00D3:	00	.

	00D4:	00	On Vel->Rel (0) -100 -> 100

	00D5:	00	Off Vel->Rel (0) -100 -> 100

	00D6:	00	.

	00D7:	00	.
*/


struct akai_s6k_akp_zone
{		
	int8 unused1;
	int8 n_samplenamechars;
	int8 samplename[20];
	int8 unused2[12];
	int8 velocity_low;
	int8 velocity_high;
	int8 fine_tune;
	int8 semitone_tune;
	int8 filter_offset;
	int8 pan;
	int8 playmode;
	int8 output;
	int8 level;	
	int8 keytrack;
	int8 vel2LSB,vel2MSB;
};
/*
struct akai_s6k_akp_keygroup
{	
	int32 unused0;
	int8 _kloc[4];
	int8 unused16[8];
	int8 key_low;
	int8 key_high;
	int8 semitone_tune;
	int8 fine_tune;
	int8 fx_override;
	int8 pitch_mod[2];
	int8 amp_mod;
	int8 zone_xfade;	// 0 = off, 1 = on
	int8 mute_group;
	int8 unused2;
	int32 _env;
	int32 unused3;
	int8 unused4;
	int8 ae_attack;
	int8 unused5;
	int8 ae_decay;
	int8 ae_release;
	int8 unused6,unused7;
	int8 sustain;
	int8 unused8,unused9;
	int8 ae_velo2attack;
	int8 unused10;
	int8 keyscale;
};
*/
#pragma pack()

bool sampler::load_akai_s6k_program(const char *filename,char channel,bool replace)
{
	char path[256];
	const char *last;
	last = strrchr(filename,'\\');
	vtCopyString(path, filename, 256);
	path[last - filename + 1] = 0;

	HMMIO hmmio;

	hmmio = mmioOpen((LPSTR)filename, NULL, MMIO_READ|MMIO_ALLOCBUF);
	if (!hmmio){
		char msg[256];
		sprintf(msg,"file io error: File [%s] not found!",filename);
		write_log(msg);
		mmioClose(hmmio, 0);
		return false;
	}

	MMCKINFO mmckinfoParent; 

	mmckinfoParent.fccType = mmioFOURCC('A', 'P', 'R', 'G'); 
	if (mmioDescend(hmmio, (LPMMCKINFO)&mmckinfoParent, 0, MMIO_FINDRIFF))
	{
		
		write_log("file	io:	This file doesn't contain a APRG!");
		mmioClose(hmmio, 0);
		return false;
	}

	MMCKINFO mmckinfoSubchunk;   /* for finding chunks within the Group */
		
	mmckinfoSubchunk.ckid = mmioFOURCC('p', 'r', 'g', ' '); 
	if (mmioDescend(hmmio, &mmckinfoSubchunk, 0, MMIO_FINDCHUNK))
	{		
		write_log("file	io:	Required prg chunk was not found!");
		mmioClose(hmmio, 0);
		return false;
	}

	akai_s6k_akp_prg s6k_prg;
	/* Tell Windows to read in the "prg " chunk into a WAVEFORMATEX structure */
	if (mmioRead(hmmio, (HPSTR)&s6k_prg, mmckinfoSubchunk.cksize) != (LRESULT)mmckinfoSubchunk.cksize)
	{
		/* Oops! */
		write_log("file	io: error reading the prg chunk!");
		mmioClose(hmmio, 0);
		return false;
	}

	mmioAscend(hmmio, &mmckinfoSubchunk, 0);

	// so far so fine.. time to start creating the samplegroup
	char groupname[256];
	vtCopyString(groupname, last+1, 256);
	last = strrchr(groupname,'.');
	groupname[last - groupname] = 0;
	
	if(replace) part_init(channel,true,true);
	vtCopyString(parts[channel].name,groupname,32);	

	bool do_kg_xfade = (s6k_prg.KGXFade == 1);

	akai_s6k_akp_kloc s6k_kloc[256];
	
	assert(s6k_prg.n_keygroups <= 256);

	akai_s6k_akp_env s6k_ampenv[256];
	akai_s6k_akp_zone s6k_zone[256][4];
	bool zone_present[256][4];
	int n_zones[256];

	for(int k=0; k<s6k_prg.n_keygroups; k++)
	{
		// read keygroup
		mmckinfoSubchunk.ckid = mmioFOURCC('k', 'g', 'r', 'p'); 
		if (mmioDescend(hmmio, &mmckinfoSubchunk, 0, MMIO_FINDCHUNK)) 
		{			
			write_log("file	io: kgrp chunk not found!");
			mmioClose(hmmio, 0);
			return false;
		}

		mmckinfoSubchunk.ckid = mmioFOURCC('k', 'l', 'o', 'c'); 
		if (mmioDescend(hmmio, &mmckinfoSubchunk, 0, MMIO_FINDCHUNK)) 
		{			
			write_log("file	io: kloc chunk not found!");
			mmioClose(hmmio, 0);
			return false;
		}
		
		/* Tell Windows to read in the "kloc" chunk*/
		if (mmioRead(hmmio, (HPSTR)&s6k_kloc[k], mmckinfoSubchunk.cksize) != (LRESULT)mmckinfoSubchunk.cksize)
		{
			/* Oops! */
			write_log("file	io: error reading the kloc chunk!");
			mmioClose(hmmio, 0);
			return false;
		}
		mmioAscend(hmmio, &mmckinfoSubchunk, 0);

		mmckinfoSubchunk.ckid = mmioFOURCC('e', 'n', 'v', ' '); 
		if (mmioDescend(hmmio, &mmckinfoSubchunk, 0, MMIO_FINDCHUNK)) 
		{			
			write_log("file	io: 'env ' chunk not found!");
			mmioClose(hmmio, 0);
			return false;
		}
		
		/* Tell Windows to read in the "env " chunk*/
		if (mmioRead(hmmio, (HPSTR)&s6k_ampenv[k], mmckinfoSubchunk.cksize) != (LRESULT)mmckinfoSubchunk.cksize)
		{			
			write_log("file	io: error reading the 'env ' chunk!");
			mmioClose(hmmio, 0);
			return false;
		}
		mmioAscend(hmmio, &mmckinfoSubchunk, 0);
						
		n_zones[k] = 0;
		for(int z=0; z<4; z++)
		{			
			mmckinfoSubchunk.ckid = mmioFOURCC('z', 'o', 'n', 'e'); 
			if (mmioDescend(hmmio, &mmckinfoSubchunk, 0, MMIO_FINDCHUNK)) 
			{			
				write_log("file	io: zone chunk not found!");
				mmioClose(hmmio, 0);
				return false;
			}			
			
			int csize = min(mmckinfoSubchunk.cksize, sizeof(akai_s6k_akp_zone));
			if (mmioRead(hmmio, (HPSTR)&s6k_zone[k][z], csize) != (LRESULT)csize)
			{				
				write_log("file	io: error reading the zone chunk!");
				mmioClose(hmmio, 0);
				return false;
			}
			mmioAscend(hmmio, &mmckinfoSubchunk, 0);			

			zone_present[k][z] = (s6k_zone[k][z].samplename[0] != 0);
			if(zone_present[k][z]) n_zones[k] = z+1;
		}
		mmioAscend(hmmio, &mmckinfoSubchunk, 0);
	}	
	
	for(int k=0; k<s6k_prg.n_keygroups; k++)
	{
		bool do_vel_xfade = (s6k_kloc[k].zone_xfade == 1);

		int keylo_xfade = 0;
		int keyhi_xfade = 0;

		if(do_kg_xfade)	// assumes the keygroups are sorted by keyrange, do that prior to this step (nah, they're probably already sorted)
		{
			if(k>0)
			{
				keylo_xfade = limit_range(s6k_kloc[k-1].key_high + 1 - s6k_kloc[k].key_low,0,127);
			}
			if(k<(s6k_prg.n_keygroups-1))
			{
				keyhi_xfade = limit_range(s6k_kloc[k].key_high + 1 - s6k_kloc[k+1].key_low,0,127);
			}
		}

		for(int z=0; z<4; z++)
		{				
			if(zone_present[k][z])
			{
				char sample_filename[256];
				sprintf(sample_filename,"%s%s.wav",path,s6k_zone[k][z].samplename);
				int newzone;
				if (add_zone(sample_filename,&newzone,channel,true))
				{
					zones[newzone].key_low = s6k_kloc[k].key_low + (keylo_xfade>>1);
					zones[newzone].key_high = s6k_kloc[k].key_high - (keyhi_xfade - (keyhi_xfade>>1));
					zones[newzone].key_low_fade = keylo_xfade;
					zones[newzone].key_high_fade = keyhi_xfade;
					zones[newzone].keytrack = s6k_zone[k][z].keytrack?1.f:0.f;					
					
					if(do_vel_xfade && (z>0))
					{
						zones[newzone].velocity_low_fade = limit_range(s6k_zone[k][z-1].velocity_high + 1 - s6k_zone[k][z].velocity_low,0,127);
					}
					else zones[newzone].velocity_low_fade = 0;
					
					if(do_vel_xfade && (z<(n_zones[k]-1)))
					{
						zones[newzone].velocity_high_fade = limit_range(s6k_zone[k][z].velocity_high + 1 - s6k_zone[k][z+1].velocity_low,0,127);
					}
					else zones[newzone].velocity_high_fade = 0;

					zones[newzone].velocity_low = s6k_zone[k][z].velocity_low + (zones[newzone].velocity_low_fade>>1);
					zones[newzone].velocity_high = s6k_zone[k][z].velocity_high - (zones[newzone].velocity_high_fade - (zones[newzone].velocity_high_fade>>1));
					
					zones[newzone].aux[0].balance = 0.02f * s6k_zone[k][z].pan;
					zones[newzone].finetune = 0.01f * (s6k_kloc[k].fine_tune + s6k_zone[k][z].fine_tune);
					zones[newzone].AEG.attack = log2(powf(2,((float)s6k_ampenv[k].attack-64.0f)/6));
					zones[newzone].AEG.hold = -10;
					zones[newzone].AEG.decay = log2(powf(2,((float)s6k_ampenv[k].decay-64.0f)/6));
					zones[newzone].AEG.sustain = 0.01f*s6k_ampenv[k].sustain;
					zones[newzone].AEG.release = log2(powf(2,((float)s6k_ampenv[k].release-54.0f)/6));

					switch(s6k_zone[k][z].playmode)
					{
					case 0:
						zones[newzone].playmode = pm_forward;
						break;
					case 1:
						zones[newzone].playmode = pm_forward_shot;
						break;
					case 2:
						zones[newzone].playmode = pm_forward_loop;
						break;
					case 3:
						zones[newzone].playmode = pm_forward_loop_until_release;
						break;
					};					

					vtCopyString(zones[newzone].name, s6k_zone[k][z].samplename, 20);
					zones[newzone].transpose = s6k_kloc[k].semitone_tune + s6k_zone[k][z].semitone_tune;										
				}
			}
		}		
	}	
	return true;
}