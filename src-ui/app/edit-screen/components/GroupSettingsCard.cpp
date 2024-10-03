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

#include "GroupSettingsCard.h"
#include "app/SCXTEditor.h"
#include "connectors/PayloadDataAttachment.h"

namespace scxt::ui::app::edit_screen
{
namespace jcmp = sst::jucegui::components;

GroupSettingsCard::GroupSettingsCard(SCXTEditor *e) : HasEditor(e)
{
    auto mkg = [this](auto gl) {
        auto res = std::make_unique<jcmp::GlyphPainter>(gl);
        addAndMakeVisible(*res);
        return res;
    };
    auto mkm = [this](auto tx, auto cs) {
        auto res = std::make_unique<jcmp::MenuButton>();
        res->setLabel(tx);
        res->setOnCallback(editor->makeComingSoon(std::string() + "Group Setting Pane " + cs));
        addAndMakeVisible(*res);
        return res;
    };
    auto mkd = [this](auto idx, auto tx) {
        auto res = connectors::makeConnectedToDummy<jcmp::DraggableTextEditableValue>(
            'ptlv', tx, 0.0, false,
            editor->makeComingSoon(std::string() + "Editing the Group Setting " + tx + " Control"));
        addAndMakeVisible(*res);
        return res;
    };
    midiGlyph = mkg(jcmp::GlyphPainter::GlyphType::MIDI);
    midiMenu = mkm("PART", "Midi");
    outputGlyph = mkg(jcmp::GlyphPainter::GlyphType::SPEAKER);
    outputMenu = mkm("PART", "Output");
    polyGlygh = mkg(jcmp::GlyphPainter::GlyphType::POLYPHONY);
    polyMenu = mkm("PART", "Polyphony");
    prioGlyph = mkg(jcmp::GlyphPainter::GlyphType::NOTE_PRIORITY);
    prioMenu = mkm("LAST", "Priority");
    glideGlpyh = mkg(jcmp::GlyphPainter::GlyphType::CURVE);
    glideMenu = mkm("RATE", "Glide Rate Thingy");
    glideDrag = mkd('gdrg', "Glide");

    volGlyph = mkg(jcmp::GlyphPainter::GlyphType::VOLUME);
    volDrag = mkd('grvl', "Volume");
    panGlyph = mkg(jcmp::GlyphPainter::GlyphType::PAN);
    panDrag = mkd('grpn', "Pan");
    tuneGlyph = mkg(jcmp::GlyphPainter::GlyphType::TUNING);
    tuneDrag = mkd('grtn', "Tune");
}

void GroupSettingsCard::paint(juce::Graphics &g)
{
    auto ft = editor->themeApplier.interMediumFor(13);
    auto co = editor->themeColor(theme::ColorMap::generic_content_low);
    g.setFont(ft);
    g.setColour(co);
    g.drawText("GROUP SETTINGS", getLocalBounds(), juce::Justification::topLeft);
}

void GroupSettingsCard::resized()
{
    int rowHeight = 24;
    int componentHeight = 16;
    auto b = getLocalBounds().withTrimmedTop(rowHeight - 4);

    auto lsw = 51;
    auto r = b.withHeight(componentHeight).withWidth(lsw);
    auto osr = b.withHeight(componentHeight).withTrimmedLeft(b.getWidth() - lsw);

    auto spair = [&r, componentHeight](auto &g, auto &m) {
        g->setBounds(r.withWidth(componentHeight));
        m->setBounds(r.withTrimmedLeft(componentHeight + 2));
    };
    spair(midiGlyph, midiMenu);
    r = r.translated(0, rowHeight);
    spair(outputGlyph, outputMenu);
    r = r.translated(0, rowHeight);
    spair(polyGlygh, polyMenu);
    r = r.translated(0, rowHeight);
    spair(prioGlyph, prioMenu);
    r = r.translated(0, rowHeight);
    spair(glideGlpyh, glideMenu);
    glideDrag->setBounds(r.translated(r.getWidth() + 2, 0).withTrimmedRight(componentHeight + 2));

    auto ospair = [&osr, componentHeight](auto &g, auto &m) {
        g->setBounds(osr.withWidth(componentHeight));
        m->setBounds(osr.withTrimmedLeft(componentHeight + 2));
    };
    osr = osr.translated(0, rowHeight);
    ospair(volGlyph, volDrag);
    osr = osr.translated(0, rowHeight);
    ospair(panGlyph, panDrag);
    osr = osr.translated(0, rowHeight);
    ospair(tuneGlyph, tuneDrag);
}

} // namespace scxt::ui::app::edit_screen