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

#ifndef SCXT_TESTS_TEST_UTILS_H
#define SCXT_TESTS_TEST_UTILS_H

/*
 * Small helpers shared by more than one test file. They live in a header rather than being
 * repeated per .cpp because a unity build folds every test into one translation unit, where
 * duplicate definitions collide.
 */

#include <filesystem>
#include <string>

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

constexpr double TEST_SAMPLE_RATE = 48000.0;

inline std::filesystem::path samplePath(const std::string &relative)
{
    return std::filesystem::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / relative;
}

#endif // SCXT_TESTS_TEST_UTILS_H
