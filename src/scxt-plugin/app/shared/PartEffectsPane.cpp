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

#include "PartEffectsPane.h"
#include "app/SCXTEditor.h"
#include "app/mixer-screen/MixerScreen.h"
#include "app/edit-screen/components/PartEditScreen.h"

#include "connectors/JsonLayoutEngineSupport.h"

#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/RuledLabel.h"
#include "sst/jucegui/components/MenuButton.h"
#include "sst/jucegui/components/JogUpDownButton.h"
#include "sst/effects/EffectsPresetSupport.h"

namespace scxt::ui::app::shared
{
namespace cmsg = scxt::messaging::client;
namespace jcmp = sst::jucegui::components;

template <bool forBus>
void PartEffectsPane<forBus>::showFXSelectionMenu(parent_t *parent, int ba, int sl)
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Effects");
    p.addSeparator();
    auto go = [w = juce::Component::SafePointer(parent), ba, sl](auto q) {
        return [w, q, ba, sl]() {
            if (w)
            {
                w->setFXSlotToType(ba, sl, q);
            }
        };
    };
    auto add = [&go, &p](auto q) {
        p.addItem(shared::PartEffectsPane<true>::effectDisplayName(q, true), go(q));
    };
    add(engine::AvailableBusEffects::none);
    add(engine::AvailableBusEffects::reverb1);
    add(engine::AvailableBusEffects::reverb2);
    add(engine::AvailableBusEffects::delay);
    add(engine::AvailableBusEffects::floatydelay);
    add(engine::AvailableBusEffects::flanger);
    add(engine::AvailableBusEffects::phaser);
    add(engine::AvailableBusEffects::treemonster);
    add(engine::AvailableBusEffects::nimbus);
    add(engine::AvailableBusEffects::bonsai);
    p.showMenuAsync(parent->editor->defaultPopupMenuOptions());
}

template <bool forBus> PartEffectsPane<forBus>::~PartEffectsPane()
{
    removeAllChildren();
    // order matters here
    components.clear();
    floatAttachments.clear();
}

template <bool forBus>
void PartEffectsPane<forBus>::paintMetadata(juce::Graphics &g, const juce::Rectangle<int> &into)
{
    auto &b = getPartFXStorage();

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

template <bool forBus>
typename PartEffectsPane<forBus>::partFXStorage_t &PartEffectsPane<forBus>::getPartFXStorage()
{
    if (busAddressOrPart < 0 || fxSlot < 0)
    {
        // once you find this fix those maxes below
        SCLOG_ONCE_IF(debug, "Investigate: PartEffectsPane<"
                                 << forBus << "> busAddress or fxSlot misconfigured "
                                 << busAddressOrPart << " " << fxSlot << " - will use zero");
    }
    if constexpr (forBus)
    {
        return parent->busEffectsData[std::max(busAddressOrPart, 0)][std::max(fxSlot, 0)];
    }
    else
    {
        return parent->partsEffectsData[std::max(busAddressOrPart, 0)][std::max(fxSlot, 0)];
    }
}

template <bool forBus>
const typename PartEffectsPane<forBus>::partFXStorage_t &
PartEffectsPane<forBus>::getPartFXStorage() const
{
    if (busAddressOrPart < 0 || fxSlot < 0)
    {
        // once you find this fix those maxes below
        SCLOG_ONCE_IF(debug, "Investigate: PartEffectsPane<"
                                 << forBus << "> busAddress or fxSlot misconfigured "
                                 << busAddressOrPart << " " << fxSlot << " - will use zero");
    }
    if constexpr (forBus)
    {
        return parent->busEffectsData[std::max(busAddressOrPart, 0)][std::max(fxSlot, 0)];
    }
    else
    {
        return parent->partsEffectsData[std::max(busAddressOrPart, 0)][std::max(fxSlot, 0)];
    }
}

template <bool forBus> void PartEffectsPane<forBus>::rebuild()
{
    auto &b = getPartFXStorage();
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

#define CS(x)                                                                                      \
    case engine::AvailableBusEffects::x:                                                           \
        rebuildFromJSONLibrary("bus-effects/" #x ".json");                                         \
        break;

#define NS(x)                                                                                      \
    case engine::AvailableBusEffects::x:                                                           \
        rebuildWithJsonLayoutEngine("partfx-layouts/" #x ".json");                                 \
        break;

    switch (t)
    {
        NS(reverb1);
        NS(reverb2)
        NS(flanger);
        NS(delay);
        NS(floatydelay);
        NS(nimbus);
        NS(phaser);
        NS(treemonster);
        NS(rotaryspeaker);
        NS(bonsai);

    case engine::AvailableBusEffects::none:
        // Explicitly do nothing
        break;
    }

#undef CS

    clearAdditionalHamburgerComponents();
    bool hasTemposync{false};
    for (auto &p : params)
        hasTemposync = hasTemposync || p.canTemposync;

    if (hasTemposync)
    {
        auto res = std::make_unique<jcmp::ToggleButton>();
        res->drawMode = sst::jucegui::components::ToggleButton::DrawMode::GLYPH;
        res->setGlyph(jcmp::GlyphPainter::GlyphType::METRONOME);

        auto &data = getPartFXStorage();
        auto onGuiChange = [w = juce::Component::SafePointer(this)](auto &a) {
            if (w)
            {
                // Fix this 0
                w->busEffectStorageChangedFromGUI(a, 0);
                // Need to finish this gui action before rebuilding, which could destroy myself
                juce::Timer::callAfterDelay(0, [w]() {
                    if (w)
                        w->rebuild();
                });
                // w->rebuild();
            }
        };

        auto at = std::make_unique<boolAttachment_t>("Temposync", onGuiChange,
                                                     getPartFXStorage().second.isTemposync);
        res->setSource(at.get());
        setupWidgetForValueTooltip(res.get(), at);
        addAndMakeVisible(*res);

        boolAttachments.insert(std::move(at));
        addAdditionalHamburgerComponent(std::move(res));

        for (auto &a : floatAttachments)
        {
            if (a->description.canTemposync)
            {
                a->setTemposyncFunction([w = juce::Component::SafePointer(this)](const auto &a) {
                    if (w)
                        return w->getPartFXStorage().second.isTemposync;
                    return false;
                });
            }
        }
    }

    {
        auto onGuiChange = [w = juce::Component::SafePointer(this)](auto &a) {
            if (w)
            {
                // Fix this 0
                w->busEffectStorageChangedFromGUI(a, 0);
            }
        };
        auto at =
            std::make_unique<boolAttachment_t>(effectDisplayName(t, true) + " Active", onGuiChange,
                                               getPartFXStorage().second.isActive);
        setToggleDataSource(at.get());
        setupWidgetForValueTooltip(toggleButton.get(), at);
        boolAttachments.insert(std::move(at));
        addAndMakeVisible(*toggleButton);
    }

    repaint();
}

template <bool forBus>
template <typename T>
T *PartEffectsPane<forBus>::attachWidgetToFloat(int pidx)
{
    auto &data = getPartFXStorage();
    auto &pmd = data.first[pidx];
    auto onGuiChange = [w = juce::Component::SafePointer(this), pidx](auto &a) {
        if (w)
            w->busEffectStorageChangedFromGUI(a, pidx);
    };

    auto at =
        std::make_unique<attachment_t>(pmd, onGuiChange, getPartFXStorage().second.params[pidx]);

    if (pmd.canTemposync)
    {
        at->setTemposyncFunction([w = juce::Component::SafePointer(this)](const auto &a) {
            if (w)
                return w->getPartFXStorage().second.isTemposync;
            return false;
        });
    }
    auto w = std::make_unique<T>();
    w->setSource(at.get());

    if (pmd.canDeactivate && data.second.isDeactivated(pidx))
    {
        w->setEnabled(false);
    }
    else
    {
        w->setEnabled(true);
    }

    setupWidgetForValueTooltip(w.get(), at);
    addAndMakeVisible(*w);

    if constexpr (std::is_same_v<T, jcmp::Knob>)
    {
        w->setDrawLabel(false);
    }

    auto retVal = w.get();
    components.insert(std::move(w));
    floatAttachments.insert(std::move(at));

    return retVal;
}

template <bool forBus>
template <typename T>
juce::Component *PartEffectsPane<forBus>::attachWidgetToInt(int pidx)
{
    return nullptr;
}

template <bool forBus> juce::Component *PartEffectsPane<forBus>::attachMenuButtonToInt(int index)
{
    // TODO - this really needs some contemplation. I bet it wil lbe a pretty
    // common pattern. Where to put it?
    auto &b = getPartFXStorage();
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
        auto &b = w->getPartFXStorage();
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
                        auto &bes = w->getPartFXStorage().second;
                        bes.params[index] = i;
                        if (r)
                        {
                            r->setLabel(*sv);
                            r->repaint();
                        }

                        if (forBus)
                        {
                            w->sendToSerialization(cmsg::SetBusEffectStorage(
                                {w->busAddressOrPart, -1, w->fxSlot, bes}));
                        }
                        else
                        {
                            w->sendToSerialization(cmsg::SetBusEffectStorage(
                                {-1, w->busAddressOrPart, w->fxSlot, bes}));
                        }
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

template <bool forBus>
juce::Component *PartEffectsPane<forBus>::attachToggleToDeactivated(int index)
{
    auto res = std::make_unique<jcmp::ToggleButton>();
    res->drawMode = sst::jucegui::components::ToggleButton::DrawMode::GLYPH;
    res->setGlyph(jcmp::GlyphPainter::GlyphType::SMALL_POWER_LIGHT);

    auto &data = getPartFXStorage();
    auto &pmd = data.first[index];
    auto onGuiChange = [w = juce::Component::SafePointer(this), index](auto &a) {
        if (w)
        {
            w->busEffectStorageChangedFromGUI(a, index);
            // Need to finish this gui action before rebuilding, which could destroy myself
            juce::Timer::callAfterDelay(0, [w]() {
                if (w)
                    w->rebuild();
            });
            // w->rebuild();
        }
    };

    auto at = std::make_unique<boolAttachment_t>("Active", onGuiChange,
                                                 getPartFXStorage().second.deact[index]);
    at->isInverted = true; // deact is no power light mkay
    res->setSource(at.get());
    addAndMakeVisible(*res);

    auto retVal = res.get();
    components.insert(std::move(res));
    boolAttachments.insert(std::move(at));

    return retVal;
}

template <bool forBus>
template <typename T>
juce::Component *PartEffectsPane<forBus>::addTypedLabel(const std::string &txt)
{
    auto l = std::make_unique<T>();
    l->setText(txt);
    if constexpr (std::is_same_v<T, jcmp::Label>)
        l->setJustification(juce::Justification::centred);
    addAndMakeVisible(*l);
    auto res = l.get();
    components.insert(std::move(l));
    return res;
}

template <bool forBus> void PartEffectsPane<forBus>::rebuildDefaultLayout()
{
    namespace jcmp = sst::jucegui::components;
    // Just a stack of modulation style hsliders with labels
    int labht{15};
    static_assert(engine::BusEffectStorage::maxBusEffectParams <= 12,
                  "If this is false, this 3x4 grid will be bad");
    auto bx = std::min(getHeight() / 4.f - labht, getWidth() / 3.f) * 0.9f;
    auto &b = getPartFXStorage();

    auto into = getContentArea();
    auto r =
        into.withHeight(bx + labht).withWidth(bx).translated((into.getWidth() - bx * 3) * 0.5, 0);
    int idx{0};
    bool hasTemposync{false};
    for (const auto &p : b.first)
    {
        if (p.canTemposync)
            hasTemposync = true;
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
                SCLOG_IF(jsonUI, "No widget for type " << p.type << " / " << p.name);
            }

            if (p.canDeactivate)
            {
                auto pbsz{12};
                auto da = attachToggleToDeactivated(idx);
                auto q =
                    r.withTrimmedLeft(r.getWidth() - pbsz).withTrimmedBottom(r.getHeight() - pbsz);
                da->setBounds(q);
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

template <bool forBus> void PartEffectsPane<forBus>::showHamburger()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Presets");
    p.addSeparator();
    p.addItem("Save Preset...", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->savePreset();
    });
    p.addItem("Load Preset...", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->loadPreset();
    });
    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

template <bool forBus> void PartEffectsPane<forBus>::loadPreset()
{
    auto dir = editor->browser.busFxPresetDirectory;
    auto &pes = getPartFXStorage();
    auto sfx = engine::getBusEffectDisplayName(pes.second.type);
    dir = dir / sfx;
    try
    {
        if (!fs::exists(dir))
            dir = editor->browser.voiceFxPresetDirectory;
    }
    catch (fs::filesystem_error &)
    {
    }
    editor->fileChooser = std::make_unique<juce::FileChooser>(
        "Load Effect Preset", juce::File(dir.u8string()), "*.busfx");

    editor->fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::openMode,
        [w = juce::Component::SafePointer(this)](const juce::FileChooser &c) {
            if (!w)
                return;
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            auto str = result[0].loadFileAsString().toStdString();
            w->applyPresetXML(str);
        });
}

template <bool forBus> struct PresetProviderAdapter
{
    const PartEffectsPane<forBus> &parent;
    const typename PartEffectsPane<forBus>::partFXStorage_t &pes;
    explicit PresetProviderAdapter(const PartEffectsPane<forBus> &p)
        : parent(p), pes(p.getPartFXStorage())
    {
    }

    size_t provideParamCount() const { return pes.first.size(); }
    bool isParamInt(size_t idx) const
    {
        return pes.first[idx].type != sst::basic_blocks::params::ParamMetaData::FLOAT;
    }
    float provideFloatParam(size_t idx) const { return pes.second.params[idx]; }
    int provideIntParam(size_t idx) const { return static_cast<int>(pes.second.params[idx]); }
    bool provideDeactivated(size_t idx) const { return pes.second.deact[idx]; }
    bool provideExtended(size_t idx) const { return false; }
    bool provideTemposync(size_t idx) const { return pes.second.isTemposync; }

    int provideStreamingVersion() const { return pes.second.streamingVersion; }
    std::string provideStreamingName() const
    {
        return engine::getBusEffectStreamingName(pes.second.type);
    }
};

template <bool forBus> struct PresetReceiverAdapter
{
    PartEffectsPane<forBus> &parent;
    typename PartEffectsPane<forBus>::partFXStorage_t &pes;
    explicit PresetReceiverAdapter(PartEffectsPane<forBus> &p)
        : parent(p), pes(p.getPartFXStorage())
    {
    }

    bool canReceiveForStreamingName(const std::string &s) { return true; }
    int streamingVersion;
    bool receiveStreamingVersion(int v)
    {
        streamingVersion = v;
        return true;
    }
    bool receiveFloatParam(size_t idx, float f)
    {
        pes.second.params[idx] = f;
        return true;
    }
    bool receiveIntParam(size_t idx, int i)
    {
        pes.second.params[idx] = i;
        return true;
    }
    bool receiveDeactivated(size_t idx, bool b)
    {
        pes.second.deact[idx] = b;
        return true;
    }
    bool receiveExtended(size_t idx, bool b) { return true; }
    bool receiveTemposync(size_t idx, bool b)
    {
        pes.second.isTemposync = b;
        return true;
    }

    void onError(const std::string &msg) { parent.editor->displayError("Preset Load Error", msg); }
};

template <bool forBus> void PartEffectsPane<forBus>::savePreset()
{
    auto dir = editor->browser.busFxPresetDirectory;
    auto &pes = getPartFXStorage();
    auto sfx = engine::getBusEffectDisplayName(pes.second.type);
    dir = dir / sfx;
    try
    {
        fs::create_directories(dir);
    }
    catch (fs::filesystem_error &)
    {
    }
    editor->fileChooser = std::make_unique<juce::FileChooser>(
        "Save Effect Preset", juce::File(dir.u8string()), "*.busfx");

    editor->fileChooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::warnAboutOverwriting,
        [w = juce::Component::SafePointer(this)](const juce::FileChooser &c) {
            if (!w)
                return;
            auto result = c.getResults();
            if (result.isEmpty() || result.size() > 1)
            {
                return;
            }
            auto ppa = PresetProviderAdapter<forBus>(*w);
            auto psv = sst::effects::presets::toPreset(ppa);
            if (!psv.empty())
            {
                if (!result[0].replaceWithText(psv))
                {
                    w->editor->displayError("File Save", "Unable to save file");
                }
            }
        });
}

template <bool forBus> void PartEffectsPane<forBus>::applyPresetXML(const std::string &str)
{
    auto &pes = getPartFXStorage();

    auto pra = PresetReceiverAdapter(*this);
    auto psv = sst::effects::presets::fromPreset(str, pra);
    if (psv)
    {
        // TODO : Streaming version
        if (pra.streamingVersion != pes.second.streamingVersion)
        {
            editor->displayError("Preset Load Error", "Preset version mismatch - code this!");
        }
        busEffectStorageChangedFromGUI();
        rebuild();
    }
    else
    {
        // FIX
    }
}

template <bool forBus>
template <typename Att>
void PartEffectsPane<forBus>::busEffectStorageChangedFromGUI(const Att &at, int idx)
{
    updateValueTooltip(at);
    busEffectStorageChangedFromGUI();
}

template <bool forBus> void PartEffectsPane<forBus>::busEffectStorageChangedFromGUI()
{
    auto &data = getPartFXStorage();

    if (forBus)
        sendToSerialization(cmsg::SetBusEffectStorage({busAddressOrPart, -1, fxSlot, data.second}));
    else
        sendToSerialization(cmsg::SetBusEffectStorage({-1, busAddressOrPart, fxSlot, data.second}));
}

template <bool forBus>
std::string PartEffectsPane<forBus>::effectDisplayName(engine::AvailableBusEffects t, bool forMenu)
{
    switch (t)
    {
    case engine::none:
        return forMenu ? "None" : "FX";
    case engine::flanger:
        return forMenu ? "Flanger" : "FLANGER";
    case engine::phaser:
        return forMenu ? "Phaser" : "PHASER";
    case engine::delay:
        return forMenu ? "Delay" : "DELAY";
    case engine::floatydelay:
        return forMenu ? "Floaty Delay" : "FLOATY DELAY";
    case engine::reverb1:
        return forMenu ? "Reverb 1" : "REVERB 1";
    case engine::reverb2:
        return forMenu ? "Reverb 2" : "REVERB 2";
    case engine::nimbus:
        return forMenu ? "Nimbus" : "NIMBUS";
    case engine::treemonster:
        return forMenu ? "TreeMonster" : "TREEMONSTER";
    case engine::bonsai:
        return forMenu ? "Bonsai" : "BONSAI";
    case engine::rotaryspeaker:
        return forMenu ? "Rotary Speaker" : "ROTARY";
    }

    return "GCC gives strictly correct, but not useful in this case, warnings";
}

template <bool forBus> void PartEffectsPane<forBus>::mouseDrag(const juce::MouseEvent &event)
{
    if (event.getDistanceFromDragStart() > 2)
    {
        if (!isDragging)
        {
            if (auto *container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            {
                container->startDragging("BusEffect", this);
                isDragging = true;
                swapFX = !event.mods.isShiftDown();
            }
        }
    }
}

template <bool forBus>
void PartEffectsPane<forBus>::itemDropped(const SourceDetails &dragSourceDetails)
{
    setIsAccented(false);
    repaint();
    if (dragSourceDetails.sourceComponent.get() != this)
    {
        auto fxs = dragSourceDetails.sourceComponent.get();
        auto fc = dynamic_cast<FXSlotBearing *>(fxs);
        if (fc && fc != this)
        {
            parent->swapEffects(fc->busAddressOrPart, fc->fxSlot, busAddressOrPart, fxSlot);
        }
    }
}

template <bool forBus>
void PartEffectsPane<forBus>::rebuildWithJsonLayoutEngine(const std::string &path)
{
    // order matters here
    components.clear();
    floatAttachments.clear();
    std::fill(intProxyAttachments.begin(), intProxyAttachments.end(), nullptr);

    auto resJ = scxt::ui::connectors::jsonlayout::resolveJsonFile(path);
    if (!resJ.has_value())
    {
        rebuildDefaultLayout();
        return;
    }
    auto eng = sst::jucegui::layouts::JsonLayoutEngine(*this);

    auto res = eng.processJsonPath(path);
    if (!res)
    {
        rebuildDefaultLayout();
        editor->displayError("JSON Parsing Error", *res.error);
    }
}

template <bool forBus>
std::string PartEffectsPane<forBus>::resolveJsonPath(const std::string &path) const
{
    auto res = scxt::ui::connectors::jsonlayout::resolveJsonFile(path);
    if (res.has_value())
        return *res;
    editor->displayError("JSON Path Error", "Could not resolve path '" + path + "'");
    return {};
}

template <bool forBus>
void PartEffectsPane<forBus>::createBindAndPosition(
    const sst::jucegui::layouts::json_document::Control &ctrl,
    const sst::jucegui::layouts::json_document::Class &cls)
{
    auto onError = [this, ctrl](const auto &s) {
        editor->displayError("JSON Error : " + ctrl.name, s);
    };
    auto intAttI = [](int i) { return 0; };

    auto zeroPoint = getContentArea().getTopLeft();

    if (ctrl.binding.has_value() && ctrl.binding->type == "float")
    {
        auto pidx = ctrl.binding->index;
        auto &data = getPartFXStorage();
        auto &pmd = data.first[pidx];
        auto onGuiChange = [w = juce::Component::SafePointer(this), pidx](auto &a) {
            if (w)
                w->busEffectStorageChangedFromGUI(a, pidx);
        };

        auto at = std::make_unique<attachment_t>(pmd, onGuiChange,
                                                 getPartFXStorage().second.params[pidx]);

        // auto viz = scxt::ui::connectors::jsonlayout::isVisible(ctrl, intAttI, onError);
        // if (!viz)
        //            return;

        auto ed = connectors::jsonlayout::createContinuousWidget(ctrl, cls, pmd);
        if (!ed)
        {
            onError("Could not create widget for " + ctrl.name + " with unknown control type " +
                    cls.controlType);
            return;
        }

        connectors::jsonlayout::attachAndPosition(this, ed, at, ctrl, cls, zeroPoint);
        addAndMakeVisible(*ed);

        const auto &metadata = getPartFXStorage().first;

        if (metadata[pidx].canDeactivate)
        {
            auto power = attachToggleToDeactivated(pidx);
            power->setBounds(zeroPoint.x + ctrl.position.x + ctrl.position.w - 4,
                             zeroPoint.y + ctrl.position.y, 10, 10);
            auto en = !getPartFXStorage().second.deact[pidx];
            ed->setEnabled(en);
        }

        if (auto lab = connectors::jsonlayout::createControlLabel(ctrl, cls, *this, zeroPoint))
        {
            addAndMakeVisible(*lab);
            jsonLabels.push_back(std::move(lab));
        }

        components.insert(std::move(ed));
        floatAttachments.insert(std::move(at));
    }
    else if (ctrl.binding.has_value() && ctrl.binding->type == "int")
    {
        auto pidx = ctrl.binding->index;
        if (pidx < 0 || pidx >= scxt::maxBusEffectParams)
        {
            onError("Index " + std::to_string(pidx) + " is out of range on " + ctrl.name);
            return;
        }
        auto &data = getPartFXStorage();
        auto &pmd = data.first[pidx];
        auto onGuiChange = [w = juce::Component::SafePointer(this), pidx](auto &a) {
            if (w)
                w->busEffectStorageChangedFromGUI(a, pidx);
        };

        auto at = std::make_unique<attachment_t>(pmd, onGuiChange,
                                                 getPartFXStorage().second.params[pidx]);
        auto ipt = std::make_unique<intProxyAttachment_t>(*at);

        auto ed = connectors::jsonlayout::createDiscreteWidget(ctrl, cls, onError);
        if (!ed)
        {
            onError("Unable to create editor");
            return;
        }

        addAndMakeVisible(*ed);
        connectors::jsonlayout::attachAndPosition(this, ed, ipt, ctrl, cls, zeroPoint);

        floatAttachments.insert(std::move(at));
        intProxyAttachments[pidx] = std::move(ipt);
        components.insert(std::move(ed));
    }
    else if (auto nw = connectors::jsonlayout::createAndPositionNonDataWidget(ctrl, cls, onError,
                                                                              zeroPoint))
    {
        addAndMakeVisible(*nw);
        jsonLabels.push_back(std::move(nw));
    }
}

// Explicit Instantiate
template struct PartEffectsPane<true>;
template struct PartEffectsPane<false>;

} // namespace scxt::ui::app::shared
