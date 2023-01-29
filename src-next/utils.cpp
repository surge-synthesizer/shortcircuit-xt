//
// Created by Paul Walker on 1/30/23.
//

#include <iostream>
#include "utils.h"

namespace scxt
{
#if USE_SIMPLE_LEAK_DETECTOR
std::map<std::string, std::pair<int, int>> allocLog;
void showLeakLog()
{
    std::cout << "\nMemory Allocation Log of NoCopy classes\n";
    for (const auto &[k, v] : allocLog)
    {
        auto [a, d] = v;
        std::cout << "   class=" << k << "  ctor=" << a << " dtor=" << d
                  << ((a != d) ? " ERROR!!" : " OK") << std::endl;
    }
}
#endif

} // namespace scxt