//
// Created by Paul Walker on 2/11/23.
//

#include "SCXTEditor.h"
#include "MultiScreen.h"
#include "multi/AdsrPane.h"

namespace scxt::ui
{
void SCXTEditor::onEnvelopeUpdated(const int &which, const datamodel::AdsrStorage &v)
{
    // TODO - do I want a multiScreen->onEnvelopeUpdated or just
    multiScreen->eg[which]->adsrChangedFromModel(v);
}
} // namespace scxt::ui