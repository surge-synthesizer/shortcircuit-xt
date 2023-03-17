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

#include "HeaderRegion.h"
#include "SCXTEditor.h"

namespace scxt::ui
{

namespace cmsg = scxt::messaging::client;

HeaderRegion::HeaderRegion(SCXTEditor *e) : HasEditor(e)
{
    multiPage = std::make_unique<juce::TextButton>("multi");
    multiPage->onClick = [this]() { editor->setActiveScreen(SCXTEditor::MULTI); };
    addAndMakeVisible(*multiPage);

    sendPage = std::make_unique<juce::TextButton>("send");
    sendPage->onClick = [this]() { editor->setActiveScreen(SCXTEditor::SEND_FX); };
    addAndMakeVisible(*sendPage);

    tunMenu = std::make_unique<juce::TextButton>("tun");
    tunMenu->onClick = [this]() { showTuningMenu(); };
    addAndMakeVisible(*tunMenu);
}

void HeaderRegion::resized()
{

    multiPage->setBounds(2, 2, 98, getHeight() - 4);
    sendPage->setBounds(102, 2, 98, getHeight() - 4);
    tunMenu->setBounds(getWidth() - 200, 2, 98, getHeight() - 4);
}

void HeaderRegion::showTuningMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Tuning");
    p.addSeparator();
    p.addItem("Twelve TET", [w = juce::Component::SafePointer(this)]() {
        if (w)
            cmsg::clientSendToSerialization(cmsg::SetTuningMode(tuning::MidikeyRetuner::TWELVE_TET),
                                            w->editor->msgCont);
    });
    p.addItem("MTS-ESP", [w = juce::Component::SafePointer(this)]() {
        if (w)
            cmsg::clientSendToSerialization(cmsg::SetTuningMode(tuning::MidikeyRetuner::MTS_ESP),
                                            w->editor->msgCont);
    });
    p.showMenuAsync({});
}
} // namespace scxt::ui