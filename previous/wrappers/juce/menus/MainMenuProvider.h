/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_MAINMENUPROVIDER_H
#define SHORTCIRCUIT_MAINMENUPROVIDER_H

#include "SCXTPluginEditor.h"
namespace scxt
{
namespace menus
{
struct MainMenuProvider
{
    static juce::PopupMenu createMenu(SCXTEditor *ed)
    {
        auto res = juce::PopupMenu();
        res.addSectionHeader("Main");
        res.addSeparator();
        res.addSubMenu("Zoom", createZoomMenu(ed));
        res.addSeparator();
        res.addItem("Dump Styles", [ed]() { ed->dumpStyles(); });
        res.addItem("About", [ed]() { ed->showPage(SCXTEditor::Pages::ABOUT); });
        return res;
    }

    static juce::PopupMenu createZoomMenu(SCXTEditor *ed)
    {
        auto zsm = juce::PopupMenu();
        for (auto q : {75, 100, 125, 150, 175, 200})
        {
            zsm.addItem(std::string("Zoom to ") + std::to_string(q),
                        [ed, q]() { ed->setScale(q / 100.f); });
        }
        zsm.addItem("Zoom to best", [ed]() { ed->setScale(ed->optimalScaleForDisplay()); });
        return zsm;
    }
};
} // namespace menus
} // namespace scxt
#endif // SHORTCIRCUIT_MAINMENUPROVIDER_H
