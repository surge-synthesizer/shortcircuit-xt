//
// Created by Paul Walker on 1/28/25.
//

#ifndef BROWSERPANEINTERFACES_H
#define BROWSERPANEINTERFACES_H

#include <optional>
#include "filesystem/import.h"
#include "sample/compound_file.h"

namespace scxt::ui::app::browser_ui
{
struct WithSampleInfo
{
    virtual ~WithSampleInfo() {}
    virtual std::optional<fs::directory_entry> getDirEnt() const = 0;
    virtual std::optional<sample::compound::CompoundElement> getCompoundElement() const = 0;
};

template <typename T> static bool hasSampleInfo(const T &t)
{
    return dynamic_cast<const WithSampleInfo *>(t.get()) != nullptr;
}

template <typename T> static WithSampleInfo *asSampleInfo(const T &t)
{
    return dynamic_cast<WithSampleInfo *>(t.get());
}

} // namespace scxt::ui::app::browser_ui
#endif // BROWSERPANEINTERFACES_H
