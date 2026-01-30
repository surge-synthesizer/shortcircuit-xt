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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_PLAY_SCREEN_PLAYSCREEN_H
#define SCXT_SRC_SCXT_PLUGIN_APP_PLAY_SCREEN_PLAYSCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>
#include "configuration.h"
#include "app/HasEditor.h"

#include "sst/jucegui/components/Viewport.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/component-adapters/DiscreteToReference.h"
#include "sst/jucegui/util/VisibilityParentWatcher.h"
#include "app/shared/PartSidebarCard.h"
#include "app/shared/SingleMacroEditor.h"

namespace scxt::ui::app
{

namespace browser_ui
{
struct BrowserPane;
}
namespace play_screen
{
struct PlayScreen : juce::Component, HasEditor
{
    static constexpr int browserPanelWidth = 196; // copied from multi for now

    std::array<std::array<std::unique_ptr<shared::SingleMacroEditor>, scxt::macrosPerPart>,
               scxt::numParts>
        macroEditors;
    std::array<std::unique_ptr<shared::PartSidebarCard>, scxt::numParts> partSidebars;
    std::unique_ptr<sst::jucegui::components::Viewport> viewport;
    std::unique_ptr<juce::Component> viewportContents;

    using skinnyPage_t = sst::jucegui::component_adapters::DiscreteToValueReference<
        sst::jucegui::components::MultiSwitch, bool>;
    std::array<std::unique_ptr<skinnyPage_t>, scxt::numParts> skinnyPageSwitches;
    std::array<bool, scxt::numParts> skinnyPage{};

    std::unique_ptr<sst::jucegui::components::NamedPanel> playNamedPanel;
    std::unique_ptr<browser_ui::BrowserPane> browser;
    std::unique_ptr<sst::jucegui::util::VisibilityParentWatcher> visibilityWatcher;

    PlayScreen(SCXTEditor *e);
    ~PlayScreen();

    void resized() override;
    void visibilityChanged() override;
    void macroDataChanged(int p, int i) { macroEditors[p][i]->updateFromEditorData(); }

    void rebuildPositionsAndVisibilites();
    void showMenu();

    void partConfigurationChanged();

    bool tallMode{true};
    static constexpr size_t interPartMargin{3};
    juce::Rectangle<int> rectangleForPart(int part);

    static constexpr size_t sidebarWidth{180};
};
} // namespace play_screen
} // namespace scxt::ui::app
#endif // SHORTCIRCUITXT_PLAYSCREEN_H
