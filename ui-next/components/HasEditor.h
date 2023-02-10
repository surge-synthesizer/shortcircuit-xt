//
// Created by Paul Walker on 2/11/23.
//

#ifndef SHORTCIRCUIT_HASEDITOR_H
#define SHORTCIRCUIT_HASEDITOR_H

namespace scxt::ui
{
struct SCXTEditor;

struct HasEditor
{
    SCXTEditor *editor{nullptr};
    HasEditor(SCXTEditor *e) : editor(e) {}
};
} // namespace scxt::ui
#endif // SHORTCIRCUIT_HASEDITOR_H
