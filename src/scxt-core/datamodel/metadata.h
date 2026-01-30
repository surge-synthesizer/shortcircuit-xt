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

#ifndef SCXT_SRC_SCXT_CORE_DATAMODEL_METADATA_H
#define SCXT_SRC_SCXT_CORE_DATAMODEL_METADATA_H

#include <string>
#include <optional>

#include "sst/basic-blocks/params/ParamMetadata.h"
#include "sst/basic-blocks/modulators/ADSREnvelope.h"

#include "datamodel/metadata_detail.h"

namespace scxt::datamodel
{
using pmd = sst::basic_blocks::params::ParamMetaData;

template <typename P, typename V> pmd describeValue(const P &p, const V &l)
{
    auto pd = detail::offV(p, l);
    return detail::descFor<P>(p, pd);
}
} // namespace scxt::datamodel

#endif // __SCXT_PARAMETER_H
