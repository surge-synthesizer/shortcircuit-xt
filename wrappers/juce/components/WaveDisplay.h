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

#ifndef SHORTCIRCUIT_WAVEDISPLAY_H
#define SHORTCIRCUIT_WAVEDISPLAY_H
#include <JuceHeader.h>
#include <SC3Editor.h>
#include "infrastructure/profiler.h"

class WaveDisplay : public juce::Component, public UIStateProxy
{
    ActionSender *mSender;
    SC3::Log::StreamLogger mLogger;

    SC3::Perf::Profiler prof;


    void *sampleptr;
    int dispmode;
    int zoom,zoom_max,start;
    float vzoom,vzoom_drag;
    int n_visible_pixels,n_hitpoints;
    unsigned int aatable[4][256];
    int playmode;
    int markerpos[256];
    bool draw_be_quick, draw_skip_wave_redraw;
/*
    vg_surface wavesurf;



    int controlstate,dragid;


    vg_point lastmouseloc;

    vg_bitmap bmpdata[16];

    vg_rect dragpoint[256];

    vg_menudata md;	// context menu
    */

    void queue_draw_wave(bool be_quick, bool skip_wave_redraw);
    // blast the wave to the area in g specified by bounds
    void drawWave(Graphics &g, bool be_quick, bool skip_wave_redraw, Rectangle<int> bounds);

    // implement Component
    void paint(Graphics &g) override;

    // implement UIStateProxy
    virtual bool processActionData(const actiondata &d) override;

  public:
    WaveDisplay(ActionSender *sender, SC3::Log::LoggingCallback *logger);


};

#endif // SHORTCIRCUIT_WAVEDISPLAY_H
