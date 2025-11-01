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

#ifndef SCXT_SRC_UI_APP_SHARED_PARTEFFECTSPANE_H
#define SCXT_SRC_UI_APP_SHARED_PARTEFFECTSPANE_H

#include <unordered_set>
#include <type_traits>

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/NamedPanel.h>
#include <sst/jucegui/components/Label.h>
#include <sst/jucegui/components/GlyphPainter.h>
#include <sst/jucegui/layouts/ExplicitLayout.h>

#include "app/HasEditor.h"
#include "connectors/PayloadDataAttachment.h"
#include "engine/bus.h"
#include "FXSlotBearing.h"
#include "sst/jucegui/layouts/JsonLayoutEngine.h"

namespace scxt::ui::app
{
namespace mixer_screen
{
struct MixerScreen;
}

namespace edit_screen
{
struct PartEditScreen;
}
namespace shared
{
template <bool forBus>
struct PartEffectsPane : public HasEditor,
                         sst::jucegui::components::NamedPanel,
                         juce::DragAndDropTarget,
                         FXSlotBearing,
                         sst::jucegui::layouts::JsonLayoutHost
{
    static constexpr int width{198}, height{290};

    using parent_t =
        std::conditional_t<forBus, mixer_screen::MixerScreen, edit_screen::PartEditScreen>;
    parent_t *parent{nullptr};
    PartEffectsPane(SCXTEditor *e, parent_t *ms, int i)
        : HasEditor(e), FXSlotBearing(i, -1, forBus), parent(ms),
          sst::jucegui::components::NamedPanel("FX")
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

    using partFXMD_t = std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams>;
    using partFXStorage_t = std::pair<partFXMD_t, engine::BusEffectStorage>;
    partFXStorage_t &getPartFXStorage();

    void setBusAddress(int ba)
    {
        assert(forBus);
        busAddressOrPart = ba;
        if (isVisible())
            rebuild();
    }
    void setSelectedPart(int pt)
    {
        assert(!forBus);
        busAddressOrPart = pt;
        if (isVisible())
            rebuild();
    }

    void resized() override { rebuild(); }

    void rebuild();
    void showHamburger() { showFXSelectionMenu(parent, busAddressOrPart, fxSlot); };
    static void showFXSelectionMenu(parent_t *p, int b, int s);

    void paintMetadata(juce::Graphics &g, const juce::Rectangle<int> &into);

    bool isDragging{false}, swapFX{true};
    void mouseDown(const juce::MouseEvent &event) override
    {
        isDragging = false;
        swapFX = true;
        NamedPanel::mouseDown(event);
    }
    void mouseDrag(const juce::MouseEvent &event) override;

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
    {
        return dynamic_cast<FXSlotBearing *>(dragSourceDetails.sourceComponent.get()) != nullptr;
    }
    void itemDragEnter(const SourceDetails &dragSourceDetails) override
    {
        if (dragSourceDetails.sourceComponent.get() != this)
            setIsAccented(true);
        repaint();
    }
    void itemDragExit(const SourceDetails &dragSourceDetails) override
    {
        if (dragSourceDetails.sourceComponent.get() != this)
            setIsAccented(false);
        repaint();
    };
    void itemDropped(const SourceDetails &dragSourceDetails) override;

    std::unordered_set<std::unique_ptr<juce::Component>> components;
    using attachment_t = connectors::PayloadDataAttachment<engine::BusEffectStorage>;
    using boolAttachment_t = connectors::BooleanPayloadDataAttachment<engine::BusEffectStorage>;
    using intProxyAttachment_t = connectors::DiscreteFromFloatAdapterAttachment<attachment_t>;
    std::unordered_set<std::unique_ptr<attachment_t>> floatAttachments; // todo move this to array
    std::array<std::unique_ptr<intProxyAttachment_t>, scxt::maxBusEffectParams> intProxyAttachments;
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
    T *layoutWidgetToFloat(const sst::jucegui::layouts::ExplicitLayout &elo, int index,
                           const std::string &lotag)
    {
        auto r = attachWidgetToFloat<T>(index);
        r->setBounds(elo.positionFor(lotag));
        return r;
    }
    template <typename T>
    T *layoutWidgetToFloat(const sst::jucegui::layouts::ExplicitLayout &elo, int index,
                           const std::string &lotag, const std::string &label)
    {
        auto r = layoutWidgetToFloat<T>(elo, index, lotag);
        auto l = addLabel(label);
        l->setBounds(elo.labelPositionFor(lotag));
        return r;
    }

    // Generic, no JSON
    void rebuildDefaultLayout();

    void rebuildWithJsonLayoutEngine(const std::string &path);
    std::vector<std::unique_ptr<juce::Component>> jsonLabels;

  public:
    std::string resolveJsonPath(const std::string &path) const override;
    void createBindAndPosition(const sst::jucegui::layouts::json_document::Control &,
                               const sst::jucegui::layouts::json_document::Class &) override;

  protected:
    template <typename Att> void busEffectStorageChangedFromGUI(const Att &at, int idx);

  public:
    // The effects have names like 'flanger' and 'delay' internally but we
    // want alternate display names here.
    static std::string effectDisplayName(engine::AvailableBusEffects, bool forMenu);

    // Specific
    // void rebuildDelayLayout();
};
} // namespace shared
} // namespace scxt::ui::app

#endif // SHORTCIRCUITXT_PARTEFFECTSPANEL_H
