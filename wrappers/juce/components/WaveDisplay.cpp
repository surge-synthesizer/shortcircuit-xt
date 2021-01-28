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

static void calc_aatable(unsigned int *tbl, unsigned int col1, unsigned int col2);

WaveDisplay::WaveDisplay(ActionSender *sender, SC3::Log::LoggingCallback *logger) : mSender(sender) , mLogger(logger), prof(logger, "Wave Display Paint"){
    dispmode = 0;
    mSamplePtr = 0;
    mZoom = 1;
    mZoomMax = 1;
    mVerticalZoom = 1.f;
    mLeftMostSample = 0;
    n_hitpoints = 0;

    calc_aatable(aatable[0],0xFFFFFFFF,0xFF000000);
    calc_aatable(aatable[1],0xFF000000,0xFFFFFF);
    calc_aatable(aatable[2],0xff4ea1f3,0xff000000);

    // Todo as enable once we have it
    //for(int i=0; i<7; i++)	bmpdata[i] = owner->get_art(vgct_waveedit,0,i);

    // todo e refers to xml element
    //restore_data(e);
}
void WaveDisplay::resized() {

    // actual wave dimensions we'll be drawing in and using for calculations
    mWaveBounds = getLocalBounds().reduced(WAVE_MARGIN, WAVE_MARGIN);

    mNumVisiblePixels = mWaveBounds.getWidth() - 2;

    queue_draw_wave(false,false);
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
        if(!mSamplePtr) {
            g.setColour(juce::Colours::white); // foreground color for text
            g.drawText("No sample.",r,
                       Justification::horizontallyCentred|Justification::verticallyCentred );
        } else
        {
            // render the wave to mWavePixels if need be
            if (!draw_skip_wave_redraw)
            {
                prof.enter();
                renderWave(draw_be_quick);
                prof.exit("wave render");
            }
            // output the wave
            prof.enter();
            g.drawImage(mWavePixels, mWaveBounds.toFloat());
            prof.exit("blit to graphics");

            // add the details
            prof.enter();
            drawDetails(g, r);
            prof.exit("add details");


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
    prof.reset("");
}

// this is called when the sampler is sending out a broadcast
bool WaveDisplay::processActionData(const actiondata &ad) {
    bool res = false;
    if(ad.actiontype == vga_wavedisp_sample)
    {
        auto e = (const ActionWaveDisplaySample &)(ad);
        bool same_sample = (mSamplePtr == e.samplePtr()) && (dispmode == 0);
        dispmode = 0;
        mSamplePtr = (sample *)e.samplePtr();

        playmode = e.playMode(); // todo playmode wasnt initialized
        markerpos[ActionWaveDisplayEditPoint::PointType::start] = e.start();
        markerpos[ActionWaveDisplayEditPoint::PointType::end] = e.end();
        markerpos[ActionWaveDisplayEditPoint::PointType::loopStart] = e.loopStart();
        markerpos[ActionWaveDisplayEditPoint::PointType::loopEnd] = e.loopEnd();
        markerpos[ActionWaveDisplayEditPoint::PointType::loopXFade] = e.loopCrossfade();
        n_hitpoints = e.numHitpoints();

        if(mSamplePtr)
        {
            // calculate the max amount we can zoom out (higher value is more zoomed out)
            float ratio = std::max(1.f,float(mSamplePtr->sample_length)/float(mNumVisiblePixels));
            mZoomMax = (int) ceil(ratio);
            if(same_sample)
            {
                mZoom = std::min(mZoom, mZoomMax);
            }
            else
            {
                // default is to zoom out as much as possible
                mZoom = mZoomMax;
                mLeftMostSample = 0;
                mVerticalZoom = 1.f;
            }
        }
        queue_draw_wave(false,false);
    }
    if(ad.actiontype == vga_wavedisp_editpoint)
    {
        auto e = (const ActionWaveDisplayEditPoint &)(ad);
        markerpos[e.dragId()] = e.samplePos();

        // todo do we really need a full redraw here?
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

// calculate anti-alias tables
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

void WaveDisplay::renderWave(bool quick)
{
    // draw wave within our margin
    Rectangle<int> bounds = mWaveBounds;

    int imgw = bounds.getWidth();
    int imgh = bounds.getHeight();

    // software image type has known pixel format (may be slower?)
    mWavePixels = Image(Image::PixelFormat::ARGB, imgw, imgh, true, SoftwareImageType());
    Image::BitmapData pixelsBmp(mWavePixels, Image::BitmapData::writeOnly);
    // gain access to the raw pixels which we'll be modifying
    uint32 *img = (unsigned int *)pixelsBmp.data;


    //float fratio = (float)(s->sample_length) / (float)(imgw);
    int ratio = mZoom;

    unsigned int column[2048];
    int column_d[2048];

    const int maxSamples = 16;
    int aa_p2 = ((ratio > 8) ? 4 : ((ratio > 4) ? 3 : ((ratio > 2) ? 2 : ((ratio > 1) ? 1 : 0))));
    // was commented:
    // int sample_inc = 1 << aa_p2;
    ratio = std::max(1 << aa_p2, ratio);
    mZoom = ratio;
    int sample_inc;
    int n_samples;

    if (quick)
    {
        aa_p2 = 0;
        sample_inc = ratio;
        n_samples = 1;
    } else {
        sample_inc= std::max(1, ratio / maxSamples);
        n_samples = ratio / sample_inc;
    }

    // This is the height of each waveform on a channel
    int bheight;
    {
        // total height of gap between each channel
        int totalGap = WAVE_CHANNEL_GAP * (mSamplePtr->channels - 1);
        // height of each channel's wave
        bheight = (imgh - totalGap) / mSamplePtr->channels;
    }

    prof.enter();
    for (int c = 0; c < mSamplePtr->channels; c++)
    {
        // location of top of waveform for this channel
        int btop = (bheight + WAVE_CHANNEL_GAP) * c;

        // location of bottom of waveform for this channel
        int bbottom = std::min(imgh - 1, btop + bheight);

        if (!mSamplePtr->SampleData[c]) // no data for that channel
            continue;

        /*was commented:
        const int aa_bs = 4;
        const int aa_samples = 1<<aa_bs;	*/

        // first sample position we'll be rendering
        int pos = std::max(0, mLeftMostSample);

        int mid_ypos = btop + (bheight >> 1);
        int mid_ypos_mul_w = imgw * mid_ypos;

        int midline = mid_ypos;
        int topline = btop;
        int bottomline = bbottom - 1;

        short *SampleDataI16 = mSamplePtr->GetSamplePtrI16(c);
        float *SampleDataF32 = mSamplePtr->GetSamplePtrF32(c);

        int last_imax, last_imin;

        // set start conditions. "last value" must be right to avoid line-drawing at the left edge
        {
            float val;
            if (mSamplePtr->UseInt16)
            {
                val = SampleDataI16[pos];
                val *= (-1.f / (32768.f));
            }
            else
            {
                val = -SampleDataF32[pos];
            }
            val *= std::max(1.f, mVerticalZoom);
            val = (btop + (float)(0.5f + 0.5f * val) * bheight);
            val = 256.f * (val);
            last_imax = last_imin = (int)val;
        }
        // last_imax = mid_ypos<<8;
        // last_imin = mid_ypos<<8;

        std::memset(&column_d[btop], 0, bheight * sizeof(int));

        bool debugzoom = false;

        for (int x = 0; x < imgw; x += (debugzoom ? 4 : 1))
        {
            int aaidx = 0;
            std::memset(&column[btop], 0, bheight * sizeof(int));

            if ((pos + ratio) > ((int)mSamplePtr->sample_length))
            {

                // AS no need for this because we've already blasted the bg color
                // for (int y = btop; y < (bbottom); y++)
                //{
                //    img[x+y*imgw] = bgcola;
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
                    if (mSamplePtr->UseInt16)
                    {
                        val = SampleDataI16[cpos];
                        val *= (-1.f / (32738.f));
                    }
                    else
                    {
                        val = -SampleDataF32[cpos];
                    }
                    val *= std::max(1.f, mVerticalZoom);

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
                    column[bottomline] = std::max((unsigned int)32 << aa_p2, column[bottomline]);

                    if (debugzoom)
                    {
                        for (int y = btop; y < bbottom; y++)
                        {
                            for (int z = 0; z < 4; z++)
                            {
                                img[y * imgw + x + z] =
                                    aatable[aaidx][std::min((unsigned int)255, column[y] >> aa_p2)];
                            }
                        }
                    }
                    else
                    {
                        for (int y = btop; y < bbottom; y++)
                            img[y * imgw + x] =
                                aatable[aaidx][std::min((unsigned int)255, column[y] >> aa_p2)];
                    }
                }
                pos += ratio;
            }
        }
    }
    prof.exit("main draw loop");


}

// convert sample position to pixel position
int WaveDisplay::samplePosToPixelPos(int sample)
{
    return ((sample- mLeftMostSample)/std::max(0,mZoom));
}

// draw the start/end/loop pos etc.
// bounds here is entire area
void WaveDisplay::drawDetails(Graphics &g, Rectangle<int> bounds)
{

    uint32 bgcola = juce::Colour(juce::Colours::red).getARGB(); // background
    juce::Colour wfcola = juce::Colours::white;                 // text color
    juce::Colour bgcold = juce::Colours::lightcoral;            // disabled
    juce::Colour wfcold = juce::Colours::lightgrey;             // text disabled
    juce::Colour zlcol = juce::Colour(0xff808080);
    juce::Colour zmcol = juce::Colour(0xff5d6c7d);
    juce::Colour lmcol = juce::Colour(0xff0462c2);
    juce::Colour lmcol_faint = juce::Colours::bisque; // tint area
    lmcol_faint = lmcol_faint.withAlpha(
        (uint8)100); // TODO I think we need alpha on this so it doesn't obscure the wave


    int imgw = bounds.getWidth();
    int imgh = bounds.getHeight();

    for (int i = 0; i < 256; i++) // TODO constant here
        dragpoint[i] = Rectangle<int>(0, 0, 0, 0);

    int waveLeft=mWaveBounds.getX();
    int waveTop=mWaveBounds.getY();
    int waveHeight=mWaveBounds.getHeight();
    int waveWidth=mWaveBounds.getWidth();

    if (pm_has_loop(playmode))
    {
        // AS note that the vg_rect stuff was x1,x2,y1,y2 based rather than x,y,width,height based
        int x1 = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::loopStart]);
        int x2 = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::loopEnd]);


        Rectangle<int> tr(x1+waveLeft, waveTop, x2 - x1, waveHeight);
        // ensure that it stays within our bounds (not that I think it matters)
        bounds.intersectRectangle(tr);
        g.setColour(lmcol_faint);
        g.fillRect(tr);

        dragpoint[2] = Rectangle<int>(0, 0, 13, 13);
        dragpoint[2].setCentre(x1+waveLeft, waveTop+waveHeight);

        // draw vertical line and icon for left loop marker
        if ((x1 >= 0) && (x1 < waveWidth))
        {
            g.setColour(lmcol);
            g.drawRect(x1+waveLeft, 0, 1, imgh-1);

            // TODO need to pull in this bitmap data
            // surf.blit_alphablend(dragpoint[2],bmpdata[2]);
        }

        dragpoint[3] = Rectangle<int>(0, 0, 13, 13);
        dragpoint[3].setCentre(x2+waveLeft, waveTop+waveHeight);
        if ((x2 >= 0) && (x2 < waveWidth))
        {
            g.setColour(lmcol);
            g.drawRect(x2+waveLeft, 0, 1, imgh-1);
            // TODO need to pull in this bitmap data
            // surf.blit_alphablend(dragpoint[3],bmpdata[3]);
        }
    }

    if (!pm_has_slices(playmode))
    {
        // not slice mode, so draw zone start and end markers
        int x = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::start]);

        dragpoint[0] = Rectangle<int>(0, 0, 13, 13);
        dragpoint[0].setCentre(x+waveLeft, waveTop);
        if ((x >= 0) && (x < waveWidth))
        {
            g.setColour(zmcol);
            g.drawRect(x+waveLeft,0,1,imgh-1);
            // TODO same as above
            // surf.blit_alphablend(dragpoint[0], bmpdata[0]);
        }

        x = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::end]);
        dragpoint[1] = Rectangle<int>(0, 0, 13, 13);
        dragpoint[1].setCentre(x+waveLeft,waveTop);
        if ((x >= 0) && (x < waveWidth))
        {
            g.setColour(zmcol);
            g.drawRect(x+waveLeft,0,1,imgh-1);
            // TODO same as above
            //surf.blit_alphablend(dragpoint[1], bmpdata[1]);
        }
    }
    else
    {
        // have slices. draw each
        for (int i = 0; i < n_hitpoints; i++)
        {
            int x1 = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::hitPointStart + i]);
            dragpoint[ActionWaveDisplayEditPoint::PointType::hitPointStart + i] = Rectangle<int>(0, 0, 13, 13);
            dragpoint[ActionWaveDisplayEditPoint::PointType::hitPointStart + i].setCentre(x1, imgh - 1);
            if ((x1 >= 0) && (x1 < imgw))
            {
                g.setColour(juce::Colour(0xffbc4e03));
                g.drawRect(x1,0,1,imgh-1);
                // TODO
                //surf.blit_alphablend(dragpoint[hitPointStart + i], bmpdata[5]);
            }
        }
    }

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


int vg_waveeditor::getsample(int pos)
{
	return (pos-WAVE_MARGIN)*zoom + mLeftMostSample;
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
				sample *so = mSamplePtr;
				if(so)
				{
					if (e.buttonmask & vgm_shift)
					{
						mVerticalZoom -= 0.1 * (e.y - lastmouseloc.y);
						mVerticalZoom = max(1.f,mVerticalZoom);
					}
					else
					{
						float movemult = 2.f;
						if (e.buttonmask & (vgm_RMB|vgm_control)) movemult = 8.f;
						mLeftMostSample -= (e.x - lastmouseloc.x)*zoom*movemult;
						mLeftMostSample = min(mLeftMostSample, (int)so->sample_length - zoom*mNumVisiblePixels);
						mLeftMostSample = max(0,mLeftMostSample);
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
				sample *so = mSamplePtr;
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

				mLeftMostSample -= getsample(e.x) - oldsample;
				sample *so = mSamplePtr;
				if(so) mLeftMostSample = min(mLeftMostSample, (int)so->sample_length - zoom*mNumVisiblePixels);
				mLeftMostSample = max(0,mLeftMostSample);

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
				vzoom_drag = mVerticalZoom;
				if (vzoom_drag < 1.01f) vzoom_drag = -2;
			}
			else if((e.eventtype == vget_mousedown)&&(e.activebutton == 2))
			{
				// context menu
				if((dispmode == 0) && mSamplePtr)
				{
					int s = getsample((int)e.x);
					sample *so = mSamplePtr;
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