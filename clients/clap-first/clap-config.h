//
// Created by Paul Walker on 10/25/23.
//

#ifndef SHORTCIRCUITXT_CLAP_CONFIG_H
#define SHORTCIRCUITXT_CLAP_CONFIG_H

#include <clap/helpers/plugin.hh>

namespace scxt::clap_first
{

static constexpr clap::helpers::MisbehaviourHandler misLevel =
    clap::helpers::MisbehaviourHandler::Ignore;
static constexpr clap::helpers::CheckingLevel checkLevel = clap::helpers::CheckingLevel::Maximal;

using plugHelper_t = clap::helpers::Plugin<misLevel, checkLevel>;

} // namespace scxt::clap_first
#endif // SHORTCIRCUITXT_CLAP_CONFIG_H
