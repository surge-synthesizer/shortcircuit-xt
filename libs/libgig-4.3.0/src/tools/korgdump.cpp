/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2014 - 2017 Christian Schoenebeck                       *
 *                      <cuse@users.sourceforge.net>                       *
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
#include <set>

#include "../Korg.h"

using namespace std;

static string Revision() {
    string s = "$Revision: 3175 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

static void printVersion() {
    cout << "korgdump revision " << Revision() << endl;
    cout << "using " << Korg::libraryName() << " " << Korg::libraryVersion() << endl;
}

static void printUsage() {
    cout << "korgdump - parses Korg sound files and prints out their content." << endl;
    cout << endl;
    cout << "Usage: korgdump [-v] FILE" << endl;
    cout << endl;
    cout << "   -v  Print version and exit." << endl;
    cout << endl;
}

static bool endsWith(const string& haystack, const string& needle) {
    if (haystack.size() < needle.size()) return false;
    return haystack.substr(haystack.size() - needle.size(), needle.size()) == needle;
}

static void printSample(const string& filename, int i = -1) {
    Korg::KSFSample* smpl = new Korg::KSFSample(filename);
    cout << "        ";
    if (i != -1) cout << (i+1) << ". ";
    cout << "Sample SampleFile='" << smpl->FileName() << "'" << endl;
    cout << "            Name='" << smpl->Name << "'" << endl;
    cout << "            Start=" << smpl->Start << ", Start2=" << smpl->Start2 << ", LoopStart=" << smpl->LoopStart << ", LoopEnd=" << smpl->LoopEnd << endl;
    cout << "            SampleRate=" << smpl->SampleRate << ", LoopTune=" << (int)smpl->LoopTune << ", Channels=" << (int)smpl->Channels << ", BitDepth=" << (int)smpl->BitDepth << ", SamplePoints=" << smpl->SamplePoints << endl;
    cout << "            IsCompressed=" << smpl->IsCompressed() << ", CompressionID=" << (int)smpl->CompressionID() << ", Use2ndStart=" << (int)smpl->Use2ndStart() << endl;
    cout << endl;
    delete smpl;
}

static void printRegion(int i, Korg::KMPRegion* rgn) {
    cout << "        " << (i+1) << ". Region SampleFile='" << rgn->FullSampleFileName() << "'" << endl;
    cout << "            OriginalKey=" << (int)rgn->OriginalKey << ", TopKey=" << (int)rgn->TopKey << endl;
    cout << "            Transpose=" << rgn->Transpose << ", Tune=" << (int)rgn->Tune << ", Level=" << (int)rgn->Level << ", Pan=" << (int)rgn->Pan << endl;
    cout << "            FilterCutoff=" << (int)rgn->FilterCutoff << endl;
    cout << endl;
}

static void printInstrument(Korg::KMPInstrument* instr) {
    cout << "Instrument '" << instr->Name() << "'" << endl;
    cout << "    Use2ndStart=" << instr->Use2ndStart() << endl;
    cout << endl;
    set<string> sampleFileNames;
    for (int i = 0; i < instr->GetRegionCount(); ++i) {
        Korg::KMPRegion* rgn = instr->GetRegion(i);
        printRegion(i, rgn);
        sampleFileNames.insert(rgn->FullSampleFileName());
    }

    cout << "Samples referenced by instrument:" << endl;
    cout << endl;

    int i = 0;
    for (set<string>::iterator it = sampleFileNames.begin();
         it != sampleFileNames.end(); ++it, ++i)
    {
        printSample(*it, i);
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printUsage();
        return EXIT_FAILURE;
    }
    if (argv[1][0] == '-') {
        switch (argv[1][1]) {
            case 'v':
                printVersion();
                return EXIT_SUCCESS;
        }
    }
    const char* filename = argv[1];
    FILE* hFile = fopen(filename, "r");
    if (!hFile) {
        cout << "Invalid file argument (could not open given file for reading)!" << endl;
        return EXIT_FAILURE;
    }
    fclose(hFile);
    try {
        if (endsWith(filename, ".KMP")) {
            Korg::KMPInstrument* instr = new Korg::KMPInstrument(filename);
            printInstrument(instr);
            delete instr;
        } else if (endsWith(filename, ".KSF")) {
            printSample(filename);
        } else if (endsWith(filename, ".PCG")) {
            cout << "There is no support for .PCG files in this version of korgdump yet." << endl;
            return EXIT_FAILURE;
        } else {
            cout << "Unknown file type (file name postfix)" << endl;
            return EXIT_FAILURE;
        }
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
