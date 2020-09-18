#include <iostream>
#include <iomanip>
#include <memory>

#include "sampler.h"

void *hInstance = 0;
int main( int argc, char **argv )
{
    std::cout << "ShortCircuit3 Headless." << std::endl;

    auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
    if (!sc3)
        std::cout << "Couldn't make an SC3" << std::endl;
    else
        std::cout << "Made an SC3" << std::endl;
}
