/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_ZONEKEYBOARDDISPLAY_H
#define SHORTCIRCUIT_ZONEKEYBOARDDISPLAY_H

#include <JuceHeader.h>
#include "proxies/ZoneStateProxy.h"

class ActionSender;

class ZoneKeyboardDisplay : public juce::Component
{
  public:
    ZoneKeyboardDisplay(ZoneStateProxy *z, ActionSender *sender) : zsp(z), sender(sender) {}

    void paint(Graphics &g) override;
    void mouseExit(const MouseEvent &event) override;
    void mouseMove(const MouseEvent &event) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;

  private:
    std::vector<juce::Rectangle<float>> keyLocations;
    ZoneStateProxy *zsp; // a non-owned weak copy
    int hoveredKey = -1;
    int playingKey = -1;

    ActionSender *sender;
};

#endif // SHORTCIRCUIT_ZONEKEYBOARDDISPLAY_H
