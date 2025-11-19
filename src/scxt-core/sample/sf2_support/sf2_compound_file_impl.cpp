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

#include "sample/compound_file.h"
#include "sst/plugininfra/strnatcmp.h"

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

    auto ic = sf->GetPresetCount();
    std::vector<CompoundElement> result;
    result.reserve(ic);

    std::vector<std::pair<sf2::Preset *, int>> presets;

    for (int i = 0; i < ic; ++i)
    {
        presets.push_back({sf->GetPreset(i), i});
    }

    std::sort(presets.begin(), presets.end(), [](const auto &aa, const auto &bb) {
        auto a = aa.first;
        auto b = bb.first;
        auto na = a->GetName();
        auto nb = b->GetName();

        auto abc = strnatcmp(na.c_str(), nb.c_str());
        if (abc != 0)
            return abc < 0;

        if (a->Bank != b->Bank)
            return a->Bank < b->Bank;
        return a->PresetNum < b->PresetNum;
    });

    for (auto pr : presets)
    {
        auto in = pr.first;
        auto i = pr.second;
        CompoundElement res;
        res.type = CompoundElement::Type::INSTRUMENT;
        res.name = in->GetName();
        res.sampleAddress = {Sample::SF2_FILE, p, "", i, -1, -1};
        result.push_back(res);
    }

    return result;
}
} // namespace scxt::sample::compound
