//
// Created by Paul Walker on 1/15/22.
//

#ifndef SHORTCIRCUIT_BASICTYPES_H
#define SHORTCIRCUIT_BASICTYPES_H

#include <vector>
#include <string>
#include "sampler_wrapper_actiondata.h"

namespace scxt
{
namespace data
{
struct ActionSender
{
    virtual ~ActionSender() = default;
    virtual void sendActionToEngine(const actiondata &ad) = 0;
};

struct NameList
{
    std::vector<std::string> data;
    uint64_t update_count{0};
};

} // namespace data
} // namespace scxt
#endif // SHORTCIRCUIT_BASICTYPES_H
