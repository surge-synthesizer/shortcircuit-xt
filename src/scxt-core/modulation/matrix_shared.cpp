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

// engine.h transitively pulls in matrix_shared.h together with all the prerequisites the header
// relies on (configuration, datamodel, <sstream>, ...); matrix_shared.h is not standalone.
#include "engine/engine.h"

namespace scxt::modulation::shared
{
ExplicitMenuOrder::ExplicitMenuOrder(engine::Engine *e) : engine(e)
{
    if (engine)
        engine->beginExplicitMenuOrder();
}
ExplicitMenuOrder::~ExplicitMenuOrder()
{
    if (engine)
        engine->endExplicitMenuOrder();
}
void ExplicitMenuOrder::separator()
{
    if (engine)
        engine->requestMenuSeparator();
}

SourcePolarity polarityOf(const sourcePolarityVector_t &pv, const SourceIdentifier &s)
{
    for (const auto &[si, p] : pv)
        if (si == s)
            return (SourcePolarity)p;
    return SourcePolarity::UNIPOLAR;
}

SourcePolarity routePolarity(SourcePolarity source, std::optional<SourcePolarity> via,
                             std::optional<ModulationCurves::CurveIdentifier> curve,
                             bool multiplicative)
{
    // the multiplicative path takes |offset|
    if (multiplicative)
        return SourcePolarity::UNIPOLAR;

    auto res = source;
    if (via.has_value())
        res = productPolarity(res, *via);
    if (curve.has_value())
        res = ModulationCurves::curvePolarity(*curve, res);
    return res;
}
} // namespace scxt::modulation::shared
