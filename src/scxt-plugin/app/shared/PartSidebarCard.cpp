/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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
#include "app/shared/PatchMultiIO.h"
#include "app/shared/MenuValueTypein.h"

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
    outBus->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showRoutingMenu();
    });
    addAndMakeVisible(*outBus);

    polyCount = std::make_unique<jcmp::TextPushButton>();
    polyCount->setLabel("UNL");
    polyCount->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showPolyMenu();
    });
    addAndMakeVisible(*polyCount);

    patchName = std::make_unique<jcmp::MenuButton>();
    patchName->setLabel("Instrument Name");
    patchName->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showPartIOMenu();
    });
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

    soloAtt =
        std::make_unique<boolattachment_t>("Solo", onChange, editor->partConfigurations[part].solo);
    solo = std::make_unique<jcmp::ToggleButton>();
    solo->setSource(soloAtt.get());
    solo->setLabel("S");
    addAndMakeVisible(*solo);

    using fac = connectors::SingleValueFactory<attachment_t, cmsg::UpdatePartFullConfig>;

    auto oGVC = [w = juce::Component::SafePointer(this)](const auto &a) {
        if (w)
        {
            w->resetFromEditorCache();
            w->sendToSerialization(
                cmsg::UpdatePartFullConfig({w->part, w->editor->partConfigurations[w->part]}));
            w->updateValueTooltip(a);
        }
    };
    auto att = [this, oGVC](auto &a, auto &wid, auto &att) {
        auto pmd = scxt::datamodel::describeValue(editor->partConfigurations[part], a);
        att = std::make_unique<typename std::remove_reference_t<decltype(att)>::element_type>(
            pmd, oGVC, a);

        wid = std::make_unique<typename std::remove_reference_t<decltype(wid)>::element_type>();
        wid->setSource(att.get());
        setupWidgetForValueTooltip(wid.get(), att.get());
        addAndMakeVisible(*wid);
    };
    att(editor->partConfigurations[part].level, level, levelAtt);
    att(editor->partConfigurations[part].pan, pan, panAtt);
    att(editor->partConfigurations[part].tuning, tuning, tuningAtt);
    att(editor->partConfigurations[part].transpose, transpose, transposeAtt);

    partBlurb = std::make_unique<jcmp::TextEditor>();
    partBlurb->setAllText("");
    partBlurb->setMultiLine(true);
    partBlurb->addListener(this);
    addChildComponent(*partBlurb);

    onIdleHover = [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showPartBlurbTooltip();
    };
    onIdleHoverEnd = [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->hidePartBlurbTooltip();
    };
}

void PartSidebarCard::mouseDown(const juce::MouseEvent &event)
{
    if (event.mods.isPopupMenu())
    {
        showPartIOMenu();
        return;
    }
    sendToSerialization(cmsg::SelectPart(part));
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
    auto q = r.translated(tuningTransposeSplit + r.getHeight() + 2 * rowMargin, 0);
    jcmp::GlyphPainter::paintGlyph(g, q, jcmp::GlyphPainter::GlyphType::PLUS_MINUS, med);

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
    tuning->setBounds(slR.withWidth(tuningTransposeSplit));
    transpose->setBounds(
        slR.withTrimmedLeft(tuningTransposeSplit + slR.getHeight() + 2 * rowMargin));

    if (tallMode)
    {
        partBlurb->setBounds(getLocalBounds()
                                 .withTrimmedTop(slR.getBottom() + 2 * rowMargin)
                                 .withTrimmedBottom(2 * rowMargin)
                                 .reduced(5, 0));
    }
}

void PartSidebarCard::showRoutingMenu()
{
    auto makeMenuCallback = [w = juce::Component::SafePointer(this)](int ch) {
        return [w, ch]() {
            if (!w)
                return;
            w->editor->partConfigurations[w->part].routeTo = (engine::BusAddress)ch;
            w->resetFromEditorCache();
            w->sendToSerialization(
                cmsg::UpdatePartFullConfig({w->part, w->editor->partConfigurations[w->part]}));
        };
    };

    auto p = juce::PopupMenu();
    auto ch = editor->partConfigurations[part].routeTo;
    p.addSectionHeader("Part Routing");
    p.addSeparator();

    for (int i = engine::BusAddress::DEFAULT_BUS; i <= engine::AUX_0 + numAux; ++i)
    {
        p.addItem(engine::getBusAddressLabel((engine::BusAddress)i, "Part Default"), true, i == ch,
                  makeMenuCallback(i));
    }
    p.showMenuAsync(editor->defaultPopupMenuOptions(outBus.get()));
}

void PartSidebarCard::showPolyMenu()
{
    auto makeMenuCallback = [w = juce::Component::SafePointer(this)](int le) {
        return [w, le]() {
            if (!w)
                return;
            w->editor->partConfigurations[w->part].polyLimitVoices = le;
            w->resetFromEditorCache();
            w->sendToSerialization(
                cmsg::UpdatePartFullConfig({w->part, w->editor->partConfigurations[w->part]}));
        };
    };

    auto p = juce::PopupMenu();
    auto vc = editor->partConfigurations[part].polyLimitVoices;
    p.addSectionHeader("Part Voice Limit");
    p.addSeparator();

    p.addItem("Unlimited", true, vc == 0, makeMenuCallback(0));
    for (int i = 4; i <= 16; i += 4)
        p.addItem(std::to_string(i), true, vc == i, makeMenuCallback(i));
    for (int i = 32; i <= 128; i += 32)
        p.addItem(std::to_string(i), true, vc == i, makeMenuCallback(i));
    for (int i = 256; i <= 512; i += 256)
        p.addItem(std::to_string(i), true, vc == i, makeMenuCallback(i));
    p.showMenuAsync(editor->defaultPopupMenuOptions(outBus.get()));
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

struct PartNameMenuTypein : scxt::ui::app::shared::MenuValueTypeinBase
{
    juce::Component::SafePointer<PartSidebarCard> sideBar{nullptr};
    PartNameMenuTypein(SCXTEditor *e, juce::Component::SafePointer<PartSidebarCard> p)
        : MenuValueTypeinBase(e), sideBar(p)
    {
    }
    std::string getInitialText() const override
    {
        if (!sideBar)
            return "";
        return editor->partConfigurations[sideBar->part].name;
    }
    void setValueString(const std::string &s) override
    {
        if (!sideBar)
            return;
        auto w = sideBar;

        memset(w->editor->partConfigurations[w->part].name, 0,
               engine::Part::PartConfiguration::maxName);
        strncpy(w->editor->partConfigurations[w->part].name, s.c_str(),
                engine::Part::PartConfiguration::maxName - 1);
        w->resetFromEditorCache();
        w->sendToSerialization(
            cmsg::UpdatePartFullConfig({w->part, w->editor->partConfigurations[w->part]}));
    }
};

void PartSidebarCard::showPartIOMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Part I/O");
    p.addSeparator();
    p.addCustomItem(-1, std::make_unique<PartNameMenuTypein>(editor, this));
    p.addSeparator();
    p.addItem("Save Part", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        shared::doSavePart(w.getComponent(), w->fileChooser, w->part,
                           patch_io::SaveStyles::NO_SAMPLES);
    });
    p.addItem("Save Part as Monolith", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        shared::doSavePart(w.getComponent(), w->fileChooser, w->part,
                           patch_io::SaveStyles::AS_MONOLITH);
    });
    p.addItem("Save Part with Collected Samples", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        shared::doSavePart(w.getComponent(), w->fileChooser, w->part,
                           patch_io::SaveStyles::COLLECT_SAMPLES);
    });
    p.addSeparator();
    p.addItem("Load Part", [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        shared::doLoadPartInto(w.getComponent(), w->fileChooser, w->part);
    });
    p.addSeparator();
    p.addItem("Deactivate Part", [p = this->part, w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->sendToSerialization(cmsg::DeactivatePart(p));
    });

    p.showMenuAsync(editor->defaultPopupMenuOptions(patchName.get()));
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
        midiMode->setLabel("CH " + std::to_string(mc + 1));
    }

    auto rt = conf.routeTo;
    std::string st = engine::getBusAddressLabel(rt, "PART", true);
    outBus->setLabel(st);

    auto pv = conf.polyLimitVoices;
    if (pv == 0)
        polyCount->setLabel("UNL");
    else
        polyCount->setLabel(std::to_string(pv));

    patchName->setLabel(conf.name);

    partBlurb->setAllText(conf.blurb);
    repaint();
}

void PartSidebarCard::textEditorTextChanged(juce::TextEditor &e)
{
    memset(editor->partConfigurations[part].blurb, 0,
           engine::Part::PartConfiguration::maxDescription);
    strncpy(editor->partConfigurations[part].blurb, e.getText().toStdString().c_str(),
            engine::Part::PartConfiguration::maxDescription - 1);
    sendToSerialization(cmsg::UpdatePartFullConfig({part, editor->partConfigurations[part]}));
}

void PartSidebarCard::showPartBlurbTooltip()
{
    if (!tallMode)
    {
        std::string b = editor->partConfigurations[part].blurb;

        if (!b.empty())
        {
            // We should probably build this in but
            std::vector<std::string> lines;
            auto splitOver{30};
            std::string currentLine;
            for (auto c : b)
            {
                if ((c == ' ' || c == '\n') && currentLine.size() > splitOver)
                {
                    lines.push_back(currentLine);
                    currentLine.clear();
                }
                else
                {
                    currentLine += c;
                }
            }
            if (!currentLine.empty())
                lines.push_back(currentLine);

            editor->setTooltipContents("Description", lines);
            editor->showTooltip(*this, {getWidth() + 3, 0});
        }
    }
}

void PartSidebarCard::hidePartBlurbTooltip() { editor->hideTooltip(); }

bool PartSidebarCard::isInterestedInFileDrag(const juce::StringArray &files)
{
    return files.size() == 1 && files[0].endsWithIgnoreCase(".scp");
}

void PartSidebarCard::filesDropped(const juce::StringArray &files, int x, int y)
{
    if (files.size() == 1)
    {
        auto f = files[0];
        if (f.endsWithIgnoreCase(".scp"))
        {
            editor->promptOKCancel(
                "Load into Part " + std::to_string(part + 1),
                "By loading the file you will clear the part and replace it with the contents of "
                "the file. Continue?",
                [w = juce::Component::SafePointer(this), f]() {
                    if (!w)
                        return;

                    w->sendToSerialization(cmsg::LoadPartInto({std::string(f.toUTF8()), w->part}));
                });
        }
    }
}
} // namespace scxt::ui::app::shared
