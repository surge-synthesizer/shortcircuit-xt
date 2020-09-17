//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "sample.h"
#include "resampling.h"
#include <assert.h>
#include <vt_dsp/endian.h>
#include "configuration.h"
#include <windows.h>
#include <mmreg.h>

sample::sample(configuration *conf)
{
	refcount = 1;		
	this->conf = conf;	
	
	// zero pointers	
	SampleData[0] = 0;
	SampleData[1] = 0;
	meta.slice_start = 0;
	meta.slice_end = 0;	
	graintable = 0;	
	Embedded = true;
	UseInt16 = false;
	
	clear_data();
}

short* sample::GetSamplePtrI16(int Channel)
{
	if(!UseInt16) return 0;
	return &((short*)SampleData[Channel])[FIRoffset];
}
float* sample::GetSamplePtrF32(int Channel)
{
	if(UseInt16) return 0;
	return &((float*)SampleData[Channel])[FIRoffset];
}

bool sample::AllocateI16(int Channel, int Samples)
{
	//int samplesizewithmargin = Samples + 2*FIRipol_N + block_size + FIRoffset;
	int samplesizewithmargin = Samples + FIRipol_N;
	if(SampleData[Channel]) delete SampleData[Channel];
	SampleData[Channel] = new short[samplesizewithmargin];
	if(!SampleData[Channel]) return false;
	UseInt16 = true;
	
	// clear pre/post zero area
	memset(SampleData[Channel],0,FIRoffset*sizeof(short));
	memset((char*)SampleData[Channel] + (Samples+FIRoffset)*sizeof(short),0,FIRoffset*sizeof(short));

	return true;
}
bool sample::AllocateF32(int Channel, int Samples)
{	
	int samplesizewithmargin = Samples + FIRipol_N;
	if(SampleData[Channel]) delete SampleData[Channel];
	SampleData[Channel] = new float[samplesizewithmargin];
	if(!SampleData[Channel]) return false;
	UseInt16 = false;
	
	// clear pre/post zero area
	memset(SampleData[Channel],0,FIRoffset*sizeof(float));
	memset((char*)SampleData[Channel] + (Samples+FIRoffset)*sizeof(float),0,FIRoffset*sizeof(float));

	return true;
}

int sample::GetRefCount()
{
	return refcount;
}
size_t sample::GetDataSize()
{
	return sample_length * (UseInt16?2:4) * channels;
}
char* sample::GetName()
{
	return name;
	// TODO samplingar behöver numera namn!
	// TODO som även ska sparas i RIFFdata (använd wav's metastruktur)
}

void sample::clear_data()
{
	sample_loaded = false;
	grains_initialized = false;		
	
	// free any allocated data	
	if(SampleData[0]) delete SampleData[0];
	if(SampleData[1]) delete SampleData[1];	
	if(meta.slice_start) delete meta.slice_start;
	if(meta.slice_end) delete meta.slice_end;
	if(graintable) delete graintable;

	// and zero their pointers
	SampleData[0] = 0;	
	SampleData[1] = 0;	
	meta.slice_start = 0;
	meta.slice_end = 0;
	graintable = 0;

	memset(name,0,64);
	memset(&meta,0,sizeof(meta));	
	filename[0] = 0;
}

sample::~sample()
{
	clear_data(); // this should free everything
}

bool sample::load(const char *filename)
{
	assert(filename);
	wchar_t wfilename[pathlength];
	int result = MultiByteToWideChar(CP_UTF8,0,filename,-1,wfilename,1024);
	if(!result) return false;
	bool r = load(wfilename);
	//if(r) vtCopyString(this->filename,filename,256);
	return r;
}

bool sample::SetMeta(unsigned int Channels, unsigned int SampleRate, unsigned int SampleLength)
{
	if (Channels>2) return false;		// not supported
	
	channels = Channels;
	sample_length = SampleLength;
	sample_rate = SampleRate;
	InvSampleRate = 1.f / (float)SampleRate;
	
	
	return true;
}

bool sample::load(const wchar_t *filename)
{
	assert(conf);
	assert(filename);
	wchar_t filename_decoded[pathlength],extension[64];	
	int sample_id,program_id;
	conf->decode_pathW(filename,filename_decoded,extension,&program_id,&sample_id);

	HANDLE hf = CreateFileW(filename_decoded, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if(!hf) return false;
	size_t datasize = GetFileSize(hf,NULL);

	HANDLE hmf = CreateFileMappingW(hf,0,PAGE_READONLY,0,0,0);
	if(!hmf)
	{
		CloseHandle(hf);
		return false;
	}

	void *data = MapViewOfFile(hmf,FILE_MAP_READ,0,0,0);	

	clear_data();	// clear to a more predictable state
	
	const wchar_t *sname = wcsrchr(filename,L'\\');	
	if(sname)
	{
		sname++;
		int length = wcsrchr(sname,'.') - sname;
		if(length > 0) WideCharToMultiByte(CP_UTF8,0,sname,length,name,64,0,0);			
	}


	bool r = false;
	if(wcsicmp(extension,L"wav") == 0) 
	{
		r = parse_riff_wave(data,datasize);
	}
	else if(wcsicmp(extension,L"sf2") == 0) 
	{
		r = parse_sf2_sample(data,datasize,sample_id);	
	}
	else if((wcsicmp(extension,L"dls") == 0)||(wcsicmp(extension,L"gig") == 0))
	{
		r = parse_dls_sample(data,datasize,sample_id);		
	}	
	else if((wcsicmp(extension,L"aif") == 0)||(wcsicmp(extension,L"aiff") == 0))
	{
		r = parse_aiff(data,datasize);	
	}

	if (r)
	{
		assert(SampleData[0]);
		if(channels==2) assert(SampleData[1]);	
	}
		
	UnmapViewOfFile(data);
  
	CloseHandle(hmf);
	CloseHandle(hf);
	if(r) wcsncpy(this->filename,filename,pathlength);
	
	if (!r)
	{
		wchar_t tmp[512];
		swprintf(tmp, 512, L"Could not read file %s", filename);
		MessageBoxW(::GetActiveWindow(), tmp, L"File I/O Error", MB_OK | MB_ICONERROR);
	}

	return r;
}

void sample::init_grains()
{
	return;	// disabled atm

	// TODO inkompatible med short
	/*

	if (!grains_initialized)
	{
		const int max_grains = 65536;
		const int grainsize = 200;
		uint32 temp_grains[max_grains];

		num_grains = 0;
		uint32 samplepos = 1;
		while(num_grains<max_grains)
		{
			if(samplepos > sample_length)
				break;

			// check if upwards zc has occured
			if ((sample_data[samplepos+FIRoffset]>0) && (sample_data[samplepos+FIRoffset-1]<0))
			{
				temp_grains[num_grains++] = samplepos;
				samplepos += grainsize;
			} 
			else 
			{
				samplepos++;
			}
		}
		num_grains -= 1;
		if (num_grains<1) return;
		int i;
		graintable = new uint32[num_grains];
		for(i=0; i<num_grains; i++)
		{
			graintable[i] = temp_grains[i];
		}
		num_grains -= 1;	// number of total loops instead of sample locations
		grains_initialized = true;
	}*/
}

bool sample::get_filename(char* utf8name)
{	
	int result = WideCharToMultiByte(CP_UTF8,0,filename,-1,utf8name,256,0,0);				
	if(!result) return false;
	return true;
}

bool sample::compare_filename(const char* utf8name)
{
	assert(filename);
	wchar_t wfilename[pathlength];
	int result = MultiByteToWideChar(CP_UTF8,0,utf8name,-1,wfilename,1024);
	if(!result) return false;
	return (wcscmp(filename,wfilename) == 0);
}

bool sample::load_data_ui8(int channel, void* data, unsigned int samplesize, unsigned int stride)
{	
	AllocateI16(channel,samplesize);
	short *sampledata = GetSamplePtrI16(channel);

	for(int i=0; i<samplesize; i++)
	{		
		sampledata[i] = (((short)*((unsigned char*)data + i*stride)) - 128) << 8;
	}	
	return true;	
}

bool sample::load_data_i8(int channel, void* data, unsigned int samplesize, unsigned int stride)
{	
	AllocateI16(channel,samplesize);
	short *sampledata = GetSamplePtrI16(channel);

	for(int i=0; i<samplesize; i++)
	{		
		sampledata[i] = ((short)*((char*)data + i*stride)) << 8;
	}	
	return true;
}

bool sample::load_data_i16(int channel, void* data, unsigned int samplesize, unsigned int stride)
{
	AllocateI16(channel,samplesize);
	short *sampledata = GetSamplePtrI16(channel);

	for(int i=0; i<samplesize; i++)
	{
		sampledata[i] = vt_read_int16LE(*(short*)((char*)data + i*stride));
	}
	return true;
}

bool sample::load_data_i16BE(int channel, void* data, unsigned int samplesize, unsigned int stride)
{
	AllocateI16(channel,samplesize);
	short *sampledata = GetSamplePtrI16(channel);

	for(int i=0; i<samplesize; i++)
	{		
		sampledata[i] = vt_read_int16BE(*(short*)((char*)data + i*stride));
	}	
	return true;	
}
bool sample::load_data_i32(int channel, void* data, unsigned int samplesize, unsigned int stride)
{	
	AllocateF32(channel,samplesize);
	float *sampledata = GetSamplePtrF32(channel);

	for(int i=0; i<samplesize; i++)
	{		
		int x = vt_read_int32LE(*(int*)((char*)data + i*stride));
		sampledata[i] = (4.6566128730772E-10f) * (float)x;		
	}	
	return true;
}

bool sample::load_data_i32BE(int channel, void* data, unsigned int samplesize, unsigned int stride)
{	
	AllocateF32(channel,samplesize);
	float *sampledata = GetSamplePtrF32(channel);

	for(int i=0; i<samplesize; i++)
	{				
		int x = vt_read_int32BE(*(int*)((char*)data + i*stride));
		sampledata[i] = (4.6566128730772E-10f) * (float)x;
	}	
	return true;
}

bool sample::load_data_i24(int channel, void* data, unsigned int samplesize, unsigned int stride)
{	
	AllocateF32(channel,samplesize);
	float *sampledata = GetSamplePtrF32(channel);

	for(int i=0; i<samplesize; i++)
	{		
		unsigned char *cval = (unsigned char*)data + i*stride;
		int value = (cval[2] << 16) | (cval[1] << 8) | cval[0];
		value -= (value & 0x800000)<<1;	
		sampledata[i] = 0.00000011920928955078f*float(value);;
	}	
	return true;
}

bool sample::load_data_i24BE(int channel, void* data, unsigned int samplesize, unsigned int stride)
{	
	AllocateF32(channel,samplesize);
	float *sampledata = GetSamplePtrF32(channel);

	for(int i=0; i<samplesize; i++)
	{		
		unsigned char *cval = (unsigned char*)data + i*stride;
		int value = (cval[0] << 16) | (cval[1] << 8) | cval[2];
		value -= (value & 0x800000)<<1;	
		sampledata[i] = 0.00000011920928955078f*float(value);;
	}	
	return true;
}



bool sample::load_data_f32(int channel, void* data, unsigned int samplesize, unsigned int stride)
{	
	AllocateF32(channel,samplesize);
	float *sampledata = GetSamplePtrF32(channel);

	for(int i=0; i<samplesize; i++)
	{		
		sampledata[i] = (*(float*)((char*)data + i*stride));
	}	
	return true;
}

bool sample::load_data_f64(int channel, void* data, unsigned int samplesize, unsigned int stride)
{	
	AllocateF32(channel,samplesize);
	float *sampledata = GetSamplePtrF32(channel);

	for(int i=0; i<samplesize; i++)
	{		
		sampledata[i] = (float) (*(double*)((char*)data + i*stride));
	}	
	return true;
}