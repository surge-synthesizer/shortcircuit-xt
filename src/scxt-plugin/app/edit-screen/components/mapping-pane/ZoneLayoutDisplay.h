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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_ZONELAYOUTDISPLAY_H
#define SCXT_SRC_SCXT_PLUGIN_APP_EDIT_SCREEN_COMPONENTS_MAPPING_PANE_ZONELAYOUTDISPLAY_H

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

    struct RootAndRange
    {
        RootAndRange(int16_t r, int16_t l, int16_t h) : root(r), lo(l), hi(h), vlo(0), vhi(127) {}
        RootAndRange(int16_t r, int16_t l, int16_t h, int16_t vl, int16_t vh)
            : root(r), lo(l), hi(h), vlo(vl), vhi(vh)
        {
        }
        int16_t root, lo, hi, vlo, vhi;
    };
    std::vector<RootAndRange>
    rootAndRangeForPosition(const juce::Point<int> &, size_t nFiles,
                            bool isMappedInstrument /* like an SFZ or such */);

    juce::Rectangle<float> rectangleForZone(const engine::Part::zoneMappingItem_t &sum);
    juce::Rectangle<float> rectangleForRange(int kL, int kH, int vL, int vH);
    juce::Rectangle<float> rectangleForRangeSkipEnd(int kL, int kH, int vL, int vH);

    std::unordered_map<std::string, juce::GlyphArrangement> glyphArrangementCache;
    std::unordered_map<std::string, juce::Rectangle<float>> glyphBBCache;
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
        auto hzCOrder = std::vector({juce::MouseCursor::TopLeftCornerResizeCursor,
                                     juce::MouseCursor::TopRightCornerResizeCursor,
                                     juce::MouseCursor::BottomRightCornerResizeCursor,
                                     juce::MouseCursor::BottomLeftCornerResizeCursor});

        int idx{0};
        for (const auto &h : bothHotZones)
        {
            if (h.contains(e.position))
            {
                setMouseCursor(hzCOrder[idx]);
                return;
            }
            idx++;
        }

        if (cacheLastZone.has_value())
        {
            auto r = rectangleForZone(*cacheLastZone);
            if (r.contains(e.position))
            {
                setMouseCursor(juce::MouseCursor::DraggingHandCursor);
                return;
            }
        }
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    void mouseDoubleClick(const juce::MouseEvent &e) override;
    void createEmptyZoneAt(const juce::Point<int> &);

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

        static constexpr int zoneSize{8};

        // if you change the population order here also change the from settings in mouseDown
        auto side = r.withWidth(0);
        auto side2 = side.translated(r.getWidth(), 0);
        side = side.expanded(zoneSize / 2, 0).translated(-zoneSize / 4, 0);
        side2 = side2.expanded(zoneSize / 2, 0).translated(zoneSize / 4, 0);

        if (side2.getX() < side.getRight())
        {
            auto overlap = side2.getRight() - side.getRight();
            auto sh = overlap / 2 + 1;
            side = side.translated(-sh, 0);
            side2 = side2.translated(sh, 0);
        }

        keyboardHotZones.clear();
        keyboardHotZones.push_back(side);
        keyboardHotZones.push_back(side2);

        auto top = r.withHeight(0);
        auto bot = top.translated(0, r.getHeight());
        top = top.expanded(0, zoneSize / 2).translated(0, -zoneSize / 4);
        bot = bot.expanded(0, zoneSize / 2).translated(0, zoneSize / 4);

        if (bot.getY() < top.getBottom())
        {
            auto overlap = bot.getY() - top.getBottom();
            auto sh = overlap / 2 + 1;
            top = top.translated(0, -sh);
            bot = bot.translated(0, sh);
        }
        velocityHotZones.clear();
        velocityHotZones.push_back(top);
        velocityHotZones.push_back(bot);

        auto corner = r.withWidth(10).withHeight(10).translated(-2, -2);
        // This order matters both in mouse cursor above and from-setting in the cpp
        bothHotZones.clear();
        bothHotZones.push_back(corner);
        bothHotZones.push_back(corner.translated(r.getWidth() - 8, 0));
        bothHotZones.push_back(corner.translated(r.getWidth() - 8, r.getHeight() - 8));
        bothHotZones.push_back(corner.translated(0, r.getHeight() - 8));
    }

    void clearLeadZoneBounds()
    {
        cacheLastZone = std::nullopt;
        lastSelectedZone.clear();
        keyboardHotZones.clear();
        velocityHotZones.clear();
        bothHotZones.clear();
    }

    bool isEditorInGroupMode() const;

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
    bool tooltipActive{false};

    std::vector<juce::Rectangle<float>> velocityHotZones, keyboardHotZones, bothHotZones,
        lastSelectedZone;

    juce::Point<float> firstMousePos{0.f, 0.f}, lastMousePos{0.f, 0.f};

    void showZoneMenu(const selection::SelectionManager::ZoneAddress &za);
    void showMappingNonZoneMenu(const juce::Point<int> &);

    int gcEvery{0};
    void mappingWasReset();

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
    void updateTooltipContents(bool andShow, const juce::Point<int> &pos);
    void updateCacheFromDisplay();

    juce::Rectangle<float> drawZone(juce::Graphics &g, const engine::Part::zoneMappingItem_t &z,
                                    const juce::Colour &fillColour,
                                    const juce::Colour &borderColor);
};

} // namespace scxt::ui::app::edit_screen
#endif // SHORTCIRCUITXT_ZONELAYOUTDISPLAY_H
