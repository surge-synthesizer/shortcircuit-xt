/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_WAVEDISPLAY_H
#define SHORTCIRCUIT_WAVEDISPLAY_H

#include "juce_gui_basics/juce_gui_basics.h"

#include <SCXTPluginEditor.h>
#include "infrastructure/profiler.h"

namespace scxt
{
namespace components
{
class WaveDisplay : public juce::Component, public scxt::data::UIStateProxy
{
    enum ControlState
    {
        cs_default = 0,
        cs_pan,
        cs_dragpoint,
    };

    SCXTEditor *mEditor;
    scxt::log::StreamLogger mLogger;

    scxt::Perf::Profiler prof;

    // boundary around actual waveform display (excludes margin)
    juce::Rectangle<int> mWaveBounds, mTextBounds;

    // this will hold the rendered wave
    juce::Image mWavePixels;

    sample *mSamplePtr;
    int dispmode;
    int mZoom; // zoom factor. A value of 1 means 1 pixel is 1 sample. Higher value is more zoomed
               // out
    int mZoomMax;
    int mLeftMostSample; // leftmost sample in the display (in samples)
    float mVerticalZoom, vzoom_drag;
    int mNumVisiblePixels; // width in pixels of visible wave display
    int n_hitpoints;
    unsigned int aatable[4][256];
    int playmode;
    int markerpos[256];
    bool draw_be_quick, draw_skip_wave_redraw;
    juce::Rectangle<int> dragpoint[256];
    int dragid = -1; // which item is being dragged
    int controlstate = cs_default;

    // we need a point of reference from last drag event as we are modifying state while dragging
    juce::Point<int> mZoomPanOffset;
    /*
        vg_surface wavesurf;

        vg_point lastmouseloc;
        vg_bitmap bmpdata[16];
        vg_menudata md;	// context menu
        */

    void queue_draw_wave(bool be_quick, bool skip_wave_redraw);

    // blast the wave to mWavePixels
    // if quick is specified, then no anti-aliasing is done (mouse panning or scrolling)
    void renderWave(bool quick);

    // draw the start/end/loop points
    void drawDetails(juce::Graphics &g, juce::Rectangle<int> bounds);

    // conversion
    int samplePosToPixelPos(int sample);
    int pixelPosToSamplePos(int pos);

    // implement Component
    void paint(juce::Graphics &g) override;
    void resized() override;
    // we might not need all of these...
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void mouseEnter(const juce::MouseEvent &event) override;
    void mouseExit(const juce::MouseEvent &event) override;
    void mouseMove(const juce::MouseEvent &event) override;

    // implement UIStateProxy
    virtual bool processActionData(const actiondata &d) override;

  public:
    WaveDisplay(SCXTEditor *sender, scxt::log::LoggingCallback *logger);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveDisplay);
};

} // namespace components
} // namespace scxt

#endif // SHORTCIRCUIT_WAVEDISPLAY_H
