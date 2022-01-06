/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "WaveDisplay.h"
#include "sampler_wrapper_actiondata.h"
#include "interaction_parameters.h"
// AS this is for the limit_range function
#include "vembertech/vt_dsp/basic_dsp.h"
#include <algorithm>
#include "infrastructure/profiler.h"

#include "BinaryUIAssets.h"

const int WAVE_MARGIN = 6;      // margin around wave display in pixels
const int WAVE_CHANNEL_GAP = 4; // gap between channels

static void calc_aatable(unsigned int *tbl, unsigned int col1, unsigned int col2);

WaveDisplay::WaveDisplay(ActionSender *sender, SC3::Log::LoggingCallback *logger)
    : mSender(sender), mLogger(logger), prof(logger, "Wave Display Paint")
{
    dispmode = 0;
    mSamplePtr = 0;
    mZoom = 1;
    mZoomMax = 1;
    mVerticalZoom = 1.f;
    mLeftMostSample = 0;
    n_hitpoints = 0;

    calc_aatable(aatable[0], 0xFFFFFFFF, 0xFF000000);
    calc_aatable(aatable[1], 0xFF000000, 0xFFFFFF);
    calc_aatable(aatable[2], 0xff4ea1f3, 0xff000000);

    // Todo as enable once we have it
    // for(int i=0; i<7; i++)	bmpdata[i] = owner->get_art(vgct_waveedit,0,i);

    // todo e refers to xml element
    // restore_data(e);
}
void WaveDisplay::resized()
{

    // actual wave dimensions we'll be drawing in and using for calculations
    mWaveBounds = getLocalBounds().reduced(WAVE_MARGIN, WAVE_MARGIN);

    mNumVisiblePixels = mWaveBounds.getWidth() - 2;

    queue_draw_wave(false, false);
}

void WaveDisplay::paint(juce::Graphics &g)
{

    prof.enter();
    juce::Rectangle<int> r = getLocalBounds();
    g.setColour(
        juce::Colours::darkgrey); // background color for entire control (wave bg is separate)
    g.fillRect(r);

    switch (dispmode)
    {
    case 0:
        if (!mSamplePtr)
        {
            g.setColour(juce::Colours::white); // foreground color for text
            g.drawText("No sample.", r,
                       juce::Justification::horizontallyCentred |
                           juce::Justification::verticallyCentred);
        }
        else
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
            g.drawText("Multiple zones selected. Changes will be applied to all of them.", r,
                       juce::Justification::horizontallyCentred |
                           juce::Justification::verticallyCentred);
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
bool WaveDisplay::processActionData(const actiondata &ad)
{
    bool res = false;
    if (!std::holds_alternative<VAction>(ad.actiontype))
        return false;

    auto at = std::get<VAction>(ad.actiontype);
    if (at == vga_wavedisp_sample)
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

        if (mSamplePtr)
        {
            // calculate the max amount we can zoom out (higher value is more zoomed out)
            float ratio =
                std::max(1.f, float(mSamplePtr->sample_length) / float(mNumVisiblePixels));
            mZoomMax = (int)ceil(ratio);
            if (same_sample)
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
        queue_draw_wave(false, false);
    }
    if (at == vga_wavedisp_editpoint)
    {
        auto e = (const ActionWaveDisplayEditPoint &)(ad);
        markerpos[e.dragId()] = e.samplePos();

        // todo do we really need a full redraw here?
        queue_draw_wave(false, false);
    }
    else if (at == vga_wavedisp_multiselect)
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
    for (int i = 0; i < 256; i++)
    {
        float x = i * (1.f / 255.f);
        int c[4];
        float f[4];

        f[0] = (1 - x) * powf((col1 & 0xff) * (1.f / 255.f), 1 / 2.2f) +
               x * powf((col2 & 0xff) * (1.f / 255.f), 1 / 2.2f);
        f[1] = (1 - x) * powf(((col1 >> 8) & 0xff) * (1.f / 255.f), 1 / 2.2f) +
               x * powf(((col2 >> 8) & 0xff) * (1.f / 255.f), 1 / 2.2f);
        f[2] = (1 - x) * powf(((col1 >> 16) & 0xff) * (1.f / 255.f), 1 / 2.2f) +
               x * powf(((col2 >> 16) & 0xff) * (1.f / 255.f), 1 / 2.2f);
        c[0] = std::max(0, std::min(255, (int)(255.f * f[0])));
        c[1] = std::max(0, std::min(255, (int)(255.f * f[1])));
        c[2] = std::max(0, std::min(255, (int)(255.f * f[2])));
        tbl[i] = 0xff000000 | (c[2] << 16) | (c[1] << 8) | c[0];
    }
}

void WaveDisplay::renderWave(bool quick)
{
    // draw wave within our margin
    juce::Rectangle<int> bounds = mWaveBounds;

    int imgw = bounds.getWidth();
    int imgh = bounds.getHeight();

    // software image type has known pixel format (may be slower?)
    mWavePixels =
        juce::Image(juce::Image::PixelFormat::ARGB, imgw, imgh, true, juce::SoftwareImageType());
    juce::Image::BitmapData pixelsBmp(mWavePixels, juce::Image::BitmapData::writeOnly);
    // gain access to the raw pixels which we'll be modifying
    uint32 *img = (unsigned int *)pixelsBmp.data;

    // float fratio = (float)(s->sample_length) / (float)(imgw);
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
    }
    else
    {
        sample_inc = std::max(1, ratio / maxSamples);
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
                    // if (pm_has_loop(playmode)&&(cpos >= loop_start)&&(cpos < loop_end)) aaidx =
                    // 2;
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
    return ((sample - mLeftMostSample) / std::max(0, mZoom));
}

// untested! note that there is WAVE_MARGIN that needs to be accounted for
int WaveDisplay::pixelPosToSamplePos(int pos) { return pos * mZoom + mLeftMostSample; }

// draw the start/end/loop pos etc.
// bounds here is entire area
void WaveDisplay::drawDetails(juce::Graphics &g, juce::Rectangle<int> bounds)
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
        (uint8_t)100); // TODO I think we need alpha on this so it doesn't obscure the wave

    int imgw = bounds.getWidth();
    int imgh = bounds.getHeight();

    for (int i = 0; i < 256; i++) // TODO constant here
        dragpoint[i] = juce::Rectangle<int>(0, 0, 0, 0);

    int waveLeft = mWaveBounds.getX();
    int waveTop = mWaveBounds.getY();
    int waveHeight = mWaveBounds.getHeight();
    int waveWidth = mWaveBounds.getWidth();

    if (pm_has_loop(playmode))
    {
        // AS note that the vg_rect stuff was x1,x2,y1,y2 based rather than x,y,width,height based
        int x1 = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::loopStart]);
        int x2 = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::loopEnd]);

        juce::Rectangle<int> tr(x1 + waveLeft, waveTop, x2 - x1, waveHeight);
        // ensure that it stays within our bounds (not that I think it matters)
        bounds.intersectRectangle(tr);
        g.setColour(lmcol_faint);
        g.fillRect(tr);

        dragpoint[2] = juce::Rectangle<int>(0, 0, 13, 13);
        dragpoint[2].setCentre(x1 + waveLeft, waveTop + waveHeight);

        // draw vertical line and icon for left loop marker
        if ((x1 >= 0) && (x1 < waveWidth))
        {
            g.setColour(lmcol);
            g.drawRect(x1 + waveLeft, 0, 1, imgh - 1);

            // TODO need to pull in this bitmap data
            // surf.blit_alphablend(dragpoint[2],bmpdata[2]);
        }

        dragpoint[3] = juce::Rectangle<int>(0, 0, 13, 13);
        dragpoint[3].setCentre(x2 + waveLeft, waveTop + waveHeight);
        if ((x2 >= 0) && (x2 < waveWidth))
        {
            g.setColour(lmcol);
            g.drawRect(x2 + waveLeft, 0, 1, imgh - 1);
            // TODO need to pull in this bitmap data
            // surf.blit_alphablend(dragpoint[3],bmpdata[3]);
        }
    }

    if (!pm_has_slices(playmode))
    {
        // not slice mode, so draw zone start and end markers
        int x = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::start]);

        dragpoint[0] = juce::Rectangle<int>(0, 0, 13, 13);
        dragpoint[0].setCentre(x + waveLeft, waveTop);
        if ((x >= 0) && (x < waveWidth))
        {
            g.setColour(zmcol);
            g.drawRect(x + waveLeft, 0, 1, imgh - 1);
            auto img = juce::ImageCache::getFromMemory(SCXTUIAssets::wavehandle_start_png,
                                                       SCXTUIAssets::wavehandle_start_pngSize);
            g.drawImageAt(img, x + waveLeft - (img.getWidth() / 2), 0);
        }

        x = samplePosToPixelPos(markerpos[ActionWaveDisplayEditPoint::PointType::end]);
        dragpoint[1] = juce::Rectangle<int>(0, 0, 13, 13);
        dragpoint[1].setCentre(x + waveLeft, waveTop);
        if ((x >= 0) && (x < waveWidth))
        {
            g.setColour(zmcol);
            g.drawRect(x + waveLeft, 0, 1, imgh - 1);
            auto img = juce::ImageCache::getFromMemory(SCXTUIAssets::wavehandle_end_png,
                                                       SCXTUIAssets::wavehandle_end_pngSize);
            g.drawImageAt(img, x + waveLeft - (img.getWidth() / 2), 0);
        }
    }
    else
    {
        // have slices. draw each
        for (int i = 0; i < n_hitpoints; i++)
        {
            int x1 = samplePosToPixelPos(
                markerpos[ActionWaveDisplayEditPoint::PointType::hitPointStart + i]);
            dragpoint[ActionWaveDisplayEditPoint::PointType::hitPointStart + i] =
                juce::Rectangle<int>(0, 0, 13, 13);
            dragpoint[ActionWaveDisplayEditPoint::PointType::hitPointStart + i].setCentre(x1,
                                                                                          imgh - 1);
            if ((x1 >= 0) && (x1 < imgw))
            {
                g.setColour(juce::Colour(0xffbc4e03));
                g.drawRect(x1, 0, 1, imgh - 1);
                // TODO
                // surf.blit_alphablend(dragpoint[hitPointStart + i], bmpdata[5]);
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
void WaveDisplay::mouseDrag(const juce::MouseEvent &event)
{
    if (controlstate == cs_pan)
    {
        sample *so = mSamplePtr;
        if (so)
        {
            if (event.mods.isShiftDown())
            {
                mVerticalZoom -= 0.1 * (event.y - mZoomPanOffset.y);
                mVerticalZoom = std::max(1.f, mVerticalZoom);
            }
            else
            {
                // TODO  note that in this algo, the panning doesn't exactly match the mouse pos
                //   need to fix
                float movemult = 2.f;
                if (event.mods.isRightButtonDown())
                    movemult = 8.f;
                mLeftMostSample -= (event.x - mZoomPanOffset.x) * mZoom * movemult;
                mLeftMostSample =
                    std::min(mLeftMostSample, (int)so->sample_length - mZoom * mNumVisiblePixels);
                mLeftMostSample = std::max(0, mLeftMostSample);
            }

            mZoomPanOffset = event.getPosition();
        }
        queue_draw_wave(true, false);
    }
    else if (controlstate == cs_dragpoint)
    {
        sample *so = mSamplePtr;
        if (so)
        {
            int s = pixelPosToSamplePos(event.x + WAVE_MARGIN);
            s = limit_range(s, 0, (int)(so->sample_length));
            markerpos[dragid] = s;
            queue_draw_wave(false, true);
            repaint();
            // TODO use our wrapper
            actiondata ad;
            ad.actiontype = vga_wavedisp_editpoint;
            ad.data.i[0] = dragid;
            ad.data.i[1] = s;
            mSender->sendActionToEngine(ad);
        }
    }
}

void WaveDisplay::mouseDown(const juce::MouseEvent &event)
{

    auto mp = event.getMouseDownPosition();
    // this will be updated as we are dragging
    mZoomPanOffset = mp;

    for (int i = 0; i < (5 + n_hitpoints); i++)
    {
        if (dragpoint[i].contains(mp))
        {
            controlstate = cs_dragpoint;
            // owner->take_focus(control_id);

            dragid = i;
            return;
        }
    }
    // did not click on a line, so we are in pan mode
    controlstate = cs_pan;
    // owner->take_focus(control_id);
    // owner->show_mousepointer(false);

    vzoom_drag = mVerticalZoom;
    if (vzoom_drag < 1.01f)
        vzoom_drag = -2;

#if 0
    //TODO context menu
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
#endif
}

void WaveDisplay::mouseUp(const juce::MouseEvent &event)
{

    // temporarily use this for zoom in or out (left click/right click)
    // until we have a better way
    if (event.mouseWasClicked())
    {
        int oldsample = pixelPosToSamplePos(event.x + WAVE_MARGIN);

        if (event.mods.isRightButtonDown())
            mZoom = std::min(mZoomMax, mZoom << 1);
        else
            mZoom = std::max(1, mZoom >> 1);

        mLeftMostSample -= pixelPosToSamplePos(event.x + WAVE_MARGIN) - oldsample;
        sample *so = mSamplePtr;
        if (so)
            mLeftMostSample =
                std::min(mLeftMostSample, (int)so->sample_length - mZoom * mNumVisiblePixels);
        mLeftMostSample = std::max(0, mLeftMostSample);

        queue_draw_wave(false, false);
    }
    else
    {
        // TODO Technically there is another state we should detect: mouse up without click
        // (clicked, didn't drag, let go)
        queue_draw_wave(false, false);
    }
    controlstate = cs_default;
}
void WaveDisplay::mouseEnter(const juce::MouseEvent &event) { Component::mouseEnter(event); }
void WaveDisplay::mouseExit(const juce::MouseEvent &event) { Component::mouseExit(event); }

#if 0
// TODO mouse wheel
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
#endif

// this should be used to change the mouse cursor
void WaveDisplay::mouseMove(const juce::MouseEvent &event)
{
    auto mp = event.getPosition();
    for (int i = 0; i < (5 + n_hitpoints); i++)
    {
        if (dragpoint[i].contains(mp))
        {
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            return;
        }
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);
    return;
}
