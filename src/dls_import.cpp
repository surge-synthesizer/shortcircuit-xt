#include <assert.h>
#include "logfile.h"
#include "sample.h"
#include "globals.h"
#include "resampling.h"
#include "memfile.h"
#include "sf2_import.h"
#include "dls.h"
#include "sampler.h"
#include "unitconversion.h"
#include "modmatrix.h"
#include <vt_dsp/basic_dsp.h>

#pragma pack(push, 1)

struct giga_3lnk
{
	uint32 SubRegionCount;	
	struct splitdef
	{
		uint8 DimensionType;
		uint8 DimensionBitCount;
		uint8 DimensionBitStart,DimensionBitEnd;
		uint8 skip[4];
	} Split[5];	
	uint32 RegionSampleID[32];
};

struct giga_3ewa
{
	int32 uk0;
	int32 LFO3Frequency;
	int32 EG3Attack;	
	int32 LFO1InternalDepth;	
	int32 LFO3InternalDepth;	
	int32 LFO1ControlDepth;	
	int32 LFO3ControlDepth;
	int32 EG1Attack;
	int32 EG1Decay1;
	int32 EG1Sustain1;
	int32 EG1Release;
	uint8 EG1Controller;
	uint8 EG1ControllerOptions;
	uint8 EG2Controller;
	uint8 EG2ControllerOptions;
	int32 LFO1Frequency;
	int32 EG2Attack;
	int32 EG2Decay1;
	int32 EG2Sustain;
	int32 EG2Release;
	int32 LFO2ControlDepth;
	int32 LFO2Frequency;
	int32 LFO2InternalDepth;
	int32 EG1Decay2;
	int32 EG1PreAttack;
	int32 EG2Decay2;
	int32 EG2PreAttack;
	uint8 VelocityResponse;
	uint8 ReleaseVelocityResponse;
	uint8 VelocityResponseCurveScaling;
	int8 AttenuationControllerThreshold;
	uint32 uk1;
	uint16 SampleStartOffset;
	uint16 uk2;
	uint8 pitchTrackDimensionBypass;
	uint8 Pan;
	uint8 SelfMask;
	uint8 LFO3Controller;
	uint8 AttenuationController;
	uint8 LFO2Controller;
	uint8 LFO1Controller;
	uint16 eg3depth;
	uint16 uk3;
	uint8 ChannelOffset;
	uint8 regoptions;
	uint16 uk4;
	uint8 VelocityUpperLimit;
	// forsätt senare
};

#pragma pack()

bool sample::parse_dls_sample(void* data, size_t filesize, unsigned int sample_id)
{	
	size_t datasize;
	memfile mf(data,filesize);	
	if(!mf.riff_descend_RIFF_or_LIST('DLS ',&datasize)) return false;	

	off_t startpos = mf.TellI(); // store for later use
	if(!mf.riff_descend_RIFF_or_LIST('wvpl',&datasize)) return false;
	if(!mf.riff_descend_RIFF_or_LIST('wave',&datasize,sample_id)) return false;	
	void *wavedata = mf.ReadPtr(datasize);
	if(!wavedata) return false;
	return parse_riff_wave(wavedata,datasize,true);	
}

int parse_dls_patchlist(void* data, size_t filesize, void **plist)
{
	size_t datasize;
	memfile mf(data,filesize);	
	if(!mf.riff_descend_RIFF_or_LIST('DLS ',&datasize)) return 0;	
	off_t startpos = mf.TellI(); // store for later use
	if(!mf.riff_descend_RIFF_or_LIST('INFO',&datasize)) return 0;
	if(!mf.riff_descend('colh',&datasize)) return 0;
	DWORD patchcount = mf.ReadDWORD();
	mf.SeekI(startpos);
	if(!mf.riff_descend_RIFF_or_LIST('lins',&datasize)) return 0;	

	midipatch *mp = new midipatch[patchcount];
	*plist = mp;
	for(DWORD i=0; i<patchcount; i++)
	{
		if(!mf.riff_descend_RIFF_or_LIST('ins ',&datasize)) return 0;
		off_t inspos = mf.TellI();
		if(!mf.riff_descend_RIFF_or_LIST('INFO',&datasize)) return 0;
		if(!mf.riff_descend('INAM',&datasize)) return 0;
		char *s = (char*)mf.ReadPtr(datasize);
		if(s)
		{
			vtCopyString(mp[i].name,s,32);			
		}
		mf.SeekI(inspos);
		if(!mf.riff_descend('insh',&datasize)) return false;
		INSTHEADER insh;
		mf.Read(&insh,sizeof(insh));
		mp[i].PC = insh.Locale.ulInstrument;
		mp[i].bank = insh.Locale.ulBank;
	}		
	return patchcount;	
}

int get_dls_patchlist(const wchar_t *filename, void **plist)
{
	assert(filename);		

	HANDLE hf = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if(!hf) return 0;
	size_t datasize = GetFileSize(hf,NULL);

	HANDLE hmf = CreateFileMappingW(hf,0,PAGE_READONLY,0,0,0);
	if(!hmf)
	{
		CloseHandle(hf);
		return 0;
	}

	void *data = MapViewOfFile(hmf,FILE_MAP_READ,0,0,0);	

	int result = parse_dls_patchlist(data,datasize,plist);

	UnmapViewOfFile(data);
  
	CloseHandle(hmf);
	CloseHandle(hf);

	return result;
}

bool sampler::parse_dls_preset(void *data, size_t filesize, char channel, int patch, char *filename)
{
	if(patch<0) return false;	
	size_t datasize;
	memfile mf(data,filesize);	
	if(!mf.riff_descend_RIFF_or_LIST('DLS ',&datasize)) return false;	
	off_t startpos = mf.TellI(); // store for later use	
	if(!mf.riff_descend_RIFF_or_LIST('lins',&datasize)) return false;	
	if(!mf.riff_descend_RIFF_or_LIST('ins ',&datasize,patch)) return false;	
	off_t startins = mf.TellI(); // store for later use	
	if(!mf.riff_descend('insh',&datasize)) return false;
	INSTHEADER insh;
	mf.Read(&insh,sizeof(insh));
	mf.SeekI(startins);
	if(!mf.riff_descend_RIFF_or_LIST('lrgn',&datasize)) return false;
	size_t startrgn = mf.TellI();
	
	// ok so far, init part
	part_init(channel,true,true);
	
	for(unsigned int i=0; i<insh.cRegions; i++)
	{				
		if(!mf.riff_descend_RIFF_or_LIST('rgn ',&datasize)) return false;
		size_t thisrgn = mf.TellI();
		size_t next = mf.TellI() + datasize;				
		RGNHEADER rgnh;
		if(!mf.riff_descend('rgnh',&datasize)) return false;
		if(!mf.Read(&rgnh,sizeof(rgnh))) return false;
		mf.SeekI(thisrgn);
		WSMPL wsmp;
		if(!mf.riff_descend('wsmp',&datasize)) return false;
		if(!mf.Read(&wsmp,sizeof(wsmp))) return false;
		mf.SeekI(thisrgn);
		WAVELINK wlnk;
		if(!mf.riff_descend('wlnk',&datasize)) return false;
		if(!mf.Read(&wlnk,sizeof(wlnk))) return false;

		mf.SeekI(thisrgn);
		giga_3lnk _3lnk;
		if(mf.riff_descend('3lnk',&datasize))
		{
			// sub-regions, do the giga thing
			if(!mf.Read(&_3lnk,sizeof(_3lnk))) return false;
			mf.SeekI(thisrgn);
			if(!mf.riff_descend_RIFF_or_LIST('3prg',&datasize)) return false;
			for(unsigned int j=0; j<_3lnk.SubRegionCount; j++)
			{				
				if(!mf.riff_descend_RIFF_or_LIST('3ewl',&datasize)) return false;
				size_t nextewl = mf.TellI() + datasize;
				size_t thisewl = mf.TellI();
								
				if(!mf.riff_descend('wsmp',&datasize)) return false;
				if(!mf.Read(&wsmp,sizeof(wsmp))) return false;
				
				mf.SeekI(thisewl);
				giga_3ewa _3ewa;
				if(!mf.riff_descend('3ewa',&datasize)) return false;
				if(!mf.Read(&_3ewa,sizeof(_3ewa))) return false;
				{
					bool do_load=true;
					struct 
					{
						unsigned int low, high, type, i;
					} dims[5];

					for(unsigned int k=0; k<5; k++)
					{
						dims[k].type = _3lnk.Split[k].DimensionType;
						unsigned int e = j >> _3lnk.Split[k].DimensionBitStart;
						unsigned int n = 1<<_3lnk.Split[k].DimensionBitCount;
						e = e & (n-1);
						dims[k].i = e;
						unsigned int stepsize = 0x80 >> _3lnk.Split[k].DimensionBitCount;
						dims[k].low = limit_range(e*stepsize,0U,127U);
						dims[k].high = limit_range((e+1)*stepsize - 1,0U,127U);
						if((dims[k].type == 0x80)&&(e>0)) do_load = false;
					}
					
					char fn[256];
					int newzone;					
					sprintf(fn,"%s|%i",filename,_3lnk.RegionSampleID[j]);
					if(do_load && add_zone(fn,&newzone,channel))
					{
						sample_zone *z = &zones[newzone];						
						z->key_low = rgnh.RangeKey.usLow;
						z->key_high = rgnh.RangeKey.usHigh;
						z->velocity_low = rgnh.RangeVelocity.usLow;
						z->velocity_high = rgnh.RangeVelocity.usHigh;
						z->mute_group = rgnh.usKeyGroup;
						z->key_root = wsmp.usUnityNote;
						z->AEG.attack = timecent_to_envtime_GIGA(_3ewa.EG1Attack);
						z->AEG.decay = timecent_to_envtime_GIGA(_3ewa.EG1Decay1);
						z->AEG.release = timecent_to_envtime_GIGA(_3ewa.EG1Release);
						z->AEG.sustain = 0.001f * ((_3ewa.EG1Sustain1>>16)&0xffff);
						//_3ewa.
						if(_3ewa.pitchTrackDimensionBypass & 1) z->keytrack = 0.f;

						unsigned int ncid = 0;
						for(unsigned int k=0; k<5; k++)
						{
							if(!_3lnk.Split[k].DimensionBitCount) break;							

							unsigned ncsource = 0;
							switch(_3lnk.Split[k].DimensionType)
							{
							case 0x80:	// dimension_samplechannel
								break;
							case 0x81:	// dimension_layer
								break;
							case 0x82:
								z->velocity_low = dims[k].low;
								z->velocity_high = dims[k].high;								
								break;
							case 0x83:
								ncsource = get_mm_source_id("channelAT");
								break;
							case 0x84:	// dimension_releasetrigger
								break;
							case 0x85:	// dimension_keyboard
								break;
							case 0x86:	// dimension_roundrobin
								break;
							case 0x87:	// dimension_random
								break;
							case 0x01:
								ncsource = get_mm_source_id("modwheel");
								break;
							/*case 0x04:
								ncsource = get_mm_source_id("footpedal");
								break;*/
							case 0x43:	// dimension_softpedal (MIDI Controller 67)
								break;
							case 0x10:
								ncsource = get_mm_source_id("c1");
								break;								
							case 0x11:
								ncsource = get_mm_source_id("c2");
								break;						
							case 0x12:
								ncsource = get_mm_source_id("c3");
								break;								
							case 0x13:
								ncsource = get_mm_source_id("c4");
								break;						
							case 0x30:
								ncsource = get_mm_source_id("c5");
								break;								
							case 0x31:
								ncsource = get_mm_source_id("c6");
								break;						
							case 0x32:
								ncsource = get_mm_source_id("c7");
								break;								
							case 0x33:
								ncsource = get_mm_source_id("c8");
								break;						
							};
							if(ncsource)
							{
								if(ncid<nc_entries)
								{
									z->nc[ncid].source = ncsource;
									z->nc[ncid].low = dims[k].low;
									z->nc[ncid].high = dims[k].high;
									ncid++;
								}								
							}
						}
					}
				}				
				mf.SeekI(nextewl);
			}			
		}
		else
		{
			// no sub-regions, read as normal DLS
			char fn[256];
			int newzone;					
			sprintf(fn,"%s|%i",filename,wlnk.ulTableIndex);
			if(add_zone(fn,&newzone,channel))
			{
				sample_zone *z = &zones[newzone];
				z->key_low = rgnh.RangeKey.usLow;
				z->key_high = rgnh.RangeKey.usHigh;
				z->velocity_low = rgnh.RangeVelocity.usLow;
				z->velocity_high = rgnh.RangeVelocity.usHigh;
				z->mute_group = rgnh.usKeyGroup;
				z->key_root = wsmp.usUnityNote;
			}
		}
		mf.SeekI(next);
	}
	return true;	
}