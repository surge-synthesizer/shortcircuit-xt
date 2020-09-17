#include "vt_gui_controls.h"
#include "steplfo.h"

vg_steplfo::vg_steplfo(vg_window *o, int id, TiXmlElement *e) : vg_control(o,id,e)
{
	n_steps = 16;
	shape = 0.f;
	disabled = false;

	memset(step,0,max_steps*sizeof(float));

	restore_data(e);
}
vg_steplfo::~vg_steplfo()
{
}

void vg_steplfo::draw()
{
	draw_needed = false;
	assert(n_steps > 0);
	//draw_background();
	vg_rect r(0,0,surf.sizeX,surf.sizeY);
	surf.draw_rect(r,owner->get_syscolor(col_frame),true);	
	r.inset(1,1);
	if(disabled) 
	{
		surf.fill_rect(r,owner->get_syscolor(col_standard_bg_disabled));	
		return;
	}
	surf.fill_rect(r,owner->get_syscolor(col_LFOgrid));	

	int w = ((r.get_w()+2)<<8) / n_steps;	

	int ym = r.y + (r.get_h()>>1);
	for(int i=0; i<n_steps; i++)
	{
		vg_rect r2 = r;
		r2.x += (i*w)>>8;
		r2.x2 = min(r.x2,(((i+1)*w)>>8));
		steprect[i] = r2;
		surf.fill_rect(r2,(i&1)?owner->get_syscolor(col_LFOBG1):owner->get_syscolor(col_LFOBG2));
		int y = r2.y + (int)(float)((r2.get_h()-1.f) * (0.5f - 0.5f*step[i]));
		r2.y = min(y,ym);
		r2.y2 = max(y,ym)+1;
		surf.fill_rect(r2,owner->get_syscolor(col_LFObars));		

		// plot of deformed curve
		float h[4];
		h[0] = step[(i+1)%n_steps];
		h[1] = step[(i+0)%n_steps];
		h[2] = step[(n_steps + i-1)%n_steps];
		h[3] = step[(n_steps + i-2)%n_steps];
		float m = 1.f/(16.f * (float)(r2.get_w()+1.f));

		UINT col = owner->get_syscolor(col_LFOcurve);
		for(int x=0; x<((r2.get_w()+1)<<4); x++)
		{
			float phase = ((float)x)*m;						
			float f = lfo_ipol(h,phase,shape,i&1);
			y = (int)(float)((r.get_h()-1.f) * (0.5f - 0.5f*f)*256);
			int fr = (y&0xff)>>3;
			y = max(0,min(surf.sizeY-1,r.y + (y>>8)));
			if(y<surf.sizeY) alphablend_pixel(((31-fr)<<24)|(0x00ffffff&col),surf.imgdata[surf.sizeXA*y + r2.x + (x>>4)]);
			if((y+1)<surf.sizeY) alphablend_pixel((fr<<24)|(0x00ffffff&col),surf.imgdata[surf.sizeXA*(y+1) + r2.x + (x>>4)]);
		}		
	}	
}

void vg_steplfo::processevent(vg_controlevent &e)
{
	if(disabled) return;
	if((e.eventtype == vget_mousedown)||((e.eventtype == vget_mousemove)&&(e.buttonmask & (vgm_LMB|vgm_RMB))))
	{
		if (e.eventtype == vget_mousedown) owner->take_focus(control_id);
		for(int i=0; i<n_steps; i++)
		{
			if((e.x >= steprect[i].x)&&(e.x < steprect[i].x2))
			{
				float f = (e.y - steprect[i].y) / (steprect[i].get_h() - 1.0f);
				if(e.buttonmask & vgm_RMB) step[i] = 0.f;
				else step[i] = min(1.f, max(-1.f, 1.f - 2.f*f));

				actiondata ad;
				ad.actiontype = vga_steplfo_data_single;				
				ad.data.i[0] = i;
				ad.data.f[1] = step[i];
				owner->post_action(ad,this);
			}
		}
		set_dirty();
	} 
	else if (e.eventtype == vget_mouseup) owner->drop_focus(control_id);		
}

void vg_steplfo::post_action(actiondata ad)
{
	if(ad.actiontype == vga_disable_state)
	{
		disabled = ad.data.i[0];
		set_dirty();		
	} 
	else if(ad.actiontype == vga_steplfo_repeat)
	{
		n_steps = min(max_steps,max(2,ad.data.i[0]));
		set_dirty();		
	} else if(ad.actiontype == vga_steplfo_shape)
	{
		shape = ad.data.f[0];
		set_dirty();		
	} /*else if(ad.actiontype == vga_steplfo_data)
	{
		for(int i=0; i<32; i+=2)
		{
			step[i] = (ad.data[i>>1].i & 0xffff) * (1.f/65536.f);
			step[i+1] = ((ad.data[i>>1].i>>16) & 0xffff) * (1.f/65536.f);
			step[i] = 2.f*data[i] - 1.f;
			step[i+1] = 2.f*data[i+1] - 1.f;
		}		
		draw();		
	} */else if(ad.actiontype == vga_steplfo_data_single)
	{
		step[ad.data.i[0] & 0x1f] = ad.data.f[1];
		set_dirty();		
	}
}