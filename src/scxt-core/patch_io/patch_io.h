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

#ifndef SCXT_SRC_SCXT_CORE_PATCH_IO_PATCH_IO_H
#define SCXT_SRC_SCXT_CORE_PATCH_IO_PATCH_IO_H

#include "engine/patch.h"
#include "engine/part.h"

namespace scxt::patch_io
{
enum SaveStyles
{
    NO_SAMPLES = 0,
    COLLECT_SAMPLES,
    AS_MONOLITH
};
bool saveMulti(const fs::path &toFile, scxt::engine::Engine &, SaveStyles saveStyle);
bool loadMulti(const fs::path &fromFile, scxt::engine::Engine &);
bool savePart(const fs::path &toFile, scxt::engine::Engine &, int part, SaveStyles saveStyles);
bool loadPartInto(const fs::path &fromFile, scxt::engine::Engine &, int part);

bool initFromResourceBundle(scxt::engine::Engine &e, const std::string &file);

struct SCMonolithSampleReader
{
    SCMonolithSampleReader(RIFF::File *f);
    ~SCMonolithSampleReader();

    size_t getSampleCount();

    struct SampleData
    {
        std::string filename;
        std::vector<uint8_t> data;
    };
    // Lets avoid copying around that data vector; return bool and populate the ref
    bool getSampleData(size_t index, SampleData &data);

  private:
    RIFF::File *file;
    int version;
    size_t cacheSampleCount{0};
    bool cacheSampleCountDone{false};
};
} // namespace scxt::patch_io

#endif // SHORTCIRCUITXT_PATCH_IO_H
