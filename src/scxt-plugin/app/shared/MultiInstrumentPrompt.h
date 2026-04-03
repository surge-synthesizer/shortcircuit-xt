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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_SHARED_MULTIINSTRUMENTPROMPT_H
#define SCXT_SRC_SCXT_PLUGIN_APP_SHARED_MULTIINSTRUMENTPROMPT_H

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/screens/ModalBase.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/Label.h"
#include "sample/compound_file.h"
#include "app/SCXTEditor.h"

namespace scxt::ui::app::shared
{
namespace jcmp = sst::jucegui::components;

// Modal dialog for selecting a specific instrument from a multi-instrument file (e.g. SF2).
// The onSelected callback is called with the chosen CompoundElement when the user clicks OK.
struct InstrumentPrompt : sst::jucegui::screens::ModalBase
{
    std::vector<sample::compound::CompoundElement> instruments;
    std::unique_ptr<jcmp::TextPushButton> ok, cancel;
    std::unique_ptr<jcmp::MenuButton> multiInst;
    int selInstrument{0};
    std::function<void(const sample::compound::CompoundElement &)> onSelected;

    InstrumentPrompt(const std::vector<sample::compound::CompoundElement> &i,
                     std::function<void(const sample::compound::CompoundElement &)> cb)
        : instruments(i), onSelected(std::move(cb))
    {
        ok = std::make_unique<jcmp::TextPushButton>();
        ok->setLabel("OK");
        ok->setOnCallback([this]() {
            if (onSelected)
                onSelected(instruments[selInstrument]);
            setVisible(false);
        });
        cancel = std::make_unique<jcmp::TextPushButton>();
        cancel->setLabel("Cancel");
        cancel->setOnCallback([this]() { setVisible(false); });

        selInstrument = 0;
        multiInst = std::make_unique<jcmp::MenuButton>();
        multiInst->setLabel(instruments[0].name);
        multiInst->setOnCallback([this]() { showMenu(); });
        addAndMakeVisible(*ok);
        addAndMakeVisible(*cancel);
        addAndMakeVisible(*multiInst);
    }

    void showMenu()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Select Instrument");
        p.addSeparator();
        int idx{0};
        for (auto &i : instruments)
        {
            p.addItem(instruments[idx].name, [this, ci = idx]() {
                selInstrument = ci;
                multiInst->setLabel(instruments[ci].name);
            });
            idx++;
        }
        p.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(multiInst.get()));
    }

    static constexpr int titleSize{18}, margin{4}, buttonWidth{80};

    void paintContents(juce::Graphics &g) override
    {
        auto p = getContentArea().reduced(4);

        g.setColour(getColour(Styles::brightoutline));
        g.drawHorizontalLine(p.getY() + titleSize + margin, p.getX(), p.getRight());
        g.setFont(
            style()->getFont(jcmp::Label::Styles::styleClass, jcmp::Label::Styles::labelfont));
        g.setColour(
            style()->getColour(jcmp::Label::Styles::styleClass, jcmp::Label::Styles::labelcolor));
        g.drawText("Select Instrument", p.withHeight(titleSize), juce::Justification::centredLeft);
    }

    juce::Point<int> innerContentSize() override { return {300, 150}; }

    void resized() override
    {
        ModalBase::resized();
        auto p = getContentArea().reduced(4);

        auto buttonBox = p.withTrimmedTop(p.getHeight() - titleSize)
                             .withWidth(buttonWidth)
                             .translated(p.getWidth() - buttonWidth, 0);

        multiInst->setBounds(
            p.withHeight(titleSize + margin).translated(0, titleSize * 2).reduced(10, 0));
        if (cancel)
        {
            cancel->setBounds(buttonBox);
            buttonBox = buttonBox.translated(-buttonWidth - margin, 0);
        }
        if (ok)
        {
            ok->setBounds(buttonBox);
        }
    }
};

// Show the multi-instrument selection prompt, or an error dialog for ERROR_SENTINEL results.
// onSelected is called with the chosen element only if the user clicks OK.
inline void
promptForMultiInstrument(SCXTEditor *editor,
                         const std::vector<sample::compound::CompoundElement> &inst,
                         std::function<void(const sample::compound::CompoundElement &)> onSelected)
{
    if (inst.size() == 1 && inst[0].type == sample::compound::CompoundElement::ERROR_SENTINEL)
    {
        editor->displayError(inst[0].name, inst[0].emsg);
        return;
    }
    editor->displayModalOverlay(std::make_unique<InstrumentPrompt>(inst, std::move(onSelected)));
}

} // namespace scxt::ui::app::shared
#endif // SCXT_SRC_SCXT_PLUGIN_APP_SHARED_MULTIINSTRUMENTPROMPT_H