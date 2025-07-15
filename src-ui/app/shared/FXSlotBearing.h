//
// Created by Paul Walker on 7/15/25.
//

#ifndef FXSLOTBEARING_H
#define FXSLOTBEARING_H

namespace scxt::ui::app::shared
{
struct FXSlotBearing
{
    FXSlotBearing() {}
    FXSlotBearing(int f, int b, bool fb) : fxSlot(f), busAddressOrPart(b), busEffect(fb) {}
    int fxSlot{-1};
    int busAddressOrPart{-1};
    bool busEffect{true};
};
} // namespace scxt::ui::app::shared
#endif // FXSLOTBEARING_H
