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

#ifndef SHORTCIRCUIT_WAVEDISPLAY_H
#define SHORTCIRCUIT_WAVEDISPLAY_H
#include <JuceHeader.h>
#include <SC3Editor.h>
#include "infrastructure/profiler.h"

class WaveDisplay : public juce::Component, public UIStateProxy
{
    enum ControlState
    {
        cs_default = 0,
        cs_pan,
        cs_dragpoint,
    };

    ActionSender *mSender;
    SC3::Log::StreamLogger mLogger;

    SC3::Perf::Profiler prof;

    // boundary around actual waveform display (excludes margin)
    Rectangle<int> mWaveBounds;

    // this will hold the rendered wave
    Image mWavePixels;

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
    Rectangle<int> dragpoint[256];
    int dragid = -1; // which item is being dragged
    int controlstate = cs_default;

    // we need a point of reference from last drag event as we are modifying state while dragging
    Point<int> mZoomPanOffset;
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
    void drawDetails(Graphics &g, Rectangle<int> bounds);

    // conversion
    int samplePosToPixelPos(int sample);
    int pixelPosToSamplePos(int pos);

    // implement Component
    void paint(Graphics &g) override;
    void resized() override;
    // we might not need all of these...
    void mouseDrag(const MouseEvent &event) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    void mouseEnter(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;
    void mouseMove(const MouseEvent &event) override;

    // implement UIStateProxy
    virtual bool processActionData(const actiondata &d) override;

  public:
    WaveDisplay(ActionSender *sender, SC3::Log::LoggingCallback *logger);
};

#endif // SHORTCIRCUIT_WAVEDISPLAY_H
