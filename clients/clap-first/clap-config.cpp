//
// Created by Paul Walker on 10/25/23.
//

#include "clap-config.h"
#include "clap/helpers/plugin.hxx"
#include "clap/helpers/host-proxy.hxx"

namespace sxcf = scxt::clap_first;
namespace chlp = clap::helpers;

template class chlp::Plugin<sxcf::misLevel, sxcf::checkLevel>;
template class chlp::HostProxy<sxcf::misLevel, sxcf::checkLevel>;
static_assert(std::is_same_v<sxcf::plugHelper_t, chlp::Plugin<sxcf::misLevel, sxcf::checkLevel>>);