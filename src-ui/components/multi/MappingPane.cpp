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
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "connectors/PayloadDataAttachment.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"

#include "components/GlyphPainter.h"
#include "connectors/SCXTStyleSheetCreator.h"

namespace scxt::ui::multi
{

namespace cmsg = scxt::messaging::client;

struct MappingDisplay;
struct MappingZonesAndKeyboard : juce::Component
{
    MappingDisplay *display{nullptr};
    MappingZonesAndKeyboard(MappingDisplay *d) : display(d) {}
    void paint(juce::Graphics &g) override;

    static constexpr int keyboardHeight{25};
    juce::Rectangle<float> rectangleForZone(const engine::Group::zoneMappingItem_t &sum);
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

    void setActiveZone(const engine::Group::zoneMappingItem_t &az)
    {
        auto r = rectangleForZone(az);
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
        DRAG_KEY_AND_VEL
    } mouseState{NONE};

    std::vector<juce::Rectangle<float>> velocityHotZones, keyboardHotZones, bothHotZones;
    int32_t heldNote{-1};
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

struct MappingDisplay : juce::Component, HasEditor
{
    typedef connectors::PayloadDataAttachment<MappingDisplay, engine::Zone::ZoneMappingData,
                                              int16_t>
        zone_attachment_t;

    typedef connectors::PayloadDataAttachment<MappingDisplay, engine::Zone::ZoneMappingData, float>
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
    std::unique_ptr<MappingZonesAndKeyboard> zonesAndKeyboard;
    std::unique_ptr<MappingZoneHeader> zoneHeader;

    std::unordered_map<Ctrl, std::unique_ptr<zone_attachment_t>> attachments;
    std::unordered_map<Ctrl, std::unique_ptr<zone_float_attachment_t>> floatattachments;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>>
        textEds;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::Label>> labels;
    std::unordered_map<Ctrl, std::unique_ptr<glyph::GlyphPainter>> glyphs;

    MappingDisplay(MappingPane *p) : HasEditor(p->editor), mappingView(p->mappingView)
    {
        // TODO: Upgrade all these attachments with the new factory style
        zonesAndKeyboard = std::make_unique<MappingZonesAndKeyboard>(this);
        addAndMakeVisible(*zonesAndKeyboard);
        zoneHeader = std::make_unique<MappingZoneHeader>();
        addAndMakeVisible(*zoneHeader);
        auto attachEditor = [this](Ctrl c, const std::string &aLabel, const auto &desc,
                                   const auto &fn, auto &v) {
            auto at = std::make_unique<zone_attachment_t>(
                this, desc, aLabel, [this](const auto &at) { this->mappingChangedFromGUI(at); }, fn,
                v);
            auto sl = std::make_unique<sst::jucegui::components::DraggableTextEditableValue>();
            sl->setSource(at.get());
            addAndMakeVisible(*sl);
            textEds[c] = std::move(sl);
            attachments[c] = std::move(at);
        };

        auto attachFloatEditor = [this](Ctrl c, const std::string &aLabel, const auto &desc,
                                        const auto &fn, auto &v) {
            auto at = std::make_unique<zone_float_attachment_t>(
                this, desc, aLabel, [this](const auto &at) { this->mappingChangedFromGUI(at); }, fn,
                v);
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

        auto addGlyph = [this](Ctrl c, glyph::GlyphPainter::Glyph g) {
            // TODO StyleSheet
            auto l = std::make_unique<glyph::GlyphPainter>(g, juce::Colours::white);
            addAndMakeVisible(*l);
            glyphs[c] = std::move(l);
        };

        attachEditor(
            Ctrl::RootKey, "RootKey", datamodel::cdMidiNote,
            [](const auto &pl) { return pl.rootKey; }, mappingView.rootKey);
        addLabel(Ctrl::RootKey, "Root Key");
        attachments[Ctrl::RootKey]->setAsMidiNote();

        attachEditor(
            Ctrl::KeyStart, "Key Start", datamodel::cdMidiNote,
            [](const auto &pl) { return pl.keyboardRange.keyStart; },
            mappingView.keyboardRange.keyStart);
        attachments[Ctrl::KeyStart]->setAsMidiNote();
        attachEditor(
            Ctrl::KeyEnd, "Key End", datamodel::cdMidiNote,
            [](const auto &pl) { return pl.keyboardRange.keyEnd; },
            mappingView.keyboardRange.keyEnd);
        attachments[Ctrl::KeyEnd]->setAsMidiNote();
        addLabel(Ctrl::KeyStart, "Key Range");
        attachEditor(
            Ctrl::FadeStart, "Fade Start", datamodel::cdMidiDistance,
            [](const auto &pl) { return pl.keyboardRange.fadeStart; },
            mappingView.keyboardRange.fadeStart);
        attachments[Ctrl::FadeStart]->setAsInteger();
        attachEditor(
            Ctrl::FadeEnd, "Fade End", datamodel::cdMidiDistance,
            [](const auto &pl) { return pl.keyboardRange.fadeEnd; },
            mappingView.keyboardRange.fadeEnd);
        attachments[Ctrl::FadeEnd]->setAsInteger();
        addLabel(Ctrl::FadeStart, "Crossfade");

        attachEditor(
            Ctrl::VelStart, "Vel Start", datamodel::cdMidiNote,
            [](const auto &pl) { return pl.velocityRange.velStart; },
            mappingView.velocityRange.velStart);
        attachments[Ctrl::VelStart]->setAsInteger();
        attachEditor(
            Ctrl::VelEnd, "Vel End", datamodel::cdMidiNote,
            [](const auto &pl) { return pl.velocityRange.velEnd; },
            mappingView.velocityRange.velEnd);
        attachments[Ctrl::VelEnd]->setAsInteger();
        addLabel(Ctrl::VelStart, "Vel Range");

        attachEditor(
            Ctrl::VelFadeStart, "Vel Fade Start", datamodel::cdMidiDistance,
            [](const auto &pl) { return pl.velocityRange.fadeStart; },
            mappingView.velocityRange.fadeStart);
        attachments[Ctrl::VelFadeStart]->setAsInteger();
        attachEditor(
            Ctrl::VelFadeEnd, "Vel Fade End", datamodel::cdMidiDistance,
            [](const auto &pl) { return pl.velocityRange.fadeEnd; },
            mappingView.velocityRange.fadeEnd);
        attachments[Ctrl::VelFadeEnd]->setAsInteger();
        addLabel(Ctrl::VelFadeStart, "Crossfade");

        attachEditor(
            Ctrl::PBDown, "PBDown", datamodel::cdMidiDistance,
            [](const auto &pl) { return pl.pbDown; }, mappingView.pbDown);
        attachments[Ctrl::PBDown]->setAsInteger();
        attachEditor(
            Ctrl::PBUp, "PBUp", datamodel::cdMidiDistance, [](const auto &pl) { return pl.pbUp; },
            mappingView.pbUp);
        attachments[Ctrl::PBUp]->setAsInteger();
        addLabel(Ctrl::PBDown, "Pitch Bend");

        attachFloatEditor(
            Ctrl::Pan, "Pan", datamodel::cdPercentBipolar,
            [](const auto &pl) -> float { return pl.pan; }, mappingView.pan);
        addGlyph(Ctrl::Pan, glyph::GlyphPainter::PAN);

        attachFloatEditor(
            Ctrl::Pitch, "Pitch", datamodel::cdMidiDistanceBipolar,
            [](const auto &pl) { return pl.pitchOffset; }, mappingView.pitchOffset);
        addGlyph(Ctrl::Pitch, glyph::GlyphPainter::TUNING);
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
        zonesAndKeyboard->setBounds(z.withWidth(mapSize));

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
        glyphs[Pan]->setBounds(cQ(2));
        textEds[Pan]->setBounds(cQ(3));

        cr = cr.translated(0, rowHeight + rowMargin);
        glyphs[Pitch]->setBounds(cQ(2));
        textEds[Pitch]->setBounds(cQ(3));
    }

    void mappingChangedFromGUI()
    {
        cmsg::clientSendToSerialization(cmsg::MappingSelectedZoneUpdateRequest(mappingView),
                                        editor->msgCont);
    }

    void mappingChangedFromGUI(const zone_attachment_t &at)
    {
        cmsg::clientSendToSerialization(cmsg::MappingSelectedZoneUpdateRequest(mappingView),
                                        editor->msgCont);
    }
    void mappingChangedFromGUI(const zone_float_attachment_t &at)
    {
        cmsg::clientSendToSerialization(cmsg::MappingSelectedZoneUpdateRequest(mappingView),
                                        editor->msgCont);
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

    void setGroupZoneMappingSummary(const engine::Group::zoneMappingSummary_t &d)
    {
        summary = d;
        setCurrentSelection(activeAddress);
        zonesAndKeyboard->repaint();
        repaint();
    }

    void setCurrentSelection(const selection::SelectionManager::ZoneAddress &za)
    {
        activeAddress = za;
        for (const auto &s : summary)
        {
            if (std::get<2>(s) == za.zone)
            {
                zonesAndKeyboard->setActiveZone(s);
            }
        }
    }

    engine::Group::zoneMappingSummary_t summary{};
    selection::SelectionManager::ZoneAddress activeAddress{};
};

void MappingZonesAndKeyboard::mouseDown(const juce::MouseEvent &e)
{
    if (!display)
        return;
    mouseState = NONE;
    if (e.position.y > keyboardHeight)
    {
        for (int i = 0; i < 128; ++i)
        {
            auto r = rectangleForKey(i);
            if (r.contains(e.position))
            {
                cmsg::clientSendToSerialization(cmsg::NoteFromGUI({i, true}),
                                                display->editor->msgCont);
                heldNote = i;
                mouseState = HELD_NOTE;
                repaint();
                return;
            }
        }
    }

    const auto &sel = display->editor->currentSingleSelection;

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

    std::vector<int> potentialZones;
    for (auto &z : display->summary)
    {
        auto r = rectangleForZone(z);
        if (r.contains(e.position))
            potentialZones.push_back(std::get<2>(z));
    }
    if (!potentialZones.empty())
    {
        if (potentialZones.size() == 1)
        {
            auto newSel = display->editor->currentSingleSelection;
            newSel.zone = potentialZones[0];
            display->editor->singleSelectItem(newSel);
        }
        else
        {
            // OK so now we have a stack. Basically what we want to
            // do is choose the 'next' one after our currently
            // selected as a heuristic
            auto cz = display->editor->currentSingleSelection.zone;

            auto selThis = -1;
            for (auto l : potentialZones)
            {
                if (l > display->editor->currentSingleSelection.zone && selThis < 0)
                    selThis = l;
            }

            if (selThis < 0)
                selThis = potentialZones.front();
            auto newSel = display->editor->currentSingleSelection;
            newSel.zone = selThis;
            display->editor->singleSelectItem(newSel);
        }
    }
}

void MappingZonesAndKeyboard::mouseDrag(const juce::MouseEvent &e)
{
    if (mouseState == DRAG_VELOCITY || mouseState == DRAG_KEY || mouseState == DRAG_KEY_AND_VEL)
    {
        auto lb = getLocalBounds().toFloat();
        auto displayRegion = lb.withTrimmedBottom(keyboardHeight);
        auto kw = displayRegion.getWidth() / 127.0;
        auto vh = displayRegion.getHeight() / 127.0;

        auto nx = std::clamp((int)std::round(e.position.x / kw), 0, 127);
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
}

void MappingZonesAndKeyboard::mouseUp(const juce::MouseEvent &e)
{
    if (mouseState == HELD_NOTE && heldNote >= 0)
    {
        cmsg::clientSendToSerialization(cmsg::NoteFromGUI({heldNote, false}),
                                        display->editor->msgCont);
        heldNote = -1;
        mouseState = NONE;
        repaint();
        return;
    }

    setMouseCursor(juce::MouseCursor::NormalCursor);
}
juce::Rectangle<float>
MappingZonesAndKeyboard::rectangleForZone(const engine::Group::zoneMappingItem_t &sum)
{
    const auto &[kb, vel, x, name] = sum;

    auto lb = getLocalBounds().toFloat();
    auto displayRegion = lb.withTrimmedBottom(keyboardHeight);
    auto kw = displayRegion.getWidth() / 127.0;
    auto vh = displayRegion.getHeight() / 127.0;

    float x0 = kb.keyStart * kw;
    float x1 = (kb.keyEnd + 1) * kw;
    if (x1 < x0)
        std::swap(x1, x0);
    float y0 = (127 - vel.velStart) * vh;
    float y1 = (127 - vel.velEnd) * vh;
    if (y1 < y0)
        std::swap(y1, y0);

    return {x0, y0, x1 - x0, y1 - y0};
}

juce::Rectangle<float> MappingZonesAndKeyboard::rectangleForKey(int midiNote)
{
    auto lb = getLocalBounds().toFloat();
    auto keyRegion = lb.withTop(lb.getBottom() - keyboardHeight + 1);
    auto kw = keyRegion.getWidth() / 127.0;

    keyRegion = keyRegion.withWidth(kw).translated(kw * midiNote, 0);

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

    const auto &sel = display->editor->currentSingleSelection;

    for (const auto &z : display->summary)
    {
        if (std::get<2>(z) == sel.zone)
            continue;

        auto r = rectangleForZone(z);

        auto nonSelZoneColor = juce::Colour(140, 140, 140);
        g.setColour(nonSelZoneColor.withAlpha(0.2f));
        g.fillRect(r);
        g.setColour(nonSelZoneColor);
        g.drawRect(r, 2.f);
        g.setColour(nonSelZoneColor.brighter());
        g.setFont(connectors::SCXTStyleSheetCreator::interMediumFor(12));
        g.drawText(std::get<3>(z), r.reduced(5, 3), juce::Justification::topLeft);
    }

    // Draw the selected zone
    if (sel.zone >= 0 && sel.zone < display->summary.size())
    {
        assert(sel.zone < display->summary.size());
        const auto &z = display->summary[sel.zone];

        auto r = rectangleForZone(z);

        auto selZoneColor = juce::Colour(0xFF, 0x90, 0x00);
        g.setColour(selZoneColor.withAlpha(0.2f));
        g.fillRect(r);

        g.setColour(selZoneColor);
        g.drawRect(r, 2.f);

        g.setColour(juce::Colours::white);
        g.setFont(connectors::SCXTStyleSheetCreator::interMediumFor(12));
        g.drawText(std::get<3>(z), r.reduced(5, 3), juce::Justification::topLeft);
    }
    else if (sel.zone >= 0)
    {
        SCDBGCOUT << "Inconsistent zone and display data skipping paint" << SCD(sel.zone)
                  << SCD(display->summary.size()) << std::endl;
    }

    for (int i = 0; i < 128; ++i)
    {
        auto n = i % 12;
        auto isBlackKey = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
        auto kr = rectangleForKey(i);
        g.setColour(isBlackKey ? juce::Colours::black : juce::Colours::white);
        if (i == heldNote)
            g.setColour(juce::Colours::red);
        g.fillRect(kr);
        g.setColour(juce::Colour(140, 140, 140));
        g.drawRect(kr, 0.5);
    }
}

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

    std::unique_ptr<connectors::BooleanPayloadDataAttachment<SampleDisplay,
                                                             engine::Zone::AssociatedSampleArray>>
        loopAttachment, reverseAttachment;
    std::unique_ptr<sst::jucegui::components::ToggleButton> loopActive, reverseActive;

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

        rebuild();

        auto attachSamplePoint = [this](Ctrl c, const std::string &aLabel, auto &v) {
            auto at = std::make_unique<connectors::SamplePointDataAttachment>(
                v, [this](const auto &) { onSamplePointChangedFromGUI(); });
            auto sl = std::make_unique<sst::jucegui::components::DraggableTextEditableValue>();
            sl->setSource(at.get());
            addAndMakeVisible(*sl);
            sampleEditors[c] = std::move(sl);
            sampleAttachments[c] = std::move(at);
        };
        attachSamplePoint(startP, "StartS", sampleView[0].startSample);
        attachSamplePoint(endP, "EndS", sampleView[0].endSample);
        attachSamplePoint(startL, "StartS", sampleView[0].startLoop);
        attachSamplePoint(endL, "EndS", sampleView[0].endLoop);

        loopAttachment = std::make_unique<connectors::BooleanPayloadDataAttachment<
            SampleDisplay, engine::Zone::AssociatedSampleArray>>(
            this, "Loop",
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->onSamplePointChangedFromGUI();
                }
            },
            [](const auto &pl) { return pl[0].loopActive; }, sampleView[0].loopActive);
        loopActive = std::make_unique<sst::jucegui::components::ToggleButton>();
        loopActive->setLabel("Loop Active");
        loopActive->setSource(loopAttachment.get());
        addAndMakeVisible(*loopActive);

        reverseAttachment = std::make_unique<connectors::BooleanPayloadDataAttachment<
            SampleDisplay, engine::Zone::AssociatedSampleArray>>(
            this, "Reverse",
            [w = juce::Component::SafePointer(this)](const auto &a) {
                if (w)
                {
                    w->onSamplePointChangedFromGUI();
                }
            },
            [](const auto &pl) { return pl[0].playReverse; }, sampleView[0].playReverse);
        reverseActive = std::make_unique<sst::jucegui::components::ToggleButton>();
        reverseActive->setLabel("Reverse");
        reverseActive->setSource(reverseAttachment.get());
        addAndMakeVisible(*reverseActive);
    }

    ~SampleDisplay() { reset(); }

    void reset()
    {
        for (auto &[k, c] : sampleEditors)
            c.reset();
    }

    void onSamplePointChangedFromGUI()
    {
        cmsg::clientSendToSerialization(cmsg::SamplesSelectedZoneUpdateRequest(sampleView),
                                        editor->msgCont);

        repaint();
    }

    void paint(juce::Graphics &g)
    {
        if (!active)
            return;

        g.setColour(juce::Colours::pink.withAlpha(0.2f));
        g.setFont(juce::Font("Comic Sans MS", 50, juce::Font::plain));
        g.drawText("Sample Region", getLocalBounds(), juce::Justification::centred);

        auto r = getLocalBounds().withTrimmedRight(sidePanelWidth);
        g.setColour(juce::Colours::white);
        g.drawRect(r, 1);

        auto &v = sampleView[0];
        auto samp = editor->sampleManager.getSample(v.sampleID);
        if (!samp)
        {
            g.setColour(juce::Colours::red);
            g.setFont(juce::Font("Comic Sans MS", 50, juce::Font::plain));
            g.drawText("NULL SAMPLE", getLocalBounds(), juce::Justification::centred);
            return;
        }

        auto l = samp->getSampleLength();

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

                    if (c >= v.startLoop && c <= v.endLoop)
                    {
                        g.setColour(juce::Colour(80, 80, 170));
                        if (!v.loopActive)
                            g.setColour(juce::Colour(40, 40, 90));
                        g.drawVerticalLine(ct, 0, getHeight());
                    }

                    if (c < v.startSample || c > v.endSample)
                    {
                        g.setColour(juce::Colour(60, 60, 80));
                        g.drawVerticalLine(ct, 0, getHeight());
                        g.setColour(juce::Colour(100, 100, 110));
                    }
                    else
                    {
                        g.setColour(juce::Colours::white);
                    }

                    g.drawVerticalLine(ct, getHeight() * nmx, getHeight() * nmn);
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

                    g.drawVerticalLine(ct, getHeight() * nmx, getHeight() * nmn);
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
    }

    void resized()
    {
        auto p = getLocalBounds().withLeft(getLocalBounds().getWidth() - sidePanelWidth).reduced(2);

        p = p.withHeight(18);
        playModeButton->setBounds(p);
        p = p.translated(0, 20);

        for (const auto m : {startP, endP, startL, endL})
        {
            sampleEditors[m]->setBounds(p);
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

        p.showMenuAsync({});
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

        p.showMenuAsync({});
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

        p.showMenuAsync({});
    }

    std::unique_ptr<juce::TextButton> playModeButton, loopModeButton, loopDirectionButton;
    engine::Zone::AssociatedSampleArray &sampleView;
    engine::Zone::ZoneMappingData &mappingView;
};

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

void MappingPane::setGroupZoneMappingSummary(const engine::Group::zoneMappingSummary_t &d)
{
    mappingDisplay->setGroupZoneMappingSummary(d);
}
void MappingPane::setCurrentSelection(const selection::SelectionManager::ZoneAddress &s)
{
    mappingDisplay->setCurrentSelection(s);
}
} // namespace scxt::ui::multi