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
#include "connectors/PayloadDataAttachment.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/client_messages.h"

namespace scxt::ui::multi
{

namespace cmsg = scxt::messaging::client;

struct MappingDisplay : juce::Component, HasEditor
{
    typedef connectors::PayloadDataAttachment<MappingDisplay, engine::Zone::ZoneMappingData,
                                              int16_t>
        zone_attachment_t;

    enum Ctrl
    {
        KeyStart,
        KeyEnd
    };
    std::unordered_map<Ctrl, std::unique_ptr<zone_attachment_t>> attachments;
    std::unordered_map<Ctrl, std::unique_ptr<sst::jucegui::components::DraggableTextEditableValue>>
        textEds;

    MappingDisplay(SCXTEditor *e) : HasEditor(e)
    {
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

        attachEditor(
            Ctrl::KeyStart, "Key Start",
            datamodel::ControlDescription{datamodel::ControlDescription::INT,
                                          datamodel::ControlDescription::LINEAR, 0, 1, 127},
            [](const auto &pl) { return pl.keyboardRange.keyStart; },
            mappingView.keyboardRange.keyStart);
        attachEditor(
            Ctrl::KeyEnd, "Key Ed",
            datamodel::ControlDescription{datamodel::ControlDescription::INT,
                                          datamodel::ControlDescription::LINEAR, 0, 1, 127},
            [](const auto &pl) { return pl.keyboardRange.keyEnd; },
            mappingView.keyboardRange.keyEnd);
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
        auto b = getLocalBounds();
        auto r = b.withLeft(b.getWidth() - 100).withHeight(20).translated(-2, 2);
        textEds[KeyStart]->setBounds(r);
        r = r.translated(0, 30);
        textEds[KeyEnd]->setBounds(r);
    }

    void mappingChangedFromGUI(const zone_attachment_t &at) {
        cmsg::clientSendToSerialization(cmsg::MappingSelectedZoneUpdateRequest(mappingView),
                                        editor->msgCont);
    }
};

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
} // namespace scxt::ui::multi