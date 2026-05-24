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

#ifndef SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_REPORT_H
#define SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_REPORT_H

#include <filesystem>

#include "perf_config.h"
#include "perf_loader.h"
#include "perf_runner.h"

namespace scxt::perf
{

void writeReport(const std::filesystem::path &out, const Config &cfg, const LoadResult &load,
                 const RunResult &run);

void writeWavFloat32(const std::filesystem::path &out, const std::vector<float> &interleaved,
                     double sampleRate);

void printConsoleSummary(const Config &cfg, const LoadResult &load, const RunResult &run);

} // namespace scxt::perf
#endif
