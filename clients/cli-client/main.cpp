#include <iostream>
#include <chrono>
#include <thread>

#include "engine/engine.h"
#include "json/stream.h"
#include "cli_client.h"
#include "patches.h"

#include <fstream>

int main(int argc, char **argv)
{
    std::cout << "CLI Client" << std::endl;

    auto samplePath = fs::current_path();

    auto rm = fs::path{"resources/test_samples/next/README.md"};

    while (samplePath.has_parent_path() && !(fs::exists(samplePath / rm)))
    {
        samplePath = samplePath.parent_path();
    }
    if (!fs::exists(samplePath / rm))
    {
        std::cout << "\nCan't open " << rm.u8string() << "\n"
                  << "Did you run from somewhere in the scxt directory tree?" << std::endl;
        return 1;
    }

    // Wrap this in a {} to delete the engine and let us run the leak checker
    {
        auto ps = new scxt::cli_client::PlaybackState();

        auto engine = scxt::engine::Engine();

        bool unstream{false};
        if (unstream)
        {
            std::cout << "UNSTREAMING" << std::endl;
            scxt::json::unstreamEngineState(engine, fs::path{"/tmp/state.json"});
        }
        else
        {
            scxt::cli_client::basicTestPatch(engine, samplePath);

            std::cout << "Streamed engine:\n" << scxt::json::streamEngineState(engine) << std::endl;
            std::ofstream ofs("/tmp/state.json");
            if (ofs.is_open())
            {
                ofs << scxt::json::streamEngineState(engine) << std::endl;
                ofs.close();
            }
        }
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
