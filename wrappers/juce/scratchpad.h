//
// The idea here is to add test actions while developing
//

#ifndef SHORTCIRCUIT_SCRATCHPAD_H
#define SHORTCIRCUIT_SCRATCHPAD_H
#include "sampler.h"
#include <vector>
#include <string>

class ScratchPadItem
{
  protected:
    std::vector<std::string> splitParameters(const std::string &p)
    {
        std::string token = ",";
        std::string str = p;
        std::vector<std::string> result;
        while (str.size())
        {
            int index = str.find(token);
            if (index != std::string::npos)
            {
                result.push_back(str.substr(0, index));
                str = str.substr(index + token.size());
                if (str.size() == 0)
                    result.push_back(str);
            }
            else
            {
                result.push_back(str);
                str = "";
            }
        }
        return result;
    }

  public:
    std::string mName;
    std::string mDescription;
    // given parameters, prepare the actiondata which will be posted.
    // if there is a problem return false, and set error
    virtual bool prepareAction(sampler *s, std::string parameters, actiondata *ad,
                               std::string *error) = 0;
};

std::vector<ScratchPadItem *> registerScratchPadItems();

#endif // SHORTCIRCUIT_SCRATCHPAD_H
