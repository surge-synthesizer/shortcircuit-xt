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

#ifndef SCXT_SRC_UI_COMPONENTS_WIDGETS_TOOLTIP_H
#define SCXT_SRC_UI_COMPONENTS_WIDGETS_TOOLTIP_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <string>

#include <sst/jucegui/components/BaseStyles.h>
#include <sst/jucegui/style/StyleAndSettingsConsumer.h>

namespace scxt::ui::widgets
{
struct Tooltip : juce::Component, sst::jucegui::style::StyleConsumer
{
    struct Styles : sst::jucegui::components::ControlStyles
    {
        using sclass = sst::jucegui::style::StyleSheet::Class;
        using sprop = sst::jucegui::style::StyleSheet::Property;
        static constexpr sclass styleClass{"tooltip"};

        static constexpr sprop bordercol{"tooltipborder.color"};
        static constexpr sprop datafont{"tooltipdata.font",
                                        sst::jucegui::style::StyleSheet::Property::FONT};

        static void initialize()
        {
            sst::jucegui::style::StyleSheet::addClass(styleClass)
                .withBaseClass(sst::jucegui::components::ControlStyles::styleClass)
                .withProperty(bordercol)
                .withProperty(datafont);
        }
    };

    Tooltip();
    void paint(juce::Graphics &g);

    void setTooltipTitleAndData(const std::string &t, const std::vector<std::string> &d)
    {
        tooltipTitle = t;
        tooltipData = d;
        resetSizeFromData();
        repaint();
    }

    void resetSizeFromData();
    std::string tooltipTitle{};
    std::vector<std::string> tooltipData;
};
} // namespace scxt::ui::widgets

#endif // SHORTCIRCUITXT_TOOLTIP_H
