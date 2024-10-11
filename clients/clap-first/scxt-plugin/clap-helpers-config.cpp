/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#include "clap-helpers-config.h"

#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hxx>

namespace sxcf = scxt::clap_first;
namespace chlp = clap::helpers;

template class chlp::Plugin<sxcf::misLevel, sxcf::checkLevel>;
template class chlp::HostProxy<sxcf::misLevel, sxcf::checkLevel>;
static_assert(std::is_same_v<sxcf::plugHelper_t, chlp::Plugin<sxcf::misLevel, sxcf::checkLevel>>);