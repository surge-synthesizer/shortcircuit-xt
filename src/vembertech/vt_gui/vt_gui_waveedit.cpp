#include "vt_gui_controls.h"
#include "sample.h"
#include "sampler_state.h"
#include "resampling.h"
#include <vt_dsp/basic_dsp.h>

const int wmargin = 6;

enum
{
	m_zs=0,
	m_ze,
	m_ls,
	m_le,
	m_lcf,
	m_h,
};

void calc_aatable(unsigned int *tbl, unsigned int col1, unsigned int col2)
{
	for(int i=0; i<256; i++)
	{
		float x = i*(1.f/255.f);
		int c[4];
		float f[4];

		f[0] = (1-x)*powf((col1&0xff)*(1.f/255.f),1/2.2f) + x*powf((col2&0xff)*(1.f/255.f),1/2.2f);
		f[1] = (1-x)*powf(((col1>>8)&0xff)*(1.f/255.f),1/2.2f) + x*powf(((col2>>8)&0xff)*(1.f/255.f),1/2.2f);
		f[2] = (1-x)*powf(((col1>>16)&0xff)*(1.f/255.f),1/2.2f) + x*powf(((col2>>16)&0xff)*(1.f/255.f),1/2.2f);
		c[0] = max(0,min(255,255.f*f[0]));
		c[1] = max(0,min(255,255.f*f[1]));
		c[2] = max(0,min(255,255.f*f[2]));
		tbl[i] = 0xff000000 | (c[2]<<16) | (c[1]<<8) | c[0];
	}
}

vg_waveeditor::vg_waveeditor(vg_window *o, int id, TiXmlElement *e) : vg_control(o,id,e)
{
	dispmode = 0;
	sampleptr = 0;
	zoom = 1;
	zoom_max = 1;
	vzoom = 1.f;
	start = 0;
	n_hitpoints = 0;

	calc_aatable(aatable[0],owner->get_syscolor(col_standard_bg),owner->get_syscolor(col_standard_text));
	calc_aatable(aatable[1],owner->get_syscolor(col_standard_text),owner->get_syscolor(col_standard_bg));	
	calc_aatable(aatable[2],0xff4ea1f3,0xff000000);	

	for(int i=0; i<7; i++)	bmpdata[i] = owner->get_art(vgct_waveedit,0,i);	

	restore_data(e);
}


vg_waveeditor::~vg_waveeditor()
{
	wavesurf.destroy();
}

int vg_waveeditor::getpos(int sample)
{
	return ((sample-start)/max(0,zoom))+wmargin;
}

int vg_waveeditor::getsample(int pos)
{
	return (pos-wmargin)*zoom + start;
}

void vg_waveeditor::draw()
{
	draw_needed = false;
	//draw_background();
	
	switch(dispmode)
	{
	case 0:
		// draw wave
		draw_wave(draw_be_quick,draw_skip_wave_redraw);
		break;
	case 1:
		// multiple zones selected
		{
			vg_rect r(0,0,surf.sizeX,surf.sizeY);			
			surf.fill_rect(r,owner->get_syscolor(col_nowavebg));
			surf.draw_text(r,"Multiple zones selected. Changes will be applied to all of them.",owner->get_syscolor(col_standard_text));
			set_dirty();
		}
		break;
	case 2:
		// filter/EG plot
		break;		
	};	
}

void vg_waveeditor::set_rect(vg_rect r)
{	
	wavesurf.destroy();
	wavesurf.create(r.get_w(),r.get_h());
	vg_control::set_rect(r);						
}

void vg_waveeditor::queue_draw_wave(bool be_quick, bool skip_wave_redraw)
{
	draw_be_quick = be_quick;
	draw_skip_wave_redraw = skip_wave_redraw;
	set_dirty();
}

void vg_waveeditor::draw_wave(bool be_quick, bool skip_wave_redraw)
{	
	vg_rect r(0,0,surf.sizeX,surf.sizeY);
	//surf.draw_rect(r,owner->get_syscolor(col_frame),true);
	//r.inset(1,1);	

	sample *s = (sample*)sampleptr;	
	if(!s)
	{
		surf.fill_rect(r,owner->get_syscolor(col_nowavebg));
		surf.draw_text(r,"No sample.",owner->get_syscolor(col_standard_text));		
		return;
	}	
	
	int margincol = owner->get_syscolor(col_standard_bg);
	
	unsigned int *img = wavesurf.imgdata;		
	int imgw = wavesurf.sizeX-2*wmargin;
	int imgwa = wavesurf.sizeXA;
	int imgh = wavesurf.sizeY;	
	img += wmargin;	

	n_visible_pixels = imgw-2;

	vg_rect rwave(0,0,wavesurf.sizeX,wavesurf.sizeY);
	vg_rect r2 = rwave;
	r2.x2 = r2.x + wmargin; wavesurf.fill_rect(r2,margincol);
	r2 = rwave; r2.x = r2.x2 - wmargin; wavesurf.fill_rect(r2,margincol);
	vg_rect rwave_marginup(rwave.x+wmargin,0,rwave.x2-wmargin,wmargin);
	wavesurf.fill_rect(rwave_marginup,margincol);	// uper margin

	float fratio = (float)(s->sample_length)/(float)(imgw);		
	int ratio = zoom;

	int bgcola = owner->get_syscolor(col_standard_bg);		// 0xAAREGGBB
	int wfcola = owner->get_syscolor(col_standard_text);
	int bgcold = owner->get_syscolor(col_standard_bg_disabled);
	int wfcold = owner->get_syscolor(col_standard_text_disabled);
	int zlcol = 0xff808080;	
	int zmcol = 0xff5d6c7d;
	int lmcol = 0xff0462c2;
	int lmcol_faint = owner->get_syscolor(col_tintarea);

	unsigned int column[2048];
	int column_d[2048];	

	const int max_samples = 16;						
	int aa_p2 = ((ratio > 8)?4:((ratio > 4)?3:((ratio > 2)?2:((ratio > 1)?1:0))));	
	//int sample_inc = 1 << aa_p2;
	ratio = max(1 << aa_p2, ratio);
	zoom = ratio;
	int sample_inc = max(1,ratio/max_samples);	
	int n_samples = ratio / sample_inc;	

	if(be_quick) 
	{
		aa_p2 = 0;
		sample_inc = ratio;
		n_samples = 1;
	}

	if(!skip_wave_redraw)
	for(int c=0; c<s->channels; c++)
	{
		int bheight = (imgh - wmargin*(1+s->channels))/s->channels;
		int btop = wmargin + (bheight + wmargin) * c;
		int bbottom = min(imgh-1, btop + bheight);		

		rwave_marginup.move(wmargin,bbottom);
		if(c == (s->channels-1)) rwave_marginup.y2 = rwave.x2;
		wavesurf.fill_rect(rwave_marginup,margincol);	// margin

		if(!s->SampleData[c]) return;		

		/*const int aa_bs = 4;
		const int aa_samples = 1<<aa_bs;	*/

		int pos = max(0,start);

		int mid_ypos = btop + (bheight>>1);
		int mid_ypos_mul_w = imgw*mid_ypos;

		int midline = mid_ypos;
		int topline = btop;
		int bottomline = bbottom-1;

		short *SampleDataI16 = s->GetSamplePtrI16(c);
		float *SampleDataF32 = s->GetSamplePtrF32(c);

		int last_imax,last_imin;

		{	// set start conditions. "last value" must be right to avoid line-drawing at the left edge
			float val;
			if(s->UseInt16)
			{
				val = SampleDataI16[pos];				
				val *= (-1.f/(32768.f));
			}
			else
			{
				val = -SampleDataF32[pos];				
			}
			val *= max(1.f,vzoom);
			val = (btop + (float)(0.5f + 0.5f*val) * bheight);
			val = 256.f*(val);							
			last_imin = (int) val;
			last_imax = last_imin;
		}
		//last_imax = mid_ypos<<8;
		//last_imin = mid_ypos<<8;

		for(int y=btop; y<bbottom; y++) column_d[y] = 0;

		bool debugzoom = false;		

		for(int x=0; x<imgw; x += (debugzoom?4:1))
		{	
			int aaidx = 0;

			for(int y=btop; y<bbottom; y++) column[y] = 0;

			if((pos+ratio)>((int)s->sample_length))
			{
				for(int y=btop; y<(bbottom); y++)
				{
					img[x + y*imgwa] = bgcola;
				}
			}
			else 
			{
				for(int sp = 0; sp<ratio; sp+=sample_inc)
				{

					int cpos = pos+sp;					
//					if (pm_has_loop(playmode)&&(cpos >= loop_start)&&(cpos < loop_end)) aaidx = 2;
					float val;
					if(s->UseInt16)
					{
						val = SampleDataI16[cpos];						
						val *= (-1.f/(32738.f));
					}
					else
					{
						val = -SampleDataF32[cpos];
						
					}
					val *= max(1.f,vzoom);

					val = (btop + (float)(0.5f + 0.5f*val) * bheight);

					val = 256.f*(val);					
					int ival = (int) val;										
					int dimax=ival,dimin=ival;

					if(dimin < last_imax) dimin = last_imax;
					else if(dimax > last_imin) dimax = last_imin;


					dimax = limit_range(dimax,(btop+2)<<8,(bbottom-2)<<8);
					dimin = limit_range(dimin,(btop+2)<<8,(bbottom-2)<<8);

					for(int y=(dimax>>8); y<(dimin>>8); y++)
					{						
						column_d[y] = n_samples;						
					}

					column[dimin>>8] += dimin&0xFF;
					column[(dimax>>8)-1] += 255 - (dimax&0xFF);

					last_imax = dimax;
					last_imin = dimin;					

					for(int y=btop; y<bbottom; y++)
					{
						if(column_d[y]>0) column[y] += 256;
						column_d[y]--;	
					}	


					column[midline] = max((unsigned int)64<<aa_p2,column[midline]);
					column[topline] = max((unsigned int)32<<aa_p2,column[topline]);
					column[bottomline] = max((unsigned int)32<<aa_p2,column[bottomline]);
					if(debugzoom)
					{
						for(int y=btop; y<bbottom; y++) 
						{
							img[y*imgwa + x] = aatable[aaidx][min((unsigned int)255,column[y]>>aa_p2)];			
							img[y*imgwa + x + 1] = img[y*imgwa + x];
							img[y*imgwa + x + 2] = img[y*imgwa + x];
							img[y*imgwa + x + 3] = img[y*imgwa + x];
						}
					}
					else for(int y=btop; y<bbottom; y++) img[y*imgwa + x] = aatable[aaidx][min((unsigned int)255,column[y]>>aa_p2)];			
				}
				pos += ratio;
			}
		}
	}	
	surf.blit(r,&wavesurf);

	vg_rect rbound = r;
	rbound.inset(wmargin,wmargin);

	for(int i=0; i<256; i++) dragpoint[i].set(0,0,0,0);

	if(pm_has_loop(playmode))
	{
		int x1 = getpos(markerpos[m_ls]);	
		int x2 = getpos(markerpos[m_le]);

		vg_rect tr(x1,wmargin,x2,imgh-wmargin-1);
		tr.bound(rbound);
		surf.fill_rect(tr,lmcol_faint);			

		dragpoint[2].set(0,0,13,13);
		dragpoint[2].move_centerpivot(x1,imgh-wmargin-1);
		if((x1 >= (wmargin))&&(x1<(imgw + wmargin)))
		{
			surf.draw_rect(vg_rect(x1,wmargin,x1+1,imgh - wmargin-1),lmcol);
			surf.blit_alphablend(dragpoint[2],bmpdata[2]);	
		}				
		dragpoint[3].set(0,0,13,13);
		dragpoint[3].move_centerpivot(x2,imgh-wmargin-1);
		if((x2 >= (wmargin))&&(x2<(imgw + wmargin)))
		{
			surf.draw_rect(vg_rect(x2,wmargin,x2+1,imgh - wmargin-1),lmcol);
			surf.blit_alphablend(dragpoint[3],bmpdata[3]);
		}				
	}	

	if(!pm_has_slices(playmode))
	{
		int x = getpos(markerpos[m_zs]);	
		dragpoint[0].set(0,0,13,13);
		dragpoint[0].move_centerpivot(x,wmargin);
		if((x >= wmargin)&&(x<(imgw + wmargin)))
		{
			surf.draw_rect(vg_rect(x,wmargin,x+1,imgh - wmargin-1),zmcol);
			surf.blit_alphablend(dragpoint[0],bmpdata[0]);	
		}

		x = getpos(markerpos[m_ze]);
		dragpoint[1].set(0,0,13,13);
		dragpoint[1].move_centerpivot(x,wmargin);
		if((x >= wmargin)&&(x<(imgw + wmargin)))
		{
			surf.draw_rect(vg_rect(x,wmargin,x+1,imgh - wmargin-1),zmcol);
			surf.blit_alphablend(dragpoint[1],bmpdata[1]);
		}
	} 
	else
	{
		for(int i=0; i<n_hitpoints; i++)
		{
			int x1 = getpos(markerpos[m_h + i]);
			dragpoint[m_h+i].set(0,0,13,13);
			dragpoint[m_h+i].move_centerpivot(x1,imgh-wmargin-1);
			if((x1 >= (wmargin))&&(x1<(imgw + wmargin)))
			{
				surf.draw_rect(vg_rect(x1,wmargin,x1+1,imgh - wmargin-1),0xffbc4e03);
				surf.blit_alphablend(dragpoint[m_h+i],bmpdata[5]);	
			}
		}
	}

	/*{
		vg_rect rt = r;
		rt.inset(10,10);
		char text[256];
		sprintf(text,
			"%s (%.3f MB)\n"
			"%i Samples\n"
			"%i Hz, %i-Bit, %i Channels\n"
			"Used by %i Zones\n",
			s->GetName(),s->GetDataSize()/(1024.f*1024.f),s->sample_length,s->sample_rate,s->UseInt16?16:32,s->channels,s->GetRefCount());
		surf.draw_text_multiline(rt,text,owner->get_syscolor(col_standard_text),1,1);
	}*/
}

void vg_waveeditor::draw_plot(){}

enum
{
	cs_default=0,
	cs_pan,
	cs_dragpoint,
};

void vg_waveeditor::processevent(vg_controlevent &e)
{	
	if(dispmode) return;

	switch(controlstate)
	{
	case cs_pan:
		{
			if((e.eventtype == vget_mouseup)&&(e.activebutton == 1))
			{
				controlstate = cs_default;
				owner->drop_focus(control_id);	
				owner->show_mousepointer(true);
				queue_draw_wave(false,false);
			} 
			else if(e.eventtype == vget_mousemove)
			{
				sample *so = (sample*)sampleptr;
				if(so)
				{										
					if (e.buttonmask & vgm_shift)
					{
						vzoom -= 0.1 * (e.y - lastmouseloc.y);
						vzoom = max(1.f,vzoom);
					}
					else
					{
						float movemult = 2.f;
						if (e.buttonmask & (vgm_RMB|vgm_control)) movemult = 8.f;
						start -= (e.x - lastmouseloc.x)*zoom*movemult;
						start = min(start, (int)so->sample_length - zoom*n_visible_pixels);
						start = max(0,start);				
					}
					//lastmouseloc.x = e.x;
					//lastmouseloc.y = e.y;
					owner->move_mousepointer(location.x + lastmouseloc.x, location.y + lastmouseloc.y);	
				}
				queue_draw_wave(true,false);
			}
		}
		break;
	case cs_dragpoint:
		{
			if(e.eventtype == vget_mouseup)
			{
				controlstate = cs_default;
				owner->drop_focus(control_id);	
				queue_draw_wave(false,true);
			} 
			else if(e.eventtype == vget_mousemove)
			{
				sample *so = (sample*)sampleptr;
				if(so)
				{
					int s = getsample((int)e.x);
					s = limit_range(s,0,(int)(so->sample_length));
					markerpos[dragid] = s;
					queue_draw_wave(false,true);

					actiondata ad;
					ad.actiontype = vga_wavedisp_editpoint;
					ad.data.i[0] = dragid;
					ad.data.i[1] = s;
					owner->post_action(ad,this);
				}
			}
		}
		break;
	default:
		{
			if(e.eventtype == vget_mousewheel)		
			{				
				int oldsample = getsample(e.x);
				
				if(e.eventdata[0] > 0) zoom  = max(1,zoom>>1);
				else zoom  = min(zoom_max,zoom<<1);

				start -= getsample(e.x) - oldsample;
				sample *so = (sample*)sampleptr;
				if(so) start = min(start, (int)so->sample_length - zoom*n_visible_pixels);
				start = max(0,start);

				queue_draw_wave(false,false);
			} 
			else if((e.eventtype == vget_mousedown)&&(e.activebutton == 1))
			{						
				for(int i=0; i<(5+n_hitpoints); i++)
				{
					if(dragpoint[i].point_within(vg_point(e.x,e.y)))
					{
						controlstate = cs_dragpoint;
						owner->take_focus(control_id);	
						lastmouseloc.x = e.x;
						lastmouseloc.y = e.y;
						dragid = i;
						return;
					}
				}				
				controlstate = cs_pan;
				owner->take_focus(control_id);	
				owner->show_mousepointer(false);
				lastmouseloc.x = e.x;
				lastmouseloc.y = e.y;
				vzoom_drag = vzoom;
				if (vzoom_drag < 1.01f) vzoom_drag = -2;
			}
			else if((e.eventtype == vget_mousedown)&&(e.activebutton == 2))
			{						
				// context menu
				if((dispmode == 0) && sampleptr)
				{
					int s = getsample((int)e.x);
					sample *so = (sample*)sampleptr;
					if(so) s = limit_range(s,0,(int)(so->sample_length));				

					md.entries.clear();
					vg_menudata_entry mde;	
					mde.submenu = 0;
					mde.ad.id = parameter_id;
					mde.ad.subid = 0;
					mde.ad.data.i[1] = s;				

					mde.ad.actiontype = vga_nothing;
					mde.label = "Zone";	 md.entries.push_back(mde);	
					mde.ad.actiontype = vga_wavedisp_editpoint;					
					mde.ad.data.i[0] = 0;
					mde.label = "set start";	 md.entries.push_back(mde);	
					mde.ad.data.i[0] = 1;
					mde.label = "set end";	 md.entries.push_back(mde);	

					mde.ad.actiontype = vga_nothing;
					mde.label = "Loop";	 md.entries.push_back(mde);					
					mde.ad.actiontype = vga_wavedisp_editpoint;	
					mde.ad.data.i[0] = 2;
					mde.label = "set start";	 md.entries.push_back(mde);	
					mde.ad.data.i[0] = 3;
					mde.label = "set end";	 md.entries.push_back(mde);	

					actiondata ad;
					ad.id = control_id;
					ad.subid = 0;
					ad.data.ptr[0] = ((void*)&md);
					ad.data.i[2] = false;	// doesn't need destruction
					ad.data.i[3] = location.x + e.x;
					ad.data.i[4] = location.y + e.y;
					ad.actiontype = vga_menu;	
					owner->post_action(ad,0);				
				}
			}
			else if(e.eventtype == vget_mousemove)
			{
				for(int i=0; i<(5+n_hitpoints); i++)
				{
					if(dragpoint[i].point_within(vg_point(e.x,e.y)))
					{
						owner->set_cursor(cursor_hmove);
						return;
					}
				}
				owner->set_cursor(cursor_hand_pan);
				return;
			}
		}
		break;
	}
}
void vg_waveeditor::post_action(actiondata ad)
{
	if(ad.actiontype == vga_wavedisp_sample)
	{
		bool same_sample = (sampleptr == ad.data.ptr[0])&&(dispmode == 0);
		dispmode = 0;		
		sampleptr = ad.data.ptr[0];
		sample *s = (sample*)sampleptr;		

		playmode = ad.data.i[2];
		markerpos[m_zs] = ad.data.i[3];
		markerpos[m_ze] = ad.data.i[4];
		markerpos[m_ls] = ad.data.i[5];
		markerpos[m_le] = ad.data.i[6];
		markerpos[m_lcf] = ad.data.i[7];		
		n_hitpoints = ad.data.i[8];

		if(s)
		{
			int width = wavesurf.sizeX-2*wmargin-2;
			float fratio = max(1.0,float(s->sample_length)/float(width));
			zoom_max = (int) ceil(fratio);
			if(same_sample)			
			{
				zoom = min(zoom,zoom_max);				
			}
			else
			{
				zoom = zoom_max;
				start = 0;
				vzoom = 1.f;
			}
		}		

		queue_draw_wave(false,false);		
	} 
	if(ad.actiontype == vga_wavedisp_editpoint)
	{
		markerpos[ad.data.i[0]] = ad.data.i[1];
		queue_draw_wave(false,false);		
	}
	else if(ad.actiontype == vga_wavedisp_multiselect)
	{
		dispmode = 1;		
		set_dirty();
	} 

	/*vga_wavedisp_sample,
	vga_wavedisp_multiselect,
	vga_wavedisp_plot,*/
}