//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "globals.h"

class configuration;

class sample
{
public:
	/// constructor
	sample(configuration *conf);
	/// deconstructor
	virtual ~sample();	
	bool load(const char *filename);
	bool load(const wchar_t *filename);
	bool get_filename(char*);
	bool compare_filename(const char*);
	bool parse_riff_wave(void* data, size_t filesize, bool skip_riffchunk=false);
	short* GetSamplePtrI16(int Channel);
	float* GetSamplePtrF32(int Channel);
	int GetRefCount();
	size_t GetDataSize();
	char* GetName();
private:			
	bool parse_aiff(void* data, size_t filesize);
	bool parse_sf2_sample(void* data, size_t filesize, unsigned int sampleid);
	bool parse_dls_sample(void* data, size_t filesize, unsigned int sampleid);
	//bool load_recycle(const char *filename);
	configuration *conf;
public:
	size_t SaveWaveChunk(void* data);
	bool save_wave_file(const char * filename);

	// public data	
	void* __restrict SampleData[2];	
	bool UseInt16;
	uint8 channels;
	bool Embedded;	// if true, sample data will be stored inside the patch/multi
	uint32 sample_length;
	uint32 sample_rate;
	float InvSampleRate;
	uint32 *graintable;
	int num_grains;
	bool grains_initialized;	
	void init_grains();
	char name[64];

	void remember(){ refcount++; }
	bool forget(){ refcount--; return (!refcount); }

	// meta data		
	struct {
		char key_low,key_high,key_root;
		char vel_low,vel_high;		
		char playmode;
		float detune;
		unsigned int loop_start,loop_end;
		bool rootkey_present,key_present,vel_present,loop_present,playmode_present;
		int n_slices;
		int *slice_start;
		int *slice_end;
		int n_beats;
	} meta;

private:	
	void clear_data();	// deletes sample data and marks sample as unused
	
	bool AllocateI16(int Channel, int Samples);
	bool AllocateF32(int Channel, int Samples);

	bool SetMeta(unsigned int channels, unsigned int SampleRate, unsigned int SampleLength);
	bool load_data_ui8 (int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_i8 (int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_i16(int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_i16BE(int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_i24(int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_i24BE(int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_i32(int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_i32BE(int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_f32(int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool load_data_f64(int channel, void* data, unsigned int samplesize, unsigned int stride);
	bool sample_loaded;	
	wchar_t filename[pathlength];
	uint32 refcount;
};