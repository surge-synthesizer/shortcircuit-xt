//
// Created by Paul Walker on 1/7/22.
//

#include "FXPage.h"
#include "components/SingleFX.h"

namespace SC3
{
namespace Pages
{
FXPage::FXPage(SC3Editor *e, SC3Editor::Pages p) : PageBase(e, p)
{
    for (auto i = 0; i < num_fxunits; ++i)
    {
        auto q = std::make_unique<Components::SingleFX>(editor, i);

        addAndMakeVisible(*q);
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

void FXPage::onProxyUpdate()
{
    for (const auto &q : fxComponents)
        q->onProxyUpdate();
}

} // namespace Pages
} // namespace SC3