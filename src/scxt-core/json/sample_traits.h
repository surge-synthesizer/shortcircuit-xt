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

#ifndef SCXT_SRC_SCXT_CORE_JSON_SAMPLE_TRAITS_H
#define SCXT_SRC_SCXT_CORE_JSON_SAMPLE_TRAITS_H

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "stream.h"

#include "sample/sample_manager.h"
#include "sample/compound_file.h"
#include "browser/browser.h"
#include "scxt_traits.h"

namespace scxt::json
{
// This was written before SC_STREAM_ENUM and i didn't want to
// break streaming at the ALPHA point which is why it is a bit odd.
SC_STREAMDEF(scxt::sample::Sample::SourceType, SC_FROM({
                 static constexpr const char *key = "sourceType";
                 /* Remember this is a bit funky - hand unstream below too */
                 switch (from)
                 {
                 case sample::Sample::WAV_FILE:
                     v = "wav";
                     break;
                 case sample::Sample::FLAC_FILE:
                     v = "flac";
                     break;
                 case sample::Sample::MP3_FILE:
                     v = "mp3";
                     break;
                 case sample::Sample::SF2_FILE:
                     v = "sf2";
                     break;
                 case sample::Sample::SFZ_FILE:
                     v = "sfz";
                     break;
                 case sample::Sample::AIFF_FILE:
                     v = "aiff";
                     break;
                 case sample::Sample::MULTISAMPLE_FILE:
                     v = "multisample";
                     break;
                 case sample::Sample::GIG_FILE:
                     v = "gig";
                     break;
                 case sample::Sample::SCXT_FILE:
                     v = "scxt";
                     break;
                 }
             }),
             SC_TO({
                 if (SC_UNSTREAMING_FROM_PRIOR_TO(0x2024'08'16))
                 {
                     static constexpr const char *key = "sourceType";
                     std::string k{};
                     findIf(v, key, k);

                     if (k == "wav_file")
                         to = sample::Sample::WAV_FILE;
                     if (k == "sf2_file")
                         to = sample::Sample::SF2_FILE;
                     if (k == "flac_file")
                         to = sample::Sample::FLAC_FILE;
                     if (k == "mp3_file")
                         to = sample::Sample::MP3_FILE;
                     if (k == "aiff_file")
                         to = sample::Sample::AIFF_FILE;
                     if (k == "multisample_file")
                         to = sample::Sample::MULTISAMPLE_FILE;
                 }
                 else
                 {
                     if (!v.is_string())
                     {
                         throw std::logic_error("Unstreamed non-string");
                     }
                     auto k = v.get_string();
                     if (k == "wav")
                         to = sample::Sample::WAV_FILE;
                     if (k == "sf2")
                         to = sample::Sample::SF2_FILE;
                     if (k == "flac")
                         to = sample::Sample::FLAC_FILE;
                     if (k == "mp3")
                         to = sample::Sample::MP3_FILE;
                     if (k == "aiff")
                         to = sample::Sample::AIFF_FILE;
                     if (k == "multisample")
                         to = sample::Sample::MULTISAMPLE_FILE;
                     if (k == "gig")
                         to = sample::Sample::GIG_FILE;
                     if (k == "scxt")
                         to = sample::Sample::SCXT_FILE;

                     // SCLOG("Unknown unstream type for format : " << k);
                 }

                 return;
             }));

SC_STREAMDEF(sample::Sample::SampleFileAddress, SC_FROM({
                 auto ppref = from.path;
                 // When you come back to this code for relative paths and stuff
                 // seriously consider streaming as preferred for windows \ to /
                 // but then go test all the libgig and xml code and stuff
                 // ppref = ppref.make_preferred();
                 v = {{"type", from.type}, {"path", ppref.u8string()}, {"md5sum", from.md5sum}};
                 if (from.type == sample::Sample::SF2_FILE ||
                     from.type == sample::Sample::MULTISAMPLE_FILE ||
                     from.type == sample::Sample::GIG_FILE ||
                     from.type == sample::Sample::SCXT_FILE)
                 {
                     addToObject<val_t>(v, "preset", from.preset);
                     addToObject<val_t>(v, "instrument", from.instrument);
                     addToObject<val_t>(v, "region", from.region);
                 }
             }),
             SC_TO({
                 findOrSet(v, "type", sample::Sample::WAV_FILE, to.type);
                 std::string p;
                 findIf(v, "path", p);

                 to.path = unstreamPathFromString(p);
                 findIf(v, "md5sum", to.md5sum);
                 findOrSet(v, "preset", 0, to.preset); // 0 here since we forgot to stream for a bit
                 findOrSet(v, "instrument", -1, to.instrument);
                 findOrSet(v, "region", -1, to.region);
             }));

SC_STREAMDEF(scxt::sample::SampleManager, SC_FROM({
                 v = {{"sampleAddresses", from.getSampleAddressesAndIDs()}};
             }),
             SC_TO({
                 to.reset();
                 sample::SampleManager::sampleAddressesAndIds_t res;
                 findIf(v, "sampleAddresses", res);
                 to.restoreFromSampleAddressesAndIDs(res);
             }));

SC_STREAMDEF(
    scxt::sample::compound::CompoundElement, SC_FROM({
        v = {{"type", (int32_t)from.type}, {"name", from.name}, {"addr", from.sampleAddress}};
    }),
    SC_TO({
        int t;
        findOrSet(v, "type", 0, t);
        to.type = (scxt::sample::compound::CompoundElement::Type)t;
        findIf(v, "name", to.name);
        findIf(v, "addr", to.sampleAddress);
    }));
} // namespace scxt::json
#endif // SHORTCIRCUIT_SAMPLE_TRAITS_H
