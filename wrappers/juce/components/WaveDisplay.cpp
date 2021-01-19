/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#include "WaveDisplay.h"
#include "sampler_wrapper_actiondata.h"
#include "interaction_parameters.h"
// AS this is for the limit_range function
#include "vembertech/vt_dsp/basic_dsp.h"
#include <algorithm>
#include "infrastructure/profiler.h"


const int WAVE_MARGIN = 6; // margin around wave display in pixels
const int WAVE_CHANNEL_GAP = 4; // gap between channels

enum
{
    m_zs=0,
    m_ze,
    m_ls,
    m_le,
    m_lcf,
    m_h,
};

static void calc_aatable(unsigned int *tbl, unsigned int col1, unsigned int col2);

WaveDisplay::WaveDisplay(ActionSender *sender, SC3::Log::LoggingCallback *logger) : mSender(sender) , mLogger(logger), prof(logger, "Wave Display Paint"){
    dispmode = 0;
    sampleptr = 0;
    zoom = 1;
    zoom_max = 1;
    vzoom = 1.f;
    start = 0;
    n_hitpoints = 0;

    calc_aatable(aatable[0],0xFFFFFFFF,0xFF000000);
    calc_aatable(aatable[1],0xFF000000,0xFFFFFF);
    calc_aatable(aatable[2],0xff4ea1f3,0xff000000);

    // Todo as enable once we have it
    //for(int i=0; i<7; i++)	bmpdata[i] = owner->get_art(vgct_waveedit,0,i);

    // todo e refers to xml element
    //restore_data(e);
}

void WaveDisplay::paint(Graphics &g)
{

    prof.enter();
    Rectangle<int> r = getLocalBounds();
    g.setColour( juce::Colours::darkgrey); // background color for entire control (wave bg is separate)
    g.fillRect(r);

    switch(dispmode)
    {
    case 0:
        if(!sampleptr) {
            g.setColour(juce::Colours::white); // foreground color for text
            g.drawText("No sample.",r,
                       Justification::horizontallyCentred|Justification::verticallyCentred );
        } else
        {
            // draw wave within our margin
            r.reduce(WAVE_MARGIN, WAVE_MARGIN);
            prof.enter();
            drawWave(g, draw_be_quick, draw_skip_wave_redraw, r);
            prof.exit("draw wave");
        }
        break;
    case 1:
        // multiple zones selected
    {
        g.setColour(juce::Colours::white); // foreground color for text
        g.drawText("Multiple zones selected. Changes will be applied to all of them.",r,
                   Justification::horizontallyCentred|Justification::verticallyCentred );
    }
        break;
    case 2:
        // filter/EG plot
        break;
    };
    prof.exit("paint");
    prof.dump("paint of wave display");
    prof.reset("resetting");
}

bool WaveDisplay::processActionData(const actiondata &ad) {
    bool res = false;
    if(ad.actiontype == vga_wavedisp_sample)
    {
        bool same_sample = (sampleptr == ad.data.ptr[0])&&(dispmode == 0);
        dispmode = 0;
        sampleptr = ad.data.ptr[0];
        sample *s = (sample*)sampleptr;

        playmode = ad.data.i[2]; // todo playmode wasnt initialized
        markerpos[m_zs] = ad.data.i[3]; //zone start?
        markerpos[m_ze] = ad.data.i[4]; //zone end?
        markerpos[m_ls] = ad.data.i[5]; // loop start?
        markerpos[m_le] = ad.data.i[6]; // loop end?
        markerpos[m_lcf] = ad.data.i[7]; // loop cross fade?
        n_hitpoints = ad.data.i[8];

        if(s)
        {
            int width = getWidth()-2* WAVE_MARGIN -2;
            float fratio = std::max(1.f,float(s->sample_length)/float(width));
            zoom_max = (int) ceil(fratio);
            if(same_sample)
            {
                zoom = std::min(zoom,zoom_max);
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
        repaint();
    }

    /*original code, this block was commented
    vga_wavedisp_sample,
    vga_wavedisp_multiselect,
    vga_wavedisp_plot,*/

    return res;
}

void WaveDisplay::queue_draw_wave(bool be_quick, bool skip_wave_redraw)
{
    draw_be_quick = be_quick;
    draw_skip_wave_redraw = skip_wave_redraw;
    repaint();
}

static void calc_aatable(unsigned int *tbl, unsigned int col1, unsigned int col2)
{
    for(int i=0; i<256; i++)
    {
        float x = i*(1.f/255.f);
        int c[4];
        float f[4];

        f[0] = (1-x)*powf((col1&0xff)*(1.f/255.f),1/2.2f) + x*powf((col2&0xff)*(1.f/255.f),1/2.2f);
        f[1] = (1-x)*powf(((col1>>8)&0xff)*(1.f/255.f),1/2.2f) + x*powf(((col2>>8)&0xff)*(1.f/255.f),1/2.2f);
        f[2] = (1-x)*powf(((col1>>16)&0xff)*(1.f/255.f),1/2.2f) + x*powf(((col2>>16)&0xff)*(1.f/255.f),1/2.2f);
        c[0] = std::max(0,std::min(255,(int)(255.f*f[0])));
        c[1] = std::max(0,std::min(255,(int)(255.f*f[1])));
        c[2] = std::max(0,std::min(255,(int)(255.f*f[2])));
        tbl[i] = 0xff000000 | (c[2]<<16) | (c[1]<<8) | c[0];
    }
}

void WaveDisplay::drawWave(Graphics &g, bool be_quick, bool skip_wave_redraw, Rectangle<int> bounds)
{
    //int imgw = pixels.getWidth() - 2 * WAVE_MARGIN;
    int imgw = bounds.getWidth();
    // this was to adjust for pixel size in memory
    int imgwa = bounds.getWidth();
    int imgh = bounds.getHeight();

    // gain access to the raw pixels which we'll be modifying
    Image pixels(Image::PixelFormat::ARGB, imgw, imgh, true, SoftwareImageType());
    Image::BitmapData pixelsBmp(pixels,Image::BitmapData::writeOnly);
    uint32 *img = (unsigned int *)pixelsBmp.data;

    sample *s = (sample *)sampleptr;

    uint32 bgcola = juce::Colour(juce::Colours::red).getARGB();  // background
    juce::Colour wfcola = juce::Colours::white;      // text color
    juce::Colour bgcold = juce::Colours::lightcoral; // disabled
    juce::Colour wfcold = juce::Colours::lightgrey;  // text disabled
    juce::Colour zlcol = juce::Colour(0xff808080);
    juce::Colour zmcol = juce::Colour(0xff5d6c7d);
    juce::Colour lmcol = juce::Colour(0xff0462c2);
    juce::Colour lmcol_faint = juce::Colours::bisque; // tint area



    // TODO why -2?
    n_visible_pixels = imgw - 2;

    float fratio = (float)(s->sample_length) / (float)(imgw);
    int ratio = zoom;

    unsigned int column[2048];
    int column_d[2048];

    const int max_samples = 16;
    int aa_p2 = ((ratio > 8) ? 4 : ((ratio > 4) ? 3 : ((ratio > 2) ? 2 : ((ratio > 1) ? 1 : 0))));
    // was commented:
    // int sample_inc = 1 << aa_p2;
    ratio = std::max(1 << aa_p2, ratio);
    zoom = ratio;
    int sample_inc = std::max(1, ratio / max_samples);
    int n_samples = ratio / sample_inc;

    if (be_quick)
    {
        aa_p2 = 0;
        sample_inc = ratio;
        n_samples = 1;
    }

    if (!skip_wave_redraw)
    {
        // This is the height of each waveform on a channel
        int bheight;
        {
            // total height of gap between each channel
            int totalGap=WAVE_CHANNEL_GAP*(s->channels-1);
            // height of each channel's wave
            bheight = (imgh - totalGap) /s->channels;
        }

        prof.enter();
        for (int c = 0; c < s->channels; c++)
        {
            // location of top of waveform
            int btop = (bheight+WAVE_CHANNEL_GAP) * c;
            // location of bottom of waveform todo what is with the imgh-1?
            int bbottom = std::min(imgh - 1, btop + bheight);

            if (!s->SampleData[c]) //no data for that channel
                continue;

            /*was commented:
            const int aa_bs = 4;
            const int aa_samples = 1<<aa_bs;	*/

            // AS start has something to do with where in the wave we are looking?
            int pos = std::max(0, start);

            int mid_ypos = btop + (bheight >> 1);
            int mid_ypos_mul_w = imgw * mid_ypos;

            int midline = mid_ypos;
            int topline = btop;
            int bottomline = bbottom - 1;

            short *SampleDataI16 = s->GetSamplePtrI16(c);
            float *SampleDataF32 = s->GetSamplePtrF32(c);

            int last_imax, last_imin;

            { // set start conditions. "last value" must be right to avoid line-drawing at the left edge
                float val;
                if (s->UseInt16)
                {
                    val = SampleDataI16[pos];
                    val *= (-1.f / (32768.f));
                }
                else
                {
                    val = -SampleDataF32[pos];
                }
                val *= std::max(1.f, vzoom);
                val = (btop + (float)(0.5f + 0.5f * val) * bheight);
                val = 256.f * (val);
                last_imin = (int)val;
                last_imax = last_imin;
            }
            // last_imax = mid_ypos<<8;
            // last_imin = mid_ypos<<8;

            std::memset(&column_d[btop], 0, bheight*sizeof(int));

            bool debugzoom = false;

            for (int x = 0; x < imgw; x += (debugzoom ? 4 : 1))
            {
                int aaidx = 0;
                std::memset(&column[btop], 0, bheight*sizeof(int));

                if ((pos + ratio) > ((int)s->sample_length))
                {
                    // AS no need for this because we've already blasted the bg color
                    //for (int y = btop; y < (bbottom); y++)
                    //{
                    //    img[x+y*imgwa] = bgcola;
                    //}
                }
                else
                {
                    for (int sp = 0; sp < ratio; sp += sample_inc)
                    {

                        int cpos = pos + sp;
                        // was commented:
                        // if (pm_has_loop(playmode)&&(cpos >= loop_start)&&(cpos < loop_end)) aaidx = 2;
                        float val;
                        if (s->UseInt16)
                        {
                            val = SampleDataI16[cpos];
                            val *= (-1.f / (32738.f));
                        }
                        else
                        {
                            val = -SampleDataF32[cpos];
                        }
                        val *= std::max(1.f, vzoom);

                        val = (btop + (float)(0.5f + 0.5f * val) * bheight);

                        val = 256.f * (val);
                        int ival = (int)val;
                        int dimax = ival, dimin = ival;

                        if (dimin < last_imax)
                            dimin = last_imax;
                        else if (dimax > last_imin)
                            dimax = last_imin;

                        dimax = limit_range(dimax, (btop + 2) << 8, (bbottom - 2) << 8);
                        dimin = limit_range(dimin, (btop + 2) << 8, (bbottom - 2) << 8);

                        for (int y = (dimax >> 8); y < (dimin >> 8); y++)
                        {
                            column_d[y] = n_samples;
                        }

                        column[dimin >> 8] += dimin & 0xFF;
                        column[(dimax >> 8) - 1] += 255 - (dimax & 0xFF);

                        last_imax = dimax;
                        last_imin = dimin;

                        for (int y = btop; y < bbottom; y++)
                        {
                            if (column_d[y] > 0)
                                column[y] += 256;
                            column_d[y]--;
                        }

                        column[midline] = std::max((unsigned int)64 << aa_p2, column[midline]);
                        column[topline] = std::max((unsigned int)32 << aa_p2, column[topline]);
                        column[bottomline] =
                            std::max((unsigned int)32 << aa_p2, column[bottomline]);

                        if (debugzoom)
                        {
                            for (int y = btop; y < bbottom; y++)
                            {
                                for (int z = 0; z < 4; z++)
                                {
                                    img[y * imgwa + x+z] =
                                        aatable[aaidx]
                                               [std::min((unsigned int)255, column[y] >> aa_p2)];
                                }
                            }
                        }
                        else
                        {
                            for (int y = btop; y < bbottom; y++)
                                img[y*imgwa + x] = aatable[aaidx][std::min((unsigned int)255,column[y]>>aa_p2)];
                        }
                    }
                    pos += ratio;
                }
            }
        }
        prof.exit("main draw loop");
    } //!skip_wave_redraw
    prof.enter();
    g.drawImage(pixels, bounds.toFloat());
    prof.exit("blit to graphics");

/* TODO AS all of this. Want to probably do the markers separate from the drawing of the wave
 *   preferably just keep the wave bitmap in memory and only recalc if need be

    vg_rect rbound = bounds;
    rbound.inset(WAVE_MARGIN,WAVE_MARGIN);

    for(int i=0; i<256; i++) dragpoint[i].set(0,0,0,0);

    if(pm_has_loop(playmode))
    {
        int x1 = getpos(markerpos[m_ls]);
        int x2 = getpos(markerpos[m_le]);

        vg_rect tr(x1,WAVE_MARGIN,x2,imgh-WAVE_MARGIN-1);
        tr.bound(rbound);
        surf.fill_rect(tr,lmcol_faint);

        dragpoint[2].set(0,0,13,13);
        dragpoint[2].move_centerpivot(x1,imgh-WAVE_MARGIN-1);
        if((x1 >= (WAVE_MARGIN))&&(x1<(imgw + WAVE_MARGIN)))
        {
            surf.draw_rect(vg_rect(x1,WAVE_MARGIN,x1+1,imgh - WAVE_MARGIN-1),lmcol);
            surf.blit_alphablend(dragpoint[2],bmpdata[2]);
        }
        dragpoint[3].set(0,0,13,13);
        dragpoint[3].move_centerpivot(x2,imgh-WAVE_MARGIN-1);
        if((x2 >= (WAVE_MARGIN))&&(x2<(imgw + WAVE_MARGIN)))
        {
            surf.draw_rect(vg_rect(x2,WAVE_MARGIN,x2+1,imgh - WAVE_MARGIN-1),lmcol);
            surf.blit_alphablend(dragpoint[3],bmpdata[3]);
        }
    }

    if(!pm_has_slices(playmode))
    {
        int x = getpos(markerpos[m_zs]);
        dragpoint[0].set(0,0,13,13);
        dragpoint[0].move_centerpivot(x,WAVE_MARGIN);
        if((x >= WAVE_MARGIN)&&(x<(imgw + WAVE_MARGIN)))
        {
            surf.draw_rect(vg_rect(x,WAVE_MARGIN,x+1,imgh - WAVE_MARGIN-1),zmcol);
            surf.blit_alphablend(dragpoint[0],bmpdata[0]);
        }

        x = getpos(markerpos[m_ze]);
        dragpoint[1].set(0,0,13,13);
        dragpoint[1].move_centerpivot(x,WAVE_MARGIN);
        if((x >= WAVE_MARGIN)&&(x<(imgw + WAVE_MARGIN)))
        {
            surf.draw_rect(vg_rect(x,WAVE_MARGIN,x+1,imgh - WAVE_MARGIN-1),zmcol);
            surf.blit_alphablend(dragpoint[1],bmpdata[1]);
        }
    }
    else
    {
        for(int i=0; i<n_hitpoints; i++)
        {
            int x1 = getpos(markerpos[m_h + i]);
            dragpoint[m_h+i].set(0,0,13,13);
            dragpoint[m_h+i].move_centerpivot(x1,imgh-WAVE_MARGIN-1);
            if((x1 >= (WAVE_MARGIN))&&(x1<(imgw + WAVE_MARGIN)))
            {
                surf.draw_rect(vg_rect(x1,WAVE_MARGIN,x1+1,imgh - WAVE_MARGIN-1),0xffbc4e03);
                surf.blit_alphablend(dragpoint[m_h+i],bmpdata[5]);
            }
        }
    }
*/
    /* this might have already been commented
     * {
            vg_rect rt = bounds;
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

#if 0
#include "vt_gui_controls.h"
#include "sample.h"
#include "sampler_state.h"
#include "resampling.h"
#include <vt_dsp/basic_dsp.h>

vg_waveeditor::vg_waveeditor(vg_window *o, int id, TiXmlElement *e) : vg_control(o,id,e)
{

}


vg_waveeditor::~vg_waveeditor()
{
	wavesurf.destroy();
}

int vg_waveeditor::getpos(int sample)
{
	return ((sample-start)/max(0,zoom))+WAVE_MARGIN;
}

int vg_waveeditor::getsample(int pos)
{
	return (pos-WAVE_MARGIN)*zoom + start;
}

void vg_waveeditor::set_rect(vg_rect r)
{
	wavesurf.destroy();
	wavesurf.create(r.get_w(),r.get_h());
	vg_control::set_rect(r);
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

#endif