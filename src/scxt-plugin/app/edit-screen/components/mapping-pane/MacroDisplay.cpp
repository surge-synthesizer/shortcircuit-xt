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

#include "MacroDisplay.h"
#include "configuration.h"
#include "app/SCXTEditor.h"

namespace scxt::ui::app::edit_screen
{
MacroDisplay::MacroDisplay(scxt::ui::app::SCXTEditor *e) : HasEditor(e)
{
    for (int i = 0; i < scxt::macrosPerPart; ++i)
    {
        macros[i] =
            std::make_unique<shared::SingleMacroEditor>(editor, editor->selectedPart, i, false);
        addAndMakeVisible(*macros[i]);
        //  grab whatever data we have
        macroDataChanged(editor->selectedPart, i);
    }
}

void MacroDisplay::resized()
{
    auto b = getLocalBounds();
    auto dx = b.getWidth() / scxt::macrosPerPart * 2;
    auto dy = b.getHeight() / 2;
    auto kr = b.withWidth(dx).withHeight(dy).reduced(3);
    for (int i = 0; i < scxt::macrosPerPart; ++i)
    {
        macros[i]->setBounds(kr);
        kr = kr.translated(dx, 0);
        if (i == scxt::macrosPerPart / 2 - 1)
            kr = kr.withX(3).translated(0, dy);
    }
}

void MacroDisplay::selectedPartChanged()
{
    for (auto &m : macros)
    {
        m->changePart(editor->selectedPart);
    }
    repaint();
}

void MacroDisplay::macroDataChanged(int part, int index)
{
    assert(part == editor->selectedPart);
    macros[index]->updateFromEditorData();
}

} // namespace scxt::ui::app::edit_screen