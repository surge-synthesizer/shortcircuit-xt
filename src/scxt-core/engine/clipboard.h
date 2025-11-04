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

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_CLIPBOARD_H
#define SCXT_SRC_SCXT_CORE_ENGINE_CLIPBOARD_H

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
