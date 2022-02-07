//
// Created by Paul Walker on 2/7/22.
//

#ifndef SHORTCIRCUIT_SAMPLER_USERDEFAULTS_H
#define SHORTCIRCUIT_SAMPLER_USERDEFAULTS_H

#include "sst/plugininfra/userdefaults.h"

namespace scxt
{
namespace defaults
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

using Provider = sst::plugininfra::defaults::Provider<DefaultKeys, DefaultKeys::nKeys>;
} // namespace defaults
} // namespace scxt

#endif // SHORTCIRCUIT_SAMPLER_USERDEFAULTS_H
