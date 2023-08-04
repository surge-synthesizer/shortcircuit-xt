/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_UI_COMPONENTS_MULTISCREEN_H
#define SCXT_SRC_UI_COMPONENTS_MULTISCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <variant>
#include "HasEditor.h"
#include "browser/BrowserPane.h"
#include "sst/jucegui/components/NamedPanel.h"

namespace scxt::ui
{
namespace multi
{
struct AdsrPane;

struct OutPaneZoneTraits;
struct OutPaneGroupTraits;
template <typename T> struct OutputPane;

struct PartGroupSidebar;
struct MappingPane;

struct ModPaneZoneTraits;
struct ModPaneGroupTraits;
template <typename T> struct ModPane;
struct ProcessorPane;
struct LfoPane;
} // namespace multi

namespace browser
{
struct BrowserPane;
}

struct MultiScreen : juce::Component, HasEditor
{
    static constexpr int numProcessorDisplays{4};
    static constexpr int sideWidths = 196; // copied from mixer for now
    static constexpr int edgeDistance = 6;

    static constexpr int envHeight = 160, modHeight = 160, fxHeight = 176;
    static constexpr int pad = 0;

    std::unique_ptr<browser::BrowserPane> browser;
    std::unique_ptr<multi::MappingPane> sample;
    std::unique_ptr<multi::PartGroupSidebar> parts;

    enum class ZoneGroupIndex
    {
        ZONE = 0,
        GROUP = 1
    };
    struct ZoneOrGroupElements
    {
        ZoneGroupIndex index;
        ZoneOrGroupElements(ZoneGroupIndex z);
        ~ZoneOrGroupElements();
        std::variant<std::unique_ptr<multi::OutputPane<multi::OutPaneZoneTraits>>,
                     std::unique_ptr<multi::OutputPane<multi::OutPaneGroupTraits>>>
            outputvariant;
        std::unique_ptr<multi::LfoPane> lfo;
        std::unique_ptr<multi::AdsrPane> eg[2];
        std::variant<std::unique_ptr<multi::ModPane<multi::ModPaneZoneTraits>>,
                     std::unique_ptr<multi::ModPane<multi::ModPaneGroupTraits>>>
            modvariant;

        juce::Component *modComponent();
        juce::Component *outputComponent();
        const std::unique_ptr<multi::ModPane<multi::ModPaneZoneTraits>> &zoneMod()
        {
            assert(index == ZoneGroupIndex::ZONE);
            return std::get<0>(modvariant);
        }

        const std::unique_ptr<multi::ModPane<multi::ModPaneGroupTraits>> &groupMod()
        {
            assert(index == ZoneGroupIndex::GROUP);
            return std::get<1>(modvariant);
        }

        const std::unique_ptr<multi::OutputPane<multi::OutPaneZoneTraits>> &zoneOut()
        {
            assert(index == ZoneGroupIndex::ZONE);
            return std::get<0>(outputvariant);
        }

        const std::unique_ptr<multi::OutputPane<multi::OutPaneGroupTraits>> &groupOut()
        {
            assert(index == ZoneGroupIndex::GROUP);
            return std::get<1>(outputvariant);
        }

        std::unique_ptr<multi::ProcessorPane> processors[numProcessorDisplays];

        void setVisible(bool b);
    };

    std::unique_ptr<ZoneOrGroupElements> zoneGroupElements[2];
    const std::unique_ptr<ZoneOrGroupElements> &getZoneElements()
    {
        return zoneGroupElements[(int)ZoneGroupIndex::ZONE];
    }
    const std::unique_ptr<ZoneOrGroupElements> &getGroupElements()
    {
        return zoneGroupElements[(int)ZoneGroupIndex::GROUP];
    }

    MultiScreen(SCXTEditor *e);
    ~MultiScreen();
    void resized() override { layout(); }

    void layout();
    void onVoiceInfoChanged();

    enum class SelectionMode
    {
        NONE,
        ZONE,
        GROUP
    } selectionMode{SelectionMode::NONE};
    void setSelectionMode(SelectionMode m);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiScreen);
};
} // namespace scxt::ui
#endif // SHORTCIRCUIT_MULTISCREEN_H
