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

#include "PartSidebarCard.h"
#include "sst/jucegui/components/GlyphPainter.h"
#include "messaging/messaging.h"
#include "app/SCXTEditor.h"

namespace scxt::ui::app::shared
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

PartSidebarCard::PartSidebarCard(int p, SCXTEditor *e) : part(p), HasEditor(e)
{
    midiMode = std::make_unique<jcmp::TextPushButton>();
    midiMode->setLabel("MIDI");
    midiMode->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->showMidiModeMenu();
    });
    addAndMakeVisible(*midiMode);

    outBus = std::make_unique<jcmp::TextPushButton>();
    outBus->setLabel("OUT");
    outBus->setOnCallback(editor->makeComingSoon("Editing the output bus from here"));
    addAndMakeVisible(*outBus);

    polyCount = std::make_unique<jcmp::TextPushButton>();
    polyCount->setLabel("512");
    polyCount->setOnCallback(editor->makeComingSoon("Polyphony Limit/Monophony"));
    addAndMakeVisible(*polyCount);

    patchName = std::make_unique<jcmp::MenuButton>();
    patchName->setLabel("Instrument Name");
    patchName->setOnCallback(editor->makeComingSoon("Instrument save/load"));
    addAndMakeVisible(*patchName);

    auto onChange = [w = juce::Component::SafePointer(this)](const auto &a) {
        if (w)
        {
            w->resetFromEditorCache();
            w->sendToSerialization(
                cmsg::UpdatePartFullConfig({w->part, w->editor->partConfigurations[w->part]}));
        }
    };
    muteAtt =
        std::make_unique<boolattachment_t>("Mute", onChange, editor->partConfigurations[part].mute);
    mute = std::make_unique<jcmp::ToggleButton>();
    mute->setSource(muteAtt.get());
    mute->setLabel("M");
    addAndMakeVisible(*mute);

    auto soloOnChange = [w = juce::Component::SafePointer(this)](const auto &a) {
        if (w)
        {
            static bool everWarned{false};
            if (!everWarned)
            {
                w->editor->makeComingSoon("Solo on Parts")();
                everWarned = true;
            }
            w->resetFromEditorCache();
            w->sendToSerialization(
                cmsg::UpdatePartFullConfig({w->part, w->editor->partConfigurations[w->part]}));
        }
    };
    soloAtt = std::make_unique<boolattachment_t>("Solo", soloOnChange,
                                                 editor->partConfigurations[part].solo);
    solo = std::make_unique<jcmp::ToggleButton>();
    solo->setSource(soloAtt.get());
    solo->setLabel("S");
    addAndMakeVisible(*solo);

    level = connectors::makeConnectedToDummy<jcmp::HSliderFilled>(
        'ptlv', "Level", 1.0, false, editor->makeComingSoon("Editing the Part Level Control"));
    addAndMakeVisible(*level);
    pan = connectors::makeConnectedToDummy<jcmp::HSliderFilled>(
        'ptpn', "Pan", 0.0, true, editor->makeComingSoon("Editing the Part Pan Control"));
    addAndMakeVisible(*pan);
    tuning = connectors::makeConnectedToDummy<jcmp::HSliderFilled>(
        'pttn', "Tuning", 0.0, true, editor->makeComingSoon("Editing the part Tuning"));
    addAndMakeVisible(*tuning);
}

void PartSidebarCard::mouseDown(const juce::MouseEvent &event)
{
    if (event.mods.isPopupMenu())
    {
        showPopup();
        return;
    }
    sendToSerialization(cmsg::SelectPart(part));
}

void PartSidebarCard::showPopup()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Part " + std::to_string(part + 1));
    p.addSeparator();
    p.addItem("Deactivate Part", [p = this->part, w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::DeactivatePart(p));
    });
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void PartSidebarCard::paint(juce::Graphics &g)
{
    if (editor->getSelectedPart() == part)
    {
        g.setColour(editor->themeColor(theme::ColorMap::accent_1a));
    }
    else
    {
        g.setColour(editor->themeColor(theme::ColorMap::accent_1a).withAlpha(0.3f));
    }
    if (selfAccent)
    {
        auto rb = getLocalBounds().reduced(1);

        g.drawRoundedRectangle(rb.toFloat(), 2, 1);
    }
    if (editor->getSelectedPart() != part)
    {
        g.setColour(editor->themeColor(theme::ColorMap::generic_content_medium));
    }
    auto r = juce::Rectangle<int>(5, row0 + rowMargin, 18, rowHeight - 2 * rowMargin);
    g.setFont(editor->themeApplier.interMediumFor(12));
    g.drawText(std::to_string(part + 1), r, juce::Justification::centred);
    auto med = editor->themeColor(theme::ColorMap::generic_content_medium);
    r = r.translated(0, rowHeight);
    jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::GlyphType::MIDI, med);
    r = r.translated(0, rowHeight);
    jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::GlyphType::SPEAKER, med);
    r = r.translated(0, rowHeight);
    jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::GlyphType::POLYPHONY, med);

    r = juce::Rectangle<int>(66, row0 + rowMargin, 18, rowHeight - 2 * rowMargin);
    r = r.translated(0, rowHeight);
    jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::GlyphType::VOLUME, med);
    r = r.translated(0, rowHeight);
    jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::GlyphType::PAN, med);
    r = r.translated(0, rowHeight);
    jcmp::GlyphPainter::paintGlyph(g, r, jcmp::GlyphPainter::GlyphType::TUNING, med);

    auto &cfg = editor->partConfigurations[part];
    if (!cfg.active)
    {
        g.setColour(juce::Colour(255, 0, 0).withAlpha(0.1f));
        g.fillRect(getLocalBounds());
    }
}

void PartSidebarCard::resized()
{
    auto sideBits = juce::Rectangle<int>(5 + 18 + 2, row0 + rowMargin + rowHeight, 35,
                                         rowHeight - 2 * rowMargin);
    midiMode->setBounds(sideBits);
    sideBits = sideBits.translated(0, rowHeight);
    outBus->setBounds(sideBits);
    sideBits = sideBits.translated(0, rowHeight);
    polyCount->setBounds(sideBits);

    auto nameR =
        juce::Rectangle<int>(5 + 18 + 2, row0 + rowMargin + 1, 103, rowHeight - 2 * rowMargin + 2);
    patchName->setBounds(nameR);

    nameR = nameR.translated(105, 0).withWidth(18);
    mute->setBounds(nameR);
    nameR = nameR.translated(20, 0);
    solo->setBounds(nameR);

    auto slR =
        juce::Rectangle<int>(86, row0 + rowMargin + rowHeight, 82, rowHeight - 2 * rowMargin);
    slR = slR.reduced(0, 1);
    level->setBounds(slR);
    slR = slR.translated(0, rowHeight);
    pan->setBounds(slR);
    slR = slR.translated(0, rowHeight);
    tuning->setBounds(slR);
}

void PartSidebarCard::showMidiModeMenu()
{
    auto makeMenuCallback = [w = juce::Component::SafePointer(this)](int ch) {
        return [w, ch]() {
            if (!w)
                return;
            w->editor->partConfigurations[w->part].channel = ch;
            w->resetFromEditorCache();
            w->sendToSerialization(
                cmsg::UpdatePartFullConfig({w->part, w->editor->partConfigurations[w->part]}));
        };
    };
    auto p = juce::PopupMenu();
    auto ch = editor->partConfigurations[part].channel;
    p.addSectionHeader("MIDI");
    p.addSeparator();
    p.addItem("OMNI", true, ch == engine::Part::PartConfiguration::omniChannel,
              makeMenuCallback(engine::Part::PartConfiguration::omniChannel));
    p.addItem("MPE", true, ch == engine::Part::PartConfiguration::mpeChannel,
              makeMenuCallback(engine::Part::PartConfiguration::mpeChannel));
    p.addSeparator();
    for (int i = 0; i < 16; ++i)
    {
        p.addItem("Ch. " + std::to_string(i + 1), true, ch == i, makeMenuCallback(i));
    }
    p.addSeparator();
    auto msm = juce::PopupMenu();
    msm.addSectionHeader("MPE Settings");
    msm.addSeparator();
    for (auto d : {12, 24, 48, 96})
    {
        msm.addItem(std::to_string(d) + " semi bend range", true,
                    d == editor->partConfigurations[part].mpePitchBendRange,
                    [d, w = juce::Component::SafePointer(this)] {
                        if (!w)
                            return;
                        w->editor->partConfigurations[w->part].mpePitchBendRange = d;
                        w->resetFromEditorCache();
                        w->sendToSerialization(cmsg::UpdatePartFullConfig(
                            {w->part, w->editor->partConfigurations[w->part]}));
                    });
    }
    p.addSubMenu("MPE Settings", msm);
    p.showMenuAsync(editor->defaultPopupMenuOptions(midiMode.get()));
}

void PartSidebarCard::resetFromEditorCache()
{
    const auto &conf = editor->partConfigurations[part];
    auto mc = conf.channel;
    if (mc == engine::Part::PartConfiguration::omniChannel)
    {
        midiMode->setLabel("OMNI");
    }
    else if (mc == engine::Part::PartConfiguration::mpeChannel)
    {
        midiMode->setLabel("MPE");
    }
    else
    {
        midiMode->setLabel(std::to_string(mc + 1));
    }
    repaint();
}
} // namespace scxt::ui::app::shared