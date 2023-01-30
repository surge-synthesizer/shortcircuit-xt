//
// Created by Paul Walker on 1/30/23.
//

#include "RtMidi.h"
#include "cli_client.h"

namespace scxt::cli_client
{

void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData)
{
    unsigned int nBytes = message->size();

    auto ud = (PlaybackState *)userData;
    if (ud && nBytes == 3)
    {
        auto wp = ud->writePoint & (PlaybackState::maxMsg - 1);
        for (unsigned int i = 0; i < nBytes; i++)
            ud->msgs[wp][i] = (uint8_t)message->at(i);
        ud->writePoint++;
    }
}

bool startMIDI(PlaybackState *ps, const std::string &device)
{
    try
    {
        std::cout << "Initializing Midi" << std::endl;
        ps->midiIn = new RtMidiIn();
    }
    catch (RtMidiError &error)
    {
        error.printMessage();
        exit(EXIT_FAILURE);
    }
    // Check inputs.
    auto midiin = ps->midiIn;

    unsigned int nPorts = midiin->getPortCount();
    std::cout << "    MIDI: There are " << nPorts << " MIDI input sources available.\n";
    bool foundLPK{false};
    for (unsigned int i = 0; i < nPorts; i++)
    {
        try
        {
            auto portName = midiin->getPortName(i);
            if (portName.find(device.c_str()) != std::string::npos)
            {
                std::cout << "    MIDI: Opening " << portName << std::endl;
                midiin->openPort(i);
                foundLPK = true;
            }
            else
            {
                std::cout << "    MIDI: Leaving " << portName << std::endl;
            }
        }
        catch (RtMidiError &error)
        {
            error.printMessage();
        }
    }
    midiin->setCallback(midiCallback, ps);
    return foundLPK;
}

bool stopMIDI(PlaybackState *ps)
{
    delete ps->midiIn;
    ps->midiIn = nullptr;
    return true;
}
} // namespace scxt::cli_client