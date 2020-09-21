#include "sampler.h"
#if ! TARGET_HEADLESS
#include "shortcircuit_editor2.h"
#endif
#include "sampler_voice.h"
#include <vt_dsp/basic_dsp.h>
#include "tools.h"
#include "interaction_parameters.h"

#include <algorithm>
using std::min;
using std::max;

void sampler::kill_notes(uint32 zone_id)
{	
	int i;
	for(i=0; i<max_voices; i++)
	{
		if(voice_state[i].active && (voice_state[i].zone_id == zone_id))
		{
			voice_state[i].active = false;
			polyphony--;
		}
	}		
}

void sampler::AllNotesOff()
{
	int i;
	for(i=0; i<max_voices; i++)
		voice_state[i].active = false;		
	
	polyphony = 0;
	highest_voice_id = 0;

	memset(keystate,0,sizeof(keystate));

	int c;
	for(c=0; c<16; c++)	
		hold[c] = false;	

	holdbuffer.clear();
}


int sampler::softkill_oldest_note(int group_id)
{
	bool no_group = (group_id == 0);
	int i,oldest_id = -1;
	float oldest = 0.f;

	for(i=0; i<max_voices; i++)
	{
		if(voice_state[i].active)
		{
			float time = voices[i]->time;
			if((time > oldest)&&(!voices[i]->is_uberrelease)&&(!voices[i]->gate))
			{
				oldest = time;
				oldest_id = i;
			}
		}
	}
	if(oldest_id<0)	// no notes in the release state found, check ALL notes..
	{
		for(i=0; i<max_voices; i++)
		{
			if(voice_state[i].active)
			{
				float time = voices[i]->time;
				if((time > oldest)&&(!voices[i]->is_uberrelease))
				{
					oldest = time;
					oldest_id = i;
				}
			}
		}
	}
	if (oldest_id >= 0) voices[oldest_id]->uberrelease();		

	return oldest_id;
}

int sampler::GetFreeVoiceId(int group_id)
{
	int i,v=0,vg=0;	
	int v_free=-1;
	int oldest_id=-1;

	for(i=0; i<max_voices; i++)
	{
		if(voice_state[i].active /* && (!voices[i]->is_uberrelease)*/)
		{			
			v++;
//			if(zones[voice_state[i].zone_id].group_id == group_id) vg++;
		}
		else if(v_free<0)
		{
			v_free = i;
			if(voice_state[i].active) polyphony--;
		}
	}	
	
	// if the polyphony limit is going to be exceeded, release the oldest note
	
	int ng = 0;
#ifndef SCPB
	/*if(group_id > 0)
	{
		ng = max(0,vg + 1 - groups[group_id].poly_limit);
		for(i=0; i<ng; i++)
		{
			oldest_id = softkill_oldest_note(group_id);
		}
	}*/
#endif		
	//if(v >= polyphony_cap)
	{
		int n = max(0,v-polyphony_cap+1-ng);		
		for(i=0; i<n; i++)
		{
			oldest_id = softkill_oldest_note();
		}
	}	
	
	if(v_free < 0)
	{
		// no free voice was found at all! (all 256 have been used up)
		// KILL the oldest note!!
		if (oldest_id >= 0)
		{	
			voice_state[oldest_id].active = false;
			polyphony--;
			v_free = oldest_id;
		}
		else 
		{
			// rescue path. this is only supposed to happen if ALL notes are in uberrelease-mode
			voice_state[0].active = false;
			polyphony--;
			v_free = 0;
		}
		::OutputDebugString("voice killed\n");
	}	

	return v_free;
}

void sampler::update_highest_voice_id()
{
	for(int i=0; i<max_voices; i++)
	{
		if(voice_state[i].active) highest_voice_id = i + 1;
	}
}

void sampler::play_zone(int z)
{	
	if(!zone_exists[z]) 
		return;

	if(zones[z].sample_id < 0) return;

	if (sample_replace_filename[0] && (selected->zone_is_active(z))) return;

	int v = GetFreeVoiceId();

	sample_zone *master = &zones[z];
	bool is_group_limited = false;
	int resultchannel = zones[z].part;							
	int ch = parts[zones[z].part].MIDIchannel;
	update_zone_switches(z);
	voices[v]->play(samples[zones[z].sample_id], &zones[z], &parts[zones[z].part&0xf], zones[z].key_root, 100, 0, &controllers[n_controllers*ch],automation,1.f);
	voice_state[v].active = true;
	voice_state[v].key = zones[z].key_root;
	voice_state[v].channel = ch;
	voice_state[v].zone_id = z;
	polyphony++;	
	update_highest_voice_id();

#if TARGET_VST2
	if (editor && editor->isOpen()) track_zone_triggered(z,true);
#endif
}

void sampler::release_zone(int zone_id)
{
	int i;
	for(i=0; i<max_voices; i++)
	{
		if ((voice_state[i].zone_id == zone_id) && voice_state[i].active)			
			voices[i]->release(127);
	}
}


bool sampler::PlayNote(char channel, char key, char velocity, bool is_release, char detune)
{		
	// update keystate
	if(!is_release) keystate[channel][key] = velocity;
	
	if(is_release) track_key_triggered(channel,key,0);
	else track_key_triggered(channel,key,velocity);

	bool require_ignore = false;	
	if(!is_release)	// look for legato notes
	{
		for(int tv=0; tv<max_voices; tv++)
		{
			if(voice_state[tv].active && 
				(parts[voice_state[tv].part].polymode == polymode_legato) && 
				(parts[voice_state[tv].part].MIDIchannel == channel) &&
				!zones[voice_state[tv].zone_id].ignore_part_polymode)
			{
				if(voices[tv]->gate)
				{
					voices[tv]->change_key(key,velocity,detune);
					voice_state[tv].key = key;
					require_ignore = true;	
				}
				else
				{
					voices[tv]->uberrelease();
				}
			}
		}
	}

	// find matching zone
	for(int z=0; z<max_zones; z++)
	{
		if (!zone_exists[z]) goto skipzone;
		if (zones[z].mute)	goto skipzone;
		if (require_ignore && !zones[z].ignore_part_polymode) goto skipzone;
		if (is_release && (zones[z].playmode != pm_forward_release)) goto skipzone;
		if (!is_release && (zones[z].playmode == pm_forward_release)) goto skipzone;
		if (channel != parts[zones[z].part&0xf].MIDIchannel) goto skipzone;
		if ((sample_replace_filename[0] != 0) && (selected->zone_is_active(z))) goto skipzone;												
		if (selected->get_solo() && !selected->zone_is_selected(z))	goto skipzone;	
		
		int p = zones[z].part&0xf;
		int zkey = key + parts[p].transpose - parts[p].formant;	// key used for range check
		if (zkey < (zones[z].key_low - zones[z].key_low_fade)) goto skipzone;
		if (zkey > (zones[z].key_high + zones[z].key_high_fade)) goto skipzone;											
		if (!partv[p].mm->check_NC(&zones[z])) goto skipzone;

		float crossfade_amp = 1.f;

		if(zones[z].key_low_fade || zones[z].key_high_fade)
		{
			if(zkey < (zones[z].key_low + zones[z].key_low_fade)) crossfade_amp = ((float)zkey - zones[z].key_low + zones[z].key_low_fade) / ((float)zones[z].key_low_fade * 2.f);
			else if(zkey > (zones[z].key_high - zones[z].key_high_fade)) crossfade_amp = 1.f - (((float)zkey - zones[z].key_high + zones[z].key_high_fade) / ((float)zones[z].key_high_fade * 2.f));					
			
			crossfade_amp = limit_range(crossfade_amp,0.f,1.f);
		}

		int n_split = parts[zones[z].part&0xf].vs_layercount;								
		if(n_split && (zones[z].layer <= n_split))
		{
			float k = zones[z].layer;
			float xf = parts[zones[z].part&0xf].vs_xfade;
			float fvlo0 = (k - 0.5f * xf) / ((float)n_split + 1.0f);
			float fvlo1 = (k + 0.5f * xf) / ((float)n_split + 1.0f);
			float fvhi0 = (k + 1.0f - 0.5f * xf) / ((float)n_split + 1.0f);
			float fvhi1 = (k + 1.0f + 0.5f * xf) / ((float)n_split + 1.0f);

			// map to -1 .. 1
			fvlo0 = saturate(fvlo0*2.f - 1.f);
			fvlo1 = saturate(fvlo1*2.f - 1.f);
			fvhi0 = saturate(fvhi0*2.f - 1.f);
			fvhi1 = saturate(fvhi1*2.f - 1.f);				

			//skew
			float sk = parts[zones[z].part&0xf].vs_distribution;				
			fvlo0 = fvlo0 - sk*fvlo0*fvlo0 + sk;
			fvlo1 = fvlo1 - sk*fvlo1*fvlo1 + sk;
			fvhi0 = fvhi0 - sk*fvhi0*fvhi0 + sk;
			fvhi1 = fvhi1 - sk*fvhi1*fvhi1 + sk;

			// map back and scale to 0 .. 127				
			fvlo0 = fvlo0 * 64.f + 64.f;	fvlo1 = fvlo1 * 64.f + 64.f;	
			fvhi0 = fvhi0 * 64.f + 64.f;	fvhi1 = fvhi1 * 64.f + 64.f;

			int lovel = fvlo0;
			int hivel = fvhi1;

			if ((velocity < lovel)||(velocity >= hivel)) goto skipzone;

			if(xf > 0.001)
			{
				if((velocity < fvlo1)&&(zones[z].layer)) crossfade_amp *= (velocity - fvlo0) / (fvlo1 - fvlo0);
				else if((velocity > fvhi0)&&(zones[z].layer<n_split)) crossfade_amp *= 1.f - ((velocity - fvhi0) / (fvhi1 - fvhi0));
				crossfade_amp = limit_range(crossfade_amp,0.f,1.f);
			}
		}			
		else
		{
			int lovel = zones[z].velocity_low - zones[z].velocity_low_fade;
			int hivel = zones[z].velocity_high + zones[z].velocity_high_fade;

			if ((velocity < lovel)||(velocity > hivel)) goto skipzone;

			if(zones[z].velocity_low_fade || zones[z].velocity_high_fade)
			{
				if(velocity < (zones[z].velocity_low + zones[z].velocity_low_fade)) crossfade_amp *= ((float)velocity - lovel) / ((float)zones[z].velocity_low_fade * 2.f);
				else if(velocity > (zones[z].velocity_high - zones[z].velocity_high_fade)) crossfade_amp *= 1.f - (((float)velocity - zones[z].velocity_high + zones[z].velocity_high_fade) / ((float)zones[z].velocity_high_fade * 2.f));					
				crossfade_amp = limit_range(crossfade_amp,0.f,1.f);
			}
		}					

		// if mono, release other voices
		//bool do_play = true;

		if(!zones[z].ignore_part_polymode && (parts[p].polymode == polymode_mono))
		{
			int tv;
			for(tv=0; tv<max_voices; tv++)
			{
				if(voice_state[tv].active && (voice_state[tv].part == p) && !zones[voice_state[tv].zone_id].ignore_part_polymode)
					voices[tv]->uberrelease();
			}
		}

		int v = GetFreeVoiceId(0);

		if(zones[z].mute_group){
			int tv;
			int mg = zones[z].mute_group;
			for(tv=0; tv<max_voices; tv++)
			{
				if(voice_state[tv].active && (zones[voice_state[tv].zone_id].mute_group == mg)/* && (z!=voice_state[tv].zone_id)*/)
				{
					voices[tv]->uberrelease();									
				}
			}
		}

		if (parts[zones[z].part&0xf].vs_xf_equality) crossfade_amp = sqrt(crossfade_amp);

		if((zones[z].sample_id>=0) && samples[zones[z].sample_id])
		{
			update_zone_switches(z);
			voices[v]->play(samples[zones[z].sample_id], &zones[z], &parts[p], key, velocity, detune, &controllers[n_controllers*channel],automation, crossfade_amp);							
			voice_state[v].active = true;
			voice_state[v].key = key;
			voice_state[v].channel = channel;
			voice_state[v].part = p;
			voice_state[v].zone_id = z;
			polyphony++;
		}

		if (editor && (parts[editorpart].MIDIchannel == channel)) track_zone_triggered(z,true);
		
		skipzone:
		int asdf = 0; // do nothing
	}	
	update_highest_voice_id();
	return true;
}

void sampler::track_zone_triggered(int z, bool state)
{	
	if(!editor) return;
	actiondata ad;
	ad.actiontype = vga_zone_playtrigger;
	ad.data.i[0] = z;
	ad.data.i[1] = state;
	ad.id = ip_kgv_or_list;
	ad.subid = 0;
	post_events_to_editor(ad);
}

void sampler::track_key_triggered(int ch, int key, int vel)
{	
	if(!editor) return;
	if(parts[editorpart].MIDIchannel != ch) return;
	actiondata ad;
	ad.actiontype = vga_note;
	ad.data.i[0] = key;
	ad.data.i[1] = vel;
	ad.id = ip_kgv_or_list;
	ad.subid = 0;
	post_events_to_editor(ad);
}

int sampler::get_zone_poly(int zone)
{
	int n=0, i;

	for(i=0; i<max_voices; i++)
	{
		if (voice_state[i].active && (voice_state[i].zone_id == zone))
			n++;
	}
	return n;
}

int sampler::get_group_poly(int group)
{
	unsigned int n=0;
/*
	for(unsigned i=0; i<max_voices; i++)
	{
		if (voice_state[i].active && (zones[voice_state[i].zone_id].group_id == group))
			n++;		
	}*/
	return n;
}

bool sampler::get_slice_state(int zone,int slice)
{
	int n=0, i;

	for(i=0; i<max_voices; i++)
	{
		if (voice_state[i].active && (voice_state[i].zone_id == zone) && (voices[i]->slice_id == slice))
			return true;
	}
	return false;
}

bool sampler::is_key_down(int channel, int key)
{	
	return (keystate[channel][key]>0);
}

void sampler::ReleaseNote(char channel, char key, char velocity)
{
	// upsate keystate
	keystate[channel][key] = 0;

	// find note 
	for(int i=0; i<max_voices; i++)
	{
		if(voice_state[i].active && (voice_state[i].key == key) && (voice_state[i].channel == channel))
		{
			if (!hold[channel])
			{
				sample_zone *z = &zones[voice_state[i].zone_id];
				int p = z->part;
				int polymode = parts[p].polymode;

				if((polymode == polymode_poly) || z->ignore_part_polymode)
				{
					// poly, release as usual..
					voices[i]->release(127);	// hold pedal is not down
				}
				else
				{
					// mono and legato.. see if other key is down and can take over the voice (if legato) or retriggered (if mono)
					
					int k;
					/*int key_high = parts[p].polymode_partlevel ? 127 : z->key_high;
					int key_low = parts[p].polymode_partlevel ? 0 : z->key_low;*/
					bool do_switch = false;					
					for(k=127; k>=0; k--)	// search downwards
					{
						if(keystate[channel][k])
						{
							do_switch = true;
							break;
						}
					}
					if(do_switch)
					{
						if(polymode == polymode_legato)
						{
							voices[i]->change_key(k,keystate[channel][k],0);	// TODO: add last-detune buffer isntead of 0
							voice_state[i].key = k;
						}
						else if(polymode == polymode_mono)
						{							
							voices[i]->uberrelease();
							this->PlayNote(channel,k,keystate[channel][k]);
						}
					}
					else
					{
						voices[i]->release(127);
					}
				}
			}
			else
			{
				// hold pedal is down, add to bufffer
				holdbuffer.push_back(i);					
			}
		}
	}

	// find matching zone and see if it has playmode == pm_forward_release
	PlayNote(channel,key,velocity,true);	
}

void sampler::voice_off(uint32 voice_id)
{	
	voice_state[voice_id].active = false;
	polyphony--;
	holdbuffer.remove(voice_id);

#if TARGET_VST2
	if (editor && editor->isOpen()) track_zone_triggered(voice_state[voice_id].zone_id,false);
#endif
}
