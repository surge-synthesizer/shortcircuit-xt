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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MODPANE_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MODPANE_H

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
