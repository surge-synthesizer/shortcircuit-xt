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

#include "GroupTriggersCard.h"

#include <array>
#include <optional>

#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/DraggableTextEditableValue.h"
#include "sst/jucegui/components/DraggableTextEditableDiscreteValue.h"

#include "app/SCXTEditor.h"
#include "messaging/client/client_messages.h"
#include "connectors/PayloadDataAttachment.h"

namespace scxt::ui::app::edit_screen
{
namespace jcmp = sst::jucegui::components;

/*
 * The two args mean something different for every trigger type, so their metadata lives here
 * rather than on the trigger (see the note in group_triggers.h). nullopt means the type doesn't
 * use that arg. Most types read the pair as a range, so the defaults span the whole scale: a
 * freshly picked type holds rather than silently muting the group until you widen it.
 */
using argMetadata_t =
    std::array<std::optional<datamodel::pmd>, engine::GroupTriggerStorage::numArgs>;

static argMetadata_t argMetadataFor(engine::GroupTriggerID id)
{
    auto arg = [](float lo, float hi, int decimals, float def) {
        return datamodel::pmd()
            .asFloat()
            .withRange(lo, hi)
            .withLinearScaleFormatting("")
            .withDecimalPlaces(decimals)
            .withDefault(def);
    };
    auto range = [&arg](float lo, float hi, int decimals) -> argMetadata_t {
        return {arg(lo, hi, decimals, lo), arg(lo, hi, decimals, hi)};
    };

    if ((int)id >= (int)engine::GroupTriggerID::MACRO &&
        (int)id < (int)engine::GroupTriggerID::MACRO + scxt::macrosPerPart)
        return range(0, 1, 2);

    if ((int)id >= (int)engine::GroupTriggerID::MIDICC &&
        (int)id <= (int)engine::GroupTriggerID::LAST_MIDICC)
        return range(0, 127, 0);

    switch (id)
    {
    case engine::GroupTriggerID::PROGRAM_CHANGE:
        return range(0, 127, 0);
    case engine::GroupTriggerID::PITCH_BEND:
        return range(-8192, 8191, 0); // signed 14 bit, as the part carries it
    case engine::GroupTriggerID::KEYSWITCH_LATCH:
    case engine::GroupTriggerID::KEYSWITCH_MOMENTARY:
        return {arg(0, 127, 0, 60), std::nullopt}; // a key, not a range
    case engine::GroupTriggerID::ROUND_ROBIN_CYCLE:
    case engine::GroupTriggerID::ROUND_ROBIN_RANDOM:
    case engine::GroupTriggerID::ROUND_ROBIN_SHUFFLE:
    {
        /*
         * Which set this group belongs to. Each kind has its own space of sets - RR1, RN1 and SH1
         * are three unrelated round robins - so the prefix names the kind as well as the number.
         * There is no prefixed-integer display scale, but at this size a map is fine, and it makes
         * the pmd an INT, which is what picks the discrete widget below.
         */
        auto namesFor = [](const char *prefix) {
            std::unordered_map<int, std::string> res;
            for (int i = 0; i < scxt::maxRoundRobinSets; ++i)
                res[i] = fmt::format("{}{}", prefix, i + 1);
            return res;
        };
        static const auto cycleNames = namesFor("RR");
        static const auto randomNames = namesFor("RN");
        static const auto shuffleNames = namesFor("SH");

        const auto &names = (id == engine::GroupTriggerID::ROUND_ROBIN_CYCLE    ? cycleNames
                             : id == engine::GroupTriggerID::ROUND_ROBIN_RANDOM ? randomNames
                                                                                : shuffleNames);
        auto set = datamodel::pmd()
                       .asInt()
                       .withRange(0, scxt::maxRoundRobinSets - 1)
                       .withDefault(0)
                       .withUnorderedMapFormatting(names);

        // Random and shuffle take their order from the set, so they show no second arg at all
        if (id != engine::GroupTriggerID::ROUND_ROBIN_CYCLE)
            return {set, std::nullopt};

        return {set, datamodel::pmd()
                         .asInt()
                         .withRange(1, scxt::maxRoundRobinOrdinal)
                         .withDefault(1)
                         .withLinearScaleFormatting("")};
    }
    case engine::GroupTriggerID::NONE:
        return {std::nullopt, std::nullopt};
    default:
        SCLOG_IF(debug, "No group trigger args for type " << (int)id);
        return {std::nullopt, std::nullopt};
    }
}

struct GroupTriggersCard::ConditionRow : juce::Component, HasEditor
{
    using booleanAttachment_t =
        connectors::BooleanPayloadDataAttachment<scxt::engine::GroupTriggerConditions>;

    using floatAttachment_t =
        connectors::PayloadDataAttachment<scxt::engine::GroupTriggerConditions>;

    /*
     * The args are floats in storage whatever they mean, so an arg whose metadata is an INT binds
     * a discrete attachment over that same float rather than dragging in fractions of a set.
     */
    using discreteAttachment_t =
        connectors::DiscretePayloadDataAttachment<scxt::engine::GroupTriggerConditions, float>;

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

            auto md = argMetadataFor(sr.id);
            for (int i = 0; i < engine::GroupTriggerStorage::numArgs; ++i)
            {
                argFA[i].reset();
                argDA[i].reset();
                argFM[i].reset();
                argDM[i].reset();
                argM[i] = nullptr;

                if (!md[i].has_value())
                    continue;

                if (md[i]->type == datamodel::pmd::INT)
                {
                    argDA[i] =
                        std::make_unique<discreteAttachment_t>(*md[i], onArgChanged, sr.args[i]);
                    argDM[i] = std::make_unique<jcmp::DraggableTextEditableDiscreteValue>();
                    argDM[i]->setSource(argDA[i].get());
                    argM[i] = argDM[i].get();
                }
                else
                {
                    argFA[i] =
                        std::make_unique<floatAttachment_t>(*md[i], onArgChanged, sr.args[i]);
                    argFM[i] = std::make_unique<jcmp::DraggableTextEditableValue>();
                    argFM[i]->setSource(argFA[i].get());
                    argM[i] = argFM[i].get();
                }
                addAndMakeVisible(*argM[i]);
            }

            resized();
        }

        typeM->setLabel(engine::getGroupTriggerDisplayName(sr.id));
        auto showRest = (sr.id != engine::GroupTriggerID::NONE);
        auto ac = parent->cond.active[index];
        for (auto *m : argM)
        {
            if (m)
            {
                m->setVisible(showRest);
                m->setEnabled(ac);
            }
        }
        if (cM)
        {
            cM->setVisible(showRest);
            cM->setEnabled(ac);
        }
        typeM->setEnabled(ac);

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
                auto &sr = w->parent->cond.storage[w->index];
                auto id = (engine::GroupTriggerID)v;
                if (sr.id != id)
                {
                    // the args mean something else now, so a CC range left in a bend
                    // control would be nonsense. Start the new type at its own defaults -
                    // except that arg 1 still means "which set" across the round robin
                    // kinds, so switching cycle to shuffle shouldn't move the group to S1.
                    auto keepSet =
                        engine::isRoundRobinTriggerID(id) && engine::isRoundRobinTriggerID(sr.id);
                    auto set = sr.args[0];

                    sr.id = id;
                    auto md = argMetadataFor(id);
                    for (int i = 0; i < engine::GroupTriggerStorage::numArgs; ++i)
                        sr.args[i] = md[i].has_value() ? md[i]->defaultVal : 0.f;

                    if (keepSet)
                        sr.args[0] = set;
                }
                w->parent->pushUpdate();
                w->setupValuesFromData();
            };
        };

        p.addItem("None", mkv((int)engine::GroupTriggerID::NONE));

        p.addItem("KeySwitch", mkv((int)engine::GroupTriggerID::KEYSWITCH_LATCH));
        p.addItem("KeySwitch (Momentary)", mkv((int)engine::GroupTriggerID::KEYSWITCH_MOMENTARY));

        p.addItem("Program Change", mkv((int)engine::GroupTriggerID::PROGRAM_CHANGE));
        p.addItem("Pitch Bend", mkv((int)engine::GroupTriggerID::PITCH_BEND));

        /*
         * A group can only be in one round robin - being in two cycles at once means nothing -
         * so if another row here already holds one, offer the choices greyed rather than
         * silently letting the engine pick a winner.
         */
        auto rrElsewhere{false};
        for (int i = 0; i < scxt::triggerConditionsPerGroup; ++i)
        {
            if (i != index && parent->cond.active[i] &&
                engine::isRoundRobinTriggerID(parent->cond.storage[i].id))
                rrElsewhere = true;
        }
        auto mrr = juce::PopupMenu();
        mrr.addItem("Cycle", !rrElsewhere, false,
                    mkv((int)engine::GroupTriggerID::ROUND_ROBIN_CYCLE));
        mrr.addItem("Random", !rrElsewhere, false,
                    mkv((int)engine::GroupTriggerID::ROUND_ROBIN_RANDOM));
        mrr.addItem("Shuffle", !rrElsewhere, false,
                    mkv((int)engine::GroupTriggerID::ROUND_ROBIN_SHUFFLE));
        p.addSubMenu("Round Robin", mrr);

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
        for (auto *m : argM)
        {
            tb = tb.translated(tb.getWidth() + 2, 0).withWidth(32);
            if (m)
                m->setBounds(tb);
        }

        if (cM)
        {
            tb = tb.translated(tb.getWidth() + 2, 0).withWidth(16);
            cM->setBounds(tb);
        }
    }

    static constexpr int numArgs{engine::GroupTriggerStorage::numArgs};

    std::unique_ptr<booleanAttachment_t> activeA;
    std::unique_ptr<jcmp::ToggleButton> activeB;
    std::unique_ptr<jcmp::TextPushButton> typeM, cM;

    // Exactly one of the float or discrete pair exists per arg; argM points at whichever it is
    std::array<std::unique_ptr<floatAttachment_t>, numArgs> argFA;
    std::array<std::unique_ptr<discreteAttachment_t>, numArgs> argDA;
    std::array<std::unique_ptr<jcmp::DraggableTextEditableValue>, numArgs> argFM;
    std::array<std::unique_ptr<jcmp::DraggableTextEditableDiscreteValue>, numArgs> argDM;
    std::array<juce::Component *, numArgs> argM{};
};
/*
 * Just the toggle and its attachment. Nested and defined here for the same reason ConditionRow
 * is - it keeps the attachment types out of the header.
 */
struct GroupTriggersCard::ReleaseRow
{
    using booleanAttachment_t =
        connectors::BooleanPayloadDataAttachment<scxt::engine::GroupTriggerConditions>;

    ReleaseRow(GroupTriggersCard *p) : parent(p)
    {
        attachment = std::make_unique<booleanAttachment_t>(
            "Release Trigger",
            [w = juce::Component::SafePointer(p)](const auto &a) {
                if (!w)
                    return;
                w->cond.voiceCreationMode = w->releaseTriggerOn
                                                ? engine::VoiceCreationMode::ON_NOTE_OFF
                                                : engine::VoiceCreationMode::ON_NOTE_ON;
                w->pushUpdate();
            },
            p->releaseTriggerOn);

        button = std::make_unique<jcmp::ToggleButton>();
        button->setLabel("RELEASE TRIGGER");
        button->setSource(attachment.get());
        p->addAndMakeVisible(*button);
    }

    void setupValuesFromData()
    {
        parent->releaseTriggerOn = parent->cond.createsVoicesOnRelease();
        button->repaint();
    }

    GroupTriggersCard *parent{nullptr};
    std::unique_ptr<booleanAttachment_t> attachment;
    std::unique_ptr<jcmp::ToggleButton> button;
};

GroupTriggersCard::GroupTriggersCard(SCXTEditor *e) : HasEditor(e)
{
    releaseRow = std::make_unique<ReleaseRow>(this);

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

    releaseRow->button->setBounds(b.withHeight(componentHeight));

    auto r = b.withTrimmedTop(releaseBlockHeight).withHeight(componentHeight);
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

    // rule between the release trigger and the conditions it decides the timing of
    auto ruleY = 20 + releaseBlockHeight - 5;
    g.setColour(co.withAlpha(0.4f));
    g.drawHorizontalLine(ruleY, 0.f, (float)getWidth());
}

void GroupTriggersCard::setGroupTriggerConditions(const scxt::engine::GroupTriggerConditions &c)
{
    cond = c;
    releaseRow->setupValuesFromData();
    for (auto &r : rows)
        r->setupValuesFromData();
    repaint();
}

void GroupTriggersCard::pushUpdate()
{
    sendToSerialization(scxt::messaging::client::UpdateGroupTriggerConditions(cond));
}

} // namespace scxt::ui::app::edit_screen