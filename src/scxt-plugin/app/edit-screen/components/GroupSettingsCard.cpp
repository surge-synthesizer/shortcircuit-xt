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
    if (hasFeature::hasGroupMIDIChannel)
    {
        spair(midiGlyph, midiMenu);
        r = r.translated(0, rowHeight);
    }

    auto q = b.withHeight(componentHeight).withTrimmedLeft(r.getWidth() + 10);
    q = q.withWidth(r.getWidth() - componentHeight - 2);
    q = q.translated(getWidth() - q.getX() - q.getWidth(), 0); // (getWidth()-q.getX()-q.getWidth()

    pbUpVal->setBounds(q);
    q = q.translated(-(q.getWidth() + 2), 0);
    pbDnVal->setBounds(q);
    q = q.translated(-(q.getWidth() + 2), 0);
    pbLabel->setBounds(q);

    spair(outputGlyph, outputMenu);
    r = r.translated(0, rowHeight);
    spair(polyGlygh, polyMenu);
    polyModeMenu->setBounds(polyMenu->getBounds().translated(polyMenu->getWidth() + 2, 0));
    r = r.translated(0, rowHeight);
    spair(prioGlyph, prioMenu);
    r = r.translated(0, rowHeight);
    spair(glideGlpyh, glideMenu);
    glideDrag->setBounds(r.translated(r.getWidth() + 2, 0));

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

void GroupSettingsCard::rebuildFromInfo()
{
    if (info.vmPlayModeInt == (int32_t)engine::Engine::voiceManager_t::PlayMode::MONO_NOTES)
    {
        polyMenu->setEnabled(false);
        prioMenu->setEnabled(true);

        if (info.vmPlayModeFeaturesInt &
            (int32_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::MONO_LEGATO)
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

    bool isLegato =
        (info.vmPlayModeInt == (int32_t)engine::Engine::voiceManager_t::PlayMode::MONO_NOTES) &&
        (info.vmPlayModeFeaturesInt &
         (int32_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::MONO_LEGATO);
    glideDrag->setEnabled(isLegato);
    glideMenu->setEnabled(isLegato);
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

    bool isAnyMono =
        info.vmPlayModeInt == (uint32_t)engine::Engine::voiceManager_t::PlayMode::MONO_NOTES;
    bool isMonoRetrig =
        info.vmPlayModeFeaturesInt &
        (uint64_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::MONO_RETRIGGER;
    bool isMonoLegato = info.vmPlayModeFeaturesInt &
                        (uint64_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::MONO_LEGATO;

    p.addItem("Poly", true, !isAnyMono, [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->info.vmPlayModeInt = (uint32_t)engine::Engine::voiceManager_t::PlayMode::POLY_VOICES;

        w->rebuildFromInfo();
        w->sendToSerialization(messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
    });
    p.addItem("Mono", true, isAnyMono && isMonoRetrig, [w = juce::Component::SafePointer(this)]() {
        if (!w)
            return;
        w->info.vmPlayModeInt = (uint32_t)engine::Engine::voiceManager_t::PlayMode::MONO_NOTES;
        w->info.vmPlayModeFeaturesInt =
            (uint64_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::NATURAL_MONO;

        w->rebuildFromInfo();
        w->sendToSerialization(messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
    });
    p.addItem(
        "Legato", true, isAnyMono && isMonoLegato, [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->info.vmPlayModeInt = (uint32_t)engine::Engine::voiceManager_t::PlayMode::MONO_NOTES;
            w->info.vmPlayModeFeaturesInt =
                (uint64_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::NATURAL_LEGATO;

            w->rebuildFromInfo();
            w->sendToSerialization(messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
        });

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

void GroupSettingsCard::showNotePrioMenu()
{
    auto p = juce::PopupMenu();

    p.addSectionHeader("Note Priority");

    p.addItem(
        "Latest", true,
        info.vmPlayModeFeaturesInt &
            (uint32_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_LATEST,
        [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->info.vmPlayModeInt = (uint32_t)engine::Engine::voiceManager_t::PlayMode::MONO_NOTES;
            w->info.vmPlayModeFeaturesInt &=
                ~(uint64_t)
                    engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_HIGHEST;
            w->info.vmPlayModeFeaturesInt &= ~(
                uint64_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_LOWEST;
            w->info.vmPlayModeFeaturesInt |= (uint64_t)
                engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_LATEST;

            w->rebuildFromInfo();
            w->sendToSerialization(messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
        });
    p.addItem(
        "Highest", true,
        info.vmPlayModeFeaturesInt &
            (uint32_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_HIGHEST,
        [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->info.vmPlayModeInt = (uint32_t)engine::Engine::voiceManager_t::PlayMode::MONO_NOTES;
            w->info.vmPlayModeFeaturesInt |= (uint64_t)
                engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_HIGHEST;
            w->info.vmPlayModeFeaturesInt &= ~(
                uint64_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_LOWEST;
            w->info.vmPlayModeFeaturesInt &= ~(
                uint64_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_LATEST;

            w->rebuildFromInfo();
            w->sendToSerialization(messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
        });
    p.addItem(
        "Lowest", true,
        info.vmPlayModeFeaturesInt &
            (uint32_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_LOWEST,
        [w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->info.vmPlayModeInt = (uint32_t)engine::Engine::voiceManager_t::PlayMode::MONO_NOTES;
            w->info.vmPlayModeFeaturesInt |= (uint64_t)
                engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_LOWEST;
            w->info.vmPlayModeFeaturesInt &= ~(
                uint64_t)engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_LOWEST;
            w->info.vmPlayModeFeaturesInt &=
                ~(uint64_t)
                    engine::Engine::voiceManager_t::MonoPlayModeFeatures::ON_RELEASE_TO_HIGHEST;

            w->rebuildFromInfo();
            w->sendToSerialization(messaging::client::UpdateGroupOutputInfoPolyphony{w->info});
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