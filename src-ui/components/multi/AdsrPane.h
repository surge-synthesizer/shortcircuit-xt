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

#ifndef SHORTCIRCUIT_ADSRPANE_H
#define SHORTCIRCUIT_ADSRPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/data/Continuous.h"
#include "datamodel/adsr_storage.h"
#include "components/HasEditor.h"

namespace scxt::ui::multi
{
struct AdsrPane : sst::jucegui::components::NamedPanel, HasEditor
{
    // TODO Factor this better obviously
    struct attachment : sst::jucegui::data::ContinunousModulatable
    {
        float &value;
        std::string label;
        AdsrPane *parent{nullptr};
        attachment(AdsrPane *p, const std::string &l, float &v) : parent(p), value(v), label(l) {}

        std::string getLabel() const override { return label; }
        float getValue() const override { return value; }
        void setValueFromGUI(const float &f) override
        {
            value = f;
            parent->adsrChangedFromGui();
        }
        void setValueFromModel(const float &f) override { value = f; }

        float getModulationValuePM1() const override { return 0; }
        void setModulationValuePM1(const float &f) override {}
        bool isModulationBipolar() const override { return false; }
    };

    // ToDo: shapes of course
    std::unique_ptr<attachment> atA, atD, atS, atR;
    std::unique_ptr<sst::jucegui::components::VSlider> slA, slD, slS, slR;

    datamodel::AdsrStorage adsrView;
    int index{0};
    AdsrPane(SCXTEditor *, int index);

    void resized() override;

    void adsrChangedFromModel(const datamodel::AdsrStorage &);
    void adsrChangedFromGui();
};
} // namespace scxt::ui::multi

#endif // SHORTCIRCUIT_ADSRPANE_H
