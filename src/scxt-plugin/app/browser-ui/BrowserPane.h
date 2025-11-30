/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_BROWSER_UI_BROWSERPANE_H
#define SCXT_SRC_SCXT_PLUGIN_APP_BROWSER_UI_BROWSERPANE_H

#include <vector>
#include "filesystem/import.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/NamedPanel.h>
#include "app/HasEditor.h"
#include "browser/browser.h"

#include "sst/jucegui/components/ToggleButtonRadioGroup.h"

namespace scxt::ui::app::browser_ui // a bit clumsy to distinguis from scxt::browser
{
struct DevicesPane;
struct FavoritesPane;
struct SearchPane;
struct BrowserPaneFooter;
struct BrowserPane : public app::HasEditor, sst::jucegui::components::NamedPanel
{
    std::unique_ptr<sst::jucegui::components::ToggleButtonRadioGroup> selectedFunction;
    std::unique_ptr<sst::jucegui::data::Discrete> selectedFunctionData;

    BrowserPane(SCXTEditor *e);
    ~BrowserPane();
    void resized() override;

    void resetRoots();
    void setIndexWorkload(std::pair<int32_t, int32_t> idx);
    int32_t fJobs{0}, dbJobs{0};

    std::vector<scxt::browser::Browser::indexedRootPath_t> roots;

    std::unique_ptr<DevicesPane> devicesPane;
    std::unique_ptr<FavoritesPane> favoritesPane;
    std::unique_ptr<SearchPane> searchPane;
    std::unique_ptr<BrowserPaneFooter> footerArea;

    void selectPane(int, bool updatePrefs = false);
    int selectedPane{0};

    int lastClickedPotentialSample{-1};
    float previewAmplitude{1.f};
    bool autoPreviewEnabled{true};
};
} // namespace scxt::ui::app::browser_ui

#endif // SHORTCIRCUITXT_BROWSERPANE_H
