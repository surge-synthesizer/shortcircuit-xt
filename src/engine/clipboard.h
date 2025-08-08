//
// Created by Paul Walker on 8/8/25.
//

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <string>

/*
 * Why is 'Clipboard' in 'engine' since it seems like a UI thing? Well the maintenance
 * of what is copied and pasted is held server side (since it involves serializing and
 * unserializing engine state) so for now we keep it here.
 *
 * In the future we may blast a message to the UI for system clipboard integration.
 */

namespace scxt::engine
{
struct Clipboard
{
    enum ContentType
    {
        NONE,
        ZONE // add after this and remember to extend inverse below
    };

    template <typename T> [[nodiscard]] ContentType streamToClipboard(ContentType c, const T &t);

    template <typename T> [[nodiscard]] bool unstreamFromClipboard(ContentType c, T &t);

    ContentType getClipboardType() const { return type; }
    std::string getClipboardContents() const { return contents; }

    static std::string toStringContentType(const ContentType &c)
    {
        switch (c)
        {
        case NONE:
            return "n";
        case ZONE:
            return "z";
        }
        return "";
    }
    static ContentType fromStringContentType(const std::string &s)
    {
        static auto inverse =
            makeEnumInverse<ContentType, toStringContentType>(ContentType::NONE, ContentType::ZONE);
        auto p = inverse.find(s);
        if (p == inverse.end())
            return NONE;
        return p->second;
    }

  protected:
    ContentType type;
    std::string contents;
};
} // namespace scxt::engine
#endif // CLIPBOARD_H
