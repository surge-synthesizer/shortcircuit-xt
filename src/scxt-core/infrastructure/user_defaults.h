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

#ifndef SCXT_SRC_SCXT_CORE_INFRASTRUCTURE_USER_DEFAULTS_H
#define SCXT_SRC_SCXT_CORE_INFRASTRUCTURE_USER_DEFAULTS_H

#include "sst/plugininfra/userdefaults.h"

namespace scxt::infrastructure
{
enum DefaultKeys
{
    zoomLevel,
    octave0,
    invertScroll,
    showKnobs,
    colormapId,
    colormapPathIfFile,
    welcomeScreenSeen,
    playModeExpanded,
    partSidebarPartExpanded,
    browserAutoPreviewEnabled,
    browserPreviewAmplitude,
    useSoftwareRenderer,

    nKeys // must be last K?
};
inline std::string defaultKeyToString(DefaultKeys k)
{
    switch (k)
    {
    case zoomLevel:
        return "zoomLevel";
    case colormapId:
        return "colormapId";
    case colormapPathIfFile:
        return "colormapPathIfFile";
    case octave0:
        return "octave0";
    case nKeys:
        return "nKeys";
    case invertScroll:
        return "invertScroll";
    case showKnobs:
        return "showKnobs";
    case welcomeScreenSeen:
        return "welcomeScreenSeen";
    case playModeExpanded:
        return "playModeExpanded";
    case partSidebarPartExpanded:
        return "partSidebarPartExpanded";
    case browserAutoPreviewEnabled:
        return "browserAutoPreviewEnabled";
    case browserPreviewAmplitude:
        return "browserPreviewAmplitude";
    case useSoftwareRenderer:
        return "useSoftwareRenderer";
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
