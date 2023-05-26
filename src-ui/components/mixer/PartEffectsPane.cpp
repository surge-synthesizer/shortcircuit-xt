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

#include "connectors/Connectors.h"
#include "PartEffectsPane.h"
#include "components/SCXTEditor.h"
#include "components/MixerScreen.h"

#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/MenuButton.h"

namespace scxt::ui::mixer
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;
void PartEffectsPane::showHamburger()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Effects");
    p.addSeparator();
    auto go = [w = juce::Component::SafePointer(this)](auto q) {
        return [w, q]() {
            if (w)
            {
                w->sendToSerialization(cmsg::SetBusEffectToType({w->busAddress, w->fxSlot, q}));
            }
        };
    };
    auto add = [this, &go, &p](auto q) { p.addItem(effectDisplayName(q, true), go(q)); };
    add(engine::AvailableBusEffects::none);
    add(engine::AvailableBusEffects::reverb1);
    add(engine::AvailableBusEffects::delay);
    add(engine::AvailableBusEffects::flanger);
    add(engine::AvailableBusEffects::bonsai);
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

PartEffectsPane::~PartEffectsPane()
{
    removeAllChildren();
    // order matters here
    components.clear();
    floatAttachments.clear();
}

void PartEffectsPane::paintMetadata(juce::Graphics &g, const juce::Rectangle<int> &into)
{
    auto &b = mixer->busEffectsData[busAddress][fxSlot];

    auto r = into.withHeight(20);
    int idx{0};
    for (const auto &p : b.first)
    {
        if (p.type != sst::basic_blocks::params::ParamMetaData::NONE)
        {
            g.setColour(juce::Colours::white);
            g.drawText(p.name, r, juce::Justification::left);

            auto s = p.valueToString(b.second.params[idx]);
            if (s.has_value())
                g.drawText(*s, r, juce::Justification::right);
        }
        r = r.translated(0, 20);
        idx++;
    }
}

void PartEffectsPane::rebuild()
{
    auto &b = mixer->busEffectsData[busAddress][fxSlot];
    auto &params = b.first;
    auto &data = b.second;
    auto t = data.type;
    name = effectDisplayName(t, false);

    removeAllChildren();
    components.clear();
    // intAttachments.clear();
    // floatAttachments.clear();

    if (getHeight() < 10)
        return;

    switch (t)
    {
    default:
        rebuildDefaultLayout();
        break;
    }

    repaint();
}

std::string PartEffectsPane::effectDisplayName(engine::AvailableBusEffects t, bool forMenu)
{
    switch (t)
    {
    case engine::none:
        return forMenu ? "None" : "FX";
    case engine::flanger:
        return forMenu ? "Flanger" : "FLANGER";
    case engine::delay:
        return forMenu ? "Delay" : "DELAY";
    case engine::reverb1:
        return forMenu ? "Reverb 1" : "REVERB 1";
    case engine::bonsai:
        return forMenu ? "Bonsai" : "BONSAI";
    }

    return "GCC gives strictly correct, but not useful in this case, warnings";
}

template <typename T> T *PartEffectsPane::attachWidgetToFloat(int pidx)
{
    auto &data = mixer->busEffectsData[busAddress][fxSlot];
    auto &pmd = data.first[pidx];
    auto onGuiChange = [w = juce::Component::SafePointer(this), pidx](auto &a) {
        if (w)
            w->floatParameterChangedFromGui(a, pidx);
    };

    auto at = std::make_unique<attachment_t>(
        this, pmd, onGuiChange,
        [w = juce::Component::SafePointer(this), pidx](const typename attachment_t::payload_t &pl) {
            if (w)
                return w->mixer->busEffectsData[w->busAddress][w->fxSlot].second.params[pidx];
            return 0.f;
        },
        mixer->busEffectsData[busAddress][fxSlot].second.params[pidx]);
    auto w = std::make_unique<T>();
    w->setSource(at.get());

    w->onBeginEdit = [this, q = juce::Component::SafePointer(w.get()), atPtr = at.get()]() {
        if (q)
            editor->showTooltip(*q);
        updateTooltip(*atPtr);
    };
    w->onEndEdit = [this]() { editor->hideTooltip(); };

    addAndMakeVisible(*w);

    auto retVal = w.get();
    components.insert(std::move(w));
    floatAttachments.insert(std::move(at));

    return retVal;
}

juce::Component *PartEffectsPane::attachMenuButtonToInt(int index)
{
    // TODO - this really needs some contemplation. I bet it wil lbe a pretty
    // common pattern. Where to put it?
    auto &b = mixer->busEffectsData[busAddress][fxSlot];
    auto &pmd = b.first;
    auto &bes = b.second;

    auto r = std::make_unique<jcmp::MenuButton>();
    auto sv = pmd[index].valueToString(bes.params[index]);
    if (sv.has_value())
        r->setLabel(*sv);
    r->setOnCallback([r = juce::Component::SafePointer(r.get()),
                      w = juce::Component::SafePointer(this), index]() {
        if (!w)
            return;
        auto &b = w->mixer->busEffectsData[w->busAddress][w->fxSlot];
        auto &pmd = b.first[index];
        auto &bes = b.second;

        auto p = juce::PopupMenu();
        p.addSectionHeader(pmd.name);
        p.addSeparator();
        for (int i = pmd.minVal; i <= pmd.maxVal; ++i)
        {
            auto sv = pmd.valueToString(i);
            if (sv.has_value())
                p.addItem(*sv, [w, r, sv, i, index]() {
                    if (w)
                    {
                        auto &bes = w->mixer->busEffectsData[w->busAddress][w->fxSlot].second;
                        bes.params[index] = i;
                        if (r)
                        {
                            r->setLabel(*sv);
                            r->repaint();
                        }

                        w->sendToSerialization(
                            cmsg::SetBusEffectStorage({w->busAddress, w->fxSlot, bes}));
                    }
                });
        }
        p.showMenuAsync(w->editor->defaultPopupMenuOptions());
    });
    addAndMakeVisible(*r);
    auto retVal = r.get();
    components.insert(std::move(r));
    return retVal;
}

void PartEffectsPane::rebuildDefaultLayout()
{
    namespace jcmp = sst::jucegui::components;
    // Just a stack of modulation style hsliders with labels
    int labht{15};
    static_assert(engine::BusEffectStorage::maxBusEffectParams <= 12,
                  "If this is false, this 3x4 grid will be bad");
    auto bx = std::min((getHeight() / 4.f + labht), getWidth() / 3.f) * 0.9f;
    auto &b = mixer->busEffectsData[busAddress][fxSlot];

    auto into = getContentArea();
    auto r =
        into.withHeight(bx + labht).withWidth(bx).translated((into.getWidth() - bx * 3) * 0.5, 0);
    int idx{0};
    for (const auto &p : b.first)
    {
        if (p.type != sst::basic_blocks::params::ParamMetaData::NONE)
        {
            auto l = std::make_unique<jcmp::Label>();
            l->setText(p.name);
            l->setBounds(r);
            l->setJustification(juce::Justification::centredBottom);
            addAndMakeVisible(*l);
            components.insert(std::move(l));

            if (p.type == sst::basic_blocks::params::ParamMetaData::FLOAT)
            {
                // make slidera
                auto w = attachWidgetToFloat<jcmp::Knob>(idx);
                w->setBounds(r.withHeight(bx).reduced(2));
            }
            else if (p.type == sst::basic_blocks::params::ParamMetaData::INT)
            {
                // make menu
                auto w = attachMenuButtonToInt(idx);
                w->setBounds(r.withHeight(bx).withSizeKeepingCentre(r.getWidth(), 24));
            }
            else
            {
                SCLOG("No widget for type " << p.type << " / " << p.name);
            }

            r = r.translated(bx, 0);
            if (!into.contains(r))
            {
                r = r.withX(into.getX()).translated((into.getWidth() - bx * 3) * 0.5, bx + labht);
            }
        }
        idx++;
    }
}

void PartEffectsPane::floatParameterChangedFromGui(
    const scxt::ui::mixer::PartEffectsPane::attachment_t &at, int idx)
{
    updateTooltip(at);
    auto &data = mixer->busEffectsData[busAddress][fxSlot];

    sendToSerialization(cmsg::SetBusEffectStorage({busAddress, fxSlot, data.second}));
}

void PartEffectsPane::updateTooltip(const attachment_t &at)
{
    editor->setTooltipContents(at.label, at.description.valueToString(at.value).value_or("Error"));
}

} // namespace scxt::ui::mixer