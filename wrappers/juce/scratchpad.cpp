//
// The idea here is to add test actions while developing.
// This allows us to test some sub features without having to implement whole chunks...
// to use: implement a scratch item and add it to the register function
//

#include "scratchpad.h"

////////////////////////////////////////////////////////////////////////////////////
// example item
class AuditionZone : public ScratchPadItem
{
  public:
    AuditionZone()
    {
        mDescription = "Audition the current zone";
        mName = "Audition zone";
    }
    bool prepareAction(sampler *s, std::string parameters, actiondata *ad,
                       std::string *error) override
    {
        // optionally parse parameters here with splitParameters if you expect multiple parameters
        // ...
        ad->actiontype = vga_audition_zone;
        ad->data.i[0] = true;
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////////
// moving points on wave display
class DragWaveDisplayPoint : public ScratchPadItem
{
  public:
    DragWaveDisplayPoint()
    {
        mDescription = "Drag a point on the wave display. Parameters are point id, location";
        mName = "Drag wave point";
    }
    bool prepareAction(sampler *s, std::string parameters, actiondata *ad,
                       std::string *error) override
    {
        auto parms = splitParameters(parameters);
        if (parms.size() < 2)
        {
            *error = "invalid # of parameters";
            return false;
        }
        ad->actiontype = vga_wavedisp_editpoint;
        try
        {
            ad->data.i[0] = std::stoi(parms[0]);
            ad->data.i[1] = std::stoi(parms[1]);
        }
        catch (...)
        {
            *error = "parameters must be numeric";
            return false;
        }
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////////
std::vector<ScratchPadItem *> registerScratchPadItems()
{
    std::vector<ScratchPadItem *> ret;
    // Add items here
    ret.push_back(new AuditionZone());
    ret.push_back(new DragWaveDisplayPoint());

    return ret;
}
