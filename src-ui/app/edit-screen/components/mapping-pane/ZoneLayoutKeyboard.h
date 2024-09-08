/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_ZONELAYOUTKEYBOARD_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_ZONELAYOUTKEYBOARD_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "app/HasEditor.h"

namespace scxt::ui::app::edit_screen
{
struct MappingDisplay;

struct ZoneLayoutKeyboard : juce::Component, HasEditor
{
    MappingDisplay *display{nullptr};

    static constexpr int firstMidiNote{0}, lastMidiNote{128};
    static constexpr int keyboardHeight{18};

    int32_t heldNote{-1};

    bool moveRootKey{false};

    ZoneLayoutKeyboard(MappingDisplay *d);

    void paint(juce::Graphics &g) override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;

    juce::Rectangle<float> rectangleForKey(int midiNote) const;

    float pctStart{0.f}, zoomFactor{1.f};
    void setHorizontalZoom(float ps, float zf)
    {
        pctStart = ps;
        zoomFactor = zf;
        repaint();
    }
};

};     // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUITXT_ZONELAYOUTKEYBOARD_H
