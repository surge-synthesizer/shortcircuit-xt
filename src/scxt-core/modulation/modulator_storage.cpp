/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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
namespace modulators
{

std::string PhasorStorage::toStringDivision(const Division &s)
{
    switch (s)
    {
    case Division::NOTE:
        return "n";
    case Division::TRIPLET:
        return "t";
    case Division::DOTTED:
        return "d";
    case Division::OF_BPM:
        return "op";
    case Division::OF_BEAT:
        return "ob";
    }
    return "ERR";
}

PhasorStorage::Division PhasorStorage::fromStringDivision(const std::string &s)
{
    static auto inverse = makeEnumInverse<PhasorStorage::Division, PhasorStorage::toStringDivision>(
        PhasorStorage::Division::NOTE, PhasorStorage::Division::OF_BEAT);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return PhasorStorage::Division::NOTE;
    return p->second;
}

std::string PhasorStorage::toStringSyncMode(const SyncMode &s)
{
    switch (s)
    {
    case SyncMode::SONGPOS:
        return "sp";
    case SyncMode::VOICEPOS:
        return "vp";
    }
    return "ERR";
}

PhasorStorage::SyncMode PhasorStorage::fromStringSyncMode(const std::string &s)
{
    static auto inverse = makeEnumInverse<PhasorStorage::SyncMode, PhasorStorage::toStringSyncMode>(
        PhasorStorage::SyncMode::SONGPOS, PhasorStorage::SyncMode::VOICEPOS);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return PhasorStorage::SyncMode::VOICEPOS;
    return p->second;
}

std::string RandomStorage::toStringStyle(const Style &s)
{
    switch (s)
    {
    case RandomStorage::Style::UNIFORM_01:
        return "u01";
    case RandomStorage::Style::HALF_NORMAL:
        return "hfn";
    case RandomStorage::Style::NORMAL:
        return "nm";
    case RandomStorage::Style::UNIFORM_BIPOLAR:
        return "ubp";
    }
    return "ERR";
}

RandomStorage::Style RandomStorage::fromStringStyle(const std::string &s)
{
    static auto inverse = makeEnumInverse<RandomStorage::Style, RandomStorage::toStringStyle>(
        RandomStorage::Style::UNIFORM_01, RandomStorage::Style::HALF_NORMAL);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return RandomStorage::Style::UNIFORM_01;
    return p->second;
}

} // namespace modulators

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
    case LFO_SAW_TRI_RAMP:
        return "lstriramp";
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