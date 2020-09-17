#if 0

#include "scpb_sampler.h"
#include <windows.h>
#include "globals.h"
#include "mathtables.h"
#include "../common/vstcontrols.h"
#include "scpb_editor.h"
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include "patch_select_dialog.h"

scpb_sampler::scpb_sampler(AEffEditor *editor, AudioEffectX	*effect)
: sampler(editor)
{	
	this->effect = effect;
	PCH = 0;
	CC0 = 0;	
	buildpatchlist();
	currentpatch = -1;
	changepatch(0);
	set_headroom(0);
	adsr[0] = 0;
	adsr[1] = 0;
	adsr[2] = 0;
	adsr[3] = 0;
	mastergain = 1.f;
	scpbtranspose = 0;
	fxbypass = false;
	polyphony_cap = 16;
	learn_tag = -1;		
	
	controllers_target[c_custom0 + kAmplitude] = 1.f;
}

scpb_sampler::~scpb_sampler()
{
	clean_up_tree(&categorylist,0);
}

float scpb_sampler::get_ccontrol(int id)
{
	if(within_range(0,id,15))
	{
		if(customcontrollers_bp[id]) return 0.5f + 0.5f*controllers_target[c_custom0 + id];
		return controllers_target[c_custom0 + id];
	}
	return 0.f;
}

bool scpb_sampler::is_ccontrol_bipolar(int id)
{
	if(within_range(0,id,15))
	{
		return customcontrollers_bp[id];
	}
	return false;
}

float scpb_sampler::get_ccontrol_lag(int id)
{
	if(within_range(0,id,15))
	{		
		return controllers[c_custom0 + id];
	}
	return 0.f;
}

void scpb_sampler::set_ccontrol(int id,float v)
{
	if(within_range(0,id,15))
	{
		if(customcontrollers_bp[id])
			controllers_target[c_custom0 + id] = 2.f*v - 1.f;
		else
			controllers_target[c_custom0 + id] = v;
	}
}

const char* scpb_sampler::getCategoryTitle()
{
//	if((currentcategory>=0)&&(currentcategory<categories.size())) return categories.at(currentcategory).c_str();
	return 0;
}

const char* scpb_sampler::getPatchTitle()
{
	if((currentpatch>=0)&&(currentpatch<patchlist.size())&&patchlist.size()) return patchlist.at(currentpatch).name.c_str();
	return 0;
}

void scpb_sampler::clean_up_tree(category_entry* e,int depth)
{	
	for(int i=0; i<e->subcategories.size(); i++)
	{
		clean_up_tree(e->subcategories.at(i),depth+1);
	}
	e->subcategories.clear();
	if(depth > 0) delete e;
}

// move somewhere else
void Trim(std::string& str, const std::string & ChrsToTrim = " \t\n\r", int TrimDir = 0)
{
	size_t startIndex = str.find_first_not_of(ChrsToTrim);
	if (startIndex == std::string::npos){str.erase(); return;}
	if (TrimDir < 2) str = str.substr(startIndex, str.size()-startIndex);
	if (TrimDir!=1) str = str.substr(0, str.find_last_not_of(ChrsToTrim) + 1);
}
inline void TrimRight(std::string& str, const std::string & ChrsToTrim = " \t\n\r")
{
	Trim(str, ChrsToTrim, 2);
}

inline void TrimLeft(std::string& str, const std::string & ChrsToTrim = " \t\n\r")
{
	Trim(str, ChrsToTrim, 1);
}

category_entry* scpb_sampler::find_category(int id)
{
	return find_category_recurse(id,&categorylist,0);
}

category_entry* scpb_sampler::find_category_recurse(int id, category_entry *e, int depth)
{
	assert(depth<16);
	if(e->id == id) return e;		
	unsigned int n = e->subcategories.size();
	
	for(unsigned int i=0; i<n; i++)
	{
		category_entry* sub_e = find_category_recurse(id,e->subcategories.at(i),depth++);
		if(sub_e) return sub_e;
	}
	return 0;
}

void scpb_sampler::traverse_dir(string dir, int depth, category_entry *parent)
{			
	assert(depth<16);

	category_entry *ep;

	if(depth == 0) 
	{
		categorylist.id = n_categories++;
		categorylist.name = "(no filter)";
		categorylist.patch_id_start = num_patches;
		categorylist.parent_ptr = 0;
		categorylist.depth = depth;
		ep = &categorylist;
	} 
	else
	{
		category_entry *e = new category_entry();
		e->name.assign(dir);
		e->name.erase(e->name.length()-1);
		e->name.erase(0,e->name.rfind('\\')+1);
		e->parent_ptr = parent;
		e->id = n_categories++;
		e->depth = depth;
		e->patch_id_start = num_patches;
		parent->subcategories.push_back(e);			
		ep = e;
	}
	

	char searchdir[512];
	sprintf(searchdir,"%s*",dir.c_str());	
	
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
		
	hFind = FindFirstFile(searchdir, &FindFileData);		
	if (hFind != INVALID_HANDLE_VALUE) 
	{
		while(1)
		{						
			if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (FindFileData.cFileName[0] == '.'))
			{
				// . and ..
				// do nothing
			}
			else if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{				
				string subdir = dir;				
				subdir.append(FindFileData.cFileName);
				subdir.append("\\");
				traverse_dir(subdir,depth+1,ep);
			} 
			else
			{
				char *extension = strrchr(FindFileData.cFileName,'.');				
				if (extension)
				{										
					if(/*(stricmp(extension,".wav") == 0)||*/(stricmp(extension,".scm") == 0)||(stricmp(extension,".scg") == 0)||(stricmp(extension,".rx2") == 0)||(stricmp(extension,".rcy") == 0)||(stricmp(extension,".rex") == 0))
					{
						patch_entry f;						
						f.path = dir + FindFileData.cFileName;
						f.category = ep->id;						
						*extension = 0;
						f.name = FindFileData.cFileName;						
						patchlist.push_back(f);
						num_patches++;
					} 
					else if(stricmp(extension,".sf2") == 0)
					{
						string sf2file = dir;
						sf2file.append(FindFileData.cFileName);
						midipatch *plist;
						int n = get_sf2_patchlist(sf2file.c_str(), (void**)&plist);
						for(int i=0; i<n; i++)
						{
							patch_entry f;						
							char s[256];
							sprintf(s,"%i",i);
							if((n>1) && (i>0)) f.path = dir + FindFileData.cFileName + ">" + s;
							else f.path = dir + FindFileData.cFileName;
							f.category = ep->id;							

							//f.path.append(i);							
							f.name = plist[i].name;
							// remove leading/trailing white space
							Trim(f.name);
							patchlist.push_back(f);
							num_patches++;							
						}
						delete plist;
					}
					else if(stricmp(extension,".sczip") == 0)
					{
						wxZipEntry *ze;
						string zn = dir;
						zn.append(FindFileData.cFileName);
						wxFileInputStream is(zn.c_str());						
						if(is.Ok())						
						{
							wxZipInputStream zip(is);									
							while(ze = zip.GetNextEntry())
							{
								char in[256];
								strncpy(in, ze->GetInternalName().c_str(), 256);
								char *ext = strrchr(in,'.');

								if(ext && ((stricmp(ext,".scg") == 0)||(stricmp(ext,".scm") == 0)))
								{
									patch_entry z;						
									z.path = dir + FindFileData.cFileName + "|" + in;
									z.category = ep->id;									
									*ext = 0;
									z.name = in;						
									patchlist.push_back(z);
									num_patches++;
								}
								delete ze;
							}														
						}
					}
				}
			}

			if (!FindNextFile(hFind,&FindFileData)) break;			
		}
		FindClose(hFind);	
	}	
	ep->patch_id_end = num_patches;	

	// remove category from list if it doesn't contain any patches
	if(depth && (ep->patch_id_end == ep->patch_id_start))
	{
		parent->subcategories.erase(parent->subcategories.end()-1);
		delete ep;
	}
}

void scpb_sampler::buildpatchlist()
{	
	char subdir[512],libsearch[512];
	extern void* hInstance;
	GetModuleFileName((HMODULE)hInstance,libroot,512);
	char *end = strrchr(libroot,'\\');
	if (!end)
	{
//		MessageBox(
		return;
	}
	patchlist.clear();

	strcpy(end,"\\swissarmylib\\");
	sprintf(libsearch,"%s",libroot);	

	n_categories = 0;
	num_patches = 0;
		
	traverse_dir(libsearch,0,0);
}

void scpb_sampler::programchange(int p)
{
	PCH = p;
	int id = p + (CC0<<7);		

	changepatch(id);	
}

int scpb_sampler::channel_controller(char channel, int cc, int value)
{
	assert(conf);	
	if(learn_tag>=0)
	{		
		int id;
		if((learn_tag>=0)&&(learn_tag<16))
		{
			conf->MIDIcontrol[learn_tag].type = mct_cc;
			conf->MIDIcontrol[learn_tag].number = cc;			
			conf->save(0);
		} 		
		learn_tag = -1;
	}
	if(cc == 0)
	{
		CC0 = value;		
		return 0;
	}
	
	return sampler::channel_controller(channel,cc,value);
}

void scpb_sampler::process_audio()
{
	sampler::process_audio();

	scpb_amplitude.newValue(controllers[c_custom0 + kAmplitude]*controllers[c_custom0 + kAmplitude]);

	for(int k=0; k<block_size; k++)
	{	
		scpb_amplitude.process();
		output[0][k] *= scpb_amplitude.v;
		output[1][k] *= scpb_amplitude.v;
	}
}

void scpb_sampler::browsepatches(int direction)
{
	changepatch(currentpatch+direction);
}

void scpb_sampler::idle()
{
	if(holdengine && (scpb_queue_patch > -1))
	{
		do_changepatch();				
	}
	sampler::idle();
}

void scpb_sampler::changepatch(int id, bool reset_controllers)
{	
	if(currentpatch == id) return;
	if (id >= patchlist.size()) return;	
	scpb_queue_patch = id;
	scpb_queue_rc = reset_controllers;
	((scpb_editor*)editor)->isLoading(true);
}	

void scpb_sampler::do_changepatch()
{	
	int id = currentpatch = scpb_queue_patch;
	scpb_queue_patch = -1;	
	
	//char filename[512];	
	
	//sprintf(filename,"%s%s\\%s.scg",libroot,categories.at(currentcategory).c_str(),patches.at(currentcategory).at(currentpatch).c_str());
	//sprintf(filename,"%s%s\\%s",libroot,categories.at(currentcategory).c_str(),patches.at(currentcategory).at(currentpatch).c_str());
	//strcpy(end,patches.at(currentpatch).c_str());
	free_all();
	
	for(int i=0; i<8; i++)	// clear old controllers
	{
		customcontrollers[i] = "-";
		customcontrollers_bp[i] = false;
	}	

	bool is_wav=false,is_recycle=false,is_sf2=false;
	const char *extension = strchr(patchlist.at(id).path.c_str(),'.');
	if(extension)
	{
		if(strnicmp(extension,".sf2",4) == 0) is_sf2 = true;		
		else if(strnicmp(extension,".wav",4) == 0) is_wav = true;
		else if(strnicmp(extension,".rcy",4) == 0) is_recycle = true;
		else if(strnicmp(extension,".rex",4) == 0) is_recycle = true;
		else if(strnicmp(extension,".rx2",4) == 0) is_recycle = true;		
	}

	if(is_sf2)
	{
		customcontrollers[0] = "cutoff";
		customcontrollers_bp[0] = true;
		customcontrollers[1] = "resonance";
		customcontrollers_bp[1] = true;
	} else if(is_wav || is_recycle)
	{
		customcontrollers[0] = "pitch";
		customcontrollers_bp[0] = true;
		customcontrollers[1] = "scratch";
		customcontrollers_bp[1] = false;

		customcontrollers[2] = "amplitude";
		customcontrollers_bp[2] = true;
		customcontrollers[3] = "pan";
		customcontrollers_bp[3] = true;				
		
		customcontrollers[4] = "filter cutoff";
		customcontrollers_bp[4] = false;
		customcontrollers[5] = "filter resonance";
		customcontrollers_bp[5] = false;				
	
		customcontrollers[6] = "lowcut";
		customcontrollers_bp[6] = false;	
		if(is_wav)
		{
			customcontrollers[7] = "sample start";
			customcontrollers_bp[7] = false;		
		}		
	}

	load_file(patchlist.at(id).path.c_str(),0,0,false,0,0,true);
	//load_file(const char *file_name,int *new_g, int *new_z, bool *is_group, char channel, int add_zones_to_groupid, bool replace)
	//load_all_from_xml(0,0,filename,true);			
	
	if(scpb_queue_rc)	// reset controllers
	{
		effect->setParameter (kAEG_A,0.5f);
		effect->setParameter (kAEG_D,0.5f);
		effect->setParameter (kAEG_S,0.5f);
		effect->setParameter (kAEG_R,0.5f);
		for(int i=0; i<8; i++)
		{
			controllers_target[c_custom0 + i] = 0.f;
			controllers[c_custom0 + i] = 0.f;
			//set_ccontrol(i,0.f);
		}
	}

	if(is_wav || is_recycle)
	{
		if(zone_exist(0))
		{				
			zones[0].key_low = 0;
			zones[0].key_high = 127;
			if((zones[0].key_root == 36) && is_wav) zones[0].key_root = 60;				
			zones[0].filter[0].type = ft_biquadLPHP_serial;
			zones[0].filter[0].p[0] = -4.f;
			zones[0].filter[0].p[1] = 0.f;
			zones[0].filter[0].p[2] = -3.f;
			zones[0].filter[0].p[3] = 0.f;

			int slot = 0;		
			zones[0].mm[slot].source = get_mm_source_id("c1");
			zones[0].mm[slot].destination = get_mm_dest_id("pitch");
			zones[0].mm[slot++].strength = 12.f;
			zones[0].mm[slot].source = get_mm_source_id("c2");
			zones[0].mm[slot].destination = get_mm_dest_id("rate");
			zones[0].mm[slot++].strength = -2.f;
			if(is_wav)
			{
				zones[0].mm[slot].source = get_mm_source_id("c2");
				zones[0].mm[slot].destination = get_mm_dest_id("samplestart");
				zones[0].mm[slot++].strength = 1.f;
			}

			zones[0].mm[slot].source = get_mm_source_id("c3");
			zones[0].mm[slot].destination = get_mm_dest_id("amplitude");
			zones[0].mm[slot++].strength = 18.f;
			zones[0].mm[slot].source = get_mm_source_id("c4");
			zones[0].mm[slot].destination = get_mm_dest_id("pan");
			zones[0].mm[slot++].strength = 1.f;

			zones[0].mm[slot].source = get_mm_source_id("c5");
			zones[0].mm[slot].destination = get_mm_dest_id("f1p3");
			zones[0].mm[slot++].strength = 8.5f;
			zones[0].mm[slot].source = get_mm_source_id("c6");
			zones[0].mm[slot].destination = get_mm_dest_id("f1p4");
			zones[0].mm[slot++].strength = 1.f;

			zones[0].mm[slot].source = get_mm_source_id("c7");
			zones[0].mm[slot].destination = get_mm_dest_id("f1p1");
			zones[0].mm[slot++].strength = 6.0f;	
			if(is_wav)
			{
				zones[0].mm[slot].source = get_mm_source_id("c8");
				zones[0].mm[slot].destination = get_mm_dest_id("samplestart");
				zones[0].mm[slot++].strength = 1.f;			
			}

		}
		// cutoff = 1 as standard
		controllers_target[c_custom0 + 4] = 1.f;
		controllers[c_custom0 + 4] = 1.f;
	}


	((scpb_editor*)editor)->patchChanged();	
	holdengine = false;
}

#endif