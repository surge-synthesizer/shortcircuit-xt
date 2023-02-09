/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_MESSAGING_CLIENT_DETAIL_CLIENT_JSON_DETAILS_H
#define SCXT_SRC_MESSAGING_CLIENT_DETAIL_CLIENT_JSON_DETAILS_H

#include "json/scxt_traits.h"

namespace scxt::messaging::client::detail
{
template <typename T> struct client_message_traits : public scxt::json::scxt_traits<T>
{
};
using client_message_value = tao::json::basic_value<client_message_traits>;
} // namespace scxt::messaging::client::detail
#endif // SHORTCIRCUIT_CLIENT_JSON_DETAILS_H
