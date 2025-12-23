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

#include "GroupTriggersCard.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"

#include "app/SCXTEditor.h"
#include "messaging/client/client_messages.h"
#include "connectors/PayloadDataAttachment.h"

namespace scxt::ui::app::edit_screen
{
namespace jcmp = sst::jucegui::components;

struct GroupTriggersCard::ConditionRow : juce::Component, HasEditor
{
    using booleanAttachment_t =
        connectors::BooleanPayloadDataAttachment<scxt::engine::GroupTriggerConditions>;

    using floatAttachment_t =
        connectors::PayloadDataAttachment<scxt::engine::GroupTriggerConditions>;

    GroupTriggersCard *parent{nullptr};
    engine::GroupTriggerID lastID{engine::GroupTriggerID::NONE};

    int index{-1};
    bool withCondition{false};
    ConditionRow(GroupTriggersCard *p, int index, bool withCond)
        : parent(p), index(index), withCondition(withCond), HasEditor(p)
    {
        activeA = std::make_unique<booleanAttachment_t>(
            "Active",
            [this](const auto &a) {
                setupValuesFromData();
                parent->pushUpdate();
            },
            parent->cond.active[index]);
        activeB = std::make_unique<jcmp::ToggleButton>();
        activeB->setLabel(std::to_string(index + 1));
        activeB->setSource(activeA.get());
        addAndMakeVisible(*activeB);

        auto mkm = [this](auto tx, auto cs) {
            auto res = std::make_unique<jcmp::TextPushButton>();
            res->setLabel(tx);
            res->setOnCallback(
                editor->makeComingSoon(std::string() + "Trigger Condition Pane " + cs));
            addAndMakeVisible(*res);
            return res;
        };
        typeM = mkm("TYPE", "Trigger Mode");
        typeM->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (!w)
                return;
            w->showTypeMenu();
        });
        if (withCond)
            cM = mkm("&", "Conjunction");

        setupValuesFromData();
    }

    void setupValuesFromData()
    {
        auto &sr = parent->cond.storage[index];

        if (sr.id != lastID)
        {
            lastID = sr.id;

            auto onArgChanged = [this](const auto &a) { parent->pushUpdate(); };

            if (sr.id == engine::GroupTriggerID::NONE)
            {
                a1A.reset();
                a2A.reset();
                a1M.reset();
                a2M.reset();
            }
            else if ((int)sr.id >= (int)engine::GroupTriggerID::MACRO &&
                     (int)sr.id < (int)engine::GroupTriggerID::MACRO + scxt::macrosPerPart)
            {
                auto dm = datamodel::pmd()
                              .asFloat()
                              .withRange(0, 1)
                              .withLinearScaleFormatting("")
                              .withDecimalPlaces(2)
                              .withDefault(0.5);
                a1A = std::make_unique<floatAttachment_t>(dm, onArgChanged, sr.args[0]);
                a2A = std::make_unique<floatAttachment_t>(dm, onArgChanged, sr.args[1]);

                a1M = std::make_unique<jcmp::DraggableTextEditableValue>();
                a1M->setSource(a1A.get());
                addAndMakeVisible(*a1M);

                a2M = std::make_unique<jcmp::DraggableTextEditableValue>();
                a2M->setSource(a2A.get());
                addAndMakeVisible(*a2M);
            }
            else if ((int)sr.id >= (int)engine::GroupTriggerID::MIDICC &&
                     (int)sr.id <= (int)engine::GroupTriggerID::LAST_MIDICC)
            {
                auto dm = datamodel::pmd()
                              .asFloat()
                              .withRange(0, 127)
                              .withLinearScaleFormatting("")
                              .withDecimalPlaces(0)
                              .withDefault(64);
                a1A = std::make_unique<floatAttachment_t>(dm, onArgChanged, sr.args[0]);
                a2A = std::make_unique<floatAttachment_t>(dm, onArgChanged, sr.args[1]);

                a1M = std::make_unique<jcmp::DraggableTextEditableValue>();
                a1M->setSource(a1A.get());
                addAndMakeVisible(*a1M);

                a2M = std::make_unique<jcmp::DraggableTextEditableValue>();
                a2M->setSource(a2A.get());
                addAndMakeVisible(*a2M);
            }
            else
            {
                SCLOG_IF(debug, "No midi CC for type " << (int)sr.id);
            }
            resized();
        }

        typeM->setLabel(engine::getGroupTriggerDisplayName(sr.id));
        auto showRest = (sr.id != engine::GroupTriggerID::NONE);
        if (a1M)
            a1M->setVisible(showRest);
        if (a2M)
            a2M->setVisible(showRest);
        if (cM)
            cM->setVisible(showRest);

        auto ac = parent->cond.active[index];
        typeM->setEnabled(ac);
        if (a1M)
            a1M->setEnabled(ac);
        if (a2M)
            a2M->setEnabled(ac);
        if (cM)
            cM->setEnabled(ac);

        repaint();
    }

    void showTypeMenu()
    {
        auto p = juce::PopupMenu();
        p.addSectionHeader("Trigger Mode");
        p.addSeparator();

        auto mkv = [this](auto v) {
            auto that = this; // darn you MSVC
            return [w = juce::Component::SafePointer(that), v]() {
                if (!w)
                    return;
                w->parent->cond.storage[w->index].id = (engine::GroupTriggerID)v;
                w->parent->pushUpdate();
                w->setupValuesFromData();
            };
        };

        p.addItem("NONE", mkv((int)engine::GroupTriggerID::NONE));

        auto mcc = juce::PopupMenu();
        for (int i = 0; i < 128; ++i)
        {
            mcc.addItem(fmt::format("CC {:3d}", i), mkv((int)engine::GroupTriggerID::MIDICC + i));
        }

        p.addSubMenu("MIDI CC", mcc);
        auto msm = juce::PopupMenu();
        for (int i = 0; i < scxt::macrosPerPart; ++i)
        {
            msm.addItem("Macro " + std::to_string(i + 1),
                        mkv((int)engine::GroupTriggerID::MACRO + i));
        }

        p.addSubMenu("Macros", msm);
        p.showMenuAsync(editor->defaultPopupMenuOptions());
    }

    void resized() override
    {
        auto rb = getLocalBounds().withHeight(16);
        auto tb = rb.withWidth(16);
        activeB->setBounds(tb);
        tb = tb.translated(tb.getWidth() + 2, 0).withWidth(72);
        typeM->setBounds(tb);
        tb = tb.translated(tb.getWidth() + 2, 0).withWidth(32);
        if (a1M)
            a1M->setBounds(tb);
        tb = tb.translated(tb.getWidth() + 2, 0).withWidth(32);
        if (a2M)
            a2M->setBounds(tb);

        if (cM)
        {
            tb = tb.translated(tb.getWidth() + 2, 0).withWidth(16);
            cM->setBounds(tb);
        }
    }

    std::unique_ptr<booleanAttachment_t> activeA;
    std::unique_ptr<floatAttachment_t> a1A, a2A;
    std::unique_ptr<jcmp::ToggleButton> activeB;
    std::unique_ptr<jcmp::TextPushButton> typeM, cM;
    std::unique_ptr<jcmp::DraggableTextEditableValue> a1M, a2M;
};
GroupTriggersCard::GroupTriggersCard(SCXTEditor *e) : HasEditor(e)
{
    for (int i = 0; i < scxt::triggerConditionsPerGroup; ++i)
    {
        rows[i] = std::make_unique<ConditionRow>(this, i, i != scxt::triggerConditionsPerGroup - 1);
        addAndMakeVisible(*rows[i]);
    }
}
GroupTriggersCard::~GroupTriggersCard() = default;

void GroupTriggersCard::resized()
{
    int rowHeight = 24;
    int componentHeight = 16;
    auto b = getLocalBounds().withTrimmedTop(rowHeight - 4);

    auto r = b.withHeight(componentHeight);
    for (int i = 0; i < scxt::triggerConditionsPerGroup; ++i)
    {
        rows[i]->setBounds(r);
        r = r.translated(0, rowHeight);
    }
}

void GroupTriggersCard::paint(juce::Graphics &g)
{
    auto ft = editor->themeApplier.interMediumFor(13);
    auto co = editor->themeColor(theme::ColorMap::generic_content_low);
    g.setFont(ft);
    g.setColour(co);
    g.drawText("TRIGGER CONDITIONS", getLocalBounds(), juce::Justification::topLeft);
}

void GroupTriggersCard::setGroupTriggerConditions(const scxt::engine::GroupTriggerConditions &c)
{
    cond = c;
    for (auto &r : rows)
        r->setupValuesFromData();
    repaint();
}

void GroupTriggersCard::pushUpdate()
{
    sendToSerialization(scxt::messaging::client::UpdateGroupTriggerConditions(cond));
}

} // namespace scxt::ui::app::edit_screen