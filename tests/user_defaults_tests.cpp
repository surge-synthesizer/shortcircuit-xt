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

#include "catch2/catch2.hpp"

#include <filesystem>
#include <memory>
#include <set>
#include <string>

#include "infrastructure/user_defaults.h"

namespace fs = std::filesystem;
using scxt::infrastructure::DefaultKeys;
using scxt::infrastructure::DefaultsProvider;

namespace
{
fs::path defaultsDir()
{
    auto dir = fs::temp_directory_path() / "scxt-user-defaults-tests";
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

std::unique_ptr<DefaultsProvider> makeProvider(const fs::path &dir)
{
    return std::make_unique<DefaultsProvider>(
        dir, "ShortcircuitXTTest",
        [](auto k) { return scxt::infrastructure::defaultKeyToString(k); },
        [](auto, auto) { FAIL("defaults error"); });
}
} // namespace

TEST_CASE("Every default key has its own name", "[defaults]")
{
    std::set<std::string> names;
    for (int k = 0; k < DefaultKeys::nKeys; ++k)
    {
        auto n = scxt::infrastructure::defaultKeyToString((DefaultKeys)k);
        INFO("key " << k << " is " << n);
        REQUIRE(!n.empty());
        REQUIRE(names.insert(n).second);
    }
}

TEST_CASE("The accessible workflow preference is off until set and survives a reload", "[defaults]")
{
    auto dir = defaultsDir();
    {
        auto dp = makeProvider(dir);
        REQUIRE(dp->getUserDefaultValue(DefaultKeys::useAccessibleWorkflow, 0) == 0);
        dp->updateUserDefaultValue(DefaultKeys::useAccessibleWorkflow, 1);
    }
    {
        auto dp = makeProvider(dir);
        REQUIRE(dp->getUserDefaultValue(DefaultKeys::useAccessibleWorkflow, 0) == 1);
        dp->updateUserDefaultValue(DefaultKeys::useAccessibleWorkflow, 0);
    }
    {
        auto dp = makeProvider(dir);
        REQUIRE(dp->getUserDefaultValue(DefaultKeys::useAccessibleWorkflow, 1) == 0);
    }
    fs::remove_all(dir);
}
