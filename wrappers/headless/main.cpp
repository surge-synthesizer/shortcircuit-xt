#include <iomanip>
#include <iostream>
#include <memory>

#include "sampler.h"
#include "version.h"

void *hInstance = 0;
int main(int argc, char **argv)
{
    std::cout << "# ShortCircuit3 Headless. " << SC3::Build::FullVersionStr << std::endl;

    auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr);
    sc3->set_samplerate(48000);
    if (!sc3)
    {
        std::cout << "Couldn't make an SC3" << std::endl;
        return 1;
    }

    if (true)
    {
        std::cout << "# Loading harpsi.sf2" << std::endl;
        auto res = sc3->load_file(string_to_path("resources/test_samples/harpsi.sf2"));
    }
    else
    {
        std::cout << "# Loading Bad Pluck" << std::endl;
#if WINDOWS
        auto res = sc3->load_file("resources\\test_samples\\BadPluckSample.wav");
#else
        auto res = sc3->load_file(string_to_path("resources/test_samples/BadPluckSample.wav"));
#endif
    }

    std::cout << sc3->generateInternalStateView() << std::endl;
}
