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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_PATCH_IO_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_PATCH_IO_MESSAGES_H

#include "patch_io/patch_io.h"
#include "sample/sfz_support/sfz_export.h"

namespace scxt::messaging::client
{
using saveMultiPayload_t = std::tuple<std::string, int>; // path, style
inline void doSaveMulti(const saveMultiPayload_t &pl, engine::Engine &engine,
                        MessageController &cont)
{
    auto [s, styleInt] = pl;
    auto style = (patch_io::SaveStyles)styleInt;
    patch_io::saveMulti(fs::path(fs::u8path(s)), engine, style);

    // engine.getBrowser()->doSomething;
}
CLIENT_TO_SERIAL(SaveMulti, c2s_save_multi, saveMultiPayload_t, doSaveMulti(payload, engine, cont));
CLIENT_TO_SERIAL(LoadMulti, c2s_load_multi, std::string,
                 patch_io::loadMulti(fs::path(fs::u8path(payload)), engine));

using savePartPayload_t = std::tuple<std::string, int, int>; // path, part (-1 for sel), style

inline void doSavePart(const savePartPayload_t &pl, engine::Engine &engine, MessageController &cont)
{
    auto [s, part, styleInt] = pl;
    auto style = (patch_io::SaveStyles)styleInt;

    if (part < 0)
        part = engine.getSelectionManager()->selectedPart;

    if (style == patch_io::SaveStyles::AS_SFZ)
    {
        sfz_support::exportSFZ(fs::path(fs::u8path(s)), engine, part);
    }
    else
    {
        patch_io::savePart(fs::path(fs::u8path(s)), engine, part, style);
    }
    // engine.getBrowser()->doSomething;
}
CLIENT_TO_SERIAL(SavePart, c2s_save_part, savePartPayload_t, doSavePart(payload, engine, cont));

using loadPartIntoPayload_t = std::tuple<std::string, int16_t>;
inline void doLoadPartInto(const loadPartIntoPayload_t &payload, engine::Engine &engine,
                           MessageController &cont)
{
    patch_io::loadPartInto(fs::path(fs::u8path(std::get<0>(payload))), engine,
                           std::get<1>(payload));
}
CLIENT_TO_SERIAL(LoadPartInto, c2s_load_part_into, loadPartIntoPayload_t,
                 doLoadPartInto(payload, engine, cont));
} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_PATCH_IO_MESSAGES_H
