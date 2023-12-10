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

#include "configuration.h"
#include "ChannelStrip.h"
#include <sst/jucegui/components/VUMeter.h>

namespace scxt::ui::mixer
{
namespace jcmp = sst::jucegui::components;
namespace cmsg = scxt::messaging::client;

ChannelStrip::ChannelStrip(scxt::ui::SCXTEditor *e, MixerScreen *m, int bi, BusType t)
    : HasEditor(e), jcmp::NamedPanel(""), mixer(m), busIndex(bi), type(t)
{
    centeredHeader = true;
    std::string nm = "PART " + std::to_string(bi);
    if (t == BusType::MAIN)
        nm = "MAIN";
    if (t == BusType::AUX)
        nm = "AUX " + std::to_string(bi - numParts);
    setName(nm);
    selectable = true;

    auto onChange = [w = juce::Component::SafePointer(this)](const auto &a) {
        if (w)
        {
            w->mixer->sendBusSendStorage(w->busIndex);
            w->updateValueTooltip(a);
        }
    };

    {
        int idx{0};
        fxLabel = std::make_unique<jcmp::Label>();
        fxLabel->setText("FX");
        addAndMakeVisible(*fxLabel);

        idx = 0;
        for (auto &fxmb : fxMenu)
        {
            fxmb = std::make_unique<jcmp::MenuButton>();
            fxmb->setLabel("-");
            fxmb->setOnCallback([idx, w = juce::Component::SafePointer(this)]() {
                w->mixer->showFXSelectionMenu(w->busIndex, idx);
            });
            addAndMakeVisible(*fxmb);
            idx++;
        }
        idx = 0;
        for (auto &fxt : fxToggle)
        {
            auto tat = std::make_unique<boolattachment_t>(
                "Bypass",
                [w = juce::Component::SafePointer(this), idx](const auto &a) {
                    if (w)
                    {
                        w->sendToSerialization(cmsg::SetBusEffectStorage(
                            {w->busIndex, idx, w->mixer->busEffectsData[w->busIndex][idx].second}));
                    }
                },
                mixer->busEffectsData[busIndex][idx].second.isActive);
            fxt = std::make_unique<jcmp::ToggleButton>();
            fxt->setDrawMode(jcmp::ToggleButton::DrawMode::FILLED);
            fxt->setSource(tat.get());
            addAndMakeVisible(*fxt);

            fxToggleAtt[idx] = std::move(tat);

            idx++;
        }
    }

    if (t == BusType::PART)
    {
        int idx{0};
        auxLabel = std::make_unique<jcmp::Label>();
        auxLabel->setText("AUX");
        addAndMakeVisible(*auxLabel);

        idx = 0;
        for (auto &axs : auxSlider)
        {
            auto ava = std::make_unique<attachment_t>(
                datamodel::pmd().asPercent().withName("Aux " + std::to_string(idx + 1) + " Send"),
                onChange, mixer->busSendData[busIndex].sendLevels[idx]);
            axs = std::make_unique<jcmp::HSliderFilled>();
            axs->setSource(ava.get());
            addAndMakeVisible(*axs);
            auxAttachments[idx] = std::move(ava);
            setupWidgetForValueTooltip(axs, auxAttachments[idx]);
            idx++;
        }
        idx = 0;
        for (auto &axt : auxToggle)
        {
            axt = std::make_unique<jcmp::ToggleButton>();
            axt->setDrawMode(jcmp::ToggleButton::DrawMode::LABELED);
            axt->setLabel(std::to_string(idx + 1));
            addAndMakeVisible(*axt);
            idx++;
        }

        idx = 0;
        for (auto &axt : auxPrePost)
        {
            axt = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::GlyphType::ARROW_L_TO_R);
            axt->setOnCallback([w = juce::Component::SafePointer(this), idx]() {
                if (w)
                    w->showAuxRouting(idx);
            });
            addAndMakeVisible(*axt);
            idx++;
        }
    }

    panAttachment =
        std::make_unique<attachment_t>(datamodel::pmd().asPercentBipolar().withName("Pan"),
                                       onChange, mixer->busSendData[busIndex].pan);
    panKnob = std::make_unique<jcmp::Knob>();
    panKnob->setSource(panAttachment.get());
    addAndMakeVisible(*panKnob);
    setupWidgetForValueTooltip(panKnob, panAttachment);

    vcaAttachment = std::make_unique<attachment_t>(
        datamodel::pmd().asCubicDecibelAttenuation().withName("Level").withDefault(1.0), onChange,
        mixer->busSendData[busIndex].level);
    vcaSlider = std::make_unique<jcmp::VSlider>();
    vcaSlider->setSource(vcaAttachment.get());
    setupWidgetForValueTooltip(vcaSlider, vcaAttachment);
    addAndMakeVisible(*vcaSlider);

    muteButton = std::make_unique<jcmp::ToggleButton>();
    muteButton->setLabel("M");
    soloButton = std::make_unique<jcmp::ToggleButton>();
    soloButton->setLabel("S");

    addAndMakeVisible(*muteButton);
    addAndMakeVisible(*soloButton);

    outputMenu = std::make_unique<jcmp::MenuButton>();
    outputMenu->setLabel("OUTPUT");
    addAndMakeVisible(*outputMenu);

    vuMeter = std::make_unique<jcmp::VUMeter>();
    addAndMakeVisible(*vuMeter);

    effectsChanged();
}

ChannelStrip::~ChannelStrip() {}

void ChannelStrip::mouseDown(const juce::MouseEvent &) { mixer->selectBus(busIndex); }

void ChannelStrip::resized()
{
    auto fxheight = 16;
    auto ca = getContentArea();

    auto fx = ca.withHeight(fxheight);
    fxLabel->setBounds(fx);
    fx = fx.translated(0, fxheight);
    for (int i = 0; i < fxMenu.size(); ++i)
    {
        auto tr = fx.withTrimmedBottom(1).withWidth(10);
        auto mr = fx.withTrimmedBottom(1).withTrimmedLeft(11);
        fxToggle[i]->setBounds(tr);
        fxMenu[i]->setBounds(mr);

        fx = fx.translated(0, fxheight);
    }

    if (type != BusType::PART)
    {
        fx = fx.translated(0, fxheight * (auxSlider.size() + 1));
    }
    else
    {
        auxLabel->setBounds(fx);
        fx = fx.translated(0, fxheight);
        for (int i = 0; i < auxSlider.size(); ++i)
        {
            auto tr = fx.withTrimmedBottom(1).withWidth(10);
            auto mr = fx.withTrimmedBottom(1).withTrimmedLeft(11).withTrimmedRight(11);
            auto gr = fx.withTrimmedBottom(1).withLeft(mr.getRight() + 1);
            auxToggle[i]->setBounds(tr);
            auxSlider[i]->setBounds(mr);
            auxPrePost[i]->setBounds(gr);

            fx = fx.translated(0, fxheight);
        }
    }
    fx = fx.translated(0, fxheight / 2);
    {
        auto r = fx.withHeight(fxheight * 2);
        auto s = r.getWidth() / 4;
        auto k = r.withTrimmedLeft(s / 2).withWidth(s * 2);
        panKnob->setBounds(k);

        auto m = r.withTrimmedLeft(s / 2 + s * 2).withWidth(s).withHeight(fxheight).reduced(1);
        muteButton->setBounds(m);
        m = m.translated(0, fxheight);
        soloButton->setBounds(m);
    }
    fx = fx.translated(0, fxheight * 2.5);

    auto restHeight = ca.getBottom() - fx.getBottom();
    auto sliderVUHeight = restHeight - fxheight;

    auto s = fx.withHeight(sliderVUHeight).withTrimmedRight(fx.getWidth() / 2).reduced(1);
    vcaSlider->setBounds(s);
    auto vs = fx.withHeight(sliderVUHeight)
                  .withTrimmedLeft(fx.getWidth() * 2.5 / 4)
                  .withWidth(fx.getWidth() / 4)
                  .reduced(1);
    vuMeter->setBounds(vs);

    fx = fx.translated(0, sliderVUHeight + fxheight / 2);
    outputMenu->setBounds(fx);
}

void ChannelStrip::effectsChanged()
{
    auto &bed = mixer->busEffectsData[busIndex];
    assert(bed.size() == fxMenu.size());
    for (int i = 0; i < bed.size(); ++i)
    {
        if (bed[i].second.type == engine::none)
            fxMenu[i]->setLabel("-");
        else
            fxMenu[i]->setLabel(mixer->effectDisplayName(bed[i].second.type, false));
    }
    repaint();
}

void ChannelStrip::showAuxRouting(int idx)
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Aux " + std::to_string(idx + 1) + " routing");
    p.addSeparator();

    // if you change the routing change this menu too
    auto cr = mixer->busSendData[busIndex].auxLocation[idx];

    static_assert(engine::Bus::BusSendStorage::AuxLocation::POST_VCA == 2);
    std::array<std::string, 3> labels = {"Pre-FX", "Post-FX, Pre VCA", "Post VCA"};
    for (int i = 0; i < 3; ++i)
    {
        p.addItem(labels[i], true, i == cr, [idx, w = juce::Component::SafePointer(this), i]() {
            if (!w)
                return;

            w->mixer->busSendData[w->busIndex].auxLocation[idx] =
                (engine::Bus::BusSendStorage::AuxLocation)i;

            w->mixer->sendBusSendStorage(w->busIndex);
        });
    }

    p.showMenuAsync(editor->defaultPopupMenuOptions());
}

} // namespace scxt::ui::mixer