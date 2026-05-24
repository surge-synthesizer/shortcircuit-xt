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

#include "perf_loader.h"

#include <chrono>
#include <fstream>
#include <iostream>

#include "engine/engine.h"
#include "messaging/messaging.h"
#include "patch_io/patch_io.h"
#include "sample/sf2_support/sf2_import.h"

#include "gig.h"
#include "SF.h"

#include "perf_fnv.h"

namespace scxt::perf
{

namespace
{
uint64_t fnv1aFile(const std::filesystem::path &p, size_t &bytesOut)
{
    std::ifstream f(p, std::ios::binary);
    uint64_t h = kFnvOffsetBasis;
    bytesOut = 0;
    char buf[64 * 1024];
    while (f)
    {
        f.read(buf, sizeof(buf));
        auto n = (size_t)f.gcount();
        bytesOut += n;
        fnv1a(h, buf, n);
    }
    return h;
}

std::string rtrim(std::string s)
{
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\0'))
        s.pop_back();
    return s;
}

// Returns the preset index for a given name (trim trailing whitespace, case
// sensitive). If no match, prints the full index→name table to stdout and
// returns nullopt — the caller turns that into a load error.
std::optional<int> resolveSF2PresetByName(const std::filesystem::path &p, const std::string &want)
{
    auto riff = std::make_unique<RIFF::File>(p.u8string());
    auto sf = std::make_unique<sf2::File>(riff.get());
    auto wantTrim = rtrim(want);
    int count = sf->GetPresetCount();
    for (int i = 0; i < count; ++i)
        if (rtrim(std::string(sf->GetPreset(i)->GetName())) == wantTrim)
            return i;

    std::cout << "scxt-perf: SF2 preset '" << want << "' not found in " << p.u8string() << "\n";
    std::cout << "Available presets:\n";
    for (int i = 0; i < count; ++i)
        std::cout << "  [" << i << "] " << sf->GetPreset(i)->GetName() << "\n";
    return std::nullopt;
}

void countStructure(scxt::engine::Engine &e, LoadResult &r)
{
    const auto &patch = e.getPatch();
    int parts{0}, groups{0}, zones{0};
    for (size_t pi = 0; pi < scxt::numParts; ++pi)
    {
        auto &part = patch->getPart(pi);
        if (part->getGroups().empty())
            continue;
        parts++;
        for (const auto &g : part->getGroups())
        {
            groups++;
            zones += (int)g->getZones().size();
        }
    }
    r.numParts = parts;
    r.numGroups = groups;
    r.numZones = zones;

    auto lk = e.getSampleManager()->acquireMapLock();
    int n = 0;
    for (auto it = e.getSampleManager()->samplesBegin(); it != e.getSampleManager()->samplesEnd();
         ++it)
        n++;
    r.numSamples = n;
}
} // namespace

LoadResult applyLoad(scxt::engine::Engine &engine, const std::vector<LoadStep> &steps)
{
    LoadResult r;
    auto bypass = engine.getMessageController()->threadingChecker.bypassChecksInScope();

    auto t0 = std::chrono::steady_clock::now();
    uint64_t composite = 0xcbf29ce484222325ULL;

    for (const auto &s : steps)
    {
        if (!std::filesystem::exists(s.path))
        {
            r.error = "File not found: " + s.path.u8string();
            return r;
        }
        size_t bytes = 0;
        auto h = fnv1aFile(s.path, bytes);
        r.totalFileBytes += bytes;
        // fold per-file hash into composite (path|hash) so order-of-load matters
        auto pathStr = s.path.u8string();
        fnv1a(composite, pathStr.data(), pathStr.size());
        fnv1a(composite, &h, sizeof(h));

        switch (s.kind)
        {
        case LoadStep::LoadMulti:
            patch_io::loadMulti(s.path, engine);
            break;
        case LoadStep::LoadPartInto:
            patch_io::loadPartInto(s.path, engine, s.part);
            break;
        case LoadStep::LoadInstrument:
            if (s.sf2Preset || s.sf2PresetName)
            {
                int idx = -1;
                if (s.sf2Preset)
                {
                    idx = *s.sf2Preset;
                }
                else
                {
                    auto resolved = resolveSF2PresetByName(s.path, *s.sf2PresetName);
                    if (!resolved)
                    {
                        r.error = "SF2 preset name '" + *s.sf2PresetName + "' not found in " +
                                  s.path.u8string();
                        return r;
                    }
                    idx = *resolved;
                }
                sf2_support::importSF2(s.path, engine, idx);
            }
            else
            {
                engine.loadSampleIntoSelectedPartAndGroup(s.path, /*forceSynchronous=*/true);
            }
            break;
        }
    }
    auto t1 = std::chrono::steady_clock::now();
    r.wallMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    r.fileHash = composite;
    r.ok = true;
    countStructure(engine, r);
    return r;
}

} // namespace scxt::perf
