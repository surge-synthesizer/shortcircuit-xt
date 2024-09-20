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

#include "ZoneLayoutDisplay.h"
#include "ZoneLayoutKeyboard.h"
#include "MappingDisplay.h"

namespace scxt::ui::app::edit_screen
{

ZoneLayoutDisplay::ZoneLayoutDisplay(MappingDisplay *d) : HasEditor(d->editor), display(d) {}

void ZoneLayoutDisplay::mouseDown(const juce::MouseEvent &e)
{
    if (!display)
        return;
    mouseState = NONE;

    if (e.mods.isPopupMenu())
    {
        bool gotOne = false;
        selection::SelectionManager::ZoneAddress za;

        for (auto &z : display->summary)
        {
            auto r = rectangleForZone(z.second);
            if (r.contains(e.position))
            {
                gotOne = display->editor->isSelected(z.first);
                if (!gotOne)
                {
                    gotOne = true;
                    display->editor->doSelectionAction(z.first, true, false, true);
                }
                za = z.first;
            }
        }
        if (gotOne)
        {
            showZoneMenu(za);
            return;
        }
    }

    if (!keyboardHotZones.empty() && keyboardHotZones[0].contains(e.position))
    {
        mouseState = DRAG_KEY;
        dragFrom[0] = FROM_START;
        return;
    }
    if (!keyboardHotZones.empty() && keyboardHotZones[1].contains(e.position))
    {
        mouseState = DRAG_KEY;
        dragFrom[0] = FROM_END;
        return;
    }

    if (!velocityHotZones.empty() && velocityHotZones[0].contains(e.position))
    {
        mouseState = DRAG_VELOCITY;
        dragFrom[1] = FROM_END;
        return;
    }
    if (!velocityHotZones.empty() && velocityHotZones[1].contains(e.position))
    {
        mouseState = DRAG_VELOCITY;
        dragFrom[1] = FROM_START;
        return;
    }

    if (!bothHotZones.empty())
    {
        for (int idx = 0; idx < 4; ++idx)
        {
            if (bothHotZones[idx].contains(e.position))
            {
                // 0 1
                // 3 2
                dragFrom[0] = (idx == 1 || idx == 2) ? FROM_END : FROM_START;
                dragFrom[1] = (idx < 2) ? FROM_END : FROM_START;
                mouseState = DRAG_KEY_AND_VEL;
                return;
            }
        }
    }

    for (const auto &ks : lastSelectedZone)
    {
        if (ks.contains(e.position))
        {
            lastMousePos = e.position;
            mouseState = DRAG_SELECTED_ZONE;
            return;
        }
    }

    std::vector<selection::SelectionManager::ZoneAddress> potentialZones;
    for (auto &z : display->summary)
    {
        auto r = rectangleForZone(z.second);
        if (r.contains(e.position) && display->editor->isAnyZoneFromGroupSelected(z.first.group))
        {
            potentialZones.push_back(z.first);
        }
    }
    selection::SelectionManager::ZoneAddress nextZone;

    if (!potentialZones.empty())
    {
        if (potentialZones.size() == 1)
        {
            nextZone = potentialZones[0];
        }
        else
        {
            // OK so now we have a stack. Basically what we want to
            // do is choose the 'next' one after our currently
            // selected as a heuristic
            auto cz = -1;
            if (display->editor->currentLeadZoneSelection.has_value())
                cz = display->editor->currentLeadZoneSelection->zone;

            auto selThis = -1;
            for (const auto &[idx, za] : sst::cpputils::enumerate(potentialZones))
            {
                if (za.zone > cz && selThis < 0)
                    selThis = idx;
            }

            if (selThis < 0 || selThis >= potentialZones.size())
                nextZone = potentialZones.front();
            else
                nextZone = potentialZones[selThis];
        }

        if (display->editor->isSelected(nextZone))
        {
            if (e.mods.isCommandDown())
            {
                // command click a selected zone deselects it
                display->editor->doSelectionAction(nextZone, false, false, false);
            }
            else if (e.mods.isAltDown())
            {
                // alt click promotes it to lead
                display->setLeadSelection(nextZone);
                display->editor->doSelectionAction(nextZone, true, false, true);
                lastMousePos = e.position;
                mouseState = DRAG_SELECTED_ZONE;
            }
            else
            {
                // single click makes it a single lead
                display->setLeadSelection(nextZone);
                display->editor->doSelectionAction(nextZone, true, true, true);
                lastMousePos = e.position;
                mouseState = DRAG_SELECTED_ZONE;
            }
        }
        else
        {
            display->setLeadSelection(nextZone);
            display->editor->doSelectionAction(
                nextZone, true, !(e.mods.isCommandDown() || e.mods.isAltDown()), true);
            lastMousePos = e.position;
            mouseState = DRAG_SELECTED_ZONE;
        }
    }
    else
    {
        if (e.mods.isCommandDown())
            mouseState = CREATE_EMPTY_ZONE;
        else
            mouseState = MULTI_SELECT;
        firstMousePos = e.position.toFloat();
    }
}

void ZoneLayoutDisplay::mouseDoubleClick(const juce::MouseEvent &e)
{
    for (const auto &ks : lastSelectedZone)
    {
        if (ks.contains(e.position))
        {
            // CHECK SPACE HERE
            display->parentPane->selectTab(2);
            return;
        }
    }
}

void ZoneLayoutDisplay::showZoneMenu(const selection::SelectionManager::ZoneAddress &za)
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Zones");
    p.addSeparator();
    p.addItem("Coming Soon", []() {});

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

template <typename MAP> void constrainMappingFade(MAP &kr, bool startChanged)
{
    int span{0};

    if constexpr (std::is_same_v<MAP, scxt::engine::KeyboardRange>)
    {
        span = kr.keyEnd - kr.keyStart;
    }
    else if constexpr (std::is_same_v<MAP, scxt::engine::VelocityRange>)
    {
        span = kr.velEnd - kr.velStart;
    }
    else
    {
        // Force a compile error
        SCLOG("Unable to compile " << kr.notAMember);
    }

    auto fade = kr.fadeStart + kr.fadeEnd;

    if (fade > span + 1)
    {
        auto amtToRemove = fade - span - 1;

        if (startChanged)
        {
            auto dimStart = std::min((int)kr.fadeStart, (int)amtToRemove);
            kr.fadeStart -= dimStart;
            amtToRemove -= dimStart;
        }
        else
        {
            auto dimEnd = std::min((int)kr.fadeEnd, (int)amtToRemove);
            kr.fadeEnd -= dimEnd;
            amtToRemove -= dimEnd;
        }

        if (amtToRemove > 0)
        {
            if (kr.fadeStart > 0)
            {
                auto dimStart = std::min((int)kr.fadeStart, (int)amtToRemove);
                kr.fadeStart -= dimStart;
                amtToRemove -= dimStart;
            }
            if (kr.fadeEnd > 0)
            {
                auto dimEnd = std::min((int)kr.fadeEnd, (int)amtToRemove);
                kr.fadeEnd -= dimEnd;
                amtToRemove -= dimEnd;
            }
            assert(amtToRemove == 0);
        }
    }
}

void ZoneLayoutDisplay::mouseDrag(const juce::MouseEvent &e)
{
    if (mouseState == DRAG_SELECTED_ZONE)
    {
        if (e.getDistanceFromDragStart() < 2)
        {
            return;
        }
        auto lb = getLocalBounds().toFloat();
        auto displayRegion = lb;

        auto kw = hZoom * displayRegion.getWidth() /
                  (ZoneLayoutKeyboard::lastMidiNote - ZoneLayoutKeyboard::firstMidiNote + 1);
        auto vh = vZoom * displayRegion.getHeight() / 127.0;

        auto dx = e.position.x - lastMousePos.x;
        auto &kr = display->mappingView.keyboardRange;
        auto nx = (int)(dx / kw) + ZoneLayoutKeyboard::firstMidiNote;

        if (kr.keyStart + nx < 0)
            nx = -kr.keyStart;
        else if (kr.keyEnd + nx > 127)
            nx = 127 - kr.keyEnd;
        if (nx != 0)
        {
            lastMousePos.x = e.position.x;
            kr.keyStart += nx;
            kr.keyEnd += nx;

            display->mappingView.rootKey = std::clamp(display->mappingView.rootKey + nx, 0, 127);
        }

        auto dy = -(e.position.y - lastMousePos.y);
        auto &vr = display->mappingView.velocityRange;
        auto vy = (int)(dy / vh);

        if (vr.velStart + vy < 0)
            vy = -vr.velStart;
        else if (vr.velEnd + vy > 127)
            vy = 127 - vr.velEnd;
        if (vy != 0)
        {
            lastMousePos.y = e.position.y;
            vr.velStart += vy;
            vr.velEnd += vy;
        }

        display->mappingChangedFromGUI();
        repaint();
    }

    if (mouseState == DRAG_VELOCITY || mouseState == DRAG_KEY || mouseState == DRAG_KEY_AND_VEL)
    {
        auto lb = getLocalBounds().toFloat();
        auto displayRegion = lb;
        auto kw =
            hZoom * displayRegion.getWidth() /
            (ZoneLayoutKeyboard::lastMidiNote -
             ZoneLayoutKeyboard::firstMidiNote); // this had a +1, which doesn't seem to be needed?
        auto vh = vZoom * displayRegion.getHeight() / 127.0;

        auto newX = e.position.x / kw + hPct * 128;
        auto newXRounded = std::clamp((int)std::round(newX), 0, 128);
        // These previously had + Keyboard::firstMidiNote, and I don't know why? They don't seem to
        // need it.

        auto newY = 127 - e.position.y / vh - vPct * 128;
        auto newYRounded = std::clamp((int)std::round(newY), 0, 128);

        // clamps to 128 on purpose, for reasons that'll get clear below

        auto &vr = display->mappingView.velocityRange;
        auto &kr = display->mappingView.keyboardRange;

        bool updatedMapping{false};
        if (mouseState == DRAG_KEY_AND_VEL || mouseState == DRAG_KEY)
        {
            auto keyEndRightEdge =
                kr.keyEnd + 1; // that's where right edge is drawn, so that's what we compare to

            auto newKeyStart{kr.keyStart};
            auto newKeyEnd{kr.keyEnd};

            if (dragFrom[0] == FROM_START)
            {
                if (newX < (kr.keyStart -
                            0.5)) // change at halfway points, else we can't get to 1 key span
                    newKeyStart = newXRounded;
                else if (newX > (kr.keyStart + 0.5))
                    newKeyStart = newXRounded;
                newKeyStart = std::min(newKeyStart, kr.keyEnd);
            }
            else
            {
                if (newX > (keyEndRightEdge + 0.5))
                    newKeyEnd =
                        newXRounded - 1; // this is -1 to make up for the +1 in keyEndRightEdge,
                // without it the right edge behavior is super weird. Hence
                // the clamp to 128, else we can't drag to the top note.
                else if (newX < (keyEndRightEdge - 0.5))
                    newKeyEnd = newXRounded - 1;

                newKeyEnd = std::max(newKeyEnd, kr.keyStart);
            }

            if (newKeyStart != kr.keyStart || newKeyEnd != kr.keyEnd)
            {
                updatedMapping = true;
            }

            if (updatedMapping)
            {
                bool startChanged = (newKeyStart != kr.keyStart);
                kr.keyStart = newKeyStart;
                kr.keyEnd = newKeyEnd;
                constrainMappingFade(kr, startChanged);
            }
        }
        // Same changes to up/down as to right/left.
        if (mouseState == DRAG_KEY_AND_VEL || mouseState == DRAG_VELOCITY)
        {
            auto velTopEdge = vr.velEnd + 1;

            auto newVelStart{vr.velStart};
            auto newVelEnd{vr.velEnd};

            if (dragFrom[1] == FROM_START)
            {
                if (newY < (vr.velStart - 0.5))
                    newVelStart = newYRounded;
                else if (newY > (vr.velStart + 0.5))
                    newVelStart = newYRounded;
                newVelStart = std::min(newVelStart, vr.velEnd);
            }
            else
            {
                if (newY > (velTopEdge + 0.5))
                    newVelEnd = newYRounded - 1;
                else if (newY < (velTopEdge - 0.5))
                    newVelEnd = newYRounded - 1;
                newVelEnd = std::max(newVelEnd, vr.velStart);
            }

            if (newVelStart != vr.velStart || newVelEnd != vr.velEnd)
            {
                updatedMapping = true;
            }

            if (updatedMapping)
            {
                bool startChanged = (newVelStart != vr.velStart);
                vr.velStart = newVelStart;
                vr.velEnd = newVelEnd;
                constrainMappingFade(vr, startChanged);
            }
        }
        if (updatedMapping)
        {
            display->mappingChangedFromGUI();
            repaint();
        }
    }

    if (mouseState == MULTI_SELECT || mouseState == CREATE_EMPTY_ZONE)
    {
        lastMousePos = e.position.toFloat();
        repaint();
    }
}

void ZoneLayoutDisplay::mouseUp(const juce::MouseEvent &e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
    if (mouseState == MULTI_SELECT)
    {
        auto rz = juce::Rectangle<float>(firstMousePos, e.position);
        bool selectedLead{false};
        if (display->editor->currentLeadZoneSelection.has_value())
        {
            const auto &sel = *(display->editor->currentLeadZoneSelection);
            for (const auto &z : display->summary)
            {
                if (!(z.first == sel))
                    continue;
                if (rz.intersects(rectangleForZone(z.second)))
                    selectedLead = true;
            }
        }
        bool additiveSelect = e.mods.isShiftDown();
        bool firstAsLead = !selectedLead && !additiveSelect;
        bool first = true;
        for (const auto &z : display->summary)
        {
            if (rz.intersects(rectangleForZone(z.second)))
            {
                display->editor->doSelectionAction(z.first, true, false, first && firstAsLead);
                first = false;
            }
            else if (!additiveSelect)
            {
                display->editor->doSelectionAction(z.first, false, false, false);
            }
        }
    }
    if (mouseState == CREATE_EMPTY_ZONE)
    {
        auto r = juce::Rectangle<float>(firstMousePos, e.position);

        auto lb = getLocalBounds().toFloat();
        auto displayRegion = lb;
        auto kw =
            displayRegion.getWidth() /
            (ZoneLayoutKeyboard::lastMidiNote -
             ZoneLayoutKeyboard::firstMidiNote); // this had a +1, which doesn't seem to be needed?
        auto vh = displayRegion.getHeight() / 127.f;

        auto ks = (int)std::clamp(std::floor(r.getX() / kw + ZoneLayoutKeyboard::firstMidiNote),
                                  0.f, 127.f);
        auto ke = (int)std::clamp(std::ceil(r.getRight() / kw + ZoneLayoutKeyboard::firstMidiNote),
                                  0.f, 127.f);
        auto vs = (int)std::clamp(127.f - std::floor(r.getBottom() / vh), 0.f, 127.f);
        auto ve = (int)std::clamp(127.f - std::ceil(r.getY() / vh), 0.f, 127.f);

        namespace cmsg = scxt::messaging::client;
        auto za{editor->currentLeadZoneSelection};

        if (!za.has_value())
        {
            sendToSerialization(cmsg::AddBlankZone({0, 0, ks, ke, vs, ve}));
        }
        else
        {
            sendToSerialization(cmsg::AddBlankZone({za->part, za->group, ks, ke, vs, ve}));
        }
    }
    mouseState = NONE;
    repaint();
}

juce::Rectangle<float>
ZoneLayoutDisplay::rectangleForZone(const engine::Part::zoneMappingItem_t &sum)
{
    const auto &[kb, vel, name] = sum;
    return rectangleForRange(kb.keyStart, kb.keyEnd, vel.velStart, vel.velEnd + 1);
}

juce::Rectangle<float> ZoneLayoutDisplay::rectangleForRange(int kL, int kH, int vL, int vH)
{
    if (kL > kH)
        std::swap(kL, kH);

    return rectangleForRangeSkipEnd(kL, kH + 1, vL, vH);
}
juce::Rectangle<float> ZoneLayoutDisplay::rectangleForRangeSkipEnd(int kL, int kH, int vL, int vH)
{
    auto lb = getLocalBounds().toFloat().withTrimmedTop(2.f);
    auto displayRegion = lb;

    // OK so keys first
    auto normkL = 1.0 * kL / (ZoneLayoutKeyboard::lastMidiNote - ZoneLayoutKeyboard::firstMidiNote);
    auto normkH = 1.0 * kH / (ZoneLayoutKeyboard::lastMidiNote - ZoneLayoutKeyboard::firstMidiNote);

    auto sckL = (normkL - hPct) * hZoom;
    auto sckH = (normkH - hPct) * hZoom;

    auto x0 = sckL * lb.getWidth();
    auto x1 = sckH * lb.getWidth();

    if (x1 < x0)
        std::swap(x1, x0);

    // Then velocities
    auto normvL = 1.0 - 1.0 * vL / 128;
    auto normvH = 1.0 - 1.0 * vH / 128;

    auto scvL = (normvL - vPct) * vZoom;
    auto scvH = (normvH - vPct) * vZoom;

    float y0 = scvL * lb.getHeight();
    float y1 = scvH * lb.getHeight();
    if (y1 < y0)
        std::swap(y1, y0);

    return {(float)x0, (float)y0, (float)(x1 - x0), (float)(y1 - y0)};
}

void ZoneLayoutDisplay::paint(juce::Graphics &g)
{
    if (!display)
        g.fillAll(juce::Colours::red);

    // Draw the background
    {
        auto lb = getLocalBounds().toFloat().withTrimmedTop(1.f);
        auto displayRegion = lb;

        auto dashCol = editor->themeColor(theme::ColorMap::generic_content_low, 0.4f);
        g.setColour(dashCol);
        g.drawVerticalLine(lb.getX() + 1, lb.getY(), lb.getY() + lb.getHeight());
        g.drawVerticalLine(lb.getX() + lb.getWidth() - 1, lb.getY(), lb.getY() + lb.getHeight());
        g.drawHorizontalLine(lb.getY(), lb.getX(), lb.getX() + lb.getWidth());
        g.drawHorizontalLine(lb.getY() + lb.getHeight() - 1, lb.getX(), lb.getX() + lb.getWidth());

        dashCol = dashCol.withAlpha(0.2f);
        g.setColour(dashCol);

        auto dVel = 1.0 / 4.0;
        if (vZoom >= 2.0)
            dVel = 1.0 / 8.0;
        if (vZoom >= 3.0)
            dVel = 1.0 / 16.0;
        for (float fv = 0; fv < 1.0; fv += dVel)
        {
            auto y = (fv - vPct) * vZoom * getHeight();
            g.drawHorizontalLine(y, lb.getX(), lb.getX() + lb.getWidth());
        }

        auto hMul = 1.0;
        if (hZoom >= 2.0)
            hMul = 0.5;
        for (float f = 0; f <= 1.0; f += hMul * 12.0 / 128.0)
        {
            auto x = (f - hPct) * hZoom * getWidth();
            g.drawVerticalLine(x, lb.getY(), lb.getY() + lb.getHeight());
        }
    }

    auto drawVoiceMarkers = [&g](const juce::Rectangle<float> &c, int ct) {
        if (ct == 0)
            return;
        auto r = c.reduced(2).withTrimmedTop(25);

        int vrad{8};
        if (r.getWidth() < vrad)
        {
            vrad = r.getWidth();
            auto b = r.withTop(r.getBottom() - vrad).withWidth(vrad);
            g.setColour(juce::Colours::orange);
            g.fillRoundedRectangle(b.toFloat(), 1);
            return;
        }
        auto b = r.withTop(r.getBottom() - vrad).withWidth(vrad);
        g.setColour(juce::Colours::orange);
        for (int i = 0; i < ct; ++i)
        {
            g.fillRoundedRectangle(b.reduced(1).toFloat(), 1);
            b = b.translated(vrad, 0);
            if (!r.contains(b))
            {
                b.setX(r.getX());
                b = b.translated(0, -vrad);

                if (!r.contains(b))
                    return;
            }
        }
    };

    for (const auto &drawSelected : {false, true})
    {
        for (const auto &z : display->summary)
        {
            if (!display->editor->isAnyZoneFromGroupSelected(z.first.group))
                continue;

            if (display->editor->isSelected(z.first) != drawSelected)
                continue;

            if (z.first == display->editor->currentLeadZoneSelection)
                continue;

            auto r = rectangleForZone(z.second);

            auto borderColor = editor->themeColor(theme::ColorMap::accent_2a);
            auto fillColor = editor->themeColor(theme::ColorMap::accent_2b).withAlpha(0.32f);
            auto textColor = editor->themeColor(theme::ColorMap::accent_2a);

            if (drawSelected)
            {
                borderColor = editor->themeColor(theme::ColorMap::accent_1b);
                fillColor = borderColor.withAlpha(0.32f);
                textColor = editor->themeColor(theme::ColorMap::accent_1a);
            }

            g.setColour(fillColor);
            g.fillRect(r);
            g.setColour(borderColor);
            g.drawRect(r, 1.f);

            labelZoneRectangle(g, r, std::get<2>(z.second), textColor);

            auto ct = display->voiceCountFor(z.first);
            drawVoiceMarkers(r, ct);
        }
    }

    if (display->editor->currentLeadZoneSelection.has_value())
    {
        const auto &sel = *(display->editor->currentLeadZoneSelection);

        for (const auto &z : display->summary)
        {
            if (!(z.first == sel))
                continue;

            const auto &[kb, vel, name] = z.second;

            auto selZoneColor = editor->themeColor(theme::ColorMap::accent_1a);
            auto c1{selZoneColor.withAlpha(0.f)};
            auto c2{selZoneColor.withAlpha(0.5f)};

            bool debugGradients = false;

            if (kb.fadeStart + kb.fadeEnd + vel.fadeStart + vel.fadeEnd == 0)
            {
                auto r{rectangleForRange(kb.keyStart, kb.keyEnd, vel.velStart, vel.velEnd + 1)};
                g.setColour(c2);

                g.fillRect(r);
            }
            else
            { // lower left corner
                {
                    auto r{rectangleForRangeSkipEnd(kb.keyStart, kb.keyStart + kb.fadeStart,
                                                    vel.velStart, vel.velStart + vel.fadeStart)};

                    double scaleY{r.getHeight() / r.getWidth()};
                    double transY{(1 - scaleY) * r.getY()};

                    juce::ColourGradient grad{c2,       r.getRight(), r.getY(), c1,
                                              r.getX(), r.getY(),     true};

                    juce::FillType originalFill(grad);
                    juce::FillType newFill(originalFill.transformed(
                        juce::AffineTransform::scale(1.f, scaleY).translated(0.f, transY)));
                    g.setFillType(newFill);
                    if (debugGradients)
                        g.setColour(juce::Colours::yellow);
                    g.fillRect(r);
                }

                // top left
                {
                    auto r{rectangleForRangeSkipEnd(kb.keyStart, kb.keyStart + kb.fadeStart,
                                                    vel.velEnd - vel.fadeEnd, vel.velEnd)};

                    float xRange{r.getWidth()};
                    float yRange{r.getHeight()};
                    double scaleY{yRange / xRange};
                    double transY{(1 - scaleY) * r.getHeight()};
                    double transY2{(1 - scaleY) * r.getBottom()};

                    juce::ColourGradient grad{c2,       r.getRight(),  r.getBottom(), c1,
                                              r.getX(), r.getBottom(), true};

                    juce::FillType originalFill(grad);
                    juce::FillType newFill(originalFill.transformed(
                        juce::AffineTransform::scale(1, scaleY).translated(0, transY2)));
                    g.setFillType(newFill);

                    if (debugGradients)
                        g.setColour(juce::Colours::aliceblue);
                    g.fillRect(r);
                }

                // left side
                {
                    auto r{rectangleForRangeSkipEnd(kb.keyStart, kb.keyStart + kb.fadeStart,
                                                    vel.velStart + vel.fadeStart,
                                                    vel.velEnd - vel.fadeEnd)};
                    juce::ColourGradient grad{c1,           r.getX(), r.getY(), c2,
                                              r.getRight(), r.getY(), false};
                    g.setGradientFill(grad);

                    if (debugGradients)
                        g.setColour(juce::Colours::green);

                    g.fillRect(r);
                }

                // gradient regions
                {
                    // bottom
                    {
                        auto r{rectangleForRange(kb.keyStart + kb.fadeStart, kb.keyEnd - kb.fadeEnd,
                                                 vel.velStart, vel.velStart + vel.fadeStart)};
                        juce::ColourGradient grad{c1,       r.getX(), r.getBottom(), c2,
                                                  r.getX(), r.getY(), false};
                        g.setGradientFill(grad);

                        if (debugGradients)
                            g.setColour(juce::Colours::bisque);

                        g.fillRect(r);
                    }

                    // top region
                    {
                        auto r{rectangleForRange(kb.keyStart + kb.fadeStart, kb.keyEnd - kb.fadeEnd,
                                                 vel.velEnd - vel.fadeEnd, vel.velEnd)};
                        juce::ColourGradient grad{c1,       r.getX(),      r.getY(), c2,
                                                  r.getX(), r.getBottom(), false};
                        g.setGradientFill(grad);

                        if (debugGradients)
                            g.setColour(juce::Colours::red);

                        g.fillRect(r);
                    }

                    // no gradient for the center
                    {
                        auto r{rectangleForRange(kb.keyStart + kb.fadeStart, kb.keyEnd - kb.fadeEnd,
                                                 vel.velStart + vel.fadeStart,
                                                 vel.velEnd - vel.fadeEnd)};
                        g.setColour(c2);

                        if (!debugGradients)
                            g.fillRect(r);
                    }
                }

                // Right side
                {
                    auto r{rectangleForRangeSkipEnd(kb.keyEnd - kb.fadeEnd + 1, kb.keyEnd + 1,
                                                    vel.velStart + vel.fadeStart,
                                                    vel.velEnd - vel.fadeEnd)};
                    juce::ColourGradient grad{c2,           r.getX(), r.getY(), c1,
                                              r.getRight(), r.getY(), false};
                    g.setGradientFill(grad);

                    if (debugGradients)
                        g.setColour(juce::Colours::beige);

                    g.fillRect(r);
                }

                // bottom right corner
                {
                    auto r{rectangleForRangeSkipEnd(kb.keyEnd - kb.fadeEnd + 1, kb.keyEnd + 1,
                                                    vel.velStart, vel.velStart + vel.fadeStart)};

                    float xRange{r.getWidth()};
                    float yRange{r.getHeight()};
                    double scaleY{yRange / xRange};
                    double transY{(1 - scaleY) * r.getY()};

                    juce::ColourGradient grad{c2,           r.getX(), r.getY(), c1,
                                              r.getRight(), r.getY(), true};

                    juce::FillType originalFill(grad);
                    juce::FillType newFill(originalFill.transformed(
                        juce::AffineTransform::scale(1, scaleY).translated(0, transY)));
                    g.setFillType(newFill);

                    if (debugGradients)
                        g.setColour(juce::Colours::orange);

                    g.fillRect(r);
                }

                // top right
                {
                    auto r{rectangleForRangeSkipEnd(kb.keyEnd - kb.fadeEnd + 1, kb.keyEnd + 1,
                                                    vel.velEnd - vel.fadeEnd, vel.velEnd)};

                    float xRange{r.getWidth()};
                    float yRange{r.getHeight()};
                    double scaleY{yRange / xRange};
                    double transY{(1 - scaleY) * r.getBottom()};

                    juce::ColourGradient grad{c2,           r.getX(),      r.getBottom(), c1,
                                              r.getRight(), r.getBottom(), true};

                    juce::FillType originalFill(grad);
                    juce::FillType newFill(originalFill.transformed(
                        juce::AffineTransform::scale(1, scaleY).translated(0, transY)));
                    g.setFillType(newFill);

                    if (debugGradients)
                        g.setColour(juce::Colours::blueviolet);

                    g.fillRect(r);
                }

                const float dashes[]{1.0f, 2.0f};
                {
                    // vertical dashes
                    {
                        auto r{rectangleForRange(kb.keyStart + kb.fadeStart, kb.keyEnd - kb.fadeEnd,
                                                 vel.velStart, vel.velEnd)};

                        g.setColour(c2);
                        if (kb.fadeStart > 0)
                            g.drawDashedLine({{r.getX(), r.getY()}, {r.getX(), r.getBottom()}},
                                             dashes, juce::numElementsInArray(dashes));

                        if (kb.fadeEnd > 0)
                            g.drawDashedLine(
                                {{r.getRight(), r.getY()}, {r.getRight(), r.getBottom()}}, dashes,
                                juce::numElementsInArray(dashes));
                    }

                    // horizontal dashes
                    {
                        auto r{rectangleForRange(kb.keyStart, kb.keyEnd,
                                                 vel.velStart + vel.fadeStart,
                                                 vel.velEnd - vel.fadeEnd)};

                        g.setColour(c2);
                        if (vel.fadeEnd > 0)
                            g.drawDashedLine({{r.getX(), r.getY()}, {r.getRight(), r.getY()}},
                                             dashes, juce::numElementsInArray(dashes));
                        if (vel.fadeStart > 0)
                            g.drawDashedLine(
                                {{r.getX(), r.getBottom()}, {r.getRight(), r.getBottom()}}, dashes,
                                juce::numElementsInArray(dashes));
                    }
                }
            }
            auto r = rectangleForZone(z.second);
            g.setColour(selZoneColor);
            g.drawRect(r, 3.f);

            labelZoneRectangle(g, r, std::get<2>(z.second),
                               editor->themeColor(theme::ColorMap::accent_1a));

            auto ct = display->voiceCountFor(z.first);
            drawVoiceMarkers(r, ct);
        }
    }

    if (display->isUndertakingDrop)
    {
        auto rr = rootAndRangeForPosition(display->currentDragPoint);
        auto rb = rectangleForRange(rr[1], rr[2], 0, 127);
        g.setColour(editor->themeColor(theme::ColorMap::accent_1a, 0.4f));
        g.fillRect(rb);
    }

    if (mouseState == MULTI_SELECT || mouseState == CREATE_EMPTY_ZONE)
    {
        auto r = juce::Rectangle<float>(firstMousePos, lastMousePos);
        if (mouseState == MULTI_SELECT)
        {
            juce::PathStrokeType stroke(1);       // Stroke of width 5
            juce::Array<float> dashLengths{5, 5}; // Dashes of 5px, spaces of 5px

            for (const auto &z : display->summary)
            {
                auto rz = rectangleForZone(z.second);
                if (rz.intersects(r))
                {
                    g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));

                    g.drawRect(rz, 2);
                }
            }

            g.setColour(editor->themeColor(theme::ColorMap::generic_content_highest));
            auto p = juce::Path();
            p.addRectangle(r);

            juce::Path dashedPath;
            stroke.createDashedStroke(dashedPath, p, dashLengths.getRawDataPointer(),
                                      dashLengths.size());
            g.strokePath(dashedPath, stroke);
        }
        else
        {
            auto col = editor->themeColor(theme::ColorMap::accent_2a);
            g.setColour(col.withAlpha(0.3f));
            g.fillRect(r);
            g.setColour(col);
            g.drawRect(r, 2);
        }
    }
}

void ZoneLayoutDisplay::resized() {}

std::array<int16_t, 3> ZoneLayoutDisplay::rootAndRangeForPosition(const juce::Point<int> &p)
{
    assert(ZoneLayoutKeyboard::lastMidiNote > ZoneLayoutKeyboard::firstMidiNote);
    auto lb = getLocalBounds().toFloat();
    auto bip = getBoundsInParent();
    auto kw =
        lb.getWidth() / (ZoneLayoutKeyboard::lastMidiNote - ZoneLayoutKeyboard::firstMidiNote);

    auto rootKey = std::clamp(
        (p.getX() - bip.getX()) * 1.f / kw + ZoneLayoutKeyboard::firstMidiNote,
        (float)ZoneLayoutKeyboard::firstMidiNote, (float)ZoneLayoutKeyboard::lastMidiNote);

    auto fromTop = std::clamp((p.getY() - bip.getY()), 0, getHeight()) * 1.f / getHeight();
    auto span = (1.0f - sqrt(fromTop)) * 80;
    auto low = std::clamp(rootKey - span, 0.f, 127.f);
    auto high = std::clamp(rootKey + span, 0.f, 127.f);
    return {(int16_t)rootKey, (int16_t)low, (int16_t)high};
}

void ZoneLayoutDisplay::labelZoneRectangle(juce::Graphics &g, const juce::Rectangle<float> &rIn,
                                           const std::string &txt, const juce::Colour &col)
{
    auto f = editor->themeApplier.interRegularFor(11);

    juce::GlyphArrangement ga;
    ga.addJustifiedText(f, txt, 0, 0, 1000, juce::Justification::topLeft);
    auto bb = ga.getBoundingBox(0, -1, false);

    auto zoomedAway = getLocalBounds().contains(rIn.toNearestInt());
    auto rr = getLocalBounds().getIntersection(rIn.toNearestInt());

    bool horiz{true};
    auto vPad{8};
    auto hPad{16};
    if (rr.getWidth() < rr.getHeight() && bb.getWidth() > rr.getWidth() - hPad)
    {
        horiz = false;
    }

    if (horiz)
    {
        while (bb.getWidth() > rr.getWidth() - hPad && ga.getNumGlyphs() > 0)
        {
            ga.removeRangeOfGlyphs(ga.getNumGlyphs() - 1, -1);
            bb = ga.getBoundingBox(0, -1, false);
        }
        if (ga.getNumGlyphs() < 1)
            return;

        auto cx = (rr.getWidth() - bb.getWidth()) / 2 + rr.getX();
        auto cy = (rr.getHeight() - bb.getHeight()) / 2 + rr.getY() - bb.getY();

        // Stay in the box
        if (rr.getBottom() < getHeight() / 2)
        {
            if (cy > rr.getBottom() - vPad)
            {
                cy = rr.getBottom() - vPad;
            }
        }
        else
        {
            if (cy - bb.getHeight() < rr.getY() + vPad)
            {
                cy = rr.getY() + vPad + bb.getHeight();
            }
        }

        auto gs = juce::Graphics::ScopedSaveState(g);
        g.addTransform(juce::AffineTransform().translated(cx, cy));
        g.setColour(editor->themeColor(theme::ColorMap::bg_2).withAlpha(0.7f));
        auto bbb = bb.expanded(4, 2);
        g.fillRoundedRectangle(bbb, 2);
        g.setColour(col.withAlpha(0.1f));
        g.drawRoundedRectangle(bbb, 2, 1);
        g.setColour(col);
        ga.draw(g);
    }
    else
    {
        while (bb.getWidth() > rr.getHeight() - hPad && ga.getNumGlyphs() > 0)
        {
            ga.removeRangeOfGlyphs(ga.getNumGlyphs() - 1, -1);
            bb = ga.getBoundingBox(0, -1, false);
        }

        // the rotations make these signs a pita
        auto cx = (rr.getWidth() + bb.getHeight() - vPad / 2) / 2 + rr.getX();
        auto cy = (rr.getHeight() - bb.getWidth()) / 2 + rr.getY() + bb.getWidth();

        // Stay in the box
        /*
        if (rr.getBottom() < getHeight() / 2)
        {
            if (cy > rr.getBottom() - vPad)
            {
                cy = rr.getBottom() - vPad;
            }
        }
        else
        {
            if (cy - bb.getHeight() < rr.getY() + vPad)
            {
                cy = rr.getY() + vPad + bb.getHeight();
            }
        }*/

        auto gs = juce::Graphics::ScopedSaveState(g);
        g.addTransform(juce::AffineTransform().rotated(-M_PI_2).translated(cx, cy));
        g.setColour(editor->themeColor(theme::ColorMap::bg_2).withAlpha(0.7f));
        auto bbb = bb.expanded(4, 2);
        g.fillRoundedRectangle(bbb, 2);
        g.setColour(col.withAlpha(0.1f));
        g.drawRoundedRectangle(bbb, 2, 1);
        g.setColour(col);
        ga.draw(g);
    }
}

} // namespace scxt::ui::app::edit_screen