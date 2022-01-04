//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_INTEGERTYPEIN_H
#define SHORTCIRCUIT_INTEGERTYPEIN_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "ActionPostingWidgetMixin.h"

struct IntegerPostingTypein : public ActionPostingWidgetMixin<IntegerPostingTypein>,
                              juce::TextEditor
{
    IntegerPostingTypein(ActionSender *s, const std::string &title)
        : ActionPostingWidgetMixin<IntegerPostingTypein>(s)
    {
        setTitle(title);
        setDescription(title);
        setText("", juce::dontSendNotification);
    }
};

#endif // SHORTCIRCUIT_INTEGERTYPEIN_H
