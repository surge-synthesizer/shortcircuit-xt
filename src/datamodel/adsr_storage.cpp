//
// Created by Paul Walker on 4/30/23.
//

#include "adsr_storage.h"

namespace scxt::datamodel
{
// The names are not stored on these right now. Fix?
datamodel::pmd AdsrStorage::paramAHDR{datamodel::envelopeThirtyTwo()},
    AdsrStorage::paramS{datamodel::pmd().asPercent()},
    AdsrStorage::paramShape{datamodel::pmd().asPercentBipolar()};

} // namespace scxt::datamodel