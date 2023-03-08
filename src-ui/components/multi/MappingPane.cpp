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
#include "connectors/PayloadDataAttachment.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"

#include "components/GlyphPainter.h"

namespace scxt::ui::multi
{

namespace cmsg = scxt::messaging::client;

struct MappingDisplay;
struct MappingZonesAndKeyboard : juce::Component
{
    MappingDisplay *display{nullptr};
    MappingZonesAndKeyboard(MappingDisplay *d) : display(d) {}
    void paint(juce::Graphics &g);
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
        g.drawText("Zones Header " + std::to_string(paints++), getLocalBounds(),
                   juce::Justification::centred);
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

    MappingDisplay(SCXTEditor *e) : HasEditor(e)
    {
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
    void paint(juce::Graphics &g) override
    {
        g.setColour(juce::Colours::orchid);
        g.setFont(juce::Font("Comic Sans MS", 50, juce::Font::plain));
        g.drawText("Mapping Region", getLocalBounds(), juce::Justification::centred);
    }

    engine::Zone::ZoneMappingData mappingView;

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
        zonesAndKeyboard->repaint();
        repaint();
    }
    engine::Group::zoneMappingSummary_t summary{};
};

void MappingZonesAndKeyboard::paint(juce::Graphics &g)
{
    if (!display)
        g.fillAll(juce::Colours::red);

    auto lb = getLocalBounds().toFloat();
    auto displayRegion = lb.withTrimmedBottom(15);
    auto kw = displayRegion.getWidth() / 127.0;
    auto vh = displayRegion.getHeight() / 127.0;

    for (const auto &[kb, vel, idx, name] : display->summary)
    {
        auto x0 = kb.keyStart * kw;
        auto x1 = kb.keyEnd * kw;
        if (x1 < x0)
            std::swap(x1, x0);
        auto y0 = (127 - vel.velStart) * vh;
        auto y1 = (127 - vel.velEnd) * vh;
        if (y1 < y0)
            std::swap(y1, y0);
        g.setColour(juce::Colours::yellow.withAlpha(0.2f));
        auto r = juce::Rectangle<float>(x0, y0, x1 - x0, y1 - y0);
        g.fillRect(r);
        g.setColour(juce::Colours::yellow);
        g.drawRect(r, 2.f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font("Comic Sans MS", 12, juce::Font::plain));
        g.drawText(name, r, juce::Justification::centred);
    }
}

struct SampleDisplay : juce::Component
{
    void paint(juce::Graphics &g)
    {
        g.setColour(juce::Colours::pink);
        g.setFont(juce::Font("Comic Sans MS", 50, juce::Font::plain));
        g.drawText("Sample Region", getLocalBounds(), juce::Justification::centred);
    }
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

    mappingDisplay = std::make_unique<MappingDisplay>(editor);
    addAndMakeVisible(*mappingDisplay);

    sampleDisplay = std::make_unique<SampleDisplay>();
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
    mappingDisplay->mappingView = m;
    mappingDisplay->repaint();
}

void MappingPane::setActive(bool b) { mappingDisplay->setActive(b); }

void MappingPane::setGroupZoneMappingSummary(const engine::Group::zoneMappingSummary_t &d)
{
    mappingDisplay->setGroupZoneMappingSummary(d);
}
} // namespace scxt::ui::multi