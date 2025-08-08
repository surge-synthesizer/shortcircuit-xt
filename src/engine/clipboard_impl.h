//
// Created by Paul Walker on 8/8/25.
//

#ifndef CLIPBOARD_IMPL_H
#define CLIPBOARD_IMPL_H

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
