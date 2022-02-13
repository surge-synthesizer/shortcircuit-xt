//
// Created by Paul Walker on 2/7/22.
//

#ifndef SHORTCIRCUIT_MAINMENUPROVIDER_H
#define SHORTCIRCUIT_MAINMENUPROVIDER_H

#include "SCXTEditor.h"
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
