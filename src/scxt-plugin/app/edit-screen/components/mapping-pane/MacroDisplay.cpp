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