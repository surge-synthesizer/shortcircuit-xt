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

#include "SCXTProcessor.h"
#include "SCXTPluginEditor.h"
#include "components/SCXTEditor.h"
#include "engine/engine.h"

//==============================================================================
SCXTPluginEditor::SCXTPluginEditor(SCXTProcessor &p, scxt::messaging::MessageController &mc,
                                   scxt::infrastructure::DefaultsProvider &d,
                                   const scxt::sample::SampleManager &s,
                                   const scxt::browser::Browser &br,
                                   const scxt::engine::Engine::SharedUIMemoryState &st)
    : juce::AudioProcessorEditor(&p)
{
    ed = std::make_unique<scxt::ui::SCXTEditor>(mc, d, s, br, st);
    ed->onZoomChanged = [this](auto f) {
        setSize(scxt::ui::SCXTEditor::edWidth * f, scxt::ui::SCXTEditor::edHeight * f);
    };
    addAndMakeVisible(*ed);
    setSize(scxt::ui::SCXTEditor::edWidth, scxt::ui::SCXTEditor::edHeight);
    ed->setBounds(0, 0, getWidth(), getHeight());
    setResizable(false, false);
}

SCXTPluginEditor::~SCXTPluginEditor() {}

void SCXTPluginEditor::resized() {}
