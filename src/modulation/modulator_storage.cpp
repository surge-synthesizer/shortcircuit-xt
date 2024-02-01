/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#include "modulator_storage.h"

namespace scxt::modulation
{
std::string ModulatorStorage::toStringModulatorShape(
    const scxt::modulation::ModulatorStorage::ModulatorShape &ms)
{
    switch (ms)
    {
    case STEP:
        return "step";
    case MSEG:
        return "mseg";
    case LFO_SINE:
        return "lsine";
    case LFO_RAMP:
        return "lramp";
    case LFO_TRI:
        return "ltri";
    case LFO_PULSE:
        return "lpulse";
    case LFO_SMOOTH_NOISE:
        return "lnoise";
    case LFO_SH_NOISE:
        return "lshn";
    case LFO_ENV:
        return "lenv";
    }
    return "ERROR";
}

ModulatorStorage::ModulatorShape ModulatorStorage::fromStringModulatorShape(const std::string &s)
{
    static auto inverse =
        makeEnumInverse<ModulatorStorage::ModulatorShape, ModulatorStorage::toStringModulatorShape>(
            ModulatorShape::STEP, ModulatorStorage::MSEG);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return STEP;
    return p->second;
}
std::string
ModulatorStorage::toStringTriggerMode(const scxt::modulation::ModulatorStorage::TriggerMode &tm)
{
    switch (tm)
    {
    case KEYTRIGGER:
        return "kt";
    case FREERUN:
        return "free";
    case RANDOM:
        return "rnd";
    case RELEASE:
        return "rel";
    case ONESHOT:
        return "ones";
    }
    return "ERROR";
}

ModulatorStorage::TriggerMode ModulatorStorage::fromStringTriggerMode(const std::string &s)
{
    static auto inverse =
        makeEnumInverse<ModulatorStorage::TriggerMode, ModulatorStorage::toStringTriggerMode>(
            TriggerMode::KEYTRIGGER, TriggerMode::ONESHOT);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return KEYTRIGGER;
    return p->second;
}

} // namespace scxt::modulation