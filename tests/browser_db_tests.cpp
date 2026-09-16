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
#include <string>

#include "console_harness.h"
#include "engine/engine.h"
#include "browser/writer_worker.h"

namespace fs = std::filesystem;
using scxt::browser::WriterWorker;

namespace
{
fs::path missingDirectory()
{
    auto base = fs::temp_directory_path() / "scxt-browser-db-tests";
    fs::remove_all(base);
    return base / "not-there";
}

int browserDatabaseErrors(scxt::clients::console_ui::ConsoleHarness &th)
{
    int res{0};
    for (const auto &err : th.editor->readErrors())
        if (std::get<1>(err) == "Browser Database Error")
            res++;
    return res;
}
} // namespace

TEST_CASE("A browser database which cannot be opened reports an error", "[browser]")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    WriterWorker w(missingDirectory(), *th.engine->getMessageController());
    w.openForWrite();
    th.stepUI(30);

    REQUIRE(browserDatabaseErrors(th) == 1);
}

TEST_CASE("Repeated writes to an unopenable browser database report once", "[browser]")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    WriterWorker w(missingDirectory(), *th.engine->getMessageController());
    w.openForWrite();
    th.stepUI(30);

    for (int i = 0; i < 3; ++i)
    {
        w.enqueueWorkItem(new WriterWorker::EnQDebugMsg("write " + std::to_string(i)));
        th.stepUI(10);
    }
    th.stepUI(30);

    REQUIRE(browserDatabaseErrors(th) == 1);
}
