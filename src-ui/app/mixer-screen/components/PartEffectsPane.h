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

#ifndef SCXT_SRC_UI_COMPONENTS_MIXER_PARTEFFECTSPANE_H
#define SCXT_SRC_UI_COMPONENTS_MIXER_PARTEFFECTSPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/NamedPanel.h>
#include <sst/jucegui/components/Label.h>
#include <sst/jucegui/components/GlyphPainter.h>
#include <sst/jucegui/layouts/ExplicitLayout.h>
#include <unordered_set>

#include "app/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "engine/bus.h"

namespace scxt::ui::app::mixer_screen
{
struct MixerScreen;

struct PartEffectsPane : public HasEditor, sst::jucegui::components::NamedPanel
{

    int fxSlot{0}, busAddress{0};
    MixerScreen *mixer{nullptr};
    PartEffectsPane(SCXTEditor *e, MixerScreen *ms, int i)
        : HasEditor(e), mixer(ms), fxSlot(i), sst::jucegui::components::NamedPanel("FX")
    {
        hasHamburger = true;
        onHamburger = [w = juce::Component::SafePointer(this)] {
            if (w)
                w->showHamburger();
        };
        hasHamburger = true;
        setTogglable(true);
    }
    ~PartEffectsPane();

    void setBusAddress(int ba)
    {
        busAddress = ba;
        if (isVisible())
            rebuild();
    }
    void resized() override { rebuild(); }

    void rebuild();
    void showHamburger();

    void paintMetadata(juce::Graphics &g, const juce::Rectangle<int> &into);

    std::unordered_set<std::unique_ptr<juce::Component>> components;
    typedef connectors::PayloadDataAttachment<engine::BusEffectStorage> attachment_t;
    typedef connectors::BooleanPayloadDataAttachment<engine::BusEffectStorage> boolAttachment_t;
    std::unordered_set<std::unique_ptr<attachment_t>> floatAttachments;
    std::unordered_set<std::unique_ptr<boolAttachment_t>> boolAttachments;

  protected:
    // This returns a pointer which is owned by the class; do not delete it or
    // move it.
    template <typename T> T *attachWidgetToFloat(int index);
    juce::Component *attachMenuButtonToInt(int index);
    template <typename T> juce::Component *attachWidgetToInt(int index);
    juce::Component *attachToggleToDeactivated(int index);
    template <typename T> juce::Component *addTypedLabel(const std::string &txt);
    juce::Component *addLabel(const std::string &txt)
    {
        return addTypedLabel<sst::jucegui::components::Label>(txt);
    }

    template <typename T>
    T *layoutWidgetToFloat(const sst::jucegui::layout::ExplicitLayout &elo, int index,
                           const std::string &lotag)
    {
        auto r = attachWidgetToFloat<T>(index);
        r->setBounds(elo.positionFor(lotag));
        return r;
    }
    template <typename T>
    T *layoutWidgetToFloat(const sst::jucegui::layout::ExplicitLayout &elo, int index,
                           const std::string &lotag, const std::string &label)
    {
        auto r = layoutWidgetToFloat<T>(elo, index, lotag);
        auto l = addLabel(label);
        l->setBounds(elo.labelPositionFor(lotag));
        return r;
    }

    // Generic, no JSON
    void rebuildDefaultLayout();
    void rebuildFromJSONLibrary(const std::string &path);

    template <typename Att> void busEffectStorageChangedFromGUI(const Att &at, int idx);

    // Specific
    // void rebuildDelayLayout();
};
} // namespace scxt::ui::app::mixer_screen

#endif // SHORTCIRCUITXT_PARTEFFECTSPANEL_H
