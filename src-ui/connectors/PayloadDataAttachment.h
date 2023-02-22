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

#ifndef SHORTCIRCUIT_PAYLOADDATAATTACHMENT_H
#define SHORTCIRCUIT_PAYLOADDATAATTACHMENT_H

#include "sst/jucegui/data/Continuous.h"
#include "datamodel/parameter.h"
#include <functional>

namespace scxt::ui::connectors
{
// TODO Factor this better obviously
template <typename Parent, typename Payload, typename ValueType = float>
struct PayloadDataAttachment : sst::jucegui::data::ContinunousModulatable
{
    ValueType &value;
    std::string label;
    std::function<void(const PayloadDataAttachment &at)> onGuiValueChanged;
    std::function<float(const Payload &)> extractFromPayload;

    PayloadDataAttachment(Parent *p, const datamodel::ControlDescription &cd, const std::string &l,
                          std::function<void(const PayloadDataAttachment &at)> oGVC,
                          std::function<float(const Payload &)> efp, ValueType &v)
        : description(cd), value(v), label(l), extractFromPayload(efp),
          onGuiValueChanged(std::move(oGVC))
    {
    }

    // TODO maybe. For now we have these descriptions as constexpr
    // in the code and apply them directly in many cases. We could
    // stream each and every one but don't. Maybe fix that one day.
    // Would def need to fix that to make web ui consistent if we write it
    datamodel::ControlDescription description;

    std::string getLabel() const override { return label; }
    float getValue() const override { return (float)value; }
    void setValueFromGUI(const float &f) override
    {
        value = (ValueType)f;
        onGuiValueChanged(*this);
    }
    void setValueFromModel(const float &f) override { value = (ValueType)f; }
    void setValueFromPayload(const Payload &p) { setValueFromModel(extractFromPayload(p)); }

    float getMin() const override { return description.min; }
    float getMax() const override { return description.max; }
    float getDefaultValue() const override { return description.def; }

    bool isBipolar() const override { return description.min * description.max < 0; }

    float getModulationValuePM1() const override { return 0; }
    void setModulationValuePM1(const float &f) override {}
    bool isModulationBipolar() const override { return false; }
};

} // namespace scxt::ui::connectors
#endif // SHORTCIRCUIT_PAYLOADDATAATTACHMENT_H
