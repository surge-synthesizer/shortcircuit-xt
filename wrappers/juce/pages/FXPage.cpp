/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "FXPage.h"
#include "PageContentBase.h"
#include "widgets/ComboBox.h"
#include "widgets/ParamEditor.h"
#include "widgets/OutlinedTextButton.h"
#include "sst/cpputils.h"

namespace scxt
{
namespace pages
{
namespace fx_contents
{
struct SingleFX : scxt::pages::contents::PageContentBase<scxt::pages::FXPage>
{
    SingleFX(FXPage &p, int id)
        : idx(id), contents::PageContentBase<FXPage>(p, "effect_" + std::to_string(id + 1),
                                                     "Effect " + std::to_string(id + 1),
                                                     juce::Colour(0xFF447744))
    {
        auto &mult = parentPage.editor->multi;
        auto &fx = parentPage.editor->multi.filters[idx];
        outputTarget = bind<widgets::IntParamComboBox>(mult.filter_output[idx],
                                                       parentPage.editor->multiFilterOutputNames);
        bind(filter, fx, parentPage.editor->multiFilterTypeNames);
    }

    ~SingleFX() = default;

    void resized() override
    {
        auto b = getContentsBounds();
        auto bl = b.withTrimmedRight(50).reduced(1, 1);
        auto br = b.withTrimmedLeft(b.getWidth() - 48).reduced(1, 1);

        auto rgl = contents::RowGenerator(bl, 4 + n_filter_parameters);
        auto rgr = contents::RowGenerator(br, 4 + n_filter_parameters);

        filter.type->setBounds(rgl.next());
        rgl.next();
        for (const auto &q : filter.fp)
            q->setBounds(rgl.next());
        rgl.next();
        outputTarget->setBounds(rgl.next());

        filter.bypass->setBounds(rgr.next());
        rgr.next();
        for (const auto &p : filter.ip)
            p->setBounds(rgr.next(5));
    }

    int idx{-1};

    FilterRegion filter;
    std::unique_ptr<widgets::IntParamComboBox> outputTarget;
};
} // namespace fx_contents
FXPage::FXPage(SCXTEditor *e, SCXTEditor::Pages p) : PageBase(e, p, scxt::style::Selector{"fx"})
{
    for (auto i = 0; i < num_fxunits; ++i)
    {
        auto q = makeContent<fx_contents::SingleFX>(*this, i);
        fxComponents[i] = std::move(q);
    }
}
FXPage::~FXPage() = default;

void FXPage::resized()
{
    jassert(num_fxunits == 8);
    auto w = getWidth() / 4;
    auto h = getHeight() / 2;

    auto r = getLocalBounds().withWidth(w).withHeight(h);
    for (int i = 0; i < num_fxunits; ++i)
    {
        fxComponents[i]->setBounds(r.reduced(1, 1));
        r = r.translated(w, 0);
        if (i == 3)
            r = r.translated(-4 * w, h);
    }
}

} // namespace pages
} // namespace scxt