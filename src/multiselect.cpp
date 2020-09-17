#include "multiselect.h"
#include <algorithm>
#include "sampler_state.h"
#include "sampler.h"

#include <list>
using std::list;

// usage set_zone_parameter_float(field, value);
// usage set_zone_parameter_float_internal(offsetof(zoneptr,field), value);
void multiselect::set_zone_parameter_float_internal(int offset, float value)
{	
	sample_zone *z;
	list<int>::const_iterator iter;
	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		if(sobj->zone_exist(*iter))
		{
			z = &sobj->zones[*iter];
			char *adr = (char*)z;
			adr += offset;

			float *v = (float*)adr;
			*v = value;
		}
	}	
}

void multiselect::set_zone_parameter_cstr_internal(int offset, char* value)
{	
	sample_zone *z;
	list<int>::const_iterator iter;
	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		if(sobj->zone_exist(*iter))
		{
			z = &sobj->zones[*iter];
			char *adr = (char*)z;
			adr += offset;
			
			vtCopyString(adr,value,32);
		}
	}	
}


void multiselect::set_zone_parameter_int_internal(int offset, int value)
{	
	sample_zone *z;
	list<int>::const_iterator iter;
	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		if(sobj->zone_exist(*iter))
		{
			z = &sobj->zones[*iter];
			char *adr = (char*)z;
			adr += offset;

			int *v = (int*)adr;
			*v = value;
		}
	}	
}

void multiselect::set_zone_parameter_char_internal(int offset, char value)
{	
	sample_zone *z;
	list<int>::const_iterator iter;
	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		if(sobj->zone_exist(*iter))
		{
			z = &sobj->zones[*iter];
			char *adr = (char*)z;
			adr += offset;				
			*adr = value;
		}
	}	
}

void multiselect::move_selected_zones_to_part(int part)
{
	if(part>15) return;	
	list<int>::const_iterator iter;	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		if(sobj->zone_exist(*iter))
		{
			sobj->zones[*iter].part = part;			
		}
	}	
}
void multiselect::move_selected_zones_to_layer(int layer)
{
	if(layer>num_layers) return;		
	list<int>::const_iterator iter;	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		if(sobj->zone_exist(*iter))
		{
			sobj->zones[*iter].layer = layer;			
		}
	}
}

void multiselect::toggle_zone_modmatrix_switches()
{	
	list<int>::const_iterator iter;
	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{		
		sobj->update_zone_switches(*iter);		
	}	
}

void multiselect::copy_lfo_waveform_to_selected_zones(steplfostruct *lfo, int entry)
{
	sample_zone *z;
	list<int>::const_iterator iter;
	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		z = &sobj->zones[*iter];
		if ((&z->LFO[entry]) != lfo)
			memcpy(&z->LFO[entry], lfo, sizeof(steplfostruct));
	}	
}

int multiselect::get_selected_zone(int id)
{
	list<int>::const_iterator iter;
	int i=0; 	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		if(i++==id) return *iter;
	}	
	return -1;
}
int multiselect::get_selected_group(int id)
{
	list<int>::const_iterator iter;
	int i=0; 	
	for (iter=selected_groups.begin(); iter != selected_groups.end(); iter++)
	{
		if(i++==id) return *iter;
	}	
	return -1;
}

multiselect::multiselect(sampler *nsobj)
{
	current_group = 0;
	active_id = 0;
	active_type = edt_group;
	sobj = nsobj;
	solo = false;
	this->deselect_all();
}

multiselect::~multiselect()
{	
}

void multiselect::set_active_zone(int id)
{
	this->deselect_all();
	sobj->toggled_samplereplace = false;
	active_id = id;
	active_type = edt_zone;
	this->select_zone(id);	
}

void multiselect::set_active_group(int id)
{
	this->deselect_all();
	active_id = id;
	active_type = edt_group;
	this->select_group(id);
	this->current_group = id;
}

void multiselect::set_active_entry(int type, int id)
{
	switch(type)
	{
	case edt_group:
		set_active_group(id);
		return;
	case edt_zone:
		set_active_zone(id);
		return;
	};	
}

void multiselect::select_zone(int id)
{
	if(find(selected_zones.begin(),selected_zones.end(),id) ==  selected_zones.end())
	{
		selected_zones.push_back(id);
	}
}

void multiselect::select_group(int id)
{
	if(find(selected_groups.begin(),selected_groups.end(),id) ==  selected_groups.end())
	{
		selected_groups.push_back(id);
	}
}

void multiselect::select_entry(int type, int id)
{
	switch(type)
	{
	case edt_group:
		select_group(id);
		return;
	case edt_zone:
		select_zone(id);
		return;
	};
}

void multiselect::toggle_select_zone(int id)
{
	if(find(selected_zones.begin(),selected_zones.end(),id) ==  selected_zones.end())
	{
		selected_zones.push_back(id);
	}
	else
	{
		this->deselect_zone(id);
	}
}

void multiselect::toggle_select_group(int id)
{
	if(find(selected_groups.begin(),selected_groups.end(),id) ==  selected_groups.end())
	{
		selected_groups.push_back(id);
	}
	else
		this->deselect_group(id);
}

void multiselect::toggle_select_entry(int type, int id)
{
	switch(type)
	{
	case edt_group:
		toggle_select_group(id);
		return;
	case edt_zone:
		toggle_select_zone(id);
		return;
	};
}

void multiselect::delete_selected()
{
	list<int>::const_iterator iter;
	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{
		sobj->free_zone(*iter);
	}
	
	this->deselect_all();	
}

void multiselect::mute_zones(bool b)
{
	list<int>::const_iterator iter;	
	for (iter=selected_zones.begin(); iter != selected_zones.end(); iter++)
	{				
		sobj->zones[*iter].mute = b; 
	}	
}

bool multiselect::zone_is_selected(int id)
{
	if(find(selected_zones.begin(),selected_zones.end(),id) !=  selected_zones.end())
	{
		return true;
	}	
	return false;
}
bool multiselect::group_is_selected(int id)
{
	if(find(selected_groups.begin(),selected_groups.end(),id) !=  selected_groups.end())
	{
		return true;
	}	
	return false;
}
bool multiselect::entry_is_selected(int type, int id)
{
	switch(type)
	{
	case edt_group:
		return group_is_selected(id);
	case edt_zone:
		return zone_is_selected(id);
	};
	return false;
}

bool multiselect::zone_is_active(int id)
{
	return entry_is_active(edt_zone,id);
}
bool multiselect::group_is_active(int id)
{
	return entry_is_active(edt_group,id);
}

int multiselect::get_active_zone()
{
	if(active_type == edt_zone) return active_id;
	return -1;
}

bool multiselect::entry_is_active(int type, int id)
{
	return (type==this->active_type)&&(id==this->active_id);
}

void multiselect::deselect_zone(int id){ selected_zones.remove(id); }
void multiselect::deselect_group(int id){ selected_groups.remove(id); }

void multiselect::deselect_all()
{
	this->deselect_groups();
	this->deselect_zones();	
}
void multiselect::deselect_groups(){ selected_groups.clear(); };
void multiselect::deselect_zones(){ selected_zones.clear(); active_id=-1; };