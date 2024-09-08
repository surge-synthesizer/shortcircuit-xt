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

#ifndef SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_ZONELAYOUTDISPLAY_H
#define SCXT_SRC_UI_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_ZONELAYOUTDISPLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "app/SCXTEditor.h"

namespace scxt::ui::app::edit_screen
{
struct MappingDisplay;

struct ZoneLayoutDisplay : juce::Component, HasEditor
{
    MappingDisplay *display{nullptr};
    ZoneLayoutDisplay(MappingDisplay *d);
    void paint(juce::Graphics &g) override;
    void resized() override;

    std::array<int16_t, 3> rootAndRangeForPosition(const juce::Point<int> &);

    juce::Rectangle<float> rectangleForZone(const engine::Part::zoneMappingItem_t &sum);
    juce::Rectangle<float> rectangleForRange(int kL, int kH, int vL, int vH);
    juce::Rectangle<float> rectangleForRangeSkipEnd(int kL, int kH, int vL, int vH);

    void labelZoneRectangle(juce::Graphics &g, const juce::Rectangle<float> &, const std::string &,
                            const juce::Colour &);

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseMove(const juce::MouseEvent &e) override
    {
        for (const auto &h : keyboardHotZones)
        {
            if (h.contains(e.position))
            {
                setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                return;
            }
        }
        for (const auto &h : velocityHotZones)
        {
            if (h.contains(e.position))
            {
                setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
                return;
            }
        }
        for (const auto &h : bothHotZones)
        {
            if (h.contains(e.position))
            {
                setMouseCursor(juce::MouseCursor::UpDownLeftRightResizeCursor);
                return;
            }
        }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    void mouseDoubleClick(const juce::MouseEvent &e) override;

    void resetLeadZoneBounds()
    {
        if (cacheLastZone.has_value())
            setLeadZoneBounds(*cacheLastZone);
    }
    std::optional<engine::Part::zoneMappingItem_t> cacheLastZone;
    void setLeadZoneBounds(const engine::Part::zoneMappingItem_t &az)
    {
        cacheLastZone = az;
        auto r = rectangleForZone(az);
        lastSelectedZone.clear();
        lastSelectedZone.push_back(r);

        // if you change the population order here also change the from settings in mouseDown
        auto side = r.withWidth(8).translated(0, -2).withTrimmedTop(10).withTrimmedBottom(10);
        keyboardHotZones.clear();
        keyboardHotZones.push_back(side);
        keyboardHotZones.push_back(side.translated(r.getWidth() - 6, 0));

        auto top = r.withHeight(8).translated(-2, 0).withTrimmedLeft(10).withTrimmedRight(10);
        velocityHotZones.clear();
        velocityHotZones.push_back(top);
        velocityHotZones.push_back(top.translated(0, r.getHeight() - 6));

        auto corner = r.withWidth(10).withHeight(10).translated(-2, -2);
        bothHotZones.clear();
        bothHotZones.push_back(corner);
        bothHotZones.push_back(corner.translated(r.getWidth() - 8, 0));
        bothHotZones.push_back(corner.translated(r.getWidth() - 8, r.getHeight() - 8));
        bothHotZones.push_back(corner.translated(0, r.getHeight() - 8));
    }

    enum MouseState
    {
        NONE,
        DRAG_VELOCITY,
        DRAG_KEY,
        DRAG_KEY_AND_VEL,
        DRAG_SELECTED_ZONE,
        MULTI_SELECT,
        CREATE_EMPTY_ZONE
    } mouseState{NONE};
    enum DragFrom
    {
        FROM_START,
        FROM_END
    } dragFrom[2]; // key and velocity

    std::vector<juce::Rectangle<float>> velocityHotZones, keyboardHotZones, bothHotZones,
        lastSelectedZone;

    juce::Point<float> firstMousePos{0.f, 0.f}, lastMousePos{0.f, 0.f};

    void showZoneMenu(const selection::SelectionManager::ZoneAddress &za);

    float hPct{0.0}, hZoom{1.0}, vPct{0.0}, vZoom{1.0};
    void setHorizontalZoom(float pctStart, float zoomFactor)
    {
        hPct = pctStart;
        hZoom = zoomFactor;
        resetLeadZoneBounds();
        repaint();
    }
    void setVerticalZoom(float pctStart, float zoomFactor)
    {
        vPct = pctStart;
        vZoom = zoomFactor;
        resetLeadZoneBounds();
        repaint();
    }
};

} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUITXT_ZONELAYOUTDISPLAY_H
