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

#ifndef SCXT_SRC_MESSAGING_CLIENT_PATCH_IO_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_PATCH_IO_MESSAGES_H

#include "patch_io/patch_io.h"

namespace scxt::messaging::client
{
inline void doSaveMulti(const std::string &s, engine::Engine &engine, MessageController &cont)
{
    patch_io::saveMulti(fs::path{s}, engine);

    SCLOG("Remember to update the browser also");
    // engine.getBrowser()->doSomething;
}
CLIENT_TO_SERIAL(SaveMulti, c2s_save_multi, std::string, doSaveMulti(payload, engine, cont));

CLIENT_TO_SERIAL(LoadMulti, c2s_load_multi, std::string,
                 patch_io::loadMulti(fs::path{payload}, engine));
} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_PATCH_IO_MESSAGES_H
