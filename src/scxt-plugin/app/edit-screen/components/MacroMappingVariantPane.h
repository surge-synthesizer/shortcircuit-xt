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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MACROMAPPINGVARIANTPANE_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MACROMAPPINGVARIANTPANE_H

#include "sst/jucegui/components/NamedPanel.h"
#include "app/HasEditor.h"
#include "engine/zone.h"
#include "engine/part.h"
#include "selection/selection_manager.h"

namespace scxt::ui::app::edit_screen
{
struct MappingDisplay;
struct VariantDisplay;
struct MacroDisplay;

struct MacroMappingVariantPane : sst::jucegui::components::NamedPanel, HasEditor
{
    MacroMappingVariantPane(SCXTEditor *e);
    ~MacroMappingVariantPane();

    void resized() override;

    void setMappingData(const engine::Zone::ZoneMappingData &);
    void setSampleData(const engine::Zone::Variants &);
    void setGroupZoneMappingSummary(const engine::Part::zoneMappingSummary_t &);
    void selectedPartChanged();
    void macroDataChanged(int part, int index);
    void editorSelectionChanged();
    void setActive(bool b);
    void setSelectedTab(int i);

    std::unique_ptr<MappingDisplay> mappingDisplay;
    std::unique_ptr<VariantDisplay> sampleDisplay;
    std::unique_ptr<MacroDisplay> macroDisplay;

    engine::Zone::ZoneMappingData mappingView;
    engine::Zone::Variants sampleView;

    void addSamplePlaybackPosition(size_t sampleIndex, int64_t samplePos);
    void clearSamplePlaybackPositions();

    void invertScroll(bool invert);
    bool invertScroll() const;

    void showHamburgerMenu();
};
} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUIT_MAPPINGPANE_H
