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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MODPANE_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MODPANE_H

#include "sst/jucegui/components/NamedPanel.h"
#include "app/HasEditor.h"
#include "modulation/voice_matrix.h"
#include "modulation/group_matrix.h"

namespace scxt::ui::app::edit_screen
{

struct ModPaneZoneTraits
{
    static constexpr bool forZone{true};
    using metadata = scxt::voice::modulation::voiceMatrixMetadata_t;
    using matrix = scxt::voice::modulation::Matrix;
    using routing = scxt::voice::modulation::Matrix::RoutingTable;
    static constexpr uint32_t rowCount{scxt::voice::modulation::MatrixConfig::FixedMatrixSize};

    static int32_t defaultSmoothingTimeFor(const matrix::TR::SourceIdentifier &s)
    {
        auto res = matrix::TR::defaultLagFor(s);
        return res;
    }
};

struct ModPaneGroupTraits
{
    static constexpr bool forZone{false};
    using metadata = scxt::modulation::groupMatrixMetadata_t;
    using matrix = scxt::modulation::GroupMatrix;
    using routing = scxt::modulation::GroupMatrix::RoutingTable;
    static constexpr uint32_t rowCount{scxt::modulation::GroupMatrixConfig::FixedMatrixSize};
    static int32_t defaultSmoothingTimeFor(const matrix::TR::SourceIdentifier &s)
    {
        auto res = matrix::TR::defaultLagFor(s);
        return res;
    }
};

template <typename GZTrait> struct ModRow;

// We will explicity instantiate these on the trait in ModPane.cpp
template <typename GZTrait> struct ModPane : sst::jucegui::components::NamedPanel, HasEditor
{
    ModPane(SCXTEditor *e, bool forZone);
    ~ModPane();

    void resized() override;

    void rebuildMatrix(bool enableChanged = false); // entirely new components
    void refreshMatrix();                           // new routing table, no new components
    void setActive(bool b);

    std::array<std::unique_ptr<ModRow<GZTrait>>, GZTrait::rowCount> rows;

    typename GZTrait::metadata matrixMetadata;
    typename GZTrait::routing routingTable;

    bool forZone{GZTrait::forZone};
    int tabRange{0};

    std::unique_ptr<juce::Viewport> viewPort;
    std::unique_ptr<juce::Component> viewPortComponents;
};
} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUIT_MAPPINGPANE_H
