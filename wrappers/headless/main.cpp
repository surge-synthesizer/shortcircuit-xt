/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

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
