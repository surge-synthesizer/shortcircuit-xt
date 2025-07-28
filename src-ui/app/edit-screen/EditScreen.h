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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_EDITSCREEN_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_EDITSCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <variant>
#include "app/HasEditor.h"
#include "app/browser-ui/BrowserPane.h"
#include "components/PartGroupSidebar.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "selection/selection_manager.h"

namespace scxt::ui::app
{

namespace browser_ui
{
struct BrowserPane;
}
namespace edit_screen
{
struct AdsrPane;

struct OutPaneZoneTraits;
struct OutPaneGroupTraits;
template <typename T> struct OutputPane;

struct PartGroupSidebar;
struct MacroMappingVariantPane;

struct ModPaneZoneTraits;
struct ModPaneGroupTraits;
template <typename T> struct ModPane;
struct ProcessorPane;
struct LfoPane;

struct PartEditScreen;

struct EditScreen : juce::Component, HasEditor
{
    static constexpr int numProcessorDisplays{4};
    static constexpr int sideWidths = 196; // copied from mixer for now
    static constexpr int edgeDistance = 6;

    static constexpr int envHeight = 160, modHeight = 160, fxHeight = 176;
    static constexpr int pad = 0;

    std::unique_ptr<browser_ui::BrowserPane> browser;
    std::unique_ptr<MacroMappingVariantPane> mappingPane;
    std::unique_ptr<PartGroupSidebar> partSidebar;

    std::unique_ptr<PartEditScreen> partEditScreen;

    struct ZoneTraits
    {
        using OutPaneTraits = OutPaneZoneTraits;
        using ModPaneTraits = ModPaneZoneTraits;
        static constexpr bool forZone{true};
    };
    struct GroupTraits
    {
        using OutPaneTraits = OutPaneGroupTraits;
        using ModPaneTraits = ModPaneGroupTraits;
        static constexpr bool forZone{false};
    };

    template <typename ZGTrait> struct ZoneOrGroupElements
    {
        static constexpr bool forZone{ZGTrait::forZone};
        static constexpr bool forGroup{!ZGTrait::forZone};
        ZoneOrGroupElements(EditScreen *parent);
        ~ZoneOrGroupElements();
        std::unique_ptr<OutputPane<typename ZGTrait::OutPaneTraits>> outPane;
        std::unique_ptr<LfoPane> lfo;
        std::array<std::unique_ptr<AdsrPane>, 2> eg;

        std::unique_ptr<ModPane<typename ZGTrait::ModPaneTraits>> modPane;

        std::unique_ptr<ProcessorPane> processors[numProcessorDisplays];

        void setVisible(bool b);
        void layoutInto(const juce::Rectangle<int> &mainRect);
    };

    std::unique_ptr<ZoneOrGroupElements<ZoneTraits>> zoneElements;
    const std::unique_ptr<ZoneOrGroupElements<ZoneTraits>> &getZoneElements()
    {
        return zoneElements;
    }
    std::unique_ptr<ZoneOrGroupElements<GroupTraits>> groupElements;
    const std::unique_ptr<ZoneOrGroupElements<GroupTraits>> &getGroupElements()
    {
        return groupElements;
    }

    EditScreen(SCXTEditor *e);
    ~EditScreen();
    void resized() override { layout(); }

    void layout();
    std::map<selection::SelectionManager::ZoneAddress, size_t> voiceCountByZoneAddress;
    void onVoiceInfoChanged();
    void addSamplePlaybackPosition(size_t sampleIndex, int64_t samplePos);
    void clearSamplePlaybackPositions();

    void selectedPartChanged();
    void macroDataChanged(int part, int index);

    enum class SelectionMode
    {
        NONE,
        ZONE,
        GROUP,
        PART
    } selectionMode{SelectionMode::NONE};
    void setSelectionMode(SelectionMode m);
    void viewZone() { partSidebar->setSelectedTab(2); }
    void viewGroup() { partSidebar->setSelectedTab(1); }
    void viewPart() { partSidebar->setSelectedTab(0); }
    void swapGroupZone()
    {
        if (partSidebar->selectedTab == 2)
        {
            // zone to group
            partSidebar->setSelectedTab(1);
        }
        else
        {
            // alwyas to zone
            partSidebar->setSelectedTab(2);
        }
    }

    void onOtherTabSelection();
    // This allows us, in the future, to make this return s + selected part to have
    // part differentiated selection
    std::string tabKey(const std::string &s) { return s; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditScreen);
};
} // namespace edit_screen
} // namespace scxt::ui::app
#endif // SHORTCIRCUIT_MULTISCREEN_H
