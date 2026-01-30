/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
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

std::string AdsrStorage::toStringGateMode(const GateMode &s)
{
    switch (s)
    {
    case GateMode::GATED:
        return "g";
    case GateMode::SEMI_GATED:
        return "s";
    case GateMode::ONESHOT:
        return "o";
    }
    return "g";
}

AdsrStorage::GateMode AdsrStorage::fromStringGateMode(const std::string &s)
{
    static auto inverse = makeEnumInverse<AdsrStorage::GateMode, AdsrStorage::toStringGateMode>(
        AdsrStorage::GateMode::GATED, AdsrStorage::GateMode::ONESHOT);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return GateMode::GATED;
    return p->second;
}

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
    case Division::X_BPM:
        return "op";
    }
    return "ERR";
}

PhasorStorage::Division PhasorStorage::fromStringDivision(const std::string &s)
{
    static auto inverse = makeEnumInverse<PhasorStorage::Division, PhasorStorage::toStringDivision>(
        PhasorStorage::Division::NOTE, PhasorStorage::Division::X_BPM);
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

std::string PhasorStorage::toStringDirection(const Direction &d)
{
    switch (d)
    {
    case ASCENDING:
        return "a";
    case DESCENDING:
        return "d";
    }
    return "ERR";
}

PhasorStorage::Direction PhasorStorage::fromStringDirection(const std::string &d)
{
    static auto inverse = makeEnumInverse<Direction, PhasorStorage::toStringDirection>(
        PhasorStorage::Direction::ASCENDING, PhasorStorage::Direction::DESCENDING);
    auto p = inverse.find(d);
    if (p == inverse.end())
        return PhasorStorage::Direction::ASCENDING;
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