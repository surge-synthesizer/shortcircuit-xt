//
// The idea here is to add test actions while developing.
// This allows us to test some sub features without having to implement whole chunks...
// to use: implement a scratch item and add it to the register function
//

#include "scratchpad.h"
#include "juce_gui_basics/juce_gui_basics.h"

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
class RequestRefresh : public ScratchPadItem
{
  public:
    RequestRefresh()
    {
        mDescription = "Request the sampler to send out refresh";
        mName = "Refresh";
    }
    bool prepareAction(sampler *s, std::string parameters, actiondata *ad,
                       std::string *error) override
    {
        // optionally parse parameters here with splitParameters if you expect multiple parameters
        // ...
        ad->actiontype = vga_request_refresh;
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
        mDescription =
            "Drag a point on the wave display. Parameters are point id, location. "
            "Point id's are 0=start, 1=end, 2=loop start, 3=loop end, 4=xfade, 5..n=hitpoints."
            "Location is in samples";
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

        try
        {
            ActionWaveDisplayEditPoint a((ActionWaveDisplayEditPoint::PointType)std::stoi(parms[0]),
                                         std::stoi(parms[1]));
            *ad = a;
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
// Load a known sample
class LoadSampleFile : public ScratchPadItem
{
    juce::ApplicationProperties ap;

  public:
    LoadSampleFile()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "ShortcircuitXT_scratch";
        ap.setStorageParameters(options);
        auto user = ap.getUserSettings();
        auto f = user->getValue("scratchLoadFile");

        mDescription = "Load a sample file into the sampler";
        mName = "Load Sample";
        mDefaultParameter = f.toStdString();
    }
    bool prepareAction(sampler *s, std::string parameters, actiondata *ad,
                       std::string *error) override
    {
        auto user = ap.getUserSettings();
        juce::String v = parameters;
        user->setValue("scratchLoadFile", v);
        user->saveIfNeeded();

        auto d = new DropList();
        auto fd = DropList::File();
        fd.p = parameters;
        d->files.push_back(fd);
        ad->actiontype = vga_load_dropfiles;
        ad->data.dropList = d;
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
    ret.push_back(new RequestRefresh());
    ret.push_back(new LoadSampleFile());
    return ret;
}

void unregisterScratchPadItems(std::vector<ScratchPadItem *> items)
{
    for (auto i : items)
    {
        delete i;
    }
}
