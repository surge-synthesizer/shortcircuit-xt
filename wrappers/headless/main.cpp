#include <iomanip>
#include <iostream>
#include <memory>

#include "sampler.h"
#include "version.h"
#include "infrastructure/logfile.h"

class HeadlessLogger : public scxt::log::LoggingCallback
{
    scxt::log::Level getLevel() override { return scxt::log::Level::Debug; }
    void message(scxt::log::Level lev, const std::string &msg) override
    {
        std::cout << scxt::log::getShortLevelStr(lev) << msg << std::endl;
    }
};

void *hInstance = 0;
int main(int argc, char **argv)
{
    HeadlessLogger logger;
    std::cout << "# Shortcircuit XT Headless. " << scxt::build::FullVersionStr << std::endl;

    auto sc3 = std::make_unique<sampler>(nullptr, 2, nullptr, &logger);
    sc3->set_samplerate(48000);
    if (!sc3)
    {
        std::cout << "Couldn't make an scxt" << std::endl;
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
