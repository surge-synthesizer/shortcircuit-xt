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
    AS_MONOLITH,

    AS_SFZ // implies collect samples
};

std::optional<std::pair<std::string, std::string>> retrieveSCManifestAndPayload(const fs::path &);

bool saveMulti(const fs::path &toFile, scxt::engine::Engine &, SaveStyles saveStyle);
bool loadMulti(const fs::path &fromFile, scxt::engine::Engine &);
bool savePart(const fs::path &toFile, scxt::engine::Engine &, int part, SaveStyles saveStyles);
bool loadPartInto(const fs::path &fromFile, scxt::engine::Engine &, int part);

bool initFromResourceBundle(scxt::engine::Engine &e, const std::string &file);

// use -1 for all parts
std::unordered_map<SampleID, fs::path> collectSamplesInto(const fs::path &collectDir,
                                                          const scxt::engine::Engine &e, int part);

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

    void resetErrorString() { errStack.clear(); }
    void addError(const std::string &msg) { errStack += msg + "\n"; }
    [[nodiscard]] std::string getErrorString() const { return errStack; }

  private:
    RIFF::File *file;
    int version;
    size_t cacheSampleCount{0};
    bool cacheSampleCountDone{false};
    std::string errStack;
};
} // namespace scxt::patch_io

#endif // SHORTCIRCUITXT_PATCH_IO_H
