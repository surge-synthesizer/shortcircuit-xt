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

#ifndef SCXT_SRC_JSON_SAMPLE_TRAITS_H
#define SCXT_SRC_JSON_SAMPLE_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "stream.h"

#include "sample/sample_manager.h"
#include "scxt_traits.h"

namespace scxt::json
{
// TODO - REPLACE THIS with an Enum Stream fn
template <> struct scxt_traits<scxt::sample::Sample::SourceType>
{
    static constexpr const char *key = "sourceType";
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::sample::Sample::SourceType &e)
    {
        switch (e)
        {
        case sample::Sample::WAV_FILE:
            v = {{key, "wav_file"}};
            break;
        case sample::Sample::FLAC_FILE:
            v = {{key, "flac_file"}};
            break;
        case sample::Sample::MP3_FILE:
            v = {{key, "mp3_file"}};
            break;
        case sample::Sample::SF2_FILE:
            v = {{key, "sf2_file"}};
            break;
        case sample::Sample::AIFF_FILE:
            v = {{key, "aiff_file"}};
            break;
        case sample::Sample::MULTISAMPLE_FILE:
            v = {{key, "multisample_file"}};
            break;
        }
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::sample::Sample::SourceType &r)
    {
        std::string k{};
        findIf(v, key, k);

        if (k == "wav_file")
            r = sample::Sample::WAV_FILE;
        if (k == "sf2_file")
            r = sample::Sample::SF2_FILE;
        if (k == "flac_file")
            r = sample::Sample::FLAC_FILE;
        if (k == "mp3_file")
            r = sample::Sample::MP3_FILE;
        if (k == "aiff_file")
            r = sample::Sample::AIFF_FILE;
        if (k == "multisample_file")
            r = sample::Sample::MULTISAMPLE_FILE;

        return;
    }
};

template <> struct scxt_traits<sample::Sample::SampleFileAddress>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v,
                       const scxt::sample::Sample::SampleFileAddress &f)
    {
        v = {{"type", f.type},     {"path", f.path.u8string()},  {"md5sum", f.md5sum},
             {"preset", f.preset}, {"instrument", f.instrument}, {"region", f.region}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v,
                   scxt::sample::Sample::SampleFileAddress &f)
    {
        findOrSet(v, "type", sample::Sample::WAV_FILE, f.type);
        std::string p;
        findIf(v, "path", p);
        f.path = fs::path{p};
        findIf(v, "md5sum", f.md5sum);
        findOrSet(v, "preset", 0, f.preset); // 0 here since we forgot to stream for a bit
        findOrSet(v, "instrument", -1, f.instrument);
        findOrSet(v, "region", -1, f.region);
    }
};
template <> struct scxt_traits<scxt::sample::SampleManager>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const scxt::sample::SampleManager &e)
    {
        // TODO: Probably stream samples better than this
        v = {{"sampleAddresses", e.getSampleAddressesAndIDs()}};
    }

    template <template <typename...> class Traits>
    static void to(const tao::json::basic_value<Traits> &v, scxt::sample::SampleManager &mgr)
    {
        mgr.reset();
        sample::SampleManager::sampleAddressesAndIds_t res;
        findIf(v, "sampleAddresses", res);
        mgr.restoreFromSampleAddressesAndIDs(res);
    }
};
} // namespace scxt::json
#endif // SHORTCIRCUIT_SAMPLE_TRAITS_H
