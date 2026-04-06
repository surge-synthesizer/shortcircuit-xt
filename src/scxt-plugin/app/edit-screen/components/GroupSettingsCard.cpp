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

#include "GroupSettingsCard.h"
#include "messaging/messaging.h"
#include "app/SCXTEditor.h"

namespace scxt::ui::app::edit_screen
{
namespace jcmp = sst::jucegui::components;

GroupSettingsCard::GroupSettingsCard(SCXTEditor *e)
    : HasEditor(e), info(e->editorDataCache.groupOutputInfo)
{
    using fac = connectors::SingleValueFactory<attachment_t, floatMsg_t>;
    using ifac = connectors::SingleValueFactory<iattachment_t, intMsg_t>;

    auto mkg = [this](auto gl) {
        auto res = std::make_unique<jcmp::GlyphPainter>(gl);
        addAndMakeVisible(*res);
        return res;
    };
    auto mkm = [this](auto tx, auto cs) {
        auto res = std::make_unique<jcmp::TextPushButton>();
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
    if (hasFeature::hasGroupMIDIChannel)
    {
        midiGlyph = mkg(jcmp::GlyphPainter::GlyphType::MIDI);
        midiMenu = std::make_unique<jcmp::TextPushButton>();
        midiMenu->setLabel("PART");
        midiMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showMidiChannelMenu();
        });
        addAndMakeVisible(*midiMenu);
    }
    exclusiveGroupGlyph = mkg(jcmp::GlyphPainter::GlyphType::EXCLUSIVE_GROUP);
    exclusiveGroupNotesGlyph = mkg(jcmp::GlyphPainter::GlyphType::POLYPHONY);

    outputGlyph = mkg(jcmp::GlyphPainter::GlyphType::SPEAKER);
    outputMenu = mkm("PART", "Output");
    polyGlygh = mkg(jcmp::GlyphPainter::GlyphType::POLYPHONY);
    polyMenu = std::make_unique<jcmp::TextPushButton>();
    polyMenu->setLabel("PART");
    polyMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showPolyMenu();
    });
    addAndMakeVisible(*polyMenu);

    polyModeMenu = std::make_unique<jcmp::TextPushButton>();
    polyModeMenu->setLabel("POLY");
    polyModeMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showPolyModeMenu();
    });
    addAndMakeVisible(*polyModeMenu);

    prioGlyph = mkg(jcmp::GlyphPainter::GlyphType::NOTE_PRIORITY);
    prioMenu = std::make_unique<jcmp::TextPushButton>();
    prioMenu->setLabel("LAST");
    prioMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showNotePrioMenu();
    });
    addAndMakeVisible(*prioMenu);

    glideGlpyh = mkg(jcmp::GlyphPainter::GlyphType::CURVE);
    glideMenu = std::make_unique<jcmp::TextPushButton>();
    glideMenu->setLabel("TIME");
    glideMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showGlideRateModeMenu();
    });
    addAndMakeVisible(*glideMenu);
    fac::attachAndAdd(info, info.glideTime, this, glideAttachment, glideDrag);

    volGlyph = mkg(jcmp::GlyphPainter::GlyphType::VOLUME);
    fac::attachAndAdd(info, info.amplitude, this, volAttachment, volDrag);
    panGlyph = mkg(jcmp::GlyphPainter::GlyphType::PAN);
    fac::attachAndAdd(info, info.pan, this, panAttachment, panDrag);

    tuneGlyph = mkg(jcmp::GlyphPainter::GlyphType::TUNING);
    fac::attachAndAdd(info, info.tuning, this, tuneAttachment, tuneDrag);
    tuneDrag->setDragScaleFromMinMaxHeuristic();

    ifac::attachAndAdd(info, info.pbDown, this, pbDnA, pbDnVal);
    ifac::attachAndAdd(info, info.pbUp, this, pbUpA, pbUpVal);
    pbLabel = std::make_unique<jcmp::Label>();
    pbLabel->setText("PB");
    pbLabel->setJustification(juce::Justification::centredRight);
    addAndMakeVisible(*pbLabel);

    exclusiveGroupMenu = std::make_unique<jcmp::TextPushButton>();
    exclusiveGroupMenu->setLabel("OFF");
    exclusiveGroupMenu->setOnCallback(
        editor->makeComingSoon("Group Settings Pane Exclusive Group"));
    addAndMakeVisible(*exclusiveGroupMenu);

    SRCLabel = std::make_unique<jcmp::Label>();
    SRCLabel->setText("SRC");
    SRCLabel->setJustification(juce::Justification::centredRight);
    addAndMakeVisible(*SRCLabel);

    srcMenu = std::make_unique<jcmp::TextPushButton>();
    srcMenu->setLabel("CUBIC");
    srcMenu->setOnCallback(editor->makeComingSoon("Group Settings Pane SRC Quality"));
    addAndMakeVisible(*srcMenu);

    cornerRule = std::make_unique<jcmp::RuleLabel>();
    cornerRule->setDirection(jcmp::RuleLabel::CORNER_TL);
    addAndMakeVisible(*cornerRule);

    pbRuleH = std::make_unique<jcmp::RuleLabel>();
    pbRuleH->setDirection(jcmp::RuleLabel::HORIZONTAL);
    addAndMakeVisible(*pbRuleH);

    auto mkVRule = [this]() -> std::unique_ptr<jcmp::RuleLabel> {
        auto r = std::make_unique<jcmp::RuleLabel>();
        r->setDirection(jcmp::RuleLabel::VERTICAL);
        addAndMakeVisible(*r);
        return r;
    };
    rightVRule01 = mkVRule();
    rightVRule12 = mkVRule();
    glideRuleH = std::make_unique<jcmp::RuleLabel>();
    glideRuleH->setDirection(jcmp::RuleLabel::HORIZONTAL);
    addAndMakeVisible(*glideRuleH);

    cornerRuleBottom = std::make_unique<jcmp::RuleLabel>();
    cornerRuleBottom->setDirection(jcmp::RuleLabel::CORNER_BL);
    addAndMakeVisible(*cornerRuleBottom);

    editor->editorDataCache.addNotificationCallback(&info,
                                                    [w = juce::Component::SafePointer(this)]() {
                                                        if (w)
                                                            w->rebuildFromInfo();
                                                    });
}

void GroupSettingsCard::paint(juce::Graphics &g)
{
    auto ft = editor->themeApplier.interMediumFor(13);
    auto co = editor->themeColor(theme::ColorMap::generic_content_low);
    g.setFont(ft);
    g.setColour(co);
    g.drawText("GROUP SETTINGS", getLocalBounds(), juce::Justification::topLeft);
    // Small horizontal connector between the exclusive-group area and the bracket (SVG y=41)
    g.drawHorizontalLine(41, 113.f, 117.f);
}

void GroupSettingsCard::resized()
{
    // All positions pixel-exact from Figma SVG export (182×146 reference).
    // Right column width adapts to component width; all other coords are fixed.
    const int rbtnW = getWidth() - 134;
    auto b = [](int x, int y, int w, int h) -> juce::Rectangle<int> { return {x, y, w, h}; };

    // Left column — vol / pan / tune (rows y=21 / 45 / 69)
    volGlyph->setBounds(b(0, 21, 20, 16));
    volDrag->setBounds(b(22, 21, 48, 16));
    panGlyph->setBounds(b(0, 45, 20, 16));
    panDrag->setBounds(b(22, 45, 48, 16));
    tuneGlyph->setBounds(b(0, 69, 20, 16));
    tuneDrag->setBounds(b(22, 69, 48, 16));

    // Centre — exclusive group button + XOR glyph (between rows 0 and 1)
    exclusiveGroupMenu->setBounds(b(85, 33, 24, 16));
    exclusiveGroupGlyph->setBounds(b(89, 48, 16, 16));
    exclusiveGroupNotesGlyph->setBounds(b(116, 33, 16, 16));

    // PB row (y=93) — label | dn value | rule | up value
    pbLabel->setBounds(b(0, 93, 20, 16));
    pbDnVal->setBounds(b(22, 93, 32, 16));
    pbRuleH->setBounds(b(54, 93, 8, 16));
    pbUpVal->setBounds(b(62, 93, 32, 16));

    // Glide row (y=117) — glyph | mode button | rule | drag value
    glideGlpyh->setBounds(b(0, 117, 20, 16));
    glideMenu->setBounds(b(22, 117, 32, 16));
    glideRuleH->setBounds(b(54, 117, 8, 16));
    glideDrag->setBounds(b(62, 117, 32, 16));

    // Right column — poly mode / poly count / note priority / SRC / output
    polyModeMenu->setBounds(b(134, 21, rbtnW, 16));
    polyMenu->setBounds(b(134, 45, rbtnW, 16));
    prioGlyph->setBounds(b(116, 69, 16, 16));
    prioMenu->setBounds(b(134, 69, rbtnW, 16));
    SRCLabel->setBounds(b(96, 93, 36, 16));
    srcMenu->setBounds(b(134, 93, rbtnW, 16));
    outputGlyph->setBounds(b(116, 117, 16, 16));
    outputMenu->setBounds(b(134, 117, rbtnW, 16));

    // Bracket corners grouping poly mode + poly count (┌ top, └ bottom)
    cornerRule->setBounds(b(125, 28, 7, 5));
    cornerRuleBottom->setBounds(b(125, 49, 7, 5));

    // Vertical separators in right column between rows 0-1 and 1-2
    rightVRule01->setBounds(b(155, 39, 4, 4));
    rightVRule12->setBounds(b(155, 63, 4, 4));

    polyGlygh->setBounds({});
    if (midiGlyph)
    {
        midiGlyph->setBounds({});
        midiMenu->setBounds({});
    }
}

void GroupSettingsCard::rebuildFromInfo()
{
    if (info.playMode != engine::Group::PlayMode::POLY)
    {
        polyMenu->setEnabled(false);
        prioMenu->setEnabled(true);

        if (info.playMode == engine::Group::PlayMode::LEGATO)
        {
            polyModeMenu->setLabel("LEG");
        }
        else
        {
            polyModeMenu->setLabel("MONO");
        }
    }
    else
    {
        polyMenu->setEnabled(true);
        prioMenu->setEnabled(false);

        polyModeMenu->setLabel("POLY");

        if (info.hasIndependentPolyLimit)
        {
            polyMenu->setLabel(std::to_string(info.polyLimit));
        }
        else
        {
            polyMenu->setLabel("PART");
        }
    }

    if (hasFeature::hasGroupMIDIChannel)
    {
        if (info.midiChannel < 0)
        {
            midiMenu->setLabel("PART");
        }
        else
        {
            midiMenu->setLabel(std::to_string(info.midiChannel + 1));
        }
    }

    bool isMono = (info.playMode != engine::Group::PlayMode::POLY);
    glideDrag->setEnabled(isMono);
    glideMenu->setEnabled(isMono);
    glideMenu->setLabel(info.glideRateMode == engine::Group::CONSTANT_RATE ? "RATE" : "TIME");

    repaint();
}

void GroupSettingsCard::showPolyMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Polyphony");
    p.addSeparator();
    p.addItem(
        "Part", true, !info.hasIndependentPolyLimit, [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->info.hasIndependentPolyLimit = false;

            w->rebuildFromInfo();
            w->sendToSerialization(messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
        });
    p.addSeparator();
    for (auto pol : {1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32, 48, 64})
    {
        p.addItem(std::to_string(pol), true, info.hasIndependentPolyLimit && info.polyLimit == pol,
                  [pol, w = juce::Component::SafePointer(this)]() {
                      if (!w)
                          return;
                      w->info.hasIndependentPolyLimit = true;
                      w->info.polyLimit = pol;

                      w->rebuildFromInfo();
                      w->sendToSerialization(
                          messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
                  });
    }
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void GroupSettingsCard::showMidiChannelMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("MIDI Channel");
    p.addSeparator();
    for (int i = -1; i < 16; ++i)
    {
        std::string nm{"PART"};
        if (i >= 0)
            nm = "Ch. " + std::to_string(i + 1);
        p.addItem(nm, true, info.midiChannel == i, [i, w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->info.midiChannel = i;

            w->rebuildFromInfo();
            w->sendToSerialization(messaging::client::UpdateGroupOutputInfoMidiChannel{w->info});
        });
        if (i == -1)
            p.addSeparator();
    }
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void GroupSettingsCard::showPolyModeMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Play Mode");
    p.addSeparator();

    p.addItem("Poly", true, info.playMode == engine::Group::PlayMode::POLY,
              [w = juce::Component::SafePointer(this)]() {
                  if (!w)
                      return;
                  w->info.playMode = engine::Group::PlayMode::POLY;

                  w->rebuildFromInfo();
                  w->sendToSerialization(
                      messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
              });
    p.addItem("Mono", true, info.playMode == engine::Group::PlayMode::MONO,
              [w = juce::Component::SafePointer(this)]() {
                  if (!w)
                      return;
                  w->info.playMode = engine::Group::PlayMode::MONO;

                  w->rebuildFromInfo();
                  w->sendToSerialization(
                      messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
              });
    p.addItem("Legato", true, info.playMode == engine::Group::PlayMode::LEGATO,
              [w = juce::Component::SafePointer(this)]() {
                  if (!w)
                      return;
                  w->info.playMode = engine::Group::PlayMode::LEGATO;

                  w->rebuildFromInfo();
                  w->sendToSerialization(
                      messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
              });

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void GroupSettingsCard::showNotePrioMenu()
{
    auto p = juce::PopupMenu();

    p.addSectionHeader("Note Priority");

    p.addItem("Latest", true, info.notePriority == engine::Group::NotePriority::LATEST,
              [w = juce::Component::SafePointer(this)]() {
                  if (!w)
                      return;
                  w->info.notePriority = engine::Group::NotePriority::LATEST;

                  w->rebuildFromInfo();
                  w->sendToSerialization(
                      messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
              });
    p.addItem("Highest", true, info.notePriority == engine::Group::NotePriority::HIGHEST,
              [w = juce::Component::SafePointer(this)]() {
                  if (!w)
                      return;
                  w->info.notePriority = engine::Group::NotePriority::HIGHEST;

                  w->rebuildFromInfo();
                  w->sendToSerialization(
                      messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
              });
    p.addItem("Lowest", true, info.notePriority == engine::Group::NotePriority::LOWEST,
              [w = juce::Component::SafePointer(this)]() {
                  if (!w)
                      return;
                  w->info.notePriority = engine::Group::NotePriority::LOWEST;

                  w->rebuildFromInfo();
                  w->sendToSerialization(
                      messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
              });

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void GroupSettingsCard::showGlideRateModeMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Glide Rate Mode");
    p.addSeparator();
    p.addItem("Time", true, info.glideRateMode == engine::Group::CONSTANT_TIME,
              [w = juce::Component::SafePointer(this)]() {
                  if (!w)
                      return;
                  w->info.glideRateMode = engine::Group::CONSTANT_TIME;
                  w->rebuildFromInfo();
                  w->sendToSerialization(
                      messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
              });
    p.addItem("Rate", true, info.glideRateMode == engine::Group::CONSTANT_RATE,
              [w = juce::Component::SafePointer(this)]() {
                  if (!w)
                      return;
                  w->info.glideRateMode = engine::Group::CONSTANT_RATE;
                  w->rebuildFromInfo();
                  w->sendToSerialization(
                      messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
              });
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

} // namespace scxt::ui::app::edit_screen