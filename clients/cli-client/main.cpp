#include <iostream>
#include <chrono>
#include <thread>

#include "engine/engine.h"
#include "cli_client.h"
#include "patches.h"

int main(int argc, char **argv)
{
    std::cout << "CLI Client" << std::endl;

    auto rm = fs::path{"resources/test_samples/next/README.md"};
    if (!fs::exists(rm))
    {
        std::cout << "\nCan't open " << rm.u8string() << "\n"
                  << "Did you run from the top of the scxt directory?" << std::endl;
        return 1;
    }

    // Wrap this in a {} to delete the engine and let us run the leak checker
    {
        auto ps = new scxt::cli_client::PlaybackState();

        auto engine = scxt::engine::Engine();
        scxt::cli_client::basicTestPatch(engine);
        ps->engine = &engine;

        auto foundLPK = scxt::cli_client::startMIDI(ps, "LPK");

        if (!scxt::cli_client::startAudioThread(ps, !foundLPK))
            return 1;

        if (foundLPK)
        {
            std::cout << "\nReading MIDI input ... press <enter> to quit.\n";
            char input;
            std::cin.get(input);
        }
        else
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(10s);
        }
        scxt::cli_client::stopMIDI(ps);
        scxt::cli_client::stopAudioThread(ps);

        delete ps;
    }
    scxt::showLeakLog();

    return 0;
}
