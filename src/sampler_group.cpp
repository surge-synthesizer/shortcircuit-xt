//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------

#if 0
#include "sampler_group.h"
#include "controllers.h"

sampler_group::sampler_group(sample_group *group, timedata *td, float *ctrl, float *autom)
{	
	this->group = group;	
	this->td = td;
	this->ctrl = ctrl;
	this->autom = autom;

	last_et[0] = group->effect[0].type;
	last_et[1] = group->effect[1].type;
	groupLFO = new steplfo(td);		
	mm = new modmatrix(0,0,group,0,this,&ctrl[num_controllers*group->channel],autom,true,true); // must be intiated when all modulation sources exist!!
	groupLFO->assign(&group->groupLFO,mm->get_destination_ptr(md_groupLFO_rate));	
	last_channel = group->channel;	
	fx[0] = spawn_effect(group->effect[0].type,mm->get_destination_ptr(md_effect1prm0),td);
	fx[1] = spawn_effect(group->effect[1].type,mm->get_destination_ptr(md_effect2prm0),td);	
}

sampler_group::~sampler_group()
{
	delete groupLFO;
	delete fx[0];
	delete fx[1];
	delete mm;
}

void sampler_group::process_controlpath()
{	
	if (last_channel != group->channel)
	{
		delete mm;
		mm = new modmatrix(0,0,group,0,this,&ctrl[num_controllers*group->channel],autom,true,true);	
		last_channel = group->channel;
	}
	groupLFO->process(block_size);
	mm->process();
}
	
void sampler_group::clear_busses()
{
	/*memset(e1L,0,sizeof(float)*block_size);
	memset(e1R,0,sizeof(float)*block_size);
	memset(e2L,0,sizeof(float)*block_size);
	memset(e2R,0,sizeof(float)*block_size);*/
	//memset(left,0,sizeof(float)*block_size);
	//memset(right,0,sizeof(float)*block_size);

	// joxa in brus istället för att hindra denormals
	
	int k;
	for(k=0; k<(block_size>>1); k++)
	{
		left[k<<1] = 1E-15f;
		left[(k<<1)+1] = -1E-15f;
		right[k<<1] = 1E-15f;
		right[(k<<1)+1] = -1E-15f;
	}
	left[0] = 1E-12f;
	right[0] = 1E-12f;
}

void sampler_group::process(bool bypass_fx)
{
	// has the effect type changed?
	if(!group->group_mixing) return;
	if(last_et[0] != group->effect[0].type)
	{
		delete fx[0];
		last_et[0] = group->effect[0].type;		
		fx[0] = spawn_effect(group->effect[0].type,mm->get_destination_ptr(md_effect1prm0),td);		
	}
	if(last_et[1] != group->effect[1].type)
	{
		delete fx[1];		
		last_et[1] = group->effect[1].type;		
		fx[1] = spawn_effect(group->effect[1].type,mm->get_destination_ptr(md_effect2prm0),td);
	}

	//if (!group) return;
	int i;		

	float amp = 1.414213562f * powf(10,0.05f * mm->get_destination_value(md_g_amplitude));
	float balance = limit_range(mm->get_destination_value(md_g_balance),-1,1);
	ampL.newValue((float)(sqrt(0.5 - 0.5*balance) * amp));
	ampR.newValue((float)(sqrt(0.5 + 0.5*balance) * amp));	

	if (fx[0] && (!group->effect[0].bypass) && !bypass_fx)	
	{		
		fx[0]->process(left, right);
	}
						
	if (fx[1] && (!group->effect[1].bypass) && !bypass_fx) 
	{			
		fx[1]->process(left, right);
	}

	for(i=0; i<block_size; i++)
	{
		left[i]		*= ampL.v;
		right[i]	*= ampR.v;
				
		ampL.process();
		ampR.process();
	}
}
#endif
