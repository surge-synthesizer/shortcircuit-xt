/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_ZONEKEYBOARDDISPLAY_H
#define SHORTCIRCUIT_ZONEKEYBOARDDISPLAY_H

#include "juce_gui_basics/juce_gui_basics.h"

#include "SC3Editor.h"

class ActionSender;

namespace SC3
{
namespace Components
{
class ZoneKeyboardDisplay : public juce::Component
{
  public:
    ZoneKeyboardDisplay(SC3Editor *z, ActionSender *sender) : editor(z), sender(sender) {}

    void paint(juce::Graphics &g) override;
    void mouseExit(const juce::MouseEvent &event) override;
    void mouseMove(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;

    void mouseDoubleClick(const juce::MouseEvent &event) override;

  private:
    std::vector<juce::Rectangle<float>> keyLocations;
    SC3Editor *editor{nullptr}; // a non-owned weak copy
    int hoveredKey = -1;
    int playingKey = -1;
    int hoveredZone = -1;

    ActionSender *sender;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZoneKeyboardDisplay);
};

} // namespace Components
} // namespace SC3

#endif // SHORTCIRCUIT_ZONEKEYBOARDDISPLAY_H
