#include <windows.h>
#include <assert.h>
#include <mmreg.h>
#include "sf2.h"
#include "logfile.h"
#include "sample.h"
#include "globals.h"
#include "resampling.h"
#include "memfile.h"

bool sample::parse_sf2_sample(void* data, size_t filesize, unsigned int sample_id)
{	
	size_t datasize;
	memfile mf(data,filesize);	
	if(!mf.riff_descend_RIFF_or_LIST('sfbk',&datasize)) return false;	

	off_t startpos = mf.TellI(); // store for later use
	if(!mf.riff_descend_RIFF_or_LIST('pdta',&datasize)) return false;	
	
	sf2_Sample sampledata;
	if(!mf.riff_descend('shdr',&datasize)) return false;
	off_t shdrstartpos = mf.TellI(); // store for later use

	int sample_count = (datasize/46) - 1;
	
	if(sample_id >= sample_count) return false;

	mf.SeekI(sample_id*46,mf_FromCurrent);
	mf.Read(&sampledata,46);
	vtCopyString(name,sampledata.achSampleName,20);

	mf.SeekI(startpos);
	if(!mf.riff_descend_RIFF_or_LIST('sdta',&datasize)) return false;

	off_t sdtastartpos = mf.TellI();	// store for later use
	mf.SeekI(sampledata.dwStart*2 + 8,mf_FromCurrent);
	int samplesize = sampledata.dwEnd - sampledata.dwStart;
	int16 *loaddata = (int16*)mf.ReadPtr(samplesize*sizeof(int16));	
		
	/* check for linked channel */	
	sf2_Sample sampledataL;
	int16 *loaddataL = 0;
	channels = 1;
	if((sampledata.sfSampleType == rightSample)&&(sampledata.wSampleLink < sample_count))
	{		
		mf.SeekI(shdrstartpos + 46*sampledata.wSampleLink);
		mf.Read(&sampledataL,46);
		mf.SeekI(sdtastartpos + sampledataL.dwStart*2 + 8);
		loaddataL = (int16*)mf.ReadPtr(samplesize*sizeof(int16));		
		channels = 2;
	}
		
	if(channels==2)
	{
		load_data_i16(0,loaddataL,samplesize,2);
		load_data_i16(1,loaddata,samplesize,2);
	}
	else
	{
		load_data_i16(0,loaddata,samplesize,2);		
	}

	this->sample_loaded = true;		

	if(!SetMeta(channels,sampledata.dwSampleRate,samplesize)) return false;
	
	meta.loop_present = true;
	meta.loop_start = sampledata.dwStartloop - sampledata.dwStart;
	meta.loop_end	= sampledata.dwEndloop - sampledata.dwStart;	
	return true;
}
