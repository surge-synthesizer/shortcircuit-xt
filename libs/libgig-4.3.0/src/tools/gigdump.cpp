/***************************************************************************
 *                                                                         *
 *   libgig - C++ cross-platform Gigasampler format file access library    *
 *                                                                         *
 *   Copyright (C) 2003-2019 by Christian Schoenebeck                      *
 *                              <cuse@users.sourceforge.net>               *
 *                                                                         *
 *   This program is part of libgig.                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include <string>

#include "../gig.h"

using namespace std;

string Revision();
void PrintVersion();
void PrintFileInformation(gig::File* gig);
void PrintGroups(gig::File* gig);
void PrintSamples(gig::File* gig);
void PrintScripts(gig::File* gig);
void PrintInstruments(gig::File* gig);
void PrintInstrumentNamesOnly(gig::File* gig);
void PrintRegions(gig::Instrument* instr);
void PrintUsage();
void PrintDimensionRegions(gig::Region* rgn);
bool VerifyFile(gig::File* gig);
void RebuildChecksumTable(gig::File* gig);

class PubSample : public gig::Sample {
public:
    using DLS::Sample::pCkData;
};

class PubFile : public gig::File {
public:
    using gig::File::VerifySampleChecksumTable;
    using gig::File::RebuildSampleChecksumTable;
};


int main(int argc, char *argv[])
{
    bool bVerify = false;
    bool bRebuildChecksums = false;
    bool bInstrumentNamesOnly = false;

    if (argc <= 1) {
        PrintUsage();
        return EXIT_FAILURE;
    }

    int iArg;
    for (iArg = 1; iArg < argc; ++iArg) {
        const string opt = argv[iArg];
        if (opt == "--") { // common for all command line tools: separator between initial option arguments and i.e. subsequent file arguments
            iArg++;
            break;
        }
        if (opt.substr(0, 1) != "-") break;

        if (opt == "-v") {
            PrintVersion();
            return EXIT_SUCCESS;
        } else if (opt == "--verify") {
            bVerify = true;
        } else if (opt == "--rebuild-checksums") {
            bRebuildChecksums = true;
        } else if (opt == "--instrument-names") {
            bInstrumentNamesOnly = true;
        } else {
            cerr << "Unknown option '" << opt << "'" << endl;
            cerr << endl;
            PrintUsage();
            return EXIT_FAILURE;
        }
    }
    if (iArg >= argc) {
        cout << "No file name provided!" << endl;
        return EXIT_FAILURE;
    }
    const char* filename = argv[iArg];

    FILE* hFile = fopen(filename, "r");
    if (!hFile) {
        cout << "Invalid file argument!" << endl;
        return EXIT_FAILURE;
    }
    fclose(hFile);
    try {
        RIFF::File* riff = new RIFF::File(filename);
        gig::File*  gig  = new gig::File(riff);

        if (bRebuildChecksums) {
            RebuildChecksumTable(gig);
        } else if (bVerify) {
            bool OK = VerifyFile(gig);
            if (OK) cout << "All checks passed successfully! :-)\n";
            return (OK) ? EXIT_SUCCESS : EXIT_FAILURE;
        } else {
            if (bInstrumentNamesOnly) {
                PrintInstrumentNamesOnly(gig);
            } else {
                PrintFileInformation(gig);
                cout << endl;
                PrintGroups(gig);
                cout << endl;
                PrintSamples(gig);
                cout << endl;
                PrintScripts(gig);
                cout << endl;
                PrintInstruments(gig);
            }
        }
        delete gig;
        delete riff;
    }
    catch (RIFF::Exception e) {
        e.PrintMessage();
        return EXIT_FAILURE;
    }
    catch (...) {
        cout << "Unknown exception while trying to parse file." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void PrintFileInformation(gig::File* gig) {
    cout << "Global File Information:" << endl;
    cout << "    Total instruments: " << gig->Instruments << endl;
    if (gig->pVersion) {
       cout << "    Version: " << gig->pVersion->major   << "."
                           << gig->pVersion->minor   << "."
                           << gig->pVersion->release << "."
                           << gig->pVersion->build   << endl;
    }
    if (gig->pInfo) {
        if (gig->pInfo->Name.size())
            cout << "    Name: '" << gig->pInfo->Name << "'\n";
        if (gig->pInfo->ArchivalLocation.size())
            cout << "    ArchivalLocation: '" << gig->pInfo->ArchivalLocation << "'\n";
        if (gig->pInfo->CreationDate.size())
            cout << "    CreationDate: '" << gig->pInfo->CreationDate << "'\n";
        if (gig->pInfo->Comments.size())
            cout << "    Comments: '" << gig->pInfo->Comments << "'\n";
        if (gig->pInfo->Product.size())
            cout << "    Product: '" << gig->pInfo->Product << "'\n";
        if (gig->pInfo->Copyright.size())
            cout << "    Copyright: '" << gig->pInfo->Copyright << "'\n";
        if (gig->pInfo->Artists.size())
            cout << "    Artists: '" << gig->pInfo->Artists << "'\n";
        if (gig->pInfo->Genre.size())
            cout << "    Genre: '" << gig->pInfo->Genre << "'\n";
        if (gig->pInfo->Keywords.size())
            cout << "    Keywords: '" << gig->pInfo->Keywords << "'\n";
        if (gig->pInfo->Engineer.size())
            cout << "    Engineer: '" << gig->pInfo->Engineer << "'\n";
        if (gig->pInfo->Technician.size())
            cout << "    Technician: '" << gig->pInfo->Technician << "'\n";
        if (gig->pInfo->Software.size())
            cout << "    Software: '" << gig->pInfo->Software << "'\n";
        if (gig->pInfo->Medium.size())
            cout << "    Medium: '" << gig->pInfo->Medium << "'\n";
        if (gig->pInfo->Source.size())
            cout << "    Source: '" << gig->pInfo->Source << "'\n";
        if (gig->pInfo->SourceForm.size())
            cout << "    SourceForm: '" << gig->pInfo->SourceForm << "'\n";
        if (gig->pInfo->Commissioned.size())
            cout << "    Commissioned: '" << gig->pInfo->Commissioned << "'\n";
    }
}

void PrintGroups(gig::File* gig) {
    int groups = 0;
    cout << "ALL defined Groups:" << endl;
    for (gig::Group* pGroup = gig->GetFirstGroup(); pGroup; pGroup = gig->GetNextGroup()) {
        groups++;
        string name = pGroup->Name;
        if (name == "") name = "<NO NAME>";
        else            name = '\"' + name + '\"';
        cout << "    Group " << groups << ")" << endl;
        cout << "        Name: " << name << endl;
    }
}

void PrintSamples(gig::File* gig) {
    int samples = 0;
    cout << "ALL Available Samples (as there might be more than referenced by Instruments):" << endl;
    PubSample* pSample = (PubSample*) gig->GetFirstSample();
    while (pSample) {
        samples++;
        // determine sample's name
        string name = pSample->pInfo->Name;
        if (name == "") name = "<NO NAME>";
        else            name = '\"' + name + '\"';
        // determine group this sample belongs to
        int iGroup = 1;
        for (gig::Group* pGroup = gig->GetFirstGroup(); pGroup; pGroup = gig->GetNextGroup(), iGroup++) {
            if (pGroup == pSample->GetGroup()) break;
        }
        // print sample info
        cout << "    Sample " << samples << ") " << name << ", ";
        cout << "Group " << iGroup << ", ";
        cout << pSample->SamplesPerSecond << "Hz, " << pSample->Channels << " Channels, " << pSample->Loops << " Loops";
        if (pSample->Loops) {
            cout << " (Type: ";
            switch (pSample->LoopType) {
                case gig::loop_type_normal:         cout << "normal)";   break;
                case gig::loop_type_bidirectional:  cout << "pingpong)"; break;
                case gig::loop_type_backward:       cout << "reverse)";  break;
            }
            cout << ", LoopFraction=" << pSample->LoopFraction << ", Start=" << pSample->LoopStart << ", End=" << pSample->LoopEnd;
            cout << ", LoopPlayCount=" << pSample->LoopPlayCount;
        }
        cout << flush;
        printf(", crc=%x", pSample->GetWaveDataCRC32Checksum());
        fflush(stdout);
        cout << ", Length=" << pSample->SamplesTotal << " Compressed=" << ((pSample->Compressed) ? "true" : "false")
             << " foffset=" << pSample->pCkData->GetFilePos()
             << " fsz=" << pSample->pCkData->GetSize()
             << endl;
#if 0
        {
            const uint bufSize = 64;
            unsigned char buf[bufSize] = {};
            pSample->SetPos(0);
            RIFF::file_offset_t n = pSample->pCkData->Read(&buf[0], bufSize, 1);
            //RIFF::file_offset_t n = pSample->Read(&buf[0], bufSize / pSample->FrameSize);
            cout << "        FrameSize=" << pSample->FrameSize << ",Data[" << n << "]" << flush;
            for (int x = 0; x < bufSize; ++x)
                printf("%02x ", buf[x]);
            printf("\n");
            fflush(stdout);
        }
#endif
        pSample = (PubSample*) gig->GetNextSample();
    }
}

void PrintScripts(gig::File* gig) {
    cout << "ALL Available Real-Time Instrument Scripts (as there might be more than referenced by Instruments):" << endl;
    for (int g = 0; gig->GetScriptGroup(g); ++g) {
        gig::ScriptGroup* pGroup = gig->GetScriptGroup(g);
        cout << "    Group " << g+1 << ") '" << pGroup->Name << "'\n";
        for (int s = 0; pGroup->GetScript(s); ++s) {
            gig::Script* pScript = pGroup->GetScript(s);
            cout << "        Script " << s+1 << ") '" << pScript->Name << "':\n";
            cout << "[START OF SCRIPT]\n";
            cout << pScript->GetScriptAsText();
            cout << "[END OF SCRIPT]\n";
        }
    }
}

void PrintInstruments(gig::File* gig) {
    int instruments = 0;
    cout << "Available Instruments:" << endl;
    gig::Instrument* pInstrument = gig->GetFirstInstrument();
    while (pInstrument) {
        instruments++;
        string name = pInstrument->pInfo->Name;
        if (name == "") name = "<NO NAME>";
        else            name = '\"' + name + '\"';
        cout << "    Instrument " << instruments << ") " << name << ", ";

        cout << " MIDIBank=" << pInstrument->MIDIBank << ", MIDIProgram=" << pInstrument->MIDIProgram << endl;

        cout << "        ScriptSlots=" << pInstrument->ScriptSlotCount() << endl;
        for (int s = 0; s < pInstrument->ScriptSlotCount(); ++s) {
            gig::Script* pScript = pInstrument->GetScriptOfSlot(s);
            string name = pScript->Name;
            cout << "        ScriptSlot[" << s << "]='" << name << "'\n";
        }

        PrintRegions(pInstrument);

        pInstrument = gig->GetNextInstrument();
    }
}

void PrintInstrumentNamesOnly(gig::File* gig) {
    int instruments = 0;
    gig::Instrument* pInstrument = gig->GetFirstInstrument();
    for (; pInstrument; pInstrument = gig->GetNextInstrument()) {
        instruments++;
        string name = pInstrument->pInfo->Name;
        if (name == "") name = "<NO NAME>";
        else            name = '\"' + name + '\"';
        cout << "Instrument " << instruments << ") " << name << endl;
    }
}

void PrintRegions(gig::Instrument* instr) {
    int iRegion = 1;
    gig::Region* pRegion = instr->GetFirstRegion();
    while (pRegion) {
        cout << "        Region " << iRegion++ << ") ";
        gig::Sample* pSample = pRegion->GetSample();
        if (pSample) {
            cout << "Sample: ";
            if (pSample->pInfo->Name != "") {
                cout << "\"" << pSample->pInfo->Name << "\", ";
            }
            cout << pSample->SamplesPerSecond << "Hz, " << endl;
        }
        else {
            cout << "<NO_VALID_SAMPLE_REFERENCE> ";
        }
        cout << "            KeyRange=" << pRegion->KeyRange.low << "-" << pRegion->KeyRange.high << ", ";
        cout << "VelocityRange=" << pRegion->VelocityRange.low << "-" << pRegion->VelocityRange.high << ", Layers=" << pRegion->Layers << endl;
        cout << "            Loops=" << pRegion->SampleLoops << endl;
        cout << "            Dimensions=" << pRegion->Dimensions << endl;
        for (int iDimension = 0; iDimension < pRegion->Dimensions; iDimension++) {
            cout << "            Dimension[" << iDimension << "]: Type=";
            gig::dimension_def_t DimensionDef = pRegion->pDimensionDefinitions[iDimension];
            switch (DimensionDef.dimension) {
                case gig::dimension_none:
                    cout << "NONE";
                    break;
                case gig::dimension_samplechannel: // If used sample has more than one channel (thus is not mono).
                    cout << "SAMPLECHANNEL";
                    break;
                case gig::dimension_layer: { // For layering of up to 8 instruments (and eventually crossfading of 2 or 4 layers).
                    gig::crossfade_t crossfade = pRegion->pDimensionRegions[iDimension]->Crossfade;
                    cout << "LAYER (Crossfade in_start=" << (int) crossfade.in_start << ",in_end=" << (int) crossfade.in_end << ",out_start=" << (int) crossfade.out_start << ",out_end=" << (int) crossfade.out_end << ")";
                    break;
                }
                case gig::dimension_velocity: // Key Velocity (this is the only dimension where the ranges can exactly be defined).
                    cout << "VELOCITY";
                    break;
                case gig::dimension_channelaftertouch: // Channel Key Pressure
                    cout << "AFTERTOUCH";
                    break;
                case gig::dimension_releasetrigger: // Special dimension for triggering samples on releasing a key.
                    cout << "RELEASETRIGGER";
                    break;
                case gig::dimension_keyboard: // Key Position
                    cout << "KEYBOARD";
                    break;
                case gig::dimension_roundrobin: // Different samples triggered each time a note is played, dimension regions selected in sequence
                    cout << "ROUNDROBIN";
                    break;
                case gig::dimension_random: // Different samples triggered each time a note is played, random order
                    cout << "RANDOM";
                    break;
                case gig::dimension_smartmidi: // For MIDI tools like legato and repetition mode
                    cout << "SMARTMIDI";
                    break;
                case gig::dimension_roundrobinkeyboard: // Different samples triggered each time a note is played, any key advances the counter
                    cout << "ROUNDROBINKEYBOARD";
                    break;
                case gig::dimension_modwheel: // Modulation Wheel (MIDI Controller 1)
                    cout << "MODWHEEL";
                    break;
                case gig::dimension_breath: // Breath Controller (Coarse, MIDI Controller 2)
                    cout << "BREATH";
                    break;
                case gig::dimension_foot: // Foot Pedal (Coarse, MIDI Controller 4)
                    cout << "FOOT";
                    break;
                case gig::dimension_portamentotime: // Portamento Time (Coarse, MIDI Controller 5)
                    cout << "PORTAMENTOTIME";
                    break;
                case gig::dimension_effect1: // Effect Controller 1 (Coarse, MIDI Controller 12)
                    cout << "EFFECT1";
                    break;
                case gig::dimension_effect2: // Effect Controller 2 (Coarse, MIDI Controller 13)
                    cout << "EFFECT2";
                    break;
                case gig::dimension_genpurpose1: // General Purpose Controller 1 (Slider, MIDI Controller 16)
                    cout << "GENPURPOSE1";
                    break;
                case gig::dimension_genpurpose2: // General Purpose Controller 2 (Slider, MIDI Controller 17)
                    cout << "GENPURPOSE2";
                    break;
                case gig::dimension_genpurpose3: // General Purpose Controller 3 (Slider, MIDI Controller 18)
                    cout << "GENPURPOSE3";
                    break;
                case gig::dimension_genpurpose4: // General Purpose Controller 4 (Slider, MIDI Controller 19)
                    cout << "GENPURPOSE4";
                    break;
                case gig::dimension_sustainpedal: // Sustain Pedal (MIDI Controller 64)
                    cout << "SUSTAINPEDAL";
                    break;
                case gig::dimension_portamento: // Portamento (MIDI Controller 65)
                    cout << "PORTAMENTO";
                    break;
                case gig::dimension_sostenutopedal: // Sostenuto Pedal (MIDI Controller 66)
                    cout << "SOSTENUTOPEDAL";
                    break;
                case gig::dimension_softpedal: // Soft Pedal (MIDI Controller 67)
                    cout << "SOFTPEDAL";
                    break;
                case gig::dimension_genpurpose5: // General Purpose Controller 5 (Button, MIDI Controller 80)
                    cout << "GENPURPOSE5";
                    break;
                case gig::dimension_genpurpose6: // General Purpose Controller 6 (Button, MIDI Controller 81)
                    cout << "GENPURPOSE6";
                    break;
                case gig::dimension_genpurpose7: // General Purpose Controller 7 (Button, MIDI Controller 82)
                    cout << "GENPURPOSE7";
                    break;
                case gig::dimension_genpurpose8: // General Purpose Controller 8 (Button, MIDI Controller 83)
                    cout << "GENPURPOSE8";
                    break;
                case gig::dimension_effect1depth: // Effect 1 Depth (MIDI Controller 91)
                    cout << "EFFECT1DEPTH";
                    break;
                case gig::dimension_effect2depth: // Effect 2 Depth (MIDI Controller 92)
                    cout << "EFFECT2DEPTH";
                    break;
                case gig::dimension_effect3depth: // Effect 3 Depth (MIDI Controller 93)
                    cout << "EFFECT3DEPTH";
                    break;
                case gig::dimension_effect4depth: // Effect 4 Depth (MIDI Controller 94)
                    cout << "EFFECT4DEPTH";
                    break;
                case gig::dimension_effect5depth:  // Effect 5 Depth (MIDI Controller 95)
                    cout << "EFFECT5DEPTH";
                    break;
                default:
                    cout << "UNKNOWN (" << int(DimensionDef.dimension) << ") - please report this !";
                    break;
            }
            cout << ", Bits=" << (uint) DimensionDef.bits << ", Zones=" << (uint) DimensionDef.zones;
            cout << ", SplitType=";
            switch (DimensionDef.split_type) {
                case gig::split_type_normal:
                    cout << "NORMAL" << endl;
                    break;
                case gig::split_type_bit:
                    cout << "BIT" << endl;
                    break;
                default:
                    cout << "UNKNOWN" << endl;
            }
        }

        PrintDimensionRegions(pRegion);

        pRegion = instr->GetNextRegion();
    }
}

template<typename T_int>
static void printIntArray(T_int* array, int size) {
    printf("{");
    for (int i = 0; i < size; ++i)
        printf("[%d]=%d,", i, array[i]);
    printf("}");
    fflush(stdout);
}

static string lfoWaveName(gig::lfo_wave_t wave) {
    switch (wave) {
        case gig::lfo_wave_sine: return "Sine";
        case gig::lfo_wave_triangle: return "Triangle";
        case gig::lfo_wave_saw: return "Saw";
        case gig::lfo_wave_square: return "Square";
    }
    return "Unknown";
}

void PrintDimensionRegions(gig::Region* rgn) {
    int dimensionRegions = 0;
    gig::DimensionRegion* pDimensionRegion;
    while (dimensionRegions < rgn->DimensionRegions) {
        pDimensionRegion = rgn->pDimensionRegions[dimensionRegions];
        if (!pDimensionRegion) break;

        cout << "            Dimension Region " << dimensionRegions + 1 << ")" << endl;

        gig::Sample* pSample = pDimensionRegion->pSample;
        if (pSample) {
            cout << "                Sample: ";
            if (pSample->pInfo->Name != "") {
                cout << "\"" << pSample->pInfo->Name << "\", ";
            }
            cout << pSample->SamplesPerSecond << "Hz, ";
            cout << "UnityNote=" << (int) pDimensionRegion->UnityNote << ", FineTune=" << (int) pDimensionRegion->FineTune << ", Gain=" << (-pDimensionRegion->Gain / 655360.0) << "dB, SampleStartOffset=" << pDimensionRegion->SampleStartOffset << endl;
        }
        else {
            cout << "                Sample: <NO_VALID_SAMPLE_REFERENCE> " << endl;
        }
        cout << "                LFO1WaveForm=" << lfoWaveName(pDimensionRegion->LFO1WaveForm) << ", LFO1Phase=" << pDimensionRegion->LFO1Phase << ", LFO1FlipPhase=" << pDimensionRegion->LFO1FlipPhase << endl;
        cout << "                LFO1Frequency=" << pDimensionRegion->LFO1Frequency << "Hz, LFO1InternalDepth=" << pDimensionRegion-> LFO1InternalDepth << ", LFO1ControlDepth=" << pDimensionRegion->LFO1ControlDepth << " LFO1Controller=" << pDimensionRegion->LFO1Controller << endl;
        cout << "                LFO2WaveForm=" << lfoWaveName(pDimensionRegion->LFO2WaveForm) << ", LFO2Phase=" << pDimensionRegion->LFO2Phase << ", LFO2FlipPhase=" << pDimensionRegion->LFO2FlipPhase << endl;
        cout << "                LFO2Frequency=" << pDimensionRegion->LFO2Frequency << "Hz, LFO2InternalDepth=" << pDimensionRegion-> LFO2InternalDepth << ", LFO2ControlDepth=" << pDimensionRegion->LFO2ControlDepth << " LFO2Controller=" << pDimensionRegion->LFO2Controller << endl;
        cout << "                LFO3WaveForm=" << lfoWaveName(pDimensionRegion->LFO3WaveForm) << ", LFO3Phase=" << pDimensionRegion->LFO3Phase << ", LFO3FlipPhase=" << pDimensionRegion->LFO3FlipPhase << endl;
        cout << "                LFO3Frequency=" << pDimensionRegion->LFO3Frequency << "Hz, LFO3InternalDepth=" << pDimensionRegion-> LFO3InternalDepth << ", LFO3ControlDepth=" << pDimensionRegion->LFO3ControlDepth << " LFO3Controller=" << pDimensionRegion->LFO3Controller << endl;
        cout << "                EG1PreAttack=" << pDimensionRegion->EG1PreAttack << "permille, EG1Attack=" << pDimensionRegion->EG1Attack << "s, EG1Decay1=" << pDimensionRegion->EG1Decay1 << "s,  EG1Sustain=" << pDimensionRegion->EG1Sustain << "permille, EG1Release=" << pDimensionRegion->EG1Release << "s, EG1Decay2=" << pDimensionRegion->EG1Decay2 << "s, EG1Hold=" << pDimensionRegion->EG1Hold << endl;
        cout << "                EG2PreAttack=" << pDimensionRegion->EG2PreAttack << "permille, EG2Attack=" << pDimensionRegion->EG2Attack << "s, EG2Decay1=" << pDimensionRegion->EG2Decay1 << "s,  EG2Sustain=" << pDimensionRegion->EG2Sustain << "permille, EG2Release=" << pDimensionRegion->EG2Release << "s, EG2Decay2=" << pDimensionRegion->EG2Decay2 << "s" << endl;
        cout << "                VCFEnabled=" << pDimensionRegion->VCFEnabled << ", VCFType=" << pDimensionRegion->VCFType << ", VCFCutoff=" << (int) pDimensionRegion->VCFCutoff << ",  VCFResonance=" << (int) pDimensionRegion->VCFResonance << ", VCFCutoffController=" << pDimensionRegion->VCFCutoffController << endl;
        cout << "                VelocityResponseCurve=";
        switch (pDimensionRegion->VelocityResponseCurve) {
            case gig::curve_type_nonlinear:
                cout << "NONLINEAR";
                break;
            case gig::curve_type_linear:
                cout << "LINEAR";
                break;
            case gig::curve_type_special:
                cout << "SPECIAL";
                break;
            case gig::curve_type_unknown:
            default:
                cout << "UNKNOWN - please report this !";
        }
        cout << ", VelocityResponseDepth=" << (int) pDimensionRegion->VelocityResponseDepth << ", VelocityResponseCurveScaling=" << (int) pDimensionRegion->VelocityResponseCurveScaling << endl;
        cout << "                VelocityUpperLimit=" << (int) pDimensionRegion->VelocityUpperLimit << " DimensionUpperLimits[]=" << flush;
        printIntArray(pDimensionRegion->DimensionUpperLimits, 8);
        cout << endl;
#if 0 // requires access to protected member VelocityTable, so commented out for now
        if (pDimensionRegion->VelocityTable) {
            cout << "                VelocityTable[]=" << flush;
            printIntArray(pDimensionRegion->VelocityTable, 127);
            cout << endl;
        }
#endif
        cout << "                Pan=" << (int) pDimensionRegion->Pan;
        cout << ", SustainReleaseTrigger=";
        switch (pDimensionRegion->SustainReleaseTrigger) {
            case gig::sust_rel_trg_none:
                cout << "NONE";
                break;
            case gig::sust_rel_trg_maxvelocity:
                cout << "MAXVELOCITY";
                break;
            case gig::sust_rel_trg_keyvelocity:
                cout << "KEYVELOCITY";
                break;
        }
        cout << ", NoNoteOffReleaseTrigger=" << int(pDimensionRegion->NoNoteOffReleaseTrigger);
        cout << endl;
        {
            gig::eg_opt_t& egopt = pDimensionRegion->EG1Options;
            cout << "                EG1AttackCancel=" << egopt.AttackCancel << ", EG1AttackHoldCancel=" << egopt.AttackHoldCancel << ", EG1Decay1Cancel=" << egopt.Decay1Cancel << ", EG1Decay2Cancel=" << egopt.Decay2Cancel << ", EG1ReleaseCancel=" << egopt.ReleaseCancel << endl;
        }
        {
            gig::eg_opt_t& egopt = pDimensionRegion->EG2Options;
            cout << "                EG2AttackCancel=" << egopt.AttackCancel << ", EG2AttackHoldCancel=" << egopt.AttackHoldCancel << ", EG2Decay1Cancel=" << egopt.Decay1Cancel << ", EG2Decay2Cancel=" << egopt.Decay2Cancel << ", EG2ReleaseCancel=" << egopt.ReleaseCancel << endl;
        }

        dimensionRegions++;
    }
}

struct _FailedSample {
    gig::Sample* sample;
    uint32_t calculatedCRC;
};

bool VerifyFile(gig::File* _gig) {
    PubFile* gig = (PubFile*) _gig;

    cout << "Verifying sample checksum table ... " << flush;
    if (!gig->VerifySampleChecksumTable()) {
        cout << "DAMAGED\n";
        cout << "You may use --rebuild-checksums to repair the sample checksum table.\n";
        return false;
    }
    cout << "OK\n" << flush;

    cout << "Verifying samples ... " << flush;
    std::map<int,_FailedSample> failedSamples;
    int iTotal = 0;
    for (gig::Sample* pSample = gig->GetFirstSample(); pSample; pSample = gig->GetNextSample(), ++iTotal) {
        uint32_t crc; // will be set to the actually now calculated checksum
        if (!pSample->VerifyWaveData(&crc)) {
            _FailedSample failed;
            failed.sample = pSample;
            failed.calculatedCRC = crc;
            failedSamples[iTotal] = failed;
        }
    }
    if (failedSamples.empty()) {
        cout << "ALL OK\n";
        return true;
    } else {
        cout << failedSamples.size() << " of " << iTotal << " Samples DAMAGED:\n";
        for (std::map<int,_FailedSample>::iterator it = failedSamples.begin(); it != failedSamples.end(); ++it) {
            const int i = it->first;
            gig::Sample* pSample = it->second.sample;

            string name = pSample->pInfo->Name;
            if (name == "") name = "<NO NAME>";
            else            name = '\"' + name + '\"';

            cout << "Damaged Sample " << (i+1) << ") " << name << flush;
            printf(" expectedCRC=%x calculatedCRC=%x\n", pSample->GetWaveDataCRC32Checksum(), it->second.calculatedCRC);
        }
        return false;
    }
}

void RebuildChecksumTable(gig::File* _gig) {
    PubFile* gig = (PubFile*) _gig;

    cout << "Recalculating checksums of all samples ... " << flush;
    bool bSaveRequired = gig->RebuildSampleChecksumTable();
    cout << "OK\n";
    if (bSaveRequired) {
        cout << "WARNING: File structure change required, rebuilding entire file now ..." << endl;
        gig->Save();
        cout << "DONE\n";
        cout << "NOTE: Since the entire file was rebuilt, you may need to manually check all samples in this particular case now!\n";
    }
}

string Revision() {
    string s = "$Revision: 3623 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

void PrintVersion() {
    cout << "gigdump revision " << Revision() << endl;
    cout << "using " << gig::libraryName() << " " << gig::libraryVersion() << endl;
}

void PrintUsage() {
    cout << "gigdump - parses Gigasampler files and prints out the content." << endl;
    cout << endl;
    cout << "Usage: gigdump [-v | --verify | --rebuild-checksums] FILE" << endl;
    cout << endl;
    cout << "   --rebuild-checksums  Rebuild checksum table for all samples." << endl;
    cout << endl;
    cout << "	-v                   Print version and exit." << endl;
    cout << endl;
    cout << "   --verify             Checks raw wave data integrity of all samples." << endl;
    cout << endl;
    cout << "   --instrument-names   Print only instrument names." << endl;
    cout << endl;
}
