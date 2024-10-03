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

#include "GroupTriggersCard.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/MenuButton.h"

#include "app/SCXTEditor.h"

namespace scxt::ui::app::edit_screen
{
namespace jcmp = sst::jucegui::components;

struct GroupTriggersCard::ConditionRow : juce::Component, HasEditor
{
    int index{-1};
    bool withCondition{false};
    ConditionRow(int index, bool withCond, SCXTEditor *ed)
        : index(index), withCondition(withCond), HasEditor(ed)
    {
        activeB = std::make_unique<jcmp::ToggleButton>();
        activeB->setLabel(std::to_string(index + 1));
        addAndMakeVisible(*activeB);

        auto mkm = [this](auto tx, auto cs) {
            auto res = std::make_unique<jcmp::MenuButton>();
            res->setLabel(tx);
            res->setOnCallback(
                editor->makeComingSoon(std::string() + "Trigger Condition Pane " + cs));
            addAndMakeVisible(*res);
            return res;
        };
        typeM = mkm("TYPE", "Trigger Mode");
        a1M = mkm("A1", "Argument One");
        a2M = mkm("A2", "Argument Two");
        if (withCond)
            cM = mkm("&", "Conjunction");
    }

    void resized() override
    {
        auto rb = getLocalBounds().withHeight(16);
        auto tb = rb.withWidth(16);
        activeB->setBounds(tb);
        tb = tb.translated(tb.getWidth() + 2, 0).withWidth(72);
        typeM->setBounds(tb);
        tb = tb.translated(tb.getWidth() + 2, 0).withWidth(32);
        a1M->setBounds(tb);
        tb = tb.translated(tb.getWidth() + 2, 0).withWidth(32);
        a2M->setBounds(tb);

        if (cM)
        {
            tb = tb.translated(tb.getWidth() + 2, 0).withWidth(16);
            cM->setBounds(tb);
        }
    }

    std::unique_ptr<jcmp::ToggleButton> activeB;
    std::unique_ptr<jcmp::MenuButton> typeM, a1M, a2M, cM;
};
GroupTriggersCard::GroupTriggersCard(SCXTEditor *e) : HasEditor(e)
{
    for (int i = 0; i < scxt::triggerConditionsPerGroup; ++i)
    {
        rows[i] =
            std::make_unique<ConditionRow>(i, i != scxt::triggerConditionsPerGroup - 1, editor);
        addAndMakeVisible(*rows[i]);
    }
}
GroupTriggersCard::~GroupTriggersCard() = default;

void GroupTriggersCard::resized()
{
    int rowHeight = 24;
    int componentHeight = 16;
    auto b = getLocalBounds().withTrimmedTop(rowHeight - 4);

    auto r = b.withHeight(componentHeight);
    for (int i = 0; i < scxt::triggerConditionsPerGroup; ++i)
    {
        rows[i]->setBounds(r);
        r = r.translated(0, rowHeight);
    }
}

void GroupTriggersCard::paint(juce::Graphics &g)
{
    auto ft = editor->themeApplier.interMediumFor(13);
    auto co = editor->themeColor(theme::ColorMap::generic_content_low);
    g.setFont(ft);
    g.setColour(co);
    g.drawText("TRIGGER CONDITIONS", getLocalBounds(), juce::Justification::topLeft);
}

} // namespace scxt::ui::app::edit_screen