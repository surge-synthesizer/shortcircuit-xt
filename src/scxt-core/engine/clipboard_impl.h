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

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_CLIPBOARD_IMPL_H
#define SCXT_SRC_SCXT_CORE_ENGINE_CLIPBOARD_IMPL_H

#include "clipboard.h"
#include "json/scxt_traits.h"

namespace scxt::engine
{
template <typename T> Clipboard::ContentType Clipboard::streamToClipboard(ContentType c, const T &t)
{
    auto v = json::scxt_value(t);
    type = c;
    contents = tao::json::msgpack::to_string(v);
    return type;
}

template <typename T> bool Clipboard::unstreamFromClipboard(ContentType c, T &t)
{
    if (c != type)
        return false;

    tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>> consumer;
    tao::json::msgpack::events::from_string(consumer, contents);
    auto v = std::move(consumer.value);
    v.to(t);

    return true;
}

} // namespace scxt::engine
#endif // CLIPBOARD_IMPL_H
