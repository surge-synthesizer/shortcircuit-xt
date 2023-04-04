/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_DSP_PROCESSOR_PROCESSOR_CTRLDESCRIPTIONS_H
#define SCXT_SRC_DSP_PROCESSOR_PROCESSOR_CTRLDESCRIPTIONS_H

namespace scxt::dsp::processor
{

/*const char str_freqdef[] = ("f,-5,0.04,6,5,Hz"), str_freqmoddef[] = ("f,-12,0.04,12,0,oct"),
           str_timedef[] = ("f,-10,0.1,10,4,s"), str_lfofreqdef[] = ("f,-5,0.04,6,4,Hz"),
           str_percentdef[] = ("f,0,0.005,1,1,%"), str_percentbpdef[] = ("f,-1,0.005,1,1,%"),
           str_percentmoddef[] = ("f,-32,0.005,32,1,%"), str_dbdef[] = ("f,-96,0.1,12,0,dB"),
           str_dbbpdef[] = ("f,-48,0.1,48,0,dB"), str_dbmoddef[] = ("f,-96,0.1,96,0,dB"),
           str_mpitch[] = ("f,-96,0.04,96,0,cents"), str_bwdef[] = ("f,0.001,0.005,6,0,oct");*/

static constexpr datamodel::ControlDescription cdPercentDef{datamodel::ControlDescription::FLOAT,
                                                            datamodel::ControlDescription::LINEAR,
                                                            0,
                                                            0.005,
                                                            1,
                                                            1,
                                                            "%"},
    cdMPitch{datamodel::ControlDescription::FLOAT,
             datamodel::ControlDescription::LINEAR,
             -96,
             0.04,
             96,
             0,
             "semitones"},
    cdFreqDef{datamodel::ControlDescription::FLOAT,
              datamodel::ControlDescription::FREQUENCY,
              -5,
              0.04,
              6,
              5,
              "Hz"};

} // namespace scxt::dsp::processor

#endif // SHORTCIRCUITXT_PROCESSOR_CTRLDESCRIPTIONS_H
