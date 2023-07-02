/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_INFRASTRUCTURE_USER_DEFAULTS_H
#define SCXT_SRC_INFRASTRUCTURE_USER_DEFAULTS_H

#include "sst/plugininfra/userdefaults.h"

namespace scxt::infrastructure
{
enum DefaultKeys
{
    zoomLevel,
    skinName,
    octave0,
    nKeys
};
inline std::string defaultKeyToString(DefaultKeys k)
{
    switch (k)
    {
    case zoomLevel:
        return "zoomLevel";
    case skinName:
        return "skinName";
    case octave0:
        return "octave0";
    case nKeys:
        return "nKeys";
    default:
        std::terminate(); // for now
    }
    return "ERROR";
}

struct DefaultsProvider : sst::plugininfra::defaults::Provider<DefaultKeys, DefaultKeys::nKeys>
{
    DefaultsProvider(const fs::path &defaultsDirectory, const std::string &productName,
                     const std::function<std::string(DefaultKeys)> &e2S,
                     const std::function<void(const std::string &, const std::string &)> &errHandle)
        : sst::plugininfra::defaults::Provider<DefaultKeys, DefaultKeys::nKeys>(
              defaultsDirectory, productName, e2S, errHandle)
    {
    }
};
} // namespace scxt::infrastructure

#endif // SHORTCIRCUITXT_USER_DEFAULTS_H
