//
// Created by Paul Walker on 5/5/23.
//

#ifndef SHORTCIRCUITXT_USER_DEFAULTS_H
#define SHORTCIRCUITXT_USER_DEFAULTS_H

#include "sst/plugininfra/userdefaults.h"

namespace scxt::infrastructure
{
enum DefaultKeys
{
    zoomLevel,
    nKeys
};
inline std::string defaultKeyToString(DefaultKeys k)
{
    switch (k)
    {
    case zoomLevel:
        return "zoomLevel";
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
