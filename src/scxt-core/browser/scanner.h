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

#ifndef SCXT_SRC_SCXT_CORE_BROWSER_SCANNER_H
#define SCXT_SRC_SCXT_CORE_BROWSER_SCANNER_H

#include "filesystem/import.h"

namespace scxt::messaging
{
struct MessageController;
}
namespace scxt::browser
{
struct WriterWorker;
struct ScanWorker;
struct Scanner
{
    Scanner(WriterWorker &w, messaging::MessageController &mc);
    ~Scanner();

    void scanPath(const fs::path &path, uint64_t afterMtime);

    WriterWorker &writer;
    messaging::MessageController &mc;
    std::unique_ptr<ScanWorker> scanWorker;
};
} // namespace scxt::browser
#endif // BROWSER_SCANNER_H
