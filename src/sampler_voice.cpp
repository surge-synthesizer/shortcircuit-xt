//-------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------

#include "sampler_voice.h"
#include "tools.h"
#include <fstream>
#include "controllers.h"
#include "sampler.h"
#include <intrin.h>
#include <assert.h>
#include "mathtables.h"
#include "generator.h"
#include "filter.h"
#include "sampler_state.h"
#include <vt_dsp/halfratefilter.h>
#include <vt_dsp/basic_dsp.h>
#include "sample.h"

Align64 float	SincTableF32[(FIRipol_M+1)*FIRipol_N];
Align64 float	SincOffsetF32[(FIRipol_M)*FIRipol_N];
Align64 short	SincTableI16[(FIRipol_M+1)*FIRipolI16_N];
Align64 short SincOffsetI16[(FIRipol_M)*FIRipolI16_N];

float table_dB[512],table_pitch[512],table_envrate_lpf[512],table_envrate_linear[512];
float waveshapers[8][1024]; // typ?

bool	sinc_initialized = false;

sampler_voice::sampler_voice(uint32 voice_id, timedata *td)
{	
	halfrate = (halfrate_stereo*) _mm_malloc(sizeof(halfrate_stereo),16);	
	new(halfrate) halfrate_stereo(4,false);

	voice_filter[0] = 0;
	voice_filter[1] = 0;
	
	this->voice_id = voice_id;	
	this->td = td;

	// create filters if first instance of class
	if(!sinc_initialized)
	{
		float cutoff = 0.95f; 
		float cutoffI16 = 0.95f; 
		int j;
		for(j=0; j<FIRipol_M+1; j++){
			for(int i=0; i<FIRipol_N; i++){				
				double t = -double(i) + double(FIRipol_N/2.0) + double(j)/double(FIRipol_M) - 1.0;									
				double val = (float)(SymmetricKaiser(t, FIRipol_N, 5.0)*cutoff*sincf(cutoff*t));

				SincTableF32[j*FIRipol_N + i] = val;
			}
		}
		for(j=0; j<FIRipol_M; j++){
			for(int i=0; i<FIRipol_N; i++){								
				SincOffsetF32[j*FIRipol_N + i] = (float)( (SincTableF32[(j+1)*FIRipol_N + i] - SincTableF32[j*FIRipol_N + i]) * (1.0/65536.0) );
			}
		}
				
		for(j=0; j<FIRipol_M+1; j++){
			for(int i=0; i<FIRipolI16_N; i++){				
				double t = -double(i) + double(FIRipolI16_N/2.0) + double(j)/double(FIRipol_M) - 1.0;									
				double val = (float)(SymmetricKaiser(t, FIRipol_N, 5.0)*cutoffI16*sincf(cutoffI16*t));

				SincTableI16[j*FIRipolI16_N + i] = val*16384;
			}
		}
		for(j=0; j<FIRipol_M; j++){
			for(int i=0; i<FIRipolI16_N; i++){								
				SincOffsetI16[j*FIRipolI16_N + i] = (SincTableI16[(j+1)*FIRipolI16_N + i] - SincTableI16[j*FIRipolI16_N + i]);
			}
		}
		sinc_initialized = true;		
	}
	
	faderL.set_blocksize(block_size);
	faderR.set_blocksize(block_size);
	vca.set_blocksize(block_size);	
	pfg.set_blocksize(block_size);	
	aux1L.set_blocksize(block_size);
	aux1R.set_blocksize(block_size);
	aux2L.set_blocksize(block_size);
	aux2R.set_blocksize(block_size);
	fmix1.set_blocksize(block_size);
	fmix2.set_blocksize(block_size);	
}

sampler_voice::~sampler_voice()
{	
	spawn_filter_release(voice_filter[0]);
	voice_filter[0] = 0;
	spawn_filter_release(voice_filter[1]);		
	voice_filter[1] = 0;

	halfrate->~halfrate_stereo();
	_mm_free(halfrate);	
}

void sampler_voice::play(sample *wave, sample_zone *zone, sample_part *part, uint32 key, uint32 velocity, int detune, float *ctrl, float *autom, float crossfade_amp)
{
	this->zone = zone;
	this->part = part;
	this->wave = wave;

	this->key = key;
	this->detune = detune;
	fkey = (float)key + 0.01f*detune;
	portamento_active = false;
	if(part->polymode == polymode_legato) portasrc_key = fkey;
	else 
	{		
		portasrc_key = part->last_note;		
		
		portamento_active = true;
	}
	
	part->last_note = key;

	this->velocity = velocity;	
	this->channel = channel;
	this->ctrl = ctrl;
	this->automation = autom;	
	
	lag[0] = 0;
	lag[1] = 0;
	portaphase = 0;
	
	playmode = zone->playmode;
	fvelocity = velocity / 127.0f;	
	alternate = zone->alternate?1.0f:-1.0f;
	zone->alternate = (zone->alternate+1)&1;
	keytrack = (key-60.0f) * (1.f/12.0f);
	gate = true;
	loop_gate = 0.0f;
	is_uberrelease = false;
	invert_polarity = false;	
	fgate = 1.0f;
	time = 0;
	slice_env = 0;
	time60 = 0;
	loop_pos = 0;
	random = (float) rand()/RAND_MAX;
	randombp = ((float) rand()/RAND_MAX)*2.f - 1.f;
	envelope_follower = 0;
	grain_id = 0;

	//GDIO.fReleaseCallback = uberrelease;
	GDIO.OutputL = output[0];
	GDIO.OutputR = output[1];	
	GDIO.SampleDataL = wave->SampleData[0];
	GDIO.SampleDataR = wave->SampleData[1];
	assert(wave);
	assert(GDIO.SampleDataL);
	GDIO.VoicePtr = this;
	GDIO.WaveSize = wave->sample_length;

	this->crossfade_amp = crossfade_amp;

	halfrate->reset();

	mm.assign(0,zone,part,this,ctrl,autom,td);
	mm.process();

	first_run = true;

	AEG.Assign(	mm.get_destination_ptr(md_AEG_a),
					mm.get_destination_ptr(md_AEG_h),
					mm.get_destination_ptr(md_AEG_d),
					mm.get_destination_ptr(md_AEG_s),
					mm.get_destination_ptr(md_AEG_r),
					zone->AEG.shape);

	EG2.Assign(	mm.get_destination_ptr(md_EG2_a),
					mm.get_destination_ptr(md_EG2_h),
					mm.get_destination_ptr(md_EG2_d),
					mm.get_destination_ptr(md_EG2_s),
					mm.get_destination_ptr(md_EG2_r),
					zone->EG2.shape);

	stepLFO[0].assign(&zone->LFO[0],mm.get_destination_ptr(md_LFO1_rate),td);
	stepLFO[1].assign(&zone->LFO[1],mm.get_destination_ptr(md_LFO2_rate),td);
	stepLFO[2].assign(&zone->LFO[2],mm.get_destination_ptr(md_LFO3_rate),td);
	
	AEG.Attack();
	EG2.Attack();
	
	mm.process();

	lag[0] = mm.get_destination_value(md_lag0);
	lag[1] = mm.get_destination_value(md_lag1);

	spawn_filter_release(voice_filter[0]);	
	spawn_filter_release(voice_filter[1]);	

	last_ft[0] = 0; 
	last_ft[1] = 0; 
	voice_filter[0] = 0;
	voice_filter[1] = 0;

	check_filtertypes();

	RingOut = 0;
	if (voice_filter[0]) RingOut += voice_filter[0]->tail_length();
	if (voice_filter[1]) RingOut += voice_filter[1]->tail_length();

	if(playmode == pm_forward_shot)
	{
		// shot mode wont release AEG, so the sample end MUST stop it!
		RingOut = min(RingOut, 5000);
	}

	GD.SamplePos = (int) mm.get_destination_value(md_sample_start);	
	GD.SamplePos = limit_range(GD.SamplePos,0,(int)zone->sample_stop);	

	last_loopstart = mm.get_destination_value(md_loop_start);
	last_loopend = mm.get_destination_value(md_loop_length) + mm.get_destination_value(md_loop_start);
	GD.SampleSubPos = 0;	
	GD.LowerBound = zone->sample_start;
	GD.UpperBound = zone->sample_stop;		
	GD.Direction = 1;
	GD.IsFinished = 0;	

	if(zone->reverse)
	{
		GD.SamplePos = zone->sample_stop;
		GD.Direction = -1;
	}	
	
	finished = false;	

	slice_end = &zone->sample_stop;	// the same mechanism is used to stop the sample even when slices are not used
	slice_id = 0;
	slice_env = zone->hp[0].env;
	end_offset = 0;	

	if(playmode == pm_forward_hitpoints)
	{
		end_offset = ((unsigned int)samplerate) >> 7;		// 
		int sp = key - zone->key_root;
		if((sp >= 0)&&(sp < zone->n_hitpoints)&&(!zone->hp[sp].muted))
		{
			slice_env = zone->hp[sp].env;						
			
			GD.LowerBound = zone->hp[sp].start_sample;
			GD.UpperBound = zone->hp[sp].end_sample;
			
			GD.SamplePos = zone->reverse ? GD.UpperBound : GD.LowerBound;
			//slice_end = &zone->hp[sp].end_sample;																
			slice_id = sp;
		}
		else
			finished = true;
	}

	looping_active = ((playmode == pm_forward_loop)||((playmode == pm_forward_loop_until_release) && gate)||(playmode == pm_forward_loop_bidirectional));	

	// determine whether to use oversampling
	CalcRatio();
	if(GD.Ratio < 0) GD.SamplePos = zone->sample_stop;
	//use_oversampling = resample_ratio > 16777216;
	use_oversampling = abs(GD.Ratio) > 18000000;
	use_stereo = (wave->channels==2);

	GD.BlockSize = use_oversampling ? (block_size*2) : block_size;

	if(use_oversampling)
	{		
		pfg.set_blocksize(block_size*2);
	}
	else
	{
		pfg.set_blocksize(block_size);
	}	

	
	// choose generator function pointer
	Generator = 0;

	int gmode = 0;
	switch(playmode)
	{
	case pm_forward:
	case pm_forward_hitpoints:
		gmode = 0;
		break;
	case pm_forward_loop:
	case pm_forward_loop_until_release:		// TODO specialize
		gmode = 1;
		break;
	case pm_forward_loop_bidirectional:
		gmode = 2;
		break;
	case pm_forward_release:
	case pm_forward_shot:
		gmode = 3;
		break;
	}	

	Generator = GetFPtrGeneratorSample(use_stereo,!wave->UseInt16,gmode);	
	
	assert(Generator);
}

void sampler_voice::change_key(int key,int vel, int detune)
{
	if(portaphase>1)
		portasrc_key = this->key + 0.01f*this->detune;
	else
	{
		portasrc_key = ((1-portaphase)*portasrc_key + portaphase*(this->key + 0.01f*this->detune));
	}
	
	this->key = key;	
	this->detune = detune;
	zone->last_note = key;
	portaphase = 0;
	portamento_active = true;
}

void sampler_voice::release(uint32 velocity)
{
	if (gate)
	{
		if (playmode != pm_forward_shot)
		{	
			AEG.Release();
			EG2.Release();
		}
		
		if (playmode == pm_forward_loop_until_release)
		{
			loop_gate = 0.0f;
		}
		
		gate = false;
		fgate = 0.0f;
	}
}

void sampler_voice::uberrelease()
{
	AEG.UberRelease();	
	gate = false;
	fgate = 0.0f;	
	is_uberrelease = true;
}

__forceinline int sampler_voice::get_filter_type(int id)
{	
	return zone->Filter[id].type;
}

void sampler_voice::check_filtertypes()
{
	//	have the filtertypes changed?
	if(last_ft[0] != get_filter_type(0))
	{
		spawn_filter_release(voice_filter[0]);
		voice_filter[0] = spawn_filter(get_filter_type(0),mm.get_destination_ptr(md_filter1prm0),zone->Filter[0].ip,0,true);		
		if(voice_filter[0]) voice_filter[0]->init();
		last_ft[0] = get_filter_type(0);
	}	
	if(last_ft[1] != get_filter_type(1))
	{
		spawn_filter_release(voice_filter[1]);	
		voice_filter[1] = spawn_filter(get_filter_type(1),mm.get_destination_ptr(md_filter2prm0),zone->Filter[1].ip,0,true);		
		if(voice_filter[1]) voice_filter[1]->init();
		last_ft[1] = get_filter_type(1);
	}
}

void sampler_voice::update_lag_gen(int id)
{
	// lag generators
	const float integratorconst = samplerate_inv * block_size;	
	if (zone->lag_generator[id] != 0)
	{
		float x = pi1 * note_to_pitch(-12*zone->lag_generator[id]) * integratorconst;
		float tm = 2-cos(x);
		float p = tm - sqrt(tm*tm - 1);
		lag[id] = mm.get_destination_value(md_lag0) * (1-p) + p * lag[id];
	}
	else
		lag[id] += integratorconst*mm.get_destination_value(md_lag0+id);		
}

void sampler_voice::update_portamento()
{
	float ratemult=1.f;
	if(part->portamento_mode) ratemult = 12.f/(0.00001f+fabs(((float)key+0.01f*detune)-portasrc_key));
	portaphase += block_size * note_to_pitch(-12.f*part->portamento) * samplerate_inv*ratemult;
	if(portaphase<1.f)
	{
		fkey = (1.f-portaphase)*portasrc_key + (float)portaphase*(key + 0.01f*detune);
	}
	else 
	{
		fkey = (float)key + 0.01f*detune;
		portamento_active = false;
	}
}

#define perfprof 0

#if perfprof
#define perfslot(x) perf[x] = __rdtsc();
#else 
#define perfslot(x) {}
#endif

void sampler_voice::CalcRatio()
{
	keytrack = (fkey + zone->pitch_bend_depth*ctrl[c_pitch_bend] - 60.0f) * (1/12.0f);		// keytrack as modulation source
	float kt = (playmode == pm_forward_hitpoints)?0:((fkey - zone->key_root) * zone->keytrack);
	
	fpitch = part->transpose + zone->transpose + zone->finetune + zone->pitch_bend_depth*ctrl[c_pitch_bend] + mm.get_destination_value(md_pitch);	
	
	// inte viktig nog att offra perf för.. bättre att beräkna on demand isåfall
	//loop_pos = limit_range((sample_pos - mm.get_destination_value_int(md_loop_start))/(float)mm.get_destination_value_int(md_loop_length),0,1);	
	
	GD.Ratio = Float2Int((float)((wave->sample_rate * samplerate_inv)*16777216.f*note_to_pitch(fpitch+kt-zone->pitchcorrection)*mm.get_destination_value(md_rate)));
	fpitch += fkey - 69.f; // relative to A3 (440hz)	
}

//template<bool stereo, bool oversampling, bool xfadeloop, int arch> bool sampler_voice::process_t(float *p_L, float *p_R, float *p_aux1L, float *p_aux1R, float *p_aux2L, float *p_aux2R)
bool sampler_voice::process_block(float *p_L, float *p_R, float *p_aux1L, float *p_aux1R, float *p_aux2L, float *p_aux2R)
{
#if perfprof
	unsigned __int64 perf[16];
#endif	
	int VE = zone->element_active;

	perfslot(0)

	check_filtertypes();
	
	perfslot(1)

	// process envelopes & stepLFO's
	bool continue_playing = AEG.Process(block_size);		
		
	if (VE & ve_EG2) EG2.Process(block_size);
	perfslot(2)	
	if (VE & ve_LFO1) stepLFO[0].process(block_size);
	if (VE & ve_LFO2) stepLFO[1].process(block_size);
	if (VE & ve_LFO3) stepLFO[2].process(block_size);
			
	perfslot(3)
	mm.process();				
	perfslot(4)

	if(first_run)
	{	
		pfg.set_target_instantize(db_to_linear(mm.get_destination_value(md_prefilter_gain)));	
		float amp = crossfade_amp * db_to_linear((1-fvelocity)*zone->velsense) * AEG.output;
		vca.set_target_instantize(amp);
		amp = db_to_linear(mm.get_destination_value(md_amplitude));
		float pan = mm.get_destination_value(md_pan);	
		faderL.set_target_instantize(megapanL(pan) * amp);
		faderR.set_target_instantize(megapanR(pan) * amp);			
		
		amp = db_to_linear(mm.get_destination_value(md_aux_level));
		pan = mm.get_destination_value(md_aux_balance);
		aux1L.set_target_instantize(megapanL(pan) * amp);	
		aux1R.set_target_instantize(megapanR(pan) * amp);			
		amp = db_to_linear(mm.get_destination_value(md_aux2_level));
		pan = mm.get_destination_value(md_aux2_balance);
		aux2L.set_target_instantize(megapanL(pan) * amp);	
		aux2R.set_target_instantize(megapanR(pan) * amp);
		fmix1.set_target_instantize(limit_range(mm.get_destination_value(md_filter1mix),0.f,1.f));
		fmix2.set_target_instantize(limit_range(mm.get_destination_value(md_filter2mix),0.f,1.f));		
	}
	else
	{
		pfg.set_target(db_to_linear(mm.get_destination_value(md_prefilter_gain)));	
		float amp = crossfade_amp * db_to_linear((1-fvelocity)*zone->velsense) * AEG.output;
		vca.set_target(amp);
		amp = db_to_linear(mm.get_destination_value(md_amplitude));
		float pan = mm.get_destination_value(md_pan);	
		//faderL.set_target_smoothed(sqrt(0.5 - 0.5*pan) * amp);
		//faderR.set_target_smoothed(sqrt(0.5 + 0.5*pan) * amp);	
		// megapanning test [-200% to 200%]
		faderL.set_target_smoothed(megapanL(pan) * amp);
		faderR.set_target_smoothed(megapanR(pan) * amp);	
		//aux.set_target(db_to_linear(mm.get_destination_value(md_aux_level)));	
		if(zone->aux[2].outmode)
		{
		amp = db_to_linear(mm.get_destination_value(md_aux_level));
		pan = mm.get_destination_value(md_aux_balance);
		aux1L.set_target_smoothed(megapanL(pan) * amp);	
		aux1R.set_target_smoothed(megapanR(pan) * amp);	
		}
		if(zone->aux[2].outmode)
		{
			amp = db_to_linear(mm.get_destination_value(md_aux2_level));
			pan = mm.get_destination_value(md_aux2_balance);
			aux2L.set_target_smoothed(megapanL(pan) * amp);	
			aux2R.set_target_smoothed(megapanR(pan) * amp);			
		}
		fmix1.set_target_smoothed(limit_range(mm.get_destination_value(md_filter1mix),0.f,1.f));
		fmix2.set_target_smoothed(limit_range(mm.get_destination_value(md_filter2mix),0.f,1.f));
	}

	perfslot(5)

	if (portamento_active) update_portamento();	
	CalcRatio();
	if(use_oversampling) GD.Ratio = GD.Ratio >> 1;		

	update_lag_gen(0);
	update_lag_gen(1);		
		
	int bs = GD.BlockSize;

	perfslot(6)
    	
	//GD.RatioMask = looping_active ? 0xffffffff : 0;	
	
	if(looping_active)
	{
		int end =  (int)wave->sample_length;
		int ll = mm.get_destination_value_int(md_loop_length);
		int ls = limit_range(mm.get_destination_value_int(md_loop_start),0,end);
		int le = limit_range(ls+ll,0,end);
		GD.LowerBound = Min(ls,le);
		GD.UpperBound = Max(ls,le);		
	}		
	
	Generator(&GD,&GDIO);

	if (GD.IsFinished)
	{
		RingOut -= block_size;
		if(RingOut < 0)
		{
			continue_playing = false;
		}
	}

	perfslot(7)

	if(use_stereo) pfg.multiply_2_blocks(output[0],output[1],use_oversampling?(block_size_quad<<1):block_size_quad);
	else pfg.multiply_block(output[0],use_oversampling?(block_size_quad<<1):block_size_quad);

	if(use_oversampling)
	{
		halfrate->process_block_D2(output[0],output[1],block_size << 1);
	}

	perfslot(8)

	// envelope follower		
	/*{
		const float integratorconst = samplerate_inv * block_size;	
		float bs_inv = (float)1/bs;
		float ef_newvalue = sqrt(block_rms*bs_inv);
		if (stereo) ef_newvalue *= 0.5;
		float ef_rate;

		if(ef_newvalue>envelope_follower)
			ef_rate = zone->ef_attack;
		else
			ef_rate = zone->ef_release;

		//float x = pi * powf(2,-ef_rate) * integratorconst;
		float x = pi * note_to_pitch(-12*ef_rate) * integratorconst;
		float tm = 2-cos(x);
		float p = tm - sqrt(tm*tm - 1);	

		envelope_follower = envelope_follower*p + (1-p)*ef_newvalue;
	}*/

	_MM_ALIGN16 float	tempbuf[2][block_size],
						postfader_buf[2][block_size];	

	if(use_stereo)
	{
		if ((voice_filter[0])&&(!zone->Filter[0].bypass)) 
		{
			voice_filter[0]->process_stereo(output[0],output[1],tempbuf[0],tempbuf[1],fpitch);
			filter_modout[0] = voice_filter[0]->modulation_output;
			fmix1.fade_2_blocks_to(output[0],tempbuf[0],output[1],tempbuf[1],output[0],output[1],block_size_quad);			
		}
		if ((voice_filter[1])&&(!zone->Filter[1].bypass)) 
		{
			voice_filter[1]->process_stereo(output[0],output[1],tempbuf[0],tempbuf[1],fpitch);
			filter_modout[1] = voice_filter[1]->modulation_output;
			fmix2.fade_2_blocks_to(output[0],tempbuf[0],output[1],tempbuf[1],output[0],output[1],block_size_quad);
		}
	}
	else
	{
		if ((voice_filter[0])&&(!zone->Filter[0].bypass)) 
		{
			voice_filter[0]->process(output[0],tempbuf[0],fpitch);
			filter_modout[0] = voice_filter[0]->modulation_output;
			fmix1.fade_block_to(output[0],tempbuf[0],output[0],block_size_quad);
		}		
		if ((voice_filter[1])&&(!zone->Filter[1].bypass)) 
		{
			voice_filter[1]->process(output[0],tempbuf[0],fpitch);
			filter_modout[1] = voice_filter[1]->modulation_output;
			fmix2.fade_block_to(output[0],tempbuf[0],output[0],block_size_quad);
		}
		copy_block(output[0],output[1],block_size_quad);
	}

	perfslot(9)

	const unsigned int bufof = block_size;
	vca.multiply_2_blocks(output[0],output[1],block_size_quad);
	faderL.multiply_block_to(output[0],postfader_buf[0],block_size_quad);
	faderR.multiply_block_to(output[1],postfader_buf[1],block_size_quad);		
	accumulate_block(postfader_buf[0],p_L,block_size_quad);
	accumulate_block(postfader_buf[1],p_R,block_size_quad);
	
	if(zone->aux[1].outmode && p_aux1L) 
	{				
		if(zone->aux[1].outmode == 2)
		{
			aux1L.MAC_block_to(postfader_buf[0],p_aux1L,block_size_quad);
			aux1R.MAC_block_to(postfader_buf[1],p_aux1R,block_size_quad);
		}
		else
		{
			aux1L.MAC_block_to(output[0],p_aux1L,block_size_quad);
			aux1R.MAC_block_to(output[1],p_aux1R,block_size_quad);
		}
	}
	if(zone->aux[2].outmode && p_aux2R) 
	{
		if(zone->aux[2].outmode == 2)
		{
			aux2L.MAC_block_to(postfader_buf[0],p_aux2L,block_size_quad);
			aux2R.MAC_block_to(postfader_buf[1],p_aux2R,block_size_quad);
		}
		else
		{
			aux2L.MAC_block_to(output[0],p_aux2L,block_size_quad);
			aux2R.MAC_block_to(output[1],p_aux2R,block_size_quad);
		}
	}	

	perfslot(10)

	time += block_size*samplerate_inv;
	time60 = time * 0.0166666666666667f;	

	first_run = false; 	

#if perfprof
	static unsigned __int64 ap[16];
	static unsigned __int64 apmean;
	ap[0] = perf[1] - perf[0];
	ap[1] = perf[2] - perf[1];
	ap[2] = perf[3] - perf[2];
	ap[3] = perf[4] - perf[3];
	ap[4] = perf[5] - perf[4];
	ap[5] = perf[6] - perf[5];
	ap[6] = perf[7] - perf[6];
	ap[7] = perf[8] - perf[7];
	ap[8] = perf[9] - perf[8];
	ap[9] = perf[10] - perf[9];
	ap[10] = perf[10] - perf[0];	
	apmean = ((apmean*255)>>8) + ap[10];

	if(time > 0.1)
	{
		ap[11] = ap[10];	// breakpoint to hook 
	}
#endif	

	return continue_playing;
}

