/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#include "MappingPane.h"
#include "components/SCXTEditor.h"
#include "datamodel/parameter.h"
#include "selection/selection_manager.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "connectors/PayloadDataAttachment.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"

#include "connectors/SCXTStyleSheetCreator.h"
#include "engine/part.h"

namespace scxt::ui::multi
{

namespace cmsg = scxt::messaging::client;

struct MappingDisplay;
struct MappingZonesAndKeyboard : juce::Component, HasEditor
{
    MappingDisplay *display{nullptr};
    MappingZonesAndKeyboard(MappingDisplay *d);
    void paint(juce::Graphics &g) override;
    std::array<int16_t, 3> rootAndRangeForPosition(const juce::Point<int> &);

    int firstMidiNote{0}, lastMidiNote{128};

    static constexpr int keyboardHeight{25};
    juce::Rectangle<float> rectangleForZone(const engine::Part::zoneMappingItem_t &sum);
    juce::Rectangle<float> rectangleForRange(int kL, int kH, int vL, int vH);
    juce::Rectangle<float> rectangleForKey(int midiNote);

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

    void setLeadZoneBounds(const engine::Part::zoneMappingItem_t &az)
    {
        auto r = rectangleForZone(az);
        lastSelectedZone.clear();
        lastSelectedZone.push_back(r);

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
        HELD_NOTE,
        DRAG_VELOCITY,
        DRAG_KEY,
        DRAG_KEY_AND_VEL,
        DRAG_SELECTED_ZONE
    } mouseState{NONE};

    std::vector<juce::Rectangle<float>> velocityHotZones, keyboardHotZones, bothHotZones,
        lastSelectedZone;
    int32_t heldNote{-1};

    juce::Point<float> lastMousePos{0.f, 0.f};
};

struct MappingZoneHeader : juce::Component
{
    int paints{0};
    void paint(juce::Graphics &g)
    {
        g.setColour(juce::Colours::darkgreen);
        g.fillAll();
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font("Comic Sans MS", 20, juce::Font::plain));
        g.drawText("Zones Header", getLocalBounds(), juce::Justification::centred);
    }
};

struct Zoomable : public juce::Component
{

    std::unique_ptr<juce::Viewport> viewport;
    int scrollDirection = 1;
    void invertScroll(bool invert) { scrollDirection = invert ? -1 : 1; }

    struct zoomFactor
    {
        float acc = 1.f;
        int val = 1;
        float min = 1.f;
        float max = 4.f;
    } zoomX, zoomY;

    enum class Axis
    {
        horizontalZoom,
        verticalZoom
    };

    Zoomable(juce::Component *attachedComponent,
             std::pair<float, float> minMaxX = std::make_pair(1.f, 4.f),
             std::pair<float, float> minMaxY = std::make_pair(1.f, 4.f))
        : zoomX({1.f, 1, minMaxX.first, minMaxX.second}),
          zoomY({1.f, 1, minMaxY.first, minMaxY.second})
    {
        viewport = std::make_unique<juce::Viewport>();
        viewport->setViewedComponent(attachedComponent, false);
        addAndMakeVisible(*viewport);
    }

    void resized() override
    {
        auto viewArea = getLocalBounds();
        viewport->setBounds(0, 0, getWidth(), getHeight());
        auto zoneArea =
            viewArea.withWidth(viewArea.getWidth() * zoomX.val - viewport->getScrollBarThickness())
                .withHeight(viewArea.getHeight() * zoomY.val - viewport->getScrollBarThickness());
        viewport->getViewedComponent()->setBounds(zoneArea);
    }

    void zoom(Axis axis, float delta)
    {
        constexpr auto acceleration = 10.f;
        constexpr auto zoomMin = 1.f;
        constexpr auto zoomMax = 4.f;

        auto &z = axis == Axis::horizontalZoom ? zoomX : zoomY;
        z.acc += delta * acceleration * scrollDirection;
        auto m = 0.f;
        std::modf(z.acc, &m);
        auto zoom = m != 0.f;
        if (zoom)
        {
            auto prevVal = static_cast<float>(z.val);
            z.val = std::clamp(z.val + m, z.min, z.max);
            z.acc = 0.f;

            auto viewedComp = viewport->getViewedComponent();

            auto x = viewedComp->getX();
            auto y = viewedComp->getY();
            if (axis == Axis::horizontalZoom)
            {
                x *= z.val / prevVal;
            }
            else
            {
                y *= z.val / prevVal;
            }

            auto w = viewport->getWidth() * zoomX.val - viewport->getScrollBarThickness();
            auto h = viewport->getHeight() * zoomY.val - viewport->getScrollBarThickness();
            viewedComp->setBounds(x, y, w, h);
        }
    }

    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &w) override
    {
        if (e.mods.isAltDown())
        {
            auto axis = e.mods.isShiftDown() ? Axis::horizontalZoom : Axis::verticalZoom;
            zoom(axis, w.deltaY);
        }
    }
};

struct MappingDisplay : juce::Component, HasEditor, juce::DragAndDropTarget
{
    typedef connectors::PayloadDataAttachment<engine::Zone::ZoneMappingData, int16_t>
        zone_attachment_t;

    typedef connectors::PayloadDataAttachment<engine::Zone::ZoneMappingData, float>
        zone_float_attachment_t;

    enum Ctrl
    {
        RootKey,
        KeyStart,
        KeyEnd,
        FadeStart,
        FadeEnd,
        VelStart,
        VelEnd,
        VelFadeStart,
        VelFadeEnd,
        PBDown,
        PBUp,

        VelocitySens,
        Level,
        Pan,
        Pitch
    };

    std::unique_ptr<Zoomable> mappingViewport;

    std::unique_ptr<MappingZonesAndKeyboard> zonesAndKeyboard;
    std::unique_ptr<MappingZoneHeader> zoneHeader;

    std::unordered_map<Ctrl, std::unique_ptr<zone_attachment_t>> attachments;
    std::unordered_map<Ctrl, std::unique_ptr<zone_float_attachment_t>> floatattachments;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>>
        textEds;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::Label>> labels;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::GlyphPainter>> glyphs;

    MappingDisplay(MappingPane *p) : HasEditor(p->editor), mappingView(p->mappingView)
    {
        // TODO: Upgrade all these attachments with the new factory style
        zonesAndKeyboard = std::make_unique<MappingZonesAndKeyboard>(this);

        mappingViewport = std::make_unique<Zoomable>(zonesAndKeyboard.get());
        addAndMakeVisible(*mappingViewport);

        zoneHeader = std::make_unique<MappingZoneHeader>();
        addAndMakeVisible(*zoneHeader);

        auto attachEditor = [this](Ctrl c, const auto &desc, auto &v) {
            auto at = std::make_unique<zone_attachment_t>(
                desc, [this](const auto &at) { this->mappingChangedFromGUI(at); }, v);
            auto sl = std::make_unique<sst::jucegui::components::DraggableTextEditableValue>();
            sl->setSource(at.get());
            addAndMakeVisible(*sl);
            textEds[c] = std::move(sl);
            attachments[c] = std::move(at);
        };

        auto attachFloatEditor = [this](Ctrl c, const auto &desc, auto &v) {
            auto at = std::make_unique<zone_float_attachment_t>(
                desc, [this](const auto &at) { this->mappingChangedFromGUI(at); }, v);
            auto sl = std::make_unique<sst::jucegui::components::DraggableTextEditableValue>();
            sl->setSource(at.get());
            addAndMakeVisible(*sl);
            textEds[c] = std::move(sl);
            floatattachments[c] = std::move(at);
        };

        auto addLabel = [this](Ctrl c, const std::string &label) {
            auto l = std::make_unique<sst::jucegui::components::Label>();
            l->setText(label);
            addAndMakeVisible(*l);
            labels[c] = std::move(l);
        };

        auto addGlyph = [this](Ctrl c, sst::jucegui::components::GlyphPainter::GlyphType g) {
            // TODO StyleSheet
            auto l = std::make_unique<sst::jucegui::components::GlyphPainter>(g);
            addAndMakeVisible(*l);
            glyphs[c] = std::move(l);
        };

        attachEditor(Ctrl::RootKey, datamodel::pmd().asMIDINote().withName("RootKey"),
                     mappingView.rootKey);
        addLabel(Ctrl::RootKey, "Root Key");

        attachEditor(Ctrl::KeyStart,
                     datamodel::pmd().asMIDINote().withName("Key Start").withDefault(60 - 12),
                     mappingView.keyboardRange.keyStart);

        attachEditor(Ctrl::KeyEnd,
                     datamodel::pmd().asMIDINote().withName("Key End").withDefault(60 + 12),
                     mappingView.keyboardRange.keyEnd);
        addLabel(Ctrl::KeyStart, "Key Range");

        auto mDist = []() { return datamodel::pmd().asMIDIPitch().withUnit(""); };

        attachEditor(Ctrl::FadeStart, mDist().withName("Fade Start").withDefault(0),
                     mappingView.keyboardRange.fadeStart);
        // attachments[Ctrl::FadeStart]->setAsInteger();
        attachEditor(Ctrl::FadeEnd, mDist().withName("Fade End").withDefault(0),
                     mappingView.keyboardRange.fadeEnd);
        // attachments[Ctrl::FadeEnd]->setAsInteger();
        addLabel(Ctrl::FadeStart, "Crossfade");

        attachEditor(Ctrl::VelStart, mDist().withName("Vel Start").withDefault(0),
                     mappingView.velocityRange.velStart);
        // attachments[Ctrl::VelStart]->setAsInteger();
        attachEditor(Ctrl::VelEnd, mDist().withName("Vel End").withDefault(127),
                     mappingView.velocityRange.velEnd);
        // attachments[Ctrl::VelEnd]->setAsInteger();
        addLabel(Ctrl::VelStart, "Vel Range");

        attachEditor(Ctrl::VelFadeStart, mDist().withName("Vel Fade Start").withDefault(0),
                     mappingView.velocityRange.fadeStart);
        // attachments[Ctrl::VelFadeStart]->setAsInteger();
        attachEditor(Ctrl::VelFadeEnd, mDist().withName("Vel Fade End").withDefault(0),
                     mappingView.velocityRange.fadeEnd);
        // attachments[Ctrl::VelFadeEnd]->setAsInteger();
        addLabel(Ctrl::VelFadeStart, "Crossfade");

        attachEditor(Ctrl::PBDown, mDist().withName("PBDown").withDefault(2), mappingView.pbDown);
        // attachments[Ctrl::PBDown]->setAsInteger();
        attachEditor(Ctrl::PBUp, mDist().withName("PBUp").withDefault(2), mappingView.pbUp);
        // attachments[Ctrl::PBUp]->setAsInteger();
        addLabel(Ctrl::PBDown, "Pitch Bend");

        attachFloatEditor(Ctrl::Level,
                          datamodel::pmd().asPercent().withName("Amplitude").withDefault(1.0),
                          mappingView.amplitude);
        addGlyph(Ctrl::Level, sst::jucegui::components::GlyphPainter::VOLUME);

        attachFloatEditor(Ctrl::Pan,
                          datamodel::pmd().asPercentBipolar().withName("Pan").withDefault(0),
                          mappingView.pan);
        addGlyph(Ctrl::Pan, sst::jucegui::components::GlyphPainter::PAN);

        attachFloatEditor(Ctrl::Pitch, datamodel::pitchTransposition().withName("Pitch"),
                          mappingView.pitchOffset);
        addGlyph(Ctrl::Pitch, sst::jucegui::components::GlyphPainter::TUNING);
    }

    engine::Zone::ZoneMappingData &mappingView;

    void resized() override
    {
        static constexpr int mapSize{620};
        static constexpr int headerSize{22};

        // Header
        auto b = getLocalBounds();
        zoneHeader->setBounds(b.withHeight(headerSize));

        // Mapping Display
        auto z = b.withTrimmedTop(headerSize);
        auto viewArea = z.withWidth(mapSize);
        mappingViewport->setBounds(viewArea);

        // Side Pane
        static constexpr int rowHeight{16}, rowMargin{4};
        static constexpr int typeinWidth{32};
        static constexpr int typeinPad{4}, typeinMargin{2};
        auto r = z.withLeft(mapSize + 2);
        auto cr = r.withHeight(rowHeight);

        auto co2 = [=](auto &c) {
            return c.withWidth(c.getWidth() - typeinPad - 2 * typeinWidth - 2 * typeinMargin);
        };
        auto c2 = [=](auto &c) {
            return c.withLeft(c.getRight() - typeinPad - typeinMargin - 2 * typeinWidth)
                .withWidth(typeinWidth);
        };
        auto co3 = [=](auto &c) {
            return c.withWidth(c.getWidth() - typeinPad - typeinWidth - typeinMargin);
        };
        auto c3 = [=](auto &c) {
            return c.withLeft(c.getRight() - typeinPad - typeinWidth).withWidth(typeinWidth);
        };

        labels[Ctrl::RootKey]->setBounds(co3(cr));
        textEds[Ctrl::RootKey]->setBounds(c3(cr));

        cr = cr.translated(0, rowHeight + rowMargin);
        textEds[KeyStart]->setBounds(c2(cr));
        textEds[KeyEnd]->setBounds(c3(cr));
        labels[KeyStart]->setBounds(co2(cr));

        cr = cr.translated(0, rowHeight + rowMargin);
        textEds[FadeStart]->setBounds(c2(cr));
        textEds[FadeEnd]->setBounds(c3(cr));
        labels[FadeStart]->setBounds(co2(cr));

        cr = cr.translated(0, rowHeight + rowMargin);
        textEds[VelStart]->setBounds(c2(cr));
        textEds[VelEnd]->setBounds(c3(cr));
        labels[VelStart]->setBounds(co2(cr));

        cr = cr.translated(0, rowHeight + rowMargin);
        textEds[VelFadeStart]->setBounds(c2(cr));
        textEds[VelFadeEnd]->setBounds(c3(cr));
        labels[VelFadeStart]->setBounds(co2(cr));

        cr = cr.translated(0, rowHeight + rowMargin);
        textEds[PBDown]->setBounds(c2(cr));
        textEds[PBUp]->setBounds(c3(cr));
        labels[PBDown]->setBounds(co2(cr));

        auto cQ = [&](int i) {
            auto w = cr.getWidth() / 4.0;
            return cr.withTrimmedLeft(w * i).withWidth(w).reduced(1);
        };
        cr = cr.translated(0, rowHeight + rowMargin);
        //  (volume)

        cr = cr.translated(0, rowHeight + rowMargin);
        glyphs[Level]->setBounds(cQ(2));
        textEds[Level]->setBounds(cQ(3));

        cr = cr.translated(0, rowHeight + rowMargin);
        glyphs[Pan]->setBounds(cQ(2));
        textEds[Pan]->setBounds(cQ(3));

        cr = cr.translated(0, rowHeight + rowMargin);
        glyphs[Pitch]->setBounds(cQ(2));
        textEds[Pitch]->setBounds(cQ(3));
    }

    void mappingChangedFromGUI()
    {
        sendToSerialization(cmsg::MappingSelectedZoneUpdateRequest(mappingView));
    }

    void mappingChangedFromGUI(const zone_attachment_t &at)
    {
        sendToSerialization(cmsg::MappingSelectedZoneUpdateRequest(mappingView));
    }
    void mappingChangedFromGUI(const zone_float_attachment_t &at)
    {
        sendToSerialization(cmsg::MappingSelectedZoneUpdateRequest(mappingView));
    }

    void setActive(bool b)
    {
        for (const auto &[k, l] : labels)
            l->setVisible(b);
        for (const auto &[k, t] : textEds)
            t->setVisible(b);
        for (const auto &[k, g] : glyphs)
            g->setVisible(b);
    }

    void setGroupZoneMappingSummary(const engine::Part::zoneMappingSummary_t &d)
    {
        summary = d;
        if (editor->currentLeadZoneSelection.has_value())
            setLeadSelection(*(editor->currentLeadZoneSelection));
        if (zonesAndKeyboard)
            zonesAndKeyboard->repaint();
        repaint();
    }

    void setLeadSelection(const selection::SelectionManager::ZoneAddress &za)
    {
        // but we need to call setLeadZoneBounds to make the hotspots so rename that too
        for (const auto &s : summary)
        {
            if (s.first == za)
            {
                if (zonesAndKeyboard)
                    zonesAndKeyboard->setLeadZoneBounds(s.second);
            }
        }
    }

    int voiceCountFor(const selection::SelectionManager::ZoneAddress &z)
    {
        int res{0};
        for (const auto &v : editor->sharedUiMemoryState.voiceDisplayItems)
        {
            if (v.active && v.part == z.part && v.group == z.group && v.zone == z.zone)
            {
                res++;
            }
        }
        return res;
    }

    std::optional<std::string>
    sourceDetailsDragAndDropSample(const SourceDetails &dragSourceDetails)
    {
        auto w = dragSourceDetails.sourceComponent;
        if (w)
        {
            auto p = w->getProperties().getVarPointer("DragAndDropSample");
            if (p && p->isString())
            {
                return p->toString().toStdString();
            }
        }
        return std::nullopt;
    }

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
    {
        auto os = sourceDetailsDragAndDropSample(dragSourceDetails);
        return os.has_value();
    }

    void itemDropped(const SourceDetails &dragSourceDetails) override
    {
        auto os = sourceDetailsDragAndDropSample(dragSourceDetails);
        if (os.has_value())
        {
            namespace cmsg = scxt::messaging::client;
            auto r = zonesAndKeyboard->rootAndRangeForPosition(dragSourceDetails.localPosition);
            sendToSerialization(cmsg::AddSampleWithRange({*os, r[0], r[1], r[2], 0, 127}));
        }
        isUndertakingDrop = false;
        repaint();
    }

    bool isUndertakingDrop{false};
    juce::Point<int> currentDragPoint;
    void itemDragEnter(const SourceDetails &dragSourceDetails) override
    {
        // TODO Compound types are special here
        isUndertakingDrop = true;
        currentDragPoint = dragSourceDetails.localPosition;
        repaint();
    }

    void itemDragExit(const SourceDetails &dragSourceDetails) override
    {
        isUndertakingDrop = false;
        repaint();
    }

    void itemDragMove(const SourceDetails &dragSourceDetails) override
    {
        currentDragPoint = dragSourceDetails.localPosition;
        repaint();
    }

    engine::Part::zoneMappingSummary_t summary{};
};

MappingZonesAndKeyboard::MappingZonesAndKeyboard(scxt::ui::multi::MappingDisplay *d)
    : HasEditor(d->editor), display(d)
{
}

void MappingZonesAndKeyboard::mouseDown(const juce::MouseEvent &e)
{
    if (!display)
        return;
    mouseState = NONE;
    if (e.position.y > keyboardHeight)
    {
        for (int i = firstMidiNote; i < lastMidiNote; ++i)
        {
            auto r = rectangleForKey(i);
            if (r.contains(e.position))
            {
                sendToSerialization(cmsg::NoteFromGUI({i, true}));
                heldNote = i;
                mouseState = HELD_NOTE;
                repaint();
                return;
            }
        }
    }

    // const auto &sel = display->editor->currentSingleSelection;

    for (const auto &ks : keyboardHotZones)
    {
        if (ks.contains(e.position))
        {
            mouseState = DRAG_KEY;
            return;
        }
    }

    for (const auto &ks : velocityHotZones)
    {
        if (ks.contains(e.position))
        {
            mouseState = DRAG_VELOCITY;
            return;
        }
    }

    for (const auto &ks : bothHotZones)
    {
        if (ks.contains(e.position))
        {
            mouseState = DRAG_KEY_AND_VEL;
            return;
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
            potentialZones.push_back(z.first);
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
                display->editor->doSelectionAction(nextZone, true, false, true);
            }
            else
            {
                // single click makes it a single lead
                display->editor->doSelectionAction(nextZone, true, true, true);
            }
        }
        else
        {
            display->editor->doSelectionAction(
                nextZone, true, !(e.mods.isCommandDown() || e.mods.isAltDown()), true);
        }
    }
}

void MappingZonesAndKeyboard::mouseDrag(const juce::MouseEvent &e)
{
    if (mouseState == DRAG_SELECTED_ZONE)
    {
        auto lb = getLocalBounds().toFloat();
        auto displayRegion = lb.withTrimmedBottom(keyboardHeight);

        auto kw = displayRegion.getWidth() / (lastMidiNote - firstMidiNote + 1);
        auto vh = displayRegion.getHeight() / 127.0;

        auto dx = e.position.x - lastMousePos.x;
        auto &kr = display->mappingView.keyboardRange;
        auto nx = (int)(dx / kw) + firstMidiNote;

        if (kr.keyStart + nx < 0)
            nx = -kr.keyStart;
        else if (kr.keyEnd + nx > 127)
            nx = 127 - kr.keyEnd;
        if (nx != 0)
        {
            lastMousePos.x = e.position.x;
            kr.keyStart += nx;
            kr.keyEnd += nx;
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
        auto displayRegion = lb.withTrimmedBottom(keyboardHeight);
        auto kw = displayRegion.getWidth() / (lastMidiNote - firstMidiNote + 1);
        auto vh = displayRegion.getHeight() / 127.0;

        auto nx = std::clamp((int)std::round(e.position.x / kw) + firstMidiNote, 0, 127);
        auto vy = std::clamp(127 - (int)std::round(e.position.y / vh), 0, 127);

        auto &vr = display->mappingView.velocityRange;
        auto &kr = display->mappingView.keyboardRange;

        if (mouseState == DRAG_KEY_AND_VEL || mouseState == DRAG_KEY)
        {
            auto drs = abs(kr.keyStart - nx);
            auto dre = abs(kr.keyEnd - nx);
            if (drs < dre)
                kr.keyStart = nx;
            else
                kr.keyEnd = nx;
            if (kr.keyStart > kr.keyEnd)
                std::swap(kr.keyStart, kr.keyEnd);
        }

        if (mouseState == DRAG_KEY_AND_VEL || mouseState == DRAG_VELOCITY)
        {
            auto vrs = abs(vr.velStart - vy);
            auto vre = abs(vr.velEnd - vy);
            if (vrs < vre)
                vr.velStart = vy;
            else
                vr.velEnd = vy;
            if (vr.velStart > vr.velEnd)
                std::swap(vr.velStart, vr.velEnd);
        }
        display->mappingChangedFromGUI();
        repaint();
    }

    if (mouseState == HELD_NOTE)
    {
        if (e.position.y > keyboardHeight)
        {
            for (int i = firstMidiNote; i < lastMidiNote; ++i)
            {
                auto r = rectangleForKey(i);
                if (r.contains(e.position))
                {
                    if (i == heldNote)
                    {
                        // that's OK!
                    }
                    else
                    {
                        if (heldNote > 0)
                        {
                            sendToSerialization(cmsg::NoteFromGUI({heldNote, false}));
                        }
                        heldNote = i;
                        sendToSerialization(cmsg::NoteFromGUI({i, true}));
                        repaint();
                    }
                }
            }
        }
    }
}

void MappingZonesAndKeyboard::mouseUp(const juce::MouseEvent &e)
{
    if (mouseState == HELD_NOTE && heldNote >= 0)
    {
        sendToSerialization(cmsg::NoteFromGUI({heldNote, false}));
        heldNote = -1;
        mouseState = NONE;
        repaint();
        return;
    }

    setMouseCursor(juce::MouseCursor::NormalCursor);
}
juce::Rectangle<float>
MappingZonesAndKeyboard::rectangleForZone(const engine::Part::zoneMappingItem_t &sum)
{
    const auto &[kb, vel, name] = sum;
    return rectangleForRange(kb.keyStart, kb.keyEnd, vel.velStart, vel.velEnd);
}

juce::Rectangle<float> MappingZonesAndKeyboard::rectangleForRange(int kL, int kH, int vL, int vH)
{
    auto lb = getLocalBounds().toFloat();
    auto displayRegion = lb.withTrimmedBottom(keyboardHeight);
    auto kw = displayRegion.getWidth() / (lastMidiNote - firstMidiNote - 1);
    auto vh = displayRegion.getHeight() / 127.0;

    float x0 = (kL - firstMidiNote) * kw;
    float x1 = (kH - firstMidiNote + 1) * kw;
    if (x1 < x0)
        std::swap(x1, x0);
    float y0 = (127 - vL) * vh;
    float y1 = (127 - vH) * vh;
    if (y1 < y0)
        std::swap(y1, y0);

    return {x0, y0, x1 - x0, y1 - y0};
}

juce::Rectangle<float> MappingZonesAndKeyboard::rectangleForKey(int midiNote)
{
    assert(lastMidiNote > firstMidiNote);
    auto lb = getLocalBounds().toFloat();
    auto keyRegion = lb.withTop(lb.getBottom() - keyboardHeight + 1);
    auto kw = keyRegion.getWidth() / (lastMidiNote - firstMidiNote - 1);

    keyRegion = keyRegion.withWidth(kw).translated(kw * (midiNote - firstMidiNote), 0);

    return keyRegion;
}

void MappingZonesAndKeyboard::paint(juce::Graphics &g)
{
    if (!display)
        g.fillAll(juce::Colours::red);

    // Draw the background
    {
        auto lb = getLocalBounds().toFloat();
        auto displayRegion = lb.withTrimmedBottom(keyboardHeight);

        auto dashCol = juce::Colour(80, 80, 80);
        g.setColour(dashCol);
        g.drawRect(displayRegion, 1.f);

        dashCol = juce::Colour(50, 50, 60);
        g.setColour(dashCol);

        auto dh = displayRegion.getHeight() / 4.0;
        for (int i = 1; i < 4; ++i)
        {
            g.drawHorizontalLine(i * dh, lb.getX(), lb.getX() + lb.getWidth());
        }

        auto oct = displayRegion.getWidth() / 127.0 * 12.0;
        for (int i = 0; i < 127; i += 12)
        {
            auto o = i / 12;
            g.drawVerticalLine(o * oct, lb.getY(), lb.getY() + lb.getHeight());
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

            auto nonSelZoneColor = juce::Colour(140, 140, 140);
            if (drawSelected)
                nonSelZoneColor = juce::Colour(0xFF, 0x90, 0x00);
            g.setColour(nonSelZoneColor.withAlpha(0.2f));
            g.fillRect(r);
            g.setColour(nonSelZoneColor);
            g.drawRect(r, 2.f);
            g.setColour(nonSelZoneColor.brighter());
            g.setFont(connectors::SCXTStyleSheetCreator::interMediumFor(12));
            g.drawText(std::get<2>(z.second), r.reduced(5, 3), juce::Justification::topLeft);

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

            auto r = rectangleForZone(z.second);

            auto selZoneColor = juce::Colour(0x00, 0x90, 0xFF);
            g.setColour(selZoneColor.withAlpha(0.2f));
            g.fillRect(r);

            g.setColour(selZoneColor);
            g.drawRect(r, 2.f);

            g.setColour(juce::Colours::white);
            g.setFont(connectors::SCXTStyleSheetCreator::interMediumFor(12));
            g.drawText(std::get<2>(z.second), r.reduced(5, 3), juce::Justification::topLeft);

            auto ct = display->voiceCountFor(z.first);
            drawVoiceMarkers(r, ct);
        }
    }

    std::array<int, 128> midiState; // 0 == 0ff, 1 == gated, 2 == sounding
    std::fill(midiState.begin(), midiState.end(), 0);
    for (const auto &vd : display->editor->sharedUiMemoryState.voiceDisplayItems)
    {
        if (vd.active && vd.midiNote >= 0)
        {
            midiState[vd.midiNote] = vd.gated ? 1 : 2;
        }
    }
    for (int i = firstMidiNote; i < lastMidiNote; ++i)
    {
        auto n = i % 12;
        auto isBlackKey = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
        auto kr = rectangleForKey(i);
        g.setColour(isBlackKey ? juce::Colours::black : juce::Colours::white);
        g.fillRect(kr);
        if (midiState[i] != 0)
        {
            g.setColour(midiState[i] == 1 ? juce::Colours::orange
                                          : (isBlackKey ? juce::Colour(0x80, 0x80, 0x90)
                                                        : juce::Colour(0xC0, 0xC0, 0xD0)));
            if (i == heldNote)
                g.setColour(juce::Colours::red);

            g.fillRect(kr);
        }
        g.setColour(juce::Colour(140, 140, 140));
        g.drawRect(kr, 0.5);
    }

    constexpr auto lastOctave = 11;
    for (int octave = 0; octave < lastOctave; ++octave)
    {
        assert(octave <= 10);
        auto r = rectangleForKey(octave * 12);
        r.setLeft(r.getX() + 1);
        r.setWidth(r.getWidth() * 12);
        g.setColour(juce::Colours::black);
        // defaultMidiNoteOctaveOffset = -1 => first octave is C-2
        auto offset = -1 + sst::basic_blocks::params::ParamMetaData::defaultMidiNoteOctaveOffset;
        g.drawText(fmt::format("C{}", octave + offset), r, juce::Justification::bottomLeft);
    }

    if (display->isUndertakingDrop)
    {
        auto rr = rootAndRangeForPosition(display->currentDragPoint);
        auto rb = rectangleForRange(rr[1], rr[2], 0, 127);
        g.setColour(juce::Colour(0xFF, 0x90, 0x00).withAlpha(0.4f));
        g.fillRect(rb);
    }
}

std::array<int16_t, 3> MappingZonesAndKeyboard::rootAndRangeForPosition(const juce::Point<int> &p)
{
    assert(lastMidiNote > firstMidiNote);
    auto lb = getLocalBounds().toFloat();
    auto keyRegion = lb.withTop(lb.getBottom() - keyboardHeight + 1);
    auto kw = keyRegion.getWidth() / (lastMidiNote - firstMidiNote - 1);

    auto rootKey =
        std::clamp(p.getX() * 1.f / kw + firstMidiNote, (float)firstMidiNote, (float)lastMidiNote);

    auto fromTop = std::clamp(p.getY(), 0, getHeight()) * 1.f / getHeight();
    auto span = (1.0f - fromTop) * 80 + 1;
    auto low = std::clamp(rootKey - span, 0.f, 127.f);
    auto high = std::clamp(rootKey + span, 0.f, 127.f);
    return {(int16_t)rootKey, (int16_t)low, (int16_t)high};
}

struct SampleDisplay;

struct SampleWaveform : juce::Component, HasEditor
{
    SampleDisplay *display{nullptr};
    SampleWaveform(SampleDisplay *d);
    void paint(juce::Graphics &g) override;
    void resized() override;

    juce::Path pathForSample();

    juce::Rectangle<int> startSampleHZ, endSampleHZ, startLoopHZ, endLoopHZ;
    void rebuildHotZones();

    // Anticipating future drag and so forth gestures
    enum struct MouseState
    {
        NONE,
        HZ_DRAG_SAMPSTART,
        HZ_DRAG_SAMPEND,
        HZ_DRAG_LOOPSTART,
        HZ_DRAG_LOOPEND,
    } mouseState{MouseState::NONE};

    int64_t sampleForXPixel(float xpos);
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseMove(const juce::MouseEvent &e) override;
};

struct SampleDisplay : juce::Component, HasEditor
{
    static constexpr int sidePanelWidth{150};
    enum Ctrl
    {
        startP,
        endP,
        startL,
        endL
    };

    std::unordered_map<Ctrl, std::unique_ptr<connectors::SamplePointDataAttachment>>
        sampleAttachments;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>>
        sampleEditors;

    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::Label>> labels;

    std::unique_ptr<connectors::BooleanPayloadDataAttachment<engine::Zone::AssociatedSampleArray>>
        loopAttachment, reverseAttachment;
    std::unique_ptr<sst::jucegui::components::ToggleButton> loopActive, reverseActive;

    std::unique_ptr<Zoomable> waveformViewport;
    std::unique_ptr<SampleWaveform> waveform;

    SampleDisplay(MappingPane *p)
        : HasEditor(p->editor), sampleView(p->sampleView), mappingView(p->mappingView)
    {
        playModeButton = std::make_unique<juce::TextButton>("mode");
        playModeButton->onClick = [this]() { showPlayModeMenu(); };
        addAndMakeVisible(*playModeButton);

        loopModeButton = std::make_unique<juce::TextButton>("loopmode");
        loopModeButton->onClick = [this]() { showLoopModeMenu(); };
        addAndMakeVisible(*loopModeButton);

        loopDirectionButton = std::make_unique<juce::TextButton>("loopdir");
        loopDirectionButton->onClick = [this]() { showLoopDirectionMenu(); };
        addAndMakeVisible(*loopDirectionButton);

        auto attachSamplePoint = [this](Ctrl c, const std::string &aLabel, auto &v) {
            auto at = std::make_unique<connectors::SamplePointDataAttachment>(
                v, [this](const auto &) { onSamplePointChangedFromGUI(); });
            auto sl = std::make_unique<sst::jucegui::components::DraggableTextEditableValue>();
            sl->setSource(at.get());
            addAndMakeVisible(*sl);
            sampleEditors[c] = std::move(sl);
            sampleAttachments[c] = std::move(at);
        };

        auto addLabel = [this](Ctrl c, const std::string &label) {
            auto l = std::make_unique<sst::jucegui::components::Label>();
            l->setText(label);
            addAndMakeVisible(*l);
            labels[c] = std::move(l);
        };

        attachSamplePoint(startP, "StartS", sampleView[0].startSample);
        addLabel(startP, "Sample Start");
        attachSamplePoint(endP, "EndS", sampleView[0].endSample);
        addLabel(endP, "Sample End");
        attachSamplePoint(startL, "StartL", sampleView[0].startLoop);
        addLabel(startL, "Loop Start");
        attachSamplePoint(endL, "EndL", sampleView[0].endLoop);
        addLabel(endL, "Loop End");

        loopAttachment = std::make_unique<
            connectors::BooleanPayloadDataAttachment<engine::Zone::AssociatedSampleArray>>(
            "Loop",
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->onSamplePointChangedFromGUI();
                }
            },
            sampleView[0].loopActive);
        loopActive = std::make_unique<sst::jucegui::components::ToggleButton>();
        loopActive->setLabel("Loop Active");
        loopActive->setSource(loopAttachment.get());
        addAndMakeVisible(*loopActive);

        reverseAttachment = std::make_unique<
            connectors::BooleanPayloadDataAttachment<engine::Zone::AssociatedSampleArray>>(
            "Reverse",
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->onSamplePointChangedFromGUI();
                }
            },
            sampleView[0].playReverse);
        reverseActive = std::make_unique<sst::jucegui::components::ToggleButton>();
        reverseActive->setLabel("Reverse");
        reverseActive->setSource(reverseAttachment.get());
        addAndMakeVisible(*reverseActive);

        waveform = std::make_unique<SampleWaveform>(this);

        waveformViewport = std::make_unique<Zoomable>(waveform.get(), std::make_pair(1.f, 50.f),
                                                      std::make_pair(1.f, 10.f));
        addAndMakeVisible(*waveformViewport);

        rebuild();
    }

    ~SampleDisplay() { reset(); }

    void reset()
    {
        for (auto &[k, c] : sampleEditors)
            c.reset();
    }

    void onSamplePointChangedFromGUI()
    {
        sendToSerialization(cmsg::SamplesSelectedZoneUpdateRequest(sampleView));
        waveform->rebuildHotZones();
        waveform->repaint();
        repaint();
    }

    juce::Rectangle<int> sampleDisplayRegion()
    {
        return getLocalBounds().withTrimmedRight(sidePanelWidth);
    }

    struct zoomFactor
    {
        float acc = 1.f;
        int val = 1;
    } zoomX, zoomY;

    void resized()
    {
        auto viewArea = sampleDisplayRegion();
        waveformViewport->setBounds(viewArea);

        auto p = getLocalBounds().withLeft(getLocalBounds().getWidth() - sidePanelWidth).reduced(2);

        p = p.withHeight(18);
        playModeButton->setBounds(p);
        p = p.translated(0, 20);

        auto w = p.getWidth() / 2;
        for (const auto m : {startP, endP, startL, endL})
        {
            sampleEditors[m]->setBounds(p.withLeft(p.getX() + w));
            labels[m]->setBounds(p.withWidth(w));
            p = p.translated(0, 20);
        }
        loopActive->setBounds(p);
        p = p.translated(0, 20);
        reverseActive->setBounds(p);
        p = p.translated(0, 20);
        loopModeButton->setBounds(p);
        p = p.translated(0, 20);
        loopDirectionButton->setBounds(p);
    }

    bool active{false};
    void setActive(bool b)
    {
        active = b;
        playModeButton->setVisible(b);
        loopActive->setVisible(b);
        loopModeButton->setVisible(b);
        reverseActive->setVisible(b);
        loopDirectionButton->setVisible(b);
        for (const auto &[k, p] : sampleEditors)
            p->setVisible(b);
        for (const auto &[k, l] : labels)
            l->setVisible(b);

        if (active)
            rebuild();
        repaint();
    }

    void rebuild()
    {
        switch (sampleView[0].playMode)
        {
        case engine::Zone::NORMAL:
            playModeButton->setButtonText("Normal");
            break;
        case engine::Zone::ONE_SHOT:
            playModeButton->setButtonText("Oneshot");
            break;
        case engine::Zone::ON_RELEASE:
            playModeButton->setButtonText("On Release (t/k)");
            break;
        }

        switch (sampleView[0].loopMode)
        {
        case engine::Zone::LOOP_DURING_VOICE:
            loopModeButton->setButtonText("Loop");
            break;
        case engine::Zone::LOOP_WHILE_GATED:
            loopModeButton->setButtonText("Loop While Gated");
            break;
        case engine::Zone::LOOP_FOR_COUNT:
            loopModeButton->setButtonText("For Count (t/k)");
            break;
        }
        switch (sampleView[0].loopDirection)
        {
        case engine::Zone::FORWARD_ONLY:
            loopDirectionButton->setButtonText("Loop Forward");
            break;
        case engine::Zone::ALTERNATE_DIRECTIONS:
            loopDirectionButton->setButtonText("Loop Alternate");
            break;
        }

        auto samp = editor->sampleManager.getSample(sampleView[0].sampleID);
        size_t end = 0;
        if (samp)
        {
            end = samp->getSampleLength();
        }

        for (const auto &[k, p] : sampleAttachments)
            p->sampleCount = end;
        repaint();

        waveform->rebuildHotZones();
    }

    void showPlayModeMenu()
    {
        juce::PopupMenu p;
        p.addSectionHeader("PlayMode");
        p.addSeparator();

        auto add = [&p, this](auto e, auto n) {
            p.addItem(n, true, sampleView[0].playMode == e, [this, e]() {
                sampleView[0].playMode = e;
                onSamplePointChangedFromGUI();
                rebuild();
            });
        };
        add(engine::Zone::PlayMode::NORMAL, "Normal");
        add(engine::Zone::PlayMode::ONE_SHOT, "OneShot");
        add(engine::Zone::PlayMode::ON_RELEASE, "On Release (t/k)");

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }
    void showLoopModeMenu()
    {
        juce::PopupMenu p;
        p.addSectionHeader("Loop Mode");
        p.addSeparator();

        auto add = [&p, this](auto e, auto n) {
            p.addItem(n, true, sampleView[0].loopMode == e, [this, e]() {
                sampleView[0].loopMode = e;
                onSamplePointChangedFromGUI();
                rebuild();
            });
        };
        add(engine::Zone::LoopMode::LOOP_DURING_VOICE, "Loop");
        add(engine::Zone::LoopMode::LOOP_WHILE_GATED, "Loop While Gated");
        add(engine::Zone::LoopMode::LOOP_FOR_COUNT, "For Count (t/k)");

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }
    void showLoopDirectionMenu()
    {
        juce::PopupMenu p;
        p.addSectionHeader("Loop Direction");
        p.addSeparator();

        auto add = [&p, this](auto e, auto n) {
            p.addItem(n, true, sampleView[0].loopDirection == e, [this, e]() {
                sampleView[0].loopDirection = e;
                onSamplePointChangedFromGUI();
                rebuild();
            });
        };
        add(engine::Zone::LoopDirection::FORWARD_ONLY, "Loop Forward");
        add(engine::Zone::LoopDirection::ALTERNATE_DIRECTIONS, "Loop Alternate");

        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    std::unique_ptr<juce::TextButton> playModeButton, loopModeButton, loopDirectionButton;
    engine::Zone::AssociatedSampleArray &sampleView;
    engine::Zone::ZoneMappingData &mappingView;
};

SampleWaveform::SampleWaveform(scxt::ui::multi::SampleDisplay *d)
    : display(d), HasEditor(d->editor){};

void SampleWaveform::rebuildHotZones()
{
    static constexpr int hotZoneSize{10};
    auto &v = display->sampleView[0];
    auto samp = editor->sampleManager.getSample(v.sampleID);
    if (!samp)
    {
        return;
    }
    auto r = getLocalBounds();
    auto l = samp->getSampleLength();
    auto fac = 1.0 * r.getWidth() / l;
    auto start = v.startSample * fac;
    auto end = v.endSample * fac;
    auto ls = v.startLoop * fac;
    auto le = v.endLoop * fac;

    startSampleHZ = juce::Rectangle<int>(start + r.getX(), r.getBottom() - hotZoneSize, hotZoneSize,
                                         hotZoneSize);
    endSampleHZ = juce::Rectangle<int>(end + r.getX() - hotZoneSize, r.getBottom() - hotZoneSize,
                                       hotZoneSize, hotZoneSize);
    startLoopHZ = juce::Rectangle<int>(ls + r.getX(), r.getY(), hotZoneSize, hotZoneSize);
    endLoopHZ =
        juce::Rectangle<int>(le + r.getX() - hotZoneSize, r.getY(), hotZoneSize, hotZoneSize);
}

int64_t SampleWaveform::sampleForXPixel(float xpos)
{
    // TODO probably cache this
    auto r = getLocalBounds();
    auto &v = display->sampleView[0];
    auto samp = editor->sampleManager.getSample(v.sampleID);
    auto l = samp->getSampleLength();
    return (int64_t)std::clamp(1.0 * l * xpos / r.getWidth(), 0.0, l * 1.0);
}

juce::Path SampleWaveform::pathForSample()
{
    juce::Path res;

    auto r = getLocalBounds();
    auto &v = display->sampleView[0];
    auto samp = editor->sampleManager.getSample(v.sampleID);
    if (!samp)
    {
        SCLOG("Null Sample: Null Path");
        return res;
    }

    auto l = samp->getSampleLength();
    std::vector<float> topLine, bottomLine;

    if (samp->bitDepth == sample::Sample::BD_I16)
    {
        auto fac = 1.0 * l / r.getWidth();
        auto d = samp->GetSamplePtrI16(0);
        double c = 0;
        int ct = 0;
        int16_t mx = std::numeric_limits<int16_t>::min();
        int16_t mn = std::numeric_limits<int16_t>::max();

        for (int s = 0; s < l; ++s)
        {
            if (c + fac < s)
            {
                double nmx = 1.0 - mx * 1.0 / std::numeric_limits<int16_t>::max();
                double nmn = 1.0 - mn * 1.0 / std::numeric_limits<int16_t>::max();

                nmx = (nmx + 1) * 0.25;
                nmn = (nmn + 1) * 0.25;

                topLine.push_back(nmx);
                bottomLine.push_back(nmn);

                c += fac;
                ct++;
                mx = std::numeric_limits<int16_t>::min();
                mn = std::numeric_limits<int16_t>::max();
            }
            mx = std::max(d[s], mx);
            mn = std::min(d[s], mn);
        }
    }
    else if (samp->bitDepth == sample::Sample::BD_F32)
    {
        auto fac = 1.0 * l / r.getWidth();
        auto d = samp->GetSamplePtrF32(0);
        double c = 0;
        int ct = 0;

        float mx = -100.f, mn = 100.f;

        for (int s = 0; s < l; ++s)
        {
            if (c + fac < s)
            {
                double nmx = 1.0 - mx;
                double nmn = 1.0 - mn;

                nmx = (nmx + 1) * 0.25;
                nmn = (nmn + 1) * 0.25;

                topLine.push_back(nmx);
                bottomLine.push_back(nmn);
                c += fac;
                ct++;
                mx = -100.f;
                mn = 100.f;
            }
            mx = std::max(d[s], mx);
            mn = std::min(d[s], mn);
        }
    }
    else
    {
        jassertfalse;
    }

    std::reverse(bottomLine.begin(), bottomLine.end());
    int index{0};
    for (auto &l : topLine)
    {
        if (index == 0)
        {
            res.startNewSubPath(index, l * r.getHeight());
        }
        else
        {
            res.lineTo(index, l * r.getHeight());
        }
        index++;
    }

    index--;
    for (auto &l : bottomLine)
    {
        res.lineTo(index, l * r.getHeight());
        index--;
    }
    res.closeSubPath();

    return res;
}

void SampleWaveform::mouseDown(const juce::MouseEvent &e)
{
    auto posi = e.position.roundToInt();
    if (startSampleHZ.contains(posi))
        mouseState = MouseState::HZ_DRAG_SAMPSTART;
    else if (endSampleHZ.contains(posi))
        mouseState = MouseState::HZ_DRAG_SAMPEND;
    // TODO loopActive check here
    else if (startLoopHZ.contains(posi))
        mouseState = MouseState::HZ_DRAG_LOOPSTART;
    else if (endLoopHZ.contains(posi))
        mouseState = MouseState::HZ_DRAG_LOOPEND;
    else
        mouseState = MouseState::NONE;

    // TODO cursor change and so on
}

void SampleWaveform::mouseDrag(const juce::MouseEvent &e)
{
    if (mouseState == MouseState::NONE)
        return;

    switch (mouseState)
    {
    case MouseState::HZ_DRAG_SAMPSTART:
        display->sampleView[0].startSample = sampleForXPixel(e.position.x);
        break;
    case MouseState::HZ_DRAG_SAMPEND:
        display->sampleView[0].endSample = sampleForXPixel(e.position.x);
        break;
    case MouseState::HZ_DRAG_LOOPSTART:
        display->sampleView[0].startLoop = sampleForXPixel(e.position.x);
        break;
    case MouseState::HZ_DRAG_LOOPEND:
        display->sampleView[0].endLoop = sampleForXPixel(e.position.x);
        break;
    default:
        break;
    }
    display->onSamplePointChangedFromGUI();
}

void SampleWaveform::mouseUp(const juce::MouseEvent &e)
{
    if (mouseState != MouseState::NONE)
        display->onSamplePointChangedFromGUI();
}

void SampleWaveform::mouseMove(const juce::MouseEvent &e)
{
    auto posi = e.position.roundToInt();
    if (startSampleHZ.contains(posi) || endSampleHZ.contains(posi) || startLoopHZ.contains(posi) ||
        endLoopHZ.contains(posi))
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return;
    }

    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void SampleWaveform::paint(juce::Graphics &g)
{
    if (!display->active)
        return;

    auto r = getLocalBounds();
    auto &v = display->sampleView[0];
    auto samp = editor->sampleManager.getSample(v.sampleID);
    if (!samp)
    {
        return;
    }

    auto l = samp->getSampleLength();
    auto fac = 1.0 * r.getWidth() / l;

    auto wfp = pathForSample();
    {
        juce::Graphics::ScopedSaveState gs(g);

        g.setColour(juce::Colour(0x30, 0x30, 0x40));
        g.fillRect(r);

        g.setColour(juce::Colour(0xFF, 0x90, 0x00).withAlpha(0.2f));
        g.fillPath(wfp);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.strokePath(wfp, juce::PathStrokeType(1.f));
    }

    {
        juce::Graphics::ScopedSaveState gs(g);
        auto cr = r.withLeft(fac * v.startSample).withRight(fac * v.endSample);
        g.reduceClipRegion(cr);

        g.setColour(juce::Colour(0x15, 0x15, 0x15));
        g.fillRect(r);

        g.setColour(juce::Colour(0xFF, 0x90, 0x00));
        g.fillPath(wfp);

        g.setColour(juce::Colours::white);
        g.strokePath(wfp, juce::PathStrokeType(1.f));
    }

    if (v.loopActive)
    {
        juce::Graphics::ScopedSaveState gs(g);
        auto cr = r.withLeft(fac * v.startLoop).withRight(fac * v.endLoop);
        g.reduceClipRegion(cr);

        g.setColour(juce::Colour(0x15, 0x15, 0x35));
        g.fillRect(r);

        g.setColour(juce::Colour(0x90, 0x90, 0xFF));
        g.fillPath(wfp);

        g.setColour(juce::Colours::white);
        g.strokePath(wfp, juce::PathStrokeType(1.f));
    }

    g.setColour(juce::Colours::white);
    g.fillRect(startSampleHZ);
    g.drawVerticalLine(startSampleHZ.getX(), 0, getHeight());
    g.fillRect(endSampleHZ);
    g.drawVerticalLine(endSampleHZ.getRight(), 0, getHeight());

    if (v.loopActive)
    {
        g.setColour(juce::Colours::aliceblue);
        g.fillRect(startLoopHZ);
        g.drawVerticalLine(startLoopHZ.getX(), 0, getHeight());
        g.fillRect(endLoopHZ);
        g.drawVerticalLine(endLoopHZ.getRight(), 0, getHeight());
    }
    g.setColour(juce::Colours::white);
    g.drawRect(r, 1);
}

void SampleWaveform::resized() { rebuildHotZones(); }

struct MacroDisplay : juce::Component
{
    void paint(juce::Graphics &g)
    {
        g.setColour(juce::Colours::yellow);
        g.setFont(juce::Font("Comic Sans MS", 50, juce::Font::plain));
        g.drawText("Macro Region", getLocalBounds(), juce::Justification::centred);
    }
};

MappingPane::MappingPane(SCXTEditor *e) : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    hasHamburger = true;
    isTabbed = true;
    tabNames = {"MAPPING", "SAMPLE", "MACROS"};

    mappingDisplay = std::make_unique<MappingDisplay>(this);
    addAndMakeVisible(*mappingDisplay);

    sampleDisplay = std::make_unique<SampleDisplay>(this);
    addChildComponent(*sampleDisplay);

    macroDisplay = std::make_unique<MacroDisplay>();
    addChildComponent(*macroDisplay);

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (wt)
        {
            wt->mappingDisplay->setVisible(i == 0);
            wt->sampleDisplay->setVisible(i == 1);
            wt->macroDisplay->setVisible(i == 2);
        }
    };
}

MappingPane::~MappingPane() {}

void MappingPane::resized()
{
    mappingDisplay->setBounds(getContentArea());
    sampleDisplay->setBounds(getContentArea());
    macroDisplay->setBounds(getContentArea());
}

void MappingPane::setMappingData(const engine::Zone::ZoneMappingData &m)
{
    mappingView = m;
    mappingDisplay->repaint();
}

void MappingPane::setSampleData(const engine::Zone::AssociatedSampleArray &m)
{
    sampleView = m;
    sampleDisplay->rebuild();
    sampleDisplay->repaint();
}

void MappingPane::setActive(bool b)
{
    mappingDisplay->setActive(b);
    sampleDisplay->setActive(b);
}

void MappingPane::setGroupZoneMappingSummary(const engine::Part::zoneMappingSummary_t &d)
{
    mappingDisplay->setGroupZoneMappingSummary(d);
}

void MappingPane::editorSelectionChanged()
{
    if (editor->currentLeadZoneSelection.has_value())
        mappingDisplay->setLeadSelection(*(editor->currentLeadZoneSelection));
    repaint();
}

void MappingPane::invertScroll(bool invert)
{
    mappingDisplay->mappingViewport->invertScroll(invert);
    sampleDisplay->waveformViewport->invertScroll(invert);
}

} // namespace scxt::ui::multi
