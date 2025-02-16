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

#include "sample/compound_file.h"

namespace scxt::sample::compound
{
std::vector<CompoundElement> getSF2SampleAddresses(const fs::path &p)
{
    auto riff = std::make_unique<RIFF::File>(p.u8string());
    auto sf = std::make_unique<sf2::File>(riff.get());

    auto sc = sf->GetSampleCount();
    std::vector<CompoundElement> result;
    result.reserve(sc);
    for (int i = 0; i < sc; ++i)
    {
        auto s = sf->GetSample(i);

        CompoundElement res;
        res.type = CompoundElement::Type::SAMPLE;
        res.name = s->GetName();
        res.sampleAddress = {Sample::SF2_FILE, p, "", -1, -1, i};
        result.push_back(res);
    }
    return result;
}

std::vector<CompoundElement> getSF2InstrumentAddresses(const fs::path &p)
{
    auto riff = std::make_unique<RIFF::File>(p.u8string());
    auto sf = std::make_unique<sf2::File>(riff.get());

    auto ic = sf->GetInstrumentCount();
    std::vector<CompoundElement> result;
    result.reserve(ic);
    for (int i = 0; i < ic; ++i)
    {
        auto in = sf->GetInstrument(i);

        CompoundElement res;
        res.type = CompoundElement::Type::INSTRUMENT;
        res.name = in->GetName();
        res.sampleAddress = {Sample::SF2_FILE, p, "", -1, i, -1};
        result.push_back(res);
    }

    return result;
}
} // namespace scxt::sample::compound
