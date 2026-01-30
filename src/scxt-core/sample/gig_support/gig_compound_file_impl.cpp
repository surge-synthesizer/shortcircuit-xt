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

#include "sample/compound_file.h"
#include "gig.h"

namespace scxt::sample::compound
{
std::vector<CompoundElement> getGIGCompoundList(const fs::path &p)
{
    auto riff = std::make_unique<RIFF::File>(p.u8string());
    auto gig = std::make_unique<gig::File>(riff.get());

    std::vector<CompoundElement> result;

    auto ins = gig->GetFirstInstrument();
    int instNo{0};
    while (ins)
    {
        std::string name = "Instrument " + std::to_string(instNo + 1);

        if (ins->pInfo)
        {
            name = ins->pInfo->Name;
        }
        CompoundElement res;
        res.type = CompoundElement::Type::INSTRUMENT;
        res.name = name;

        res.sampleAddress = {Sample::GIG_FILE, p, "", instNo, -1, -1};
        result.push_back(res);
        ins = gig->GetNextInstrument();
        instNo++;
    }
    auto sm = gig->GetFirstSample();
    int smNo{0};
    while (sm)
    {
        std::string name = "Sample " + std::to_string(smNo + 1);

        if (sm->pInfo)
        {
            name = sm->pInfo->Name;
        }
        CompoundElement res;
        res.type = CompoundElement::Type::SAMPLE;
        res.name = name;

        res.sampleAddress = {Sample::GIG_FILE, p, "", -1, -1, smNo};
        result.push_back(res);
        sm = gig->GetNextSample();
        smNo++;
    }

    return result;
}

} // namespace scxt::sample::compound
