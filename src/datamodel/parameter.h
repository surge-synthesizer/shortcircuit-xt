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

#ifndef SCXT_SRC_DATAMODEL_PARAMETER_H
#define SCXT_SRC_DATAMODEL_PARAMETER_H

#include <string>
#include <optional>

#include "sst/basic-blocks/modulators/ADSREnvelope.h"

namespace scxt::datamodel
{

struct ControlDescription
{
    // we want to be able to inplace new so have fixed size please
    static constexpr int labelLen{32};
    static constexpr int maxIntChoices{16};
    enum Type
    {
        NONE,
        INT,
        FLOAT
    } type{NONE};
    enum DisplayMode
    {
        LINEAR,       // mapScale * value + mapBase
        TWO_TO_THE_X, // 2 ^ (mapScale * value + mapBase)
        FREQUENCY,    // 261.5etc * 2 ^ (mapScale * value + mapBase) / 12
        MIDI_NOTE,
        DECIBEL
    } displayMode{LINEAR};
    float min{0};
    float step{0.01};
    float max{1};
    float def{0.5};
    char unit[labelLen]{}; // fixed size makes in place news easier
    float mapScale{1.f};
    float mapBase{0.f};

    char choices[maxIntChoices][32]{};

    std::string valueToString(float value) const;
    std::optional<float> stringToValue(const std::string &s) const;
};

/*
 * We have commonly used instances here
 */
static constexpr ControlDescription cdPercent{
    ControlDescription::FLOAT, ControlDescription::LINEAR, 0, 0.01, 1, 0, "%", 100.0},
    cdPercentBipolar{
        ControlDescription::FLOAT, ControlDescription::LINEAR, -1, 0.01, 1, 0, "%", 100.0},
    cdMidiNote{ControlDescription::INT, ControlDescription::MIDI_NOTE, 0, 1, 127, 60, "note"},
    cdDecibel{ControlDescription::FLOAT, ControlDescription::DECIBEL, -96, 0.1, 12, 0, "dB"},
    cdTimeUnscaledThirtyTwo{
        ControlDescription::FLOAT, ControlDescription::TWO_TO_THE_X, -8, 0.01, 5, 0.0, "seconds"},
    cdEnvelopeThirtyTwo{ControlDescription::FLOAT,
                        ControlDescription::TWO_TO_THE_X,
                        0,
                        0.01,
                        1,
                        0.5,
                        "seconds",
                        sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMax -
                            sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin,
                        sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin};

} // namespace scxt::datamodel

#endif // __SCXT_PARAMETER_H
