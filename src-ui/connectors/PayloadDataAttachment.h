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
#include <functional>

namespace scxt::ui::connectors
{
// TODO Factor this better obviously
template <typename Parent, typename Payload>
struct PayloadDataAttachment : sst::jucegui::data::ContinunousModulatable
{
    float &value;
    std::string label;
    std::function<void()> onGuiValueChanged;
    std::function<float(const Payload &)> extractFromPayload;

    PayloadDataAttachment(Parent *p, const std::string &l, std::function<void()> oGVC,
                          std::function<float(const Payload &)> efp, float &v)
        : value(v), label(l), extractFromPayload(efp), onGuiValueChanged(std::move(oGVC))
    {
    }

    std::string getLabel() const override { return label; }
    float getValue() const override { return value; }
    void setValueFromGUI(const float &f) override
    {
        value = f;
        onGuiValueChanged();
    }
    void setValueFromModel(const float &f) override { value = f; }

    void setValueFromPayload(const Payload &p) { setValueFromModel(extractFromPayload(p)); }

    float getDefaultValue() const override { return 0; }

    float getModulationValuePM1() const override { return 0; }
    void setModulationValuePM1(const float &f) override {}
    bool isModulationBipolar() const override { return false; }
};

} // namespace scxt::ui::connectors
#endif // SHORTCIRCUIT_PAYLOADDATAATTACHMENT_H
