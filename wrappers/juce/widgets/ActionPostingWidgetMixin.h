//
// Created by Paul Walker on 1/4/22.
//

#ifndef SHORTCIRCUIT_ACTIONPOSTINGWIDGETMIXIN_H
#define SHORTCIRCUIT_ACTIONPOSTINGWIDGETMIXIN_H

struct ActionSender;

template <typename T> struct ActionPostingWidgetMixin
{
    ActionPostingWidgetMixin(ActionSender *s) : sender(s) {}
    ActionSender *s{nullptr};
};

#endif // SHORTCIRCUIT_ACTIONPOSTINGWIDGETMIXIN_H
