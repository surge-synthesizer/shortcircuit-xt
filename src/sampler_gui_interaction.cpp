//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2005 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#include "sampler.h"
#include "sample.h"
#include "interaction_parameters.h"
#include "parameterids.h"
#if ! TARGET_HEADLESS
#include "shortcircuit_editor2.h"
#endif
#include "synthesis/steplfo.h"
#include "synthesis/modmatrix.h"
#include "configuration.h"
#include <vt_gui/browserdata.h>
#include <vt_util/vt_lockfree.h>

#include <vt_util/vt_string.h>

#include <vector>
using std::vector;

#include <string>
using std::string;
using std::basic_string;

void sampler::post_events_from_editor(actiondata ad)
{				
	ActionBuffer->WriteBlock(&ad);
	
	// l�gg till pseudo:
	// if (halt_engine) process_editor_events()
	// TODO om audio-processing inte �r aktiv ska guit funka �nd�

	// HACK verkar strula deluxemkt
	if(AudioHalted) process_editor_events();
}

//-------------------------------------------------------------------------------------------------

void sampler::post_events_to_editor(actiondata ad, bool AlertIfClosed)
{					
	if(editor)
	{
		editor->post_action_from_program(ad);
	}
	else if(AlertIfClosed)
	{
		DebugBreak();
	}
}

//-------------------------------------------------------------------------------------------------

void sampler::process_editor_events()
{			
	// ingoing
	actiondata *adptr;
	while(adptr = (actiondata*)ActionBuffer->ReadBlock())
	{		
		actiondata ad = *adptr;

		switch(ad.actiontype)		// intercept these actiontypes regardless of the control
		{		
		case vga_openeditor:
			//actiondatabuffer_out.clear();	// any old actions generated when the editor was closed will not be relevant
			editor_open = true;
			post_initdata();
			break;
		case vga_closeeditor:			
			editor_open = false;			
			break;
		case vga_note:
			if(ad.data.i[1]==0) ReleaseNote(parts[editorpart].MIDIchannel,ad.data.i[0]&0xff,0);
			else PlayNote(parts[editorpart].MIDIchannel,ad.data.i[0]&0xff,ad.data.i[1]&0xff);
			break;
		case vga_select_zone_clear:		
			selected->deselect_zones();			
			break;		
		case vga_select_zone_primary:
			if(selected->get_active_zone() == ad.data.i[0])
			{
				selected->set_active_zone(ad.data.i[0]);
				post_kgvdata();
			}
			else
			{
				selected->set_active_zone(ad.data.i[0]);
				post_zonedata();
			}
			break;
		case vga_select_zone_secondary:
			selected->select_zone(ad.data.i[0]);
			break;		
		case vga_createemptyzone:
			{
				int nz;
				if(add_zone(0,&nz,editorpart&0xf,false))
				{
					int key = ad.data.i[0] & 0x7f;
					zones[nz].key_low = key;
					zones[nz].key_root = key;
					zones[nz].key_high = key;
				}
				selected->set_active_zone(nz);
				post_zonedata();
			}
			break;
		case vga_load_dropfiles:
			{
				int nz = -1;				
				
				// seems unlikely to cause issues so it doesn't have to be threadsafe
				vector <dropfile>::iterator i;	
				for(i =	editor->dropfiles.begin(); i != editor->dropfiles.end(); i++)				
				{
					if (add_zone(i->path.c_str(),&nz,editorpart&0xf,false))
					{					
						zones[nz].key_low = i->key_lo;
						zones[nz].key_root = i->key_root;
						zones[nz].key_high = i->key_hi;
						zones[nz].velocity_low = i->vel_lo;
						zones[nz].velocity_high = i->vel_hi;
						//vtCopyString(zones[nz].name,i->label.c_str(),31);		// g�r i sample:: ist�llet
						zones[nz].database_id = i->database_id;
					}
				}
				if (nz != -1) 
				{
					selected->set_active_zone(nz);
				}
				post_zonedata();				
			}
			break;
		case vga_load_patch:
			{
				if(editor->dropfiles.size())
				{
					cs_patch.enter();
					mpPreview->Stop();	// Just in case, to speed up loading
					load_file(editor->dropfiles[0].path.c_str(),0,0,0,editorpart&0xf,0,true);
					parts[editorpart&0xf].database_id = editor->dropfiles[0].database_id;
					post_initdata();
					cs_patch.leave();
				}
			}
			break;
		case vga_save_patch:
			{
				//save_part_as_xml(editorpart,editor->savepart_fname.c_str());				
				SaveAllAsRIFF(0,editor->savepart_fname.c_str(),editorpart);
			}
			break;
		case vga_save_multi:
			{
				//save_all_as_xml(0,editor->savepart_fname.c_str());
				SaveAllAsRIFF(0,editor->savepart_fname.c_str());
			}
			break;
		case vga_browser_preview_start:
			{							
				vtStringToWString(mpPreview->mFilename, editor->dropfiles[0].path.c_str(), 1024);

				// ad.data.i[0] > 0 means the sample was played manually, so the AutoPreview setting deosn't matter
				if(mpPreview->mAutoPreview || (ad.data.i[0] > 0))				
				{					
					mpPreview->Start(mpPreview->mFilename);	
				}
			}
			break;
		case vga_browser_preview_stop:
			{
				mpPreview->Stop();
			}
			break;
		case vga_zonelist_mode:
			{
				parts[editorpart&0xf].zonelist_mode = ad.data.i[0];
				actiondata ad2 = ad;				
				ad2.id = ip_kgv_or_list;
				ad2.subid = -1;				
				post_events_to_editor(ad2);
			}
			break;
		case vga_request_refresh:							
			post_zonedata();	
			post_initdata_mm_part();
			break;
		case vga_deletezone:
			selected->delete_selected();
			post_zonedata();
			break;
		case vga_movezonetopart:
			selected->move_selected_zones_to_part(ad.data.i[0]);
			post_zonedata();
			break;
		case vga_movezonetolayer:
			selected->move_selected_zones_to_layer(ad.data.i[0]);
			post_zonedata();
			break;
		case vga_set_zone_keyspan:
			if(zone_exist(ad.data.i[0]))
			{
				zones[ad.data.i[0]].key_low = ad.data.i[1]&0x7f;
				zones[ad.data.i[0]].key_root = ad.data.i[2]&0x7f;
				zones[ad.data.i[0]].key_high = ad.data.i[3]&0x7f;
			}
			break;	
		case vga_set_zone_keyspan_clone:
			if(zone_exist(ad.data.i[0]))
			{
				int nz=-1;
				if(clone_zone(ad.data.i[0],&nz,true))
				{
					zones[nz].key_low = ad.data.i[1]&0x7f;
					zones[nz].key_root = ad.data.i[2]&0x7f;
					zones[nz].key_high = ad.data.i[3]&0x7f;
				}
			}
			break;				
		case vga_wavedisp_editpoint:
			{
				int z = selected->get_active_zone();
				if(zone_exist(z))
				{
					switch(ad.data.i[0])
					{
					case 0:
						zones[z].sample_start = ad.data.i[1];
						break;
					case 1:
						zones[z].sample_stop = ad.data.i[1];
						break;
					case 2:
						zones[z].loop_start = ad.data.i[1];
						break;
					case 3:
						zones[z].loop_end = ad.data.i[1];
						break;
					case 4:
						zones[z].loop_crossfade_length = ad.data.i[1];
						break;
					default:
						int hpid = ad.data.i[0]-5;
						if((hpid>=0)&&(hpid<128)) zones[z].hp[hpid].start_sample = ad.data.i[1];
						break;
					};
				}
			}
			break;
		case vga_select_zone_previous:
		case vga_select_zone_next:
			{
				int i = selected->get_active_zone();
				selected->set_active_zone(max(0,(vga_select_zone_previous==ad.actiontype)?i-1:i+1));
				post_zonedata();
			}
			break;
		case vga_audition_zone:
			{
				int i = selected->get_active_zone();
				if(zone_exist(i)) 
				{
					if(ad.data.i[0]) play_zone(i);
					else release_zone(i);
				}
			}
			break;		
		case vga_steplfo_data_single:			
			{
				int offset = ip_data[ip_lfosteps].ptr_offset + ad.subid*ip_data[ip_lfosteps].subid_ptr_offset + (ad.data.i[0]&0x1f)*sizeof(float);
				selected->set_zone_parameter_float_internal(offset, ad.data.f[1]);				
			}
			break;		
		default:	// default handler (for most actiontypes)
			{
				if((ad.id >= ip_zone_params_begin)&&(ad.id <= ip_zone_params_end)&&(ad.subid < ip_data[ad.id].n_subid))
				{
					int offset = ip_data[ad.id].ptr_offset + ip_data[ad.id].subid_ptr_offset*ad.subid;
					if((ad.actiontype == vga_intval)&&(ip_data[ad.id].vtype == ipvt_int))
					{
						selected->set_zone_parameter_int_internal(offset, ad.data.i[0]);
					}
					else if((ad.actiontype == vga_intval)&&(ip_data[ad.id].vtype == ipvt_char))
					{
						selected->set_zone_parameter_char_internal(offset, ad.data.i[0]&0xff);
					}
					else if((ad.actiontype == vga_floatval)&&(ip_data[ad.id].vtype == ipvt_float))
					{
						selected->set_zone_parameter_float_internal(offset, ad.data.f[0]);
					}
					else if((ad.actiontype == vga_text)&&(ip_data[ad.id].vtype == ipvt_string))
					{
						selected->set_zone_parameter_cstr_internal(offset, (char*)ad.data.str);
					}
				}
				else if((ad.id >= ip_part_params_begin)&&(ad.id <= ip_part_params_end)&&(ad.subid < ip_data[ad.id].n_subid))
				{										
					relay_data_to_structure(ad,(char*)&parts[editorpart&0xf]);
				}
				else if((ad.id >= ip_multi_params_begin)&&(ad.id <= ip_multi_params_end)&&(ad.subid < ip_data[ad.id].n_subid))
				{					
					relay_data_to_structure(ad,(char*)&multi);
				}
				else	
				{					
					switch(ad.id)
					{
					case ip_partselect:
						if(ad.actiontype == vga_intval) 
						{							
							set_editorpart(ad.data.i[0],parts[ad.data.i[0]&0xf].activelayer);
						}
						break;	
					case ip_layerselect:
						if(ad.actiontype == vga_intval) 
						{							
							set_editorpart(editorpart,ad.data.i[0]&(num_layers-1));
						}
						break;	
					case ip_sample_name:
						if(ad.actiontype == vga_text) 
						{							
							// TODO edit sample name
						}
						break;	
					case ip_sample_prevnext:
						if(ad.actiontype == vga_click)
						{
							int diff = (ad.subid==1)?1:-1;
							int z = selected->get_active_zone();
							if(zone_exist(z) && (zones[z].database_id >= 0))
							{								
								int ne = zones[z].database_id+diff;
								if((ne >= 0)&&(ne < editor->browserd.patchlist[0].size()))
								{
									replace_zone(z,editor->browserd.patchlist[0][ne].path);
									zones[z].database_id = ne;
									post_zonedata();
								}
							}
						}
						break;	
					case ip_patch_prevnext:
						if(ad.actiontype == vga_click)
						{
							int diff = (ad.subid==1)?1:-1;							
							if(parts[editorpart].database_id >= 0)
							{								
								int ne = parts[editorpart].database_id+diff;
								if((ne >= 0)&&(ne < editor->browserd.patchlist[1].size()))
								{									
									load_file(editor->browserd.patchlist[1][ne].path,0,0,0,editorpart,0,true);
									parts[editorpart].database_id = ne;
									post_zonedata();
								}
							}
						}
						break;						
					case ip_browser_searchtext:
						if(ad.actiontype == vga_text)
						{
							ad.id = ip_browser;
							post_events_to_editor(ad);
						}
						break;
					case ip_replace_sample:						
						if(ad.actiontype == vga_intval)
						{
							toggled_samplereplace = (ad.data.i[0] != 0);
						}
						break;
					case ip_browser:						
						if((ad.actiontype == vga_intval) && toggled_samplereplace)
						{							
							int z = selected->get_active_zone();
							if(zone_exist(z))
							{								
								int ne = ad.data.i[0];
								if((ne >= 0)&&(ne < editor->browserd.patchlist[0].size()))
								{
									replace_zone(z,editor->browserd.patchlist[0][ne].path);
									zones[z].database_id = ne;
									post_zonedata();
								}
							}
						}
						break;
					case ip_lfo_load:
						{
							int z = selected->get_active_zone();
							if(zone_exist(z))
							{
								int e = max(0,min(3,ad.subid));
								load_lfo_preset(ad.data.i[0],&zones[z].LFO[e]);
								selected->copy_lfo_waveform_to_selected_zones(&zones[z].LFO[e],e);
							}
							post_zonedata();
						}
						break;					
					//case ip_config_h_or_v:
					//case ip_config_slidersensitivity:					
					case ip_config_outputs:
						if(ad.actiontype == vga_intval) conf->stereo_outputs = max(1,min(ad.data.i[0]+1,8));						
						break;
					case ip_config_controller_id:
						if(ad.actiontype == vga_intval) conf->MIDIcontrol[ad.subid & 0xf].number = ad.data.i[0];						
						break;
					case ip_config_controller_mode:
						if(ad.actiontype == vga_intval) conf->MIDIcontrol[ad.subid & 0xf].type = (midi_controller_type)ad.data.i[0];						
						break;
					case ip_config_refresh_db:						
						break;
					case ip_config_kbdmode:
						if(ad.actiontype == vga_intval) conf->keyboardmode = ad.data.i[0];
						break;
					case ip_config_autopreview:
						if(ad.actiontype == vga_intval)
						{
							conf->mAutoPreview = ad.data.i[0] != 0;
							mpPreview->mAutoPreview = ad.data.i[0] != 0;
						}						
						break;
					case ip_config_previewvolume:
						if(ad.actiontype == vga_floatval)
						{
							conf->mPreviewLevel = ad.data.f[0];
							mpPreview->mZone.aux[0].level = conf->mPreviewLevel;
						}						
						break;
					case ip_browser_previewbutton:
						{
							if (ad.data.i[0] == 1)
							{
								mpPreview->Start(mpPreview->mFilename);
							} 
							else
							{
								mpPreview->Stop();
							}
						}
						break;
					case ip_config_save:
						conf->save(L"");
						break;
					case ip_solo:
						selected->set_solo(ad.data.i[0]);
						post_zonedata();
						break;
					case ip_select_all:
						{
							selected->deselect_zones();
							bool first = true;							
							for(int i=0; i<max_zones; i++)
							{
								if (zone_exists[i] && (zones[i].part == editorpart)){
									selected->select_zone(i);
									if(first) selected->set_active_zone(i);
									first = false;
								}
							}						
							post_zonedata();
						}
						break;
					case ip_select_layer:
						{
							selected->deselect_zones();
							bool first = true;							
							for(int i=0; i<max_zones; i++)
							{
								if (zone_exists[i] && (zones[i].part == editorpart) && (zones[i].layer == editorlayer)){
									selected->select_zone(i);
									if(first) selected->set_active_zone(i);
									first = false;
								}
							}						
							post_zonedata();
						}					
						break;
					};
				}
			}
		}	
		
		// extra checks that are run regardless of previous switch-blocks
		// generally used to refresh editor when setting filtertype etc
		switch(ad.id)
		{		
		case ip_low_key:
		case ip_high_key:
		case ip_low_vel:
		case ip_high_vel:
			if(ad.actiontype == vga_endedit)			
			{
				post_kgvdata();
			}
			break;
		case ip_zone_name:
		case ip_mute:
			post_zonedata();
			break;
		case ip_mm_dst:
			{		
				int z = selected->get_active_zone();
				if(zone_exist(z))
				{
					actiondata ad2;
					ad2.id = ip_mm_amount;
					ad2.subid = ad.subid;
					ad2.actiontype = vga_datamode;			
					modmatrix mm;
					mm.assign(conf,&zones[z],&parts[editorpart]);	
					int cmode = mm.get_destination_ctrlmode(zones[z].mm[ad.subid].destination);
					string s = datamode_from_cmode(cmode);
					vtCopyString((char*)ad2.data.str,s.c_str(),actiondata_maxstring);
					post_events_to_editor(ad2);
				}
			}
			break;
		case ip_part_mm_dst:
			{		
				actiondata ad2;
				ad2.id = ip_part_mm_amount;
				ad2.subid = ad.subid;
				ad2.actiontype = vga_datamode;			
				modmatrix mm;
				mm.assign(conf,0,&parts[editorpart]);	
				int cmode = mm.get_destination_ctrlmode(parts[editorpart].mm[ad.subid].destination);
				string s = datamode_from_cmode(cmode);
				vtCopyString((char*)ad2.data.str,s.c_str(),actiondata_maxstring);
				post_events_to_editor(ad2);
			}
			break;
		case ip_lfoshape:
			if(ad.actiontype == vga_floatval)
			{				
				actiondata ad2;
				ad2.actiontype = vga_steplfo_shape;	
				ad2.id = ip_lfosteps;
				ad2.subid = ad.subid;
				ad2.data.f[0] = ad.data.f[0];
				post_events_to_editor(ad2);
			}
			break;
		case ip_lforepeat:
			if(ad.actiontype == vga_intval)			
			{
				actiondata ad2;
				ad2.actiontype = vga_steplfo_repeat;
				ad2.id = ip_lfosteps;
				ad2.subid = ad.subid;
				ad2.data.i[0] = ad.data.i[0];
				post_events_to_editor(ad2);
			}
			break;
		case ip_lfosync:
			{				
				actiondata ad2;
				ad2.actiontype = vga_temposync;	
				ad2.id = ip_lforate;
				ad2.subid = ad.subid;
				ad2.data.i[0] = ad.data.i[0];
				post_events_to_editor(ad2);								
				break;
			}
		case ip_playmode:
			{
				post_zonedata();
			}
			break;
		case ip_filter_type:
			{
				int nz = selected->n_selected_zones();
				for(int j=0; j<nz; j++)
				{
					int zid = selected->get_selected_zone(j);											

					filter *tf = spawn_filter(zones[zid].Filter[ad.subid&1].type,zones[zid].Filter[ad.subid&1].p,zones[zid].Filter[ad.subid&1].ip,0,false);
					if(tf) 
					{
						tf->init_params();
						spawn_filter_release(tf);
					}
				}					
				post_zone_filterdata(selected->get_active_zone(),ad.subid&1,true);
				post_initdata_mm(selected->get_active_zone());
			}
			break;
		case ip_part_filter_type:
			{				
				int p = editorpart&0xf;
				filter *tf = spawn_filter(parts[p].Filter[ad.subid&1].type,parts[p].Filter[ad.subid&1].p,parts[p].Filter[ad.subid&1].ip,0,false);
				if(tf) 
				{
					tf->init_params();
					spawn_filter_release(tf);
				}				
				post_part_filterdata(p,ad.subid&1,true);
				post_initdata_mm_part();
			}
			break;
		case ip_multi_filter_type:
			{								
				filter *tf = spawn_filter(multi.Filter[ad.subid&(num_fxunits-1)].type,multi.Filter[ad.subid&(num_fxunits-1)].p,multi.Filter[ad.subid&(num_fxunits-1)].ip,0,false);
				if(tf) 
				{
					tf->init_params();
					spawn_filter_release(tf);
				}				
				post_multi_filterdata(ad.subid&(num_fxunits-1),true);
			}
			break;
		case ip_part_userparam_name:
			post_initdata_mm(selected->get_active_zone());
			post_initdata_mm_part();
			break;
		case ip_part_userparam_polarity:
			{											
				if(ad.actiontype == vga_intval)
				{
					actiondata ad2;
					ad2.actiontype = vga_datamode;
					ad2.id = ip_part_userparam_value;
					ad2.subid = ad.subid;
					
					if (ad.data.i[0]) vtCopyString((char*)ad2.data.str,"f,-1,0.005,1,1,%",actiondata_maxstring);
					else vtCopyString((char*)ad2.data.str,"f,0,0.005,1,1,%",actiondata_maxstring);
					
					post_events_to_editor(ad2);
				}
			}
			break;
		};
	}	
}

//-------------------------------------------------------------------------------------------------

void sampler::set_editorpart(int p, int layer)
{
	editorpart = p&15;
	editorlayer = layer & 7;
	parts[editorpart].activelayer = editorlayer;
	editormm = 0;
	editorlfo = 0;

	post_zonedata();
}

//-------------------------------------------------------------------------------------------------

void sampler::post_kgvdata()
{
	actiondata ad;
	// clear list/kgv
	ad.actiontype = vga_zonelist_clear;
	ad.id = ip_kgv_or_list;
	ad.subid = 0;	
	post_events_to_editor(ad);		
	// populate list/kgv
	ad.actiontype = vga_zonelist_populate;
	for(int i=0; i<max_zones; i++)
	{
		if(zone_exist(i) && (zones[i].part == editorpart) && (zones[i].layer == editorlayer))
		{						
			ad_zonedata *zd = (ad_zonedata*)&ad;
			zd->zid = i;
			zd->part = zones[i].part;
			zd->layer = zones[i].layer;
			zd->keylo = zones[i].key_low;
			zd->keylofade = zones[i].key_low_fade;
			zd->keyhi = zones[i].key_high;
			zd->keyhifade = zones[i].key_high_fade;
			zd->keyroot = zones[i].key_root;
			zd->vello = zones[i].velocity_low;
			zd->vellofade = zones[i].velocity_low_fade;
			zd->velhi = zones[i].velocity_high;
			zd->velhifade = zones[i].velocity_high_fade;
			zd->is_active = selected->zone_is_active(i);
			zd->is_selected = selected->zone_is_selected(i);
			zd->mute = zones[i].mute;
			vtCopyString(zd->name,zones[i].name,32);

			post_events_to_editor(ad);	
		}
	}		
	
	ad.actiontype = vga_zonelist_done;
	post_events_to_editor(ad);

	ad.actiontype = vga_zonelist_mode;
	ad.data.i[0] = parts[editorpart].zonelist_mode;
	post_events_to_editor(ad);

	post_samplelist();
}

//-------------------------------------------------------------------------------------------------

void sampler::post_zonedata()
{
	post_kgvdata();
	actiondata ad;

	ad.actiontype = vga_intval;
	ad.id = ip_partselect;
	ad.subid = 0;
	ad.data.i[0] = editorpart;
	post_events_to_editor(ad);	
	ad.id = ip_layerselect;
	ad.subid = 0;
	ad.data.i[0] = editorlayer;
	post_events_to_editor(ad);

	// prev/next patch buttons				
	ad.id = ip_patch_prevnext;			
	ad.actiontype = vga_disable_state;
	ad.subid = 0;	// prev			
	ad.data.i[0] = !(parts[editorpart].database_id > 0);
	post_events_to_editor(ad);
	ad.subid = 1;	// next			
	ad.data.i[0] = !(parts[editorpart].database_id >= 0);
	post_events_to_editor(ad);

	ad.actiontype = vga_intval;
	ad.id = ip_replace_sample;
	ad.subid = 0;
	ad.data.i[0] = toggled_samplereplace?1:0;
	post_events_to_editor(ad);

	// solo setting
	ad.id = ip_solo;
	ad.subid = 0;
	ad.actiontype = vga_intval;
	ad.data.i[0] = selected->get_solo()?1:0;
	post_events_to_editor(ad);	

	// set lfoload to "load"
	ad.id = ip_lfo_load;
	ad.subid = -1;
	ad.actiontype = vga_intval;
	ad.data.i[0] = 0;
	post_events_to_editor(ad);	

	bool has_sample = false;
	int z = selected->get_active_zone();
	if(zone_exist(z))
	{
		if(selected->n_selected_zones() > 1)
		{
			ad.id = ip_wavedisplay;
			ad.subid = 0;
			ad.actiontype = vga_wavedisp_multiselect;
			post_events_to_editor(ad);
		}
		else
		{
			ad.id = ip_wavedisplay;
			ad.subid = 0;
			ad.actiontype = vga_wavedisp_sample;
			sample *sptr;
			if(zones[z].sample_id < 0) sptr = 0;
			else sptr = samples[zones[z].sample_id];
			ad.data.ptr[0] = sptr;
			ad.data.i[2] = zones[z].playmode;
			ad.data.i[3] = zones[z].sample_start;
			ad.data.i[4] = zones[z].sample_stop;
			ad.data.i[5] = zones[z].loop_start;
			ad.data.i[6] = zones[z].loop_end;
			ad.data.i[7] = zones[z].loop_crossfade_length;
			ad.data.i[8] = zones[z].n_hitpoints;
			post_events_to_editor(ad);
			if(zones[z].playmode == pm_forward_hitpoints)
			{
				for(int i=0; i<zones[z].n_hitpoints; i++)
				{
					ad.actiontype = vga_wavedisp_editpoint;
					ad.data.i[0] = 5 + i;
					ad.data.i[1] = zones[z].hp[i].start_sample;
					ad.data.i[2] = zones[z].hp[i].end_sample;
					ad.data.i[3] = zones[z].hp[i].muted;
					ad.data.f[4] = zones[z].hp[i].env;
					post_events_to_editor(ad);
				}
			}
			if(sptr)
			{
				has_sample = true;
				ad.actiontype = vga_text;
				vtCopyString(ad.data.str,sptr->GetName(),actiondata_maxstring);
				ad.id = ip_sample_name;
				ad.subid = -1;	
				post_events_to_editor(ad);

				ad.actiontype = vga_text;
				_snprintf(ad.data.str,actiondata_maxstring,"\t%.3f MB\t\t%i sm\t\t%.1fkHz %s %iCh\t\tRef: %i\t",sptr->GetDataSize()/(1024.f*1024.f),sptr->sample_length,sptr->sample_rate*0.001f,sptr->UseInt16?"16i":"32f",sptr->channels,sptr->GetRefCount());
				ad.id = ip_sample_metadata;
				ad.subid = -1;
				post_events_to_editor(ad);
			}
		}
				
		// prev/next buttons				
		// TODO locate existing samples in database
		/*if(zones[z].database_id == -1)
		{
			for(int i=0; i<editor->browserd.find_file(0,zones[z].f
			zones[z].database_id = -2;
		}*/
		ad.id = ip_sample_prevnext;			
		ad.actiontype = vga_disable_state;
		ad.subid = 0;	// prev			
		ad.data.i[0] = !(zones[z].database_id > 0);
		post_events_to_editor(ad);
		ad.subid = 1;	// next			
		ad.data.i[0] = !(zones[z].database_id >= 0);
		post_events_to_editor(ad);

		// modmatrix
		modmatrix mm;
		mm.assign(conf,&zones[z],&parts[editorpart]);
		post_initdata_mm(z);

		for(int i=0; i<mm_entries; i++)
		{
			ad.id = ip_mm_amount;
			ad.subid = i;
			ad.actiontype = vga_datamode;			
			int cmode = mm.get_destination_ctrlmode(zones[z].mm[i].destination);
			string s = datamode_from_cmode(cmode);
			vtCopyString((char*)ad.data.str,s.c_str(),actiondata_maxstring);
			post_events_to_editor(ad);
		}

		post_data_from_structure((char*)&zones[z],ip_zone_params_begin,ip_zone_params_end);
	
		// zone filters		
		post_zone_filterdata(z,0);
		post_zone_filterdata(z,1);		

		// lfo stepdata		
		ad.subid = -1;
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = false;
		ad.id = ip_lfo_load; post_events_to_editor(ad);	
		ad.id = ip_replace_sample;	post_events_to_editor(ad);	
		ad.id = ip_sample_prevnext;	post_events_to_editor(ad);	
		ad.id = ip_lfosteps;
		post_events_to_editor(ad);	
		for(int i=0; i<3; i++)
		{
			ad.id = ip_lfosteps;
			ad.subid = i;
			ad.actiontype = vga_steplfo_repeat;
			ad.data.i[0] = zones[z].LFO[i].repeat;
			post_events_to_editor(ad);	
			ad.actiontype = vga_steplfo_shape;
			ad.data.f[0] = zones[z].LFO[i].smooth;
			post_events_to_editor(ad);	

			ad.id = ip_lfosteps;
			ad.subid = i;
			ad.actiontype = vga_steplfo_data_single;
			for(int s=0; s<32; s++)
			{
				ad.data.i[0] = s;
				ad.data.f[1] = zones[z].LFO[i].data[s];
				post_events_to_editor(ad);	
			}
			//ad.data.i[0];			
		}		
	}
	else 
	{
		ad.actiontype = vga_disable_state;
		ad.data.i[0] = true;
		post_control_range(ad,ip_zone_params_begin, ip_zone_params_end,-1);
		ad.id = ip_lfosteps;
		ad.subid = -1;				
		post_events_to_editor(ad);	
		ad.id = ip_lfo_load;	post_events_to_editor(ad);	
		ad.id = ip_replace_sample;	post_events_to_editor(ad);	
		ad.id = ip_sample_prevnext;	post_events_to_editor(ad);	

		ad.actiontype = vga_hide;
		ad.data.i[0] = true;
		post_control_range(ad,ip_filter1_fp, ip_filter2_fp,0,n_filter_parameters-1);
		post_control_range(ad,ip_filter1_ip, ip_filter2_ip,0,n_filter_iparameters-1);

		if(selected->n_selected_zones() > 1)
		{
			ad.id = ip_wavedisplay;
			ad.subid = 0;
			ad.actiontype = vga_wavedisp_multiselect;
			post_events_to_editor(ad);
		}
		else
		{
			ad.id = ip_wavedisplay;
			ad.subid = 0;
			ad.actiontype = vga_wavedisp_sample;
			ad.data.ptr[0] = 0;			
			post_events_to_editor(ad);
		}
	}
	if(!has_sample)
	{
		ad.actiontype = vga_text;
		ad.data.str[0] = 0;
		ad.id = ip_sample_name;
		ad.subid = -1;	
		post_events_to_editor(ad);

		ad.actiontype = vga_text;
		ad.data.str[0] = 0;
		ad.id = ip_sample_metadata;
		ad.subid = -1;
		post_events_to_editor(ad);
	}
	// part data
	int p = editorpart&0xf;
	post_part_filterdata(p,0);
	post_part_filterdata(p,1);	
	post_data_from_structure((char*)&parts[editorpart&0xf],ip_part_params_begin,ip_part_params_end);

	// part controls
	for(int i=0; i<n_custom_controllers; i++)
	{
		ad.id = ip_part_userparam_value;
		ad.subid = i;
		
		if (parts[p].userparameterpolarity[i]) 
			vtCopyString((char*)ad.data.str,"f,-1,0.005,1,1,%",actiondata_maxstring);
		else 
			vtCopyString((char*)ad.data.str,"f,0,0.005,1,1,%",actiondata_maxstring);

		ad.actiontype = vga_datamode;
		post_events_to_editor(ad);						
		//vtCopyString((char*)ad.data.str,conf->MIDIcontrol[i].name[0]?conf->MIDIcontrol[i].name:"-",actiondata_maxstring);
		//vtCopyString((char*)ad.data.str,parts[p].userparametername[i][0]?parts[p].userparametername[i]:"-",actiondata_maxstring);		
	}

	// lfo presets
	ad.id = ip_lfo_load;
	ad.subid = -1;
	ad.actiontype = vga_entry_clearall;
	post_events_to_editor(ad);
	ad.actiontype = vga_entry_add_ival_from_self_with_id;	
	for(int i=0; i<n_lfopresets; i++)	
	{				
		ad.data.i[0] = i;
		vtCopyString((char*)&ad.data.str[4],lfopreset_abberations[i],actiondata_maxstring-4);
		post_events_to_editor(ad);		
	}	
	ad.actiontype = vga_intval;
	ad.data.i[0] = 0;
	post_events_to_editor(ad);	

	// multi
	for(int i=0; i<8; i++) post_multi_filterdata(i,false);
	post_data_from_structure((char*)&multi,ip_multi_params_begin,ip_multi_params_end);	
	// multi filters 
	ad.id = ip_multi_filter_type;
	ad.subid = -1;
	ad.actiontype = vga_entry_clearall;
	post_events_to_editor(ad);
	ad.actiontype = vga_entry_add_ival_from_self_with_id;
	
	ad.data.i[0] = 0;	sprintf((char*)&ad.data.str[4],"-"); post_events_to_editor(ad);	
	
	for(int i=ft_part_first; i<=ft_part_last; i++)
	{				
		ad.data.i[0] = i;
		sprintf((char*)&ad.data.str[4],"%s",filter_descname[i]);
		post_events_to_editor(ad);		
	}
}

//-------------------------------------------------------------------------------------------------

void sampler::post_control_range(actiondata ad, int id_start, int id_end, int subid_start, int subid_end)
{
	if(subid_start == -1) subid_end = -1;

	for(int i=id_start; i<=id_end; i++)
	{
		for(int j=subid_start; j<=subid_end; j++)
		{
			ad.id = i;
			ad.subid = j;						
			post_events_to_editor(ad);	
		}
	}
}

//-------------------------------------------------------------------------------------------------

void sampler::post_data_from_structure(char *pointr, int id_start, int id_end)
{	
	for(int i=id_start; i<=id_end; i++)
	{
		for(int j=0; j<ip_data[i].n_subid; j++)
		{
			actiondata ad;			 
			char *ptr = pointr + ip_data[i].ptr_offset + ip_data[i].subid_ptr_offset*j;
			ad.id = i;
			ad.subid = j;
			if(ip_data[i].vtype == ipvt_int) 
			{
				ad.actiontype = vga_intval;
				ad.data.i[0] = *((int*)ptr);
			}
			else if(ip_data[i].vtype == ipvt_float) 
			{
				ad.actiontype = vga_floatval;
				ad.data.f[0] = *((float*)ptr);
			}
			else if(ip_data[i].vtype == ipvt_string) 
			{
				ad.actiontype = vga_text;														
				vtCopyString(ad.data.str,(char*)ptr,32);										
			}
			post_events_to_editor(ad);
			ad.actiontype = vga_disable_state;
			ad.data.i[0] = false;
			post_events_to_editor(ad);
		}
	}
}

//-------------------------------------------------------------------------------------------------

void sampler::post_zone_filterdata(int z, int i, bool send_data)
{
	if(z<0) return;
	filter *tf = spawn_filter(zones[z].Filter[i].type,zones[z].Filter[i].p,zones[z].Filter[i].ip,0,false);
	int np = 0;
	int nip = 0;
	actiondata ad;
	if(tf)
	{		
		np = tf->get_parameter_count();
		nip = tf->get_ip_count();
		for(int j=0; j<np; j++)
		{			
			ad.id = ip_filter1_fp + i;
			ad.subid = j;
			ad.actiontype = vga_label;
			vtCopyString((char*)ad.data.str,tf->get_parameter_label(j),actiondata_maxstring);
			post_events_to_editor(ad);
			ad.actiontype = vga_hide;
			ad.data.i[0] = false;
			post_events_to_editor(ad);															
			vtCopyString((char*)ad.data.str,tf->get_parameter_ctrlmode_descriptor(j),actiondata_maxstring);
			ad.actiontype = vga_datamode;
			post_events_to_editor(ad);						
		}
		for(int j=0; j<nip; j++)
		{
			ad.id = ip_filter1_ip + i;
			ad.subid = j;
			ad.actiontype = vga_hide;
			ad.data.i[0] = false;
			post_events_to_editor(ad);	

			/*
			// if using optionmenus
			ad.actiontype = vga_entry_clearall;
			post_events_to_editor(ad);
			ad.actiontype = vga_entry_add_ival_from_self;

			int ne = tf->get_ip_entry_count(j);
			for(int k=0; k<ne; k++)
			{
			vtCopyString((char*)ad.data.str,tf->get_ip_entry_label(j,k),actiondata_maxstring);
			post_events_to_editor(ad);
			}*/

			// if using multibuttons					
			ad.actiontype = vga_label;
			char tmp[512]; tmp[0] = 0;
			int ne = tf->get_ip_entry_count(j);
			for(int k=0; k<ne; k++)
			{
				if(k) strcat(tmp,";");
				strcat(tmp,tf->get_ip_entry_label(j,k));						
			}					
			vtCopyString((char*)ad.data.str,tmp,actiondata_maxstring);						
			post_events_to_editor(ad);
		}

		spawn_filter_release(tf);
	}
	
	ad.actiontype = vga_hide;
	ad.data.i[0] = true;
	post_control_range(ad,ip_filter1_fp+i, ip_filter1_fp+i,np,n_filter_parameters-1);
	post_control_range(ad,ip_filter1_ip+i, ip_filter1_ip+i,nip,n_filter_iparameters-1);

	if (send_data) post_data_from_structure((char*)&zones[z],ip_filter_type,ip_filter2_ip);
}

//-------------------------------------------------------------------------------------------------

void sampler::post_part_filterdata(int p, int i, bool send_data)
{
	filter *tf = spawn_filter(parts[p].Filter[i].type,parts[p].Filter[i].p,parts[p].Filter[i].ip,0,false);
	actiondata ad;
	int np = 0;
	int nip = 0;
	if(tf)
	{
		np = tf->get_parameter_count();
		nip = tf->get_ip_count();
		for(int j=0; j<np; j++)
		{
			int cm = tf->get_parameter_ctrlmode(j);
			ad.id = ip_part_filter1_fp + i;
			ad.subid = j;
			ad.actiontype = vga_label;
			vtCopyString((char*)ad.data.str,tf->get_parameter_label(j),actiondata_maxstring);
			post_events_to_editor(ad);
			ad.actiontype = vga_hide;
			ad.data.i[0] = false;
			post_events_to_editor(ad);															
			vtCopyString((char*)ad.data.str,tf->get_parameter_ctrlmode_descriptor(j),actiondata_maxstring);
			ad.actiontype = vga_datamode;
			post_events_to_editor(ad);						
		}
		for(int j=0; j<nip; j++)
		{
			ad.id = ip_part_filter1_ip + i;
			ad.subid = j;
			ad.actiontype = vga_hide;
			ad.data.i[0] = false;
			post_events_to_editor(ad);	

			// if using multibuttons					
			ad.actiontype = vga_label;
			char tmp[512]; tmp[0] = 0;
			int ne = tf->get_ip_entry_count(j);
			for(int k=0; k<ne; k++)
			{
				if(k) strcat(tmp,";");
				strcat(tmp,tf->get_ip_entry_label(j,k));						
			}					
			vtCopyString((char*)ad.data.str,tmp,actiondata_maxstring);						
			post_events_to_editor(ad);
			
		}
		spawn_filter_release(tf);
	}	
	
	ad.actiontype = vga_hide;
	ad.data.i[0] = true;
	post_control_range(ad,ip_part_filter1_fp+i, ip_part_filter1_fp+i,np,n_filter_parameters-1);
	post_control_range(ad,ip_part_filter1_ip+i, ip_part_filter1_ip+i,nip,n_filter_iparameters-1);
	
	if (send_data) post_data_from_structure((char*)&parts[editorpart&0xf],ip_part_filter_type,ip_part_filter2_ip);
}

//-------------------------------------------------------------------------------------------------

void sampler::post_multi_filterdata(int i, bool send_data)
{
	filter *tf = spawn_filter(multi.Filter[i].type,multi.Filter[i].p,multi.Filter[i].ip,0,false);
	actiondata ad;
	int np = 0;
	int nip = 0;
	if(tf)
	{
		np = tf->get_parameter_count();
		nip = tf->get_ip_count();
		for(int j=0; j<np; j++)
		{
			int cm = tf->get_parameter_ctrlmode(j);
			ad.id = ip_multi_filter_fp1 + j;
			ad.subid = i;
			ad.actiontype = vga_label;
			vtCopyString((char*)ad.data.str,tf->get_parameter_label(j),actiondata_maxstring);
			post_events_to_editor(ad);
			ad.actiontype = vga_hide;
			ad.data.i[0] = false;
			post_events_to_editor(ad);															
			vtCopyString((char*)ad.data.str,tf->get_parameter_ctrlmode_descriptor(j),actiondata_maxstring);
			ad.actiontype = vga_datamode;
			post_events_to_editor(ad);						
		}
		for(int j=0; j<nip; j++)
		{
			ad.id = ip_multi_filter_ip1 + j;
			ad.subid = i;
			ad.actiontype = vga_hide;
			ad.data.i[0] = false;
			post_events_to_editor(ad);	

			// if using multibuttons					
			ad.actiontype = vga_label;
			char tmp[512]; tmp[0] = 0;
			int ne = tf->get_ip_entry_count(j);
			for(int k=0; k<ne; k++)
			{
				if(k) strcat(tmp,";");
				strcat(tmp,tf->get_ip_entry_label(j,k));						
			}					
			vtCopyString((char*)ad.data.str,tmp,actiondata_maxstring);						
			post_events_to_editor(ad);

		}
		spawn_filter_release(tf);
	}
	
	ad.actiontype = vga_hide;
	ad.data.i[0] = true;
	post_control_range(ad,ip_multi_filter_fp1+np, ip_multi_filter_fp9,i,i);
	post_control_range(ad,ip_multi_filter_ip1+nip, ip_multi_filter_ip2,i,i);		

	// update the names of part & zone outputs 
	int ft = multi.Filter[i].type;
	ad.actiontype = vga_entry_replace_label_on_id;
	ad.id = ip_zone_aux_output;
	ad.subid = -1;	// send to all
	ad.data.i[0] = out_fx1 + i;
	if(valid_filtertype(ft)) sprintf((char*)&ad.data.str[4],"%s - %s",output_abberations[out_fx1+i],filter_descname[ft]);
	else sprintf((char*)&ad.data.str[4],"%s",output_abberations[out_fx1+i]);
	ad.id = ip_zone_aux_output;
	post_events_to_editor(ad);		
	ad.id = ip_part_aux_output;
	post_events_to_editor(ad);		

	if (send_data) post_data_from_structure((char*)&multi,ip_multi_filter_type,ip_multi_filter_ip2);
}

//-------------------------------------------------------------------------------------------------

void sampler::relay_data_to_structure(actiondata ad, char *pt)
{	
	pt += ip_data[ad.id].ptr_offset + ip_data[ad.id].subid_ptr_offset*ad.subid;		 
	// pointer changed, make sure not to reuse it

	if((ad.actiontype == vga_intval)&&(ip_data[ad.id].vtype == ipvt_int))
	{						
		*((int*)pt) = ad.data.i[0];						
	}
	else if((ad.actiontype == vga_intval)&&(ip_data[ad.id].vtype == ipvt_char))
	{
		//selected->set_zone_parameter_char_internal(offset, ad.data.i[0]&0xff);
	}
	else if((ad.actiontype == vga_floatval)&&(ip_data[ad.id].vtype == ipvt_float))
	{
		*((float*)pt) = ad.data.f[0];						
	}
	else if((ad.actiontype == vga_text)&&(ip_data[ad.id].vtype == ipvt_string))
	{
		vtCopyString(pt,(char*)ad.data.str,32);
		//selected->set_zone_parameter_cstr_internal(offset, (char*)ad.data.str);
	}
}

//-------------------------------------------------------------------------------------------------

void sampler::post_initdata_mm(int zone)
{
	if(!zone_exist(zone)) return;

	actiondata ad;
	{
		// zone matrix
		modmatrix mm;
		mm.assign(conf,&zones[zone],&parts[editorpart]);
		ad.subid = -1;	// send to all
		ad.actiontype = vga_entry_clearall;
		ad.id = ip_mm_src;
		post_events_to_editor(ad);
		ad.id = ip_mm_src2;
		post_events_to_editor(ad);
		ad.actiontype = vga_entry_add_ival_from_self;	
		for(int i=0; i<mm.get_n_sources(); i++)
		{						
			vtCopyString(ad.data.str,mm.get_source_name(i),actiondata_maxstring);
			ad.id = ip_mm_src;
			post_events_to_editor(ad);
			ad.id = ip_mm_src2;
			post_events_to_editor(ad);
		}	
		ad.id = ip_mm_dst;
		ad.subid = -1;	// send to all
		ad.actiontype = vga_entry_clearall;
		post_events_to_editor(ad);
		ad.actiontype = vga_entry_add_ival_from_self;	
		for(int i=0; i<mm.get_n_destinations(); i++)
		{				
			vtCopyString(ad.data.str,mm.get_destination_name(i),actiondata_maxstring);
			post_events_to_editor(ad);
		}

		ad.id = ip_mm_curve;
		ad.subid = -1;	// send to all
		ad.actiontype = vga_entry_clearall;
		post_events_to_editor(ad);
		ad.actiontype = vga_entry_add_ival_from_self;	
		for(int i=0; i<mmc_num_types; i++)
		{				
			vtCopyString(ad.data.str,mmc_abberations[i],actiondata_maxstring);
			post_events_to_editor(ad);
		}
	}	
}

//-------------------------------------------------------------------------------------------------

void sampler::post_initdata_mm_part()
{
	// part matrix
	actiondata ad;
	{
		modmatrix mm;
		mm.assign(conf,0,&parts[editorpart]);
		ad.subid = -1;	// send to all
		ad.actiontype = vga_entry_clearall;
		ad.id = ip_part_mm_src;
		post_events_to_editor(ad);
		ad.id = ip_part_mm_src2;
		post_events_to_editor(ad);
		ad.actiontype = vga_entry_add_ival_from_self;	
		for(int i=0; i<mm.get_n_sources(); i++)
		{						
			vtCopyString(ad.data.str,mm.get_source_name(i),actiondata_maxstring);
			ad.id = ip_part_mm_src;
			post_events_to_editor(ad);
			ad.id = ip_part_mm_src2;
			post_events_to_editor(ad);
		}	
		ad.id = ip_part_mm_dst;
		ad.subid = -1;	// send to all
		ad.actiontype = vga_entry_clearall;
		post_events_to_editor(ad);
		ad.actiontype = vga_entry_add_ival_from_self;	
		for(int i=0; i<mm.get_n_destinations(); i++)
		{				
			vtCopyString(ad.data.str,mm.get_destination_name(i),actiondata_maxstring);
			post_events_to_editor(ad);
		}

		ad.id = ip_part_mm_curve;
		ad.subid = -1;	// send to all
		ad.actiontype = vga_entry_clearall;
		post_events_to_editor(ad);
		ad.actiontype = vga_entry_add_ival_from_self;	
		for(int i=0; i<mmc_num_types; i++)
		{				
			vtCopyString(ad.data.str,mmc_abberations[i],actiondata_maxstring);
			post_events_to_editor(ad);
		}

		// note-on conditions
		ad.actiontype = vga_entry_clearall;
		ad.id = ip_part_nc_src; 	post_events_to_editor(ad);
		ad.id = ip_nc_src; 			post_events_to_editor(ad);
		ad.subid = -1;
		ad.actiontype = vga_entry_add_ival_from_self;	
		for(int i=0; i<mm.get_n_sources(); i++)
		{						
			vtCopyString(ad.data.str,mm.get_source_name(i),actiondata_maxstring);
			ad.id = ip_nc_src;
			post_events_to_editor(ad);
			ad.id = ip_part_nc_src;
			post_events_to_editor(ad);
		}
	}
}

//-------------------------------------------------------------------------------------------------

void sampler::post_initdata()
{	
	// fill various option-menus etc etc

	actiondata ad;
	ad.id = ip_playmode;
	ad.subid = -1;	// send to all
	ad.actiontype = vga_entry_clearall;
	post_events_to_editor(ad);
	ad.actiontype = vga_entry_add_ival_from_self;
	for(int i=0; i<n_playmodes; i++)
	{				
		vtCopyString((char*)ad.data.str,playmode_names[i],actiondata_maxstring);
		post_events_to_editor(ad);
	}		

	// output selection
	ad.id = ip_zone_aux_output;
	ad.subid = -1;	// send to all
	ad.actiontype = vga_entry_clearall;
	post_events_to_editor(ad);
	ad.actiontype = vga_entry_add_ival_from_self_with_id;	
	
	for(int i=out_part; i<n_output_types; i++)
	{				
		ad.data.i[0] = i;
		sprintf((char*)&ad.data.str[4],"%s",output_abberations[i]);
		if (is_output_visible(i, mNumOutputs)) post_events_to_editor(ad);
	}	

	// part output selection
	ad.id = ip_part_aux_output;
	ad.subid = -1;	// send to all
	ad.actiontype = vga_entry_clearall;
	post_events_to_editor(ad);
	ad.actiontype = vga_entry_add_ival_from_self_with_id;	
	
	for(int i=out_output1; i<n_output_types; i++)
	{				
		ad.data.i[0] = i;
		sprintf((char*)&ad.data.str[4],"%s",output_abberations[i]);
		if (is_output_visible(i, mNumOutputs)) post_events_to_editor(ad);
	}

	// multi filter output selection
	ad.id = ip_multi_filter_output;
	ad.subid = -1;	// send to all
	ad.actiontype = vga_entry_clearall;
	post_events_to_editor(ad);
	ad.actiontype = vga_entry_add_ival_from_self_with_id;	
	
	for(int i=out_output1; i<out_fx1; i++)
	{				
		ad.data.i[0] = i;
		sprintf((char*)&ad.data.str[4],"%s",output_abberations[i]);
		if (is_output_visible(i, mNumOutputs)) post_events_to_editor(ad);
	}

	// filters
	ad.id = ip_filter_type;
	ad.subid = -1;
	ad.actiontype = vga_entry_clearall;
	post_events_to_editor(ad);
	ad.actiontype = vga_entry_add_ival_from_self_with_id;	

	ad.data.i[0] = 0;	sprintf((char*)&ad.data.str[4],"-"); post_events_to_editor(ad);	

	for(int i=ft_zone_first; i<=ft_zone_last; i++)
	{				
		ad.data.i[0] = i;
		sprintf((char*)&ad.data.str[4],"%s",filter_descname[i],actiondata_maxstring-4);
		post_events_to_editor(ad);		
	}	

	// part filters
	ad.id = ip_part_filter_type;
	ad.subid = -1;
	ad.actiontype = vga_entry_clearall;
	post_events_to_editor(ad);
	ad.actiontype = vga_entry_add_ival_from_self_with_id;
	
	ad.data.i[0] = 0;	sprintf((char*)&ad.data.str[4],"-"); post_events_to_editor(ad);	
	
	for(int i=ft_part_first; i<=ft_part_last; i++)
	{				
		ad.data.i[0] = i;
		sprintf((char*)&ad.data.str[4],"%s",filter_descname[i]);
		post_events_to_editor(ad);		
	}	

	// config data

	/*ip_config_outputs,	
	ip_config_slidersensitivity,
	ip_config_controller_id,
	ip_config_controller_mode,	*/
	
	ad.id = ip_config_outputs;
	ad.subid = 0;
	ad.actiontype = vga_intval;
	ad.data.i[0] = conf->stereo_outputs-1;
	post_events_to_editor(ad);	
	for(int i = 0; i<n_custom_controllers; i++)
	{
		ad.id = ip_config_controller_id;
		ad.subid = i;
		ad.actiontype = vga_intval;
		ad.data.i[0] = conf->MIDIcontrol[i].number;
		post_events_to_editor(ad);	
		
		ad.id = ip_config_controller_mode;
		ad.actiontype = vga_intval;
		ad.data.i[0] = conf->MIDIcontrol[i].type;
		post_events_to_editor(ad);	
	}	

	ad.id = ip_config_kbdmode;
	ad.subid = 0;
	ad.actiontype = vga_intval;
	ad.data.i[0] = conf->keyboardmode;	
	post_events_to_editor(ad);	

	ad.id = ip_config_previewvolume;
	ad.actiontype = vga_floatval;
	ad.data.f[0] = conf->mPreviewLevel;
	post_events_to_editor(ad);	

	ad.id = ip_config_autopreview;
	ad.actiontype = vga_intval;
	ad.data.i[0] = conf->mAutoPreview ? 1 : 0;
	post_events_to_editor(ad);	

	ad.id = ip_browser_previewbutton;
	ad.actiontype = vga_intval;
	ad.data.i[0] = 0;
	post_events_to_editor(ad);	

	post_initdata_mm_part();

	post_zonedata();	
}

//-------------------------------------------------------------------------------------------------

string datamode_from_cmode(int cmode)
{
	switch(cmode)
	{
	case cm_mod_decibel:
		return "f,-144,0.1,144,0,dB";
		break;
	case cm_mod_pitch:
		return "f,-128,0.1,128,0,st";
		break;
	case cm_mod_freq:
		return "f,-12,0.01,12,0,oct";
		break;
	case cm_mod_percent:
		return "f,-32,0.01,32,1,%";
		break;
	};
	return "f,-32,0.1,32,0,";
};

//-------------------------------------------------------------------------------------------------

void sampler::post_samplelist()
{
	int n_samples = 0;
	for(int i=0; i<max_samples; i++)
	{
		if(samples[i])
		{
			n_samples++;
		}
	}

	if(!n_samples) goto submit;	
	
	if(dbSampleListDataPtr) 
	{		
		if (*((int*)dbSampleListDataPtr) == 'done') free(dbSampleListDataPtr);
		else return;	// upptagen
	}

	dbSampleListDataPtr = (void*)malloc(sizeof(database_samplelist)*n_samples);

	int j = 0;
	for(int i=0; i<max_samples; i++)
	{
		if(samples[i])
		{
			database_samplelist *dbe = (database_samplelist*)((char*)dbSampleListDataPtr + sizeof(database_samplelist)*j);
			dbe->id = i;
			vtCopyString(dbe->name, samples[i]->GetName(), 64);
			dbe->refcount = samples[i]->GetRefCount();
			dbe->size = samples[i]->GetDataSize();
			dbe->type = samples[i]->Embedded ? 1 : 0;

			j++;
		}
	}
submit:
	actiondata ad;	
	ad.actiontype = vga_database_samplelist;	
	ad.data.ptr[0] = n_samples ? dbSampleListDataPtr : 0;
	ad.data.i[2] = n_samples;
	ad.subid = -1;	
	ad.id = -1;	
	post_events_to_editor(ad);
}

//-------------------------------------------------------------------------------------------------