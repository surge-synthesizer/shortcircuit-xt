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

#include "ThemeEditor.h"

#include "app/SCXTEditor.h"
#include "messaging/client/client_messages.h"
#include "theme/ColorMap.h"
#include "theme/ThemeApplier.h"

namespace scxt::ui::app::other_screens
{
namespace jcmp = sst::jucegui::components;
namespace jscr = sst::jucegui::screens;

namespace
{
// Colours whose RGB tracks a leader colour with a fixed alpha byte. Only the
// leader is shown in the editor; the derivatives are rewritten from it.
struct AlphaDeriv
{
    theme::ColorMap::Colors color;
    uint8_t alpha;
};
struct LeaderGroup
{
    theme::ColorMap::Colors leader;
    std::array<AlphaDeriv, 3> derived;
};
// These alphas match the wireframe alphas in figma.
constexpr std::array<LeaderGroup, 3> leaderGroups{{
    {theme::ColorMap::accent_1b,
     {{{theme::ColorMap::accent_1b_alpha_a, 0x14},
       {theme::ColorMap::accent_1b_alpha_b, 0x52},
       {theme::ColorMap::accent_1b_alpha_c, 0x80}}}},
    {theme::ColorMap::accent_2a,
     {{{theme::ColorMap::accent_2a_alpha_a, 0x14},
       {theme::ColorMap::accent_2a_alpha_b, 0x52},
       {theme::ColorMap::accent_2a_alpha_c, 0x80}}}},
    {theme::ColorMap::accent_2b,
     {{{theme::ColorMap::accent_2b_alpha_a, 0x14},
       {theme::ColorMap::accent_2b_alpha_b, 0x52},
       {theme::ColorMap::accent_2b_alpha_c, 0x80}}}},
}};

bool isDerivative(theme::ColorMap::Colors c)
{
    for (auto &g : leaderGroups)
        for (auto &d : g.derived)
            if (d.color == c)
                return true;
    return false;
}

void applyLeaderDerivatives(theme::ColorMap &cm, theme::ColorMap::Colors leader, juce::Colour col)
{
    for (auto &g : leaderGroups)
    {
        if (g.leader != leader)
            continue;
        for (auto &d : g.derived)
            cm.setColor(d.color, col.withAlpha(d.alpha));
        return;
    }
}
} // namespace

ThemeEditor::ThemeEditor(SCXTEditor *e) : HasEditor(e)
{
    loadButton = std::make_unique<jcmp::TextPushButton>();
    loadButton->setLabel("Load Theme");
    loadButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->editor->promptForLoadTheme();
    });
    addAndMakeVisible(*loadButton);

    saveButton = std::make_unique<jcmp::TextPushButton>();
    saveButton->setLabel("Save Theme");
    saveButton->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->editor->promptForSaveTheme();
    });
    addAndMakeVisible(*saveButton);

    rebuildFromThemeApplier();
}

ThemeEditor::~ThemeEditor() = default;

void ThemeEditor::rebuildFromThemeApplier()
{
    if (colorEditor)
    {
        removeChildComponent(colorEditor.get());
        colorEditor.reset();
    }

    const auto &cmap = editor->themeApplier.currentColorMap();

    std::vector<jscr::ColorEditor::ColorEntry> entries;
    entries.reserve(theme::ColorMap::lastColor + 1);
    for (int i = 0; i <= theme::ColorMap::lastColor; ++i)
    {
        auto c = (theme::ColorMap::Colors)i;
        if (isDerivative(c))
            continue;
        // getImpl rather than get so the editor shows the raw stored colour;
        // get() zeroes out knob_fill when hasKnobs is off, which would paint
        // the swatch transparent and hide whatever value the user last set.
        entries.push_back({theme::ColorMap::nameOf(c), cmap.getImpl(c)});
    }

    auto cb = [this, ed = juce::Component::SafePointer(editor)](const std::string &tag,
                                                                juce::Colour col) {
        if (!ed)
            return;
        auto &cm = ed->themeApplier.currentColorMap();
        for (int i = 0; i <= theme::ColorMap::lastColor; ++i)
        {
            auto c = (theme::ColorMap::Colors)i;
            if (theme::ColorMap::nameOf(c) == tag)
            {
                cm.setColor(c, col);
                applyLeaderDerivatives(cm, c, col);
                break;
            }
        }
        // First edit transitions us into the custom-map state; the menu reads
        // this to show a ticked "Custom Colormap" entry.
        cm.myId =
            static_cast<theme::ColorMap::BuiltInColorMaps>(theme::ColorMap::CUSTOM_COLORMAP_ID);
        ed->themeApplier.recolorStylesheet(ed->style());
        ed->setStyle(ed->style());
        startTimer(500);
    };

    colorEditor = std::make_unique<jscr::ColorEditor>(std::move(entries), std::move(cb), true);
    colorEditor->onAnyColorChanged = [ed = juce::Component::SafePointer(editor)]() {
        if (ed)
            ed->repaint();
    };
    addAndMakeVisible(*colorEditor);
    resized();
}

void ThemeEditor::timerCallback()
{
    stopTimer();
    const auto &cm = editor->themeApplier.currentColorMap();
    sendToSerialization(scxt::messaging::client::StoreColormap{cm.toJson()});
}

void ThemeEditor::resized()
{
    auto bh = 28;
    auto pad = 6;
    auto lb = getLocalBounds().reduced(pad);
    auto bottom = lb.removeFromBottom(bh);
    if (colorEditor)
        colorEditor->setBounds(lb.withTrimmedBottom(pad));

    auto buttonW = 120;
    auto right = bottom.withLeft(bottom.getRight() - buttonW);
    if (saveButton)
        saveButton->setBounds(right);
    auto left = right.translated(-buttonW - pad, 0);
    if (loadButton)
        loadButton->setBounds(left);
}

ThemeEditorWindow::ThemeEditorWindow(SCXTEditor *e)
    : juce::DocumentWindow("Edit Theme", juce::Colours::black, juce::DocumentWindow::closeButton)
{
    setUsingNativeTitleBar(true);
    setAlwaysOnTop(true); // those pesky daws
    content = std::make_unique<ThemeEditor>(e);
    content->setStyle(e->style());
    content->setSize(520, 700);
    setContentNonOwned(content.get(), true);
    setResizable(true, false);
    centreWithSize(getWidth(), getHeight());
}

ThemeEditorWindow::~ThemeEditorWindow() { clearContentComponent(); }

void ThemeEditorWindow::rebuildFromThemeApplier()
{
    if (content)
        content->rebuildFromThemeApplier();
}

} // namespace scxt::ui::app::other_screens
