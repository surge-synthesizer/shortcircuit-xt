/***************************************************************************
 *                                                                         *
 *   libgig - C++ cross-platform Gigasampler format file access library    *
 *                                                                         *
 *   Copyright (C) 2003-2014 by Christian Schoenebeck                      *
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
#include <config.h>
#endif

#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdlib>

#include "../RIFF.h"

using namespace std;

string Revision();
void PrintVersion();
void PrintUsage();
void PrintChunkList(RIFF::List* list, bool PrintSize);
static uint32_t strToChunkID(string s);

int main(int argc, char *argv[])
{
    bool bPrintSize = false;
    RIFF::endian_t endian = RIFF::endian_native;
    RIFF::layout_t layout = RIFF::layout_standard;
    bool bExpectFirstChunkID = false;
    uint32_t uiExpectedFirstChunkID;

    // validate & parse arguments provided to this program
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
        } else if (opt == "-s") {
            bPrintSize = true;
        } else if (opt == "--big-endian") {
            endian = RIFF::endian_big;
        } else if (opt == "--little-endian") {
            endian = RIFF::endian_little;
        } else if (opt == "--flat") {
            layout = RIFF::layout_flat;
        } else if (opt == "--first-chunk-id") {
            iArg++;
            if (iArg >= argc) {
                PrintVersion();
                return EXIT_SUCCESS;
            }
            bExpectFirstChunkID = true;
            uiExpectedFirstChunkID = strToChunkID(argv[iArg]);
        } else {
            cerr << "Unknown option '" << opt << "'" << endl;
            cerr << endl;
            PrintUsage();
            return EXIT_FAILURE;
        }
    }

    const int nFileArguments = argc - iArg;
    if (nFileArguments != 1) {
        cerr << "You must provide a file argument." << endl;
        return EXIT_FAILURE;
    }
    const int FileArgIndex = iArg;

    FILE* hFile = fopen(argv[FileArgIndex], "r");
    if (!hFile) {
        cerr << "Invalid file argument!" << endl;
        return EXIT_FAILURE;
    }
    fclose(hFile);
    try {
        RIFF::File* riff = NULL;
        switch (layout) {
            case RIFF::layout_standard:
                riff = new RIFF::File(argv[FileArgIndex]);
                cout << "RIFF(" << riff->GetListTypeString() << ")->";
                break;
            case RIFF::layout_flat:
                if (!bExpectFirstChunkID) {
                    cerr << "If you are using '--flat' then you must also use '--first-chunk-id'." << endl; 
                    return EXIT_FAILURE;
                }
                riff = new RIFF::File(
                    argv[FileArgIndex], uiExpectedFirstChunkID, endian, layout
                );
                cout << "Flat RIFF-alike file";
                break;
        }
        if (bPrintSize) cout << " (" << riff->GetSize() << " Bytes)";
        cout << endl;
        PrintChunkList(riff, bPrintSize);
        delete riff;
    }
    catch (RIFF::Exception e) {
        e.PrintMessage();
        return EXIT_FAILURE;
    }
    catch (...) {
        cerr << "Unknown exception while trying to parse file." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void PrintChunkList(RIFF::List* list, bool PrintSize) {
    RIFF::Chunk* ck = list->GetFirstSubChunk();
    while (ck != NULL) {
        RIFF::Chunk* ckParent = ck;
        while (ckParent->GetParent() != NULL) {
            cout << "            "; // e.g. 'LIST(INFO)->'
            ckParent = ckParent->GetParent();
        }
        cout << ck->GetChunkIDString();
        switch (ck->GetChunkID()) {
            case CHUNK_ID_LIST: case CHUNK_ID_RIFF:
              {
                RIFF::List* l = (RIFF::List*) ck;
                cout << "(" << l->GetListTypeString() << ")->";
                if (PrintSize) cout << " (" << l->GetSize() << " Bytes)";
                cout << endl;
                PrintChunkList(l, PrintSize);
                break;
              }
            default:
                cout << ";";
                if (PrintSize) cout << " (" << ck->GetSize() << " Bytes)";
                cout << endl;
        }
        ck = list->GetNextSubChunk();
    }
}

static uint32_t strToChunkID(string s) {
    if (s.size() != 4) {
        cerr << "Argument after '--first-chunk-id' must be exactly 4 characters long." << endl;
        exit(-1);
    }
    uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
       uint32_t byte = s[i];
       result |= byte << i*8;
    }
    return result;
}

string Revision() {
    string s = "$Revision: 2573 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

void PrintVersion() {
    cout << "rifftree revision " << Revision() << endl;
    cout << "using " << RIFF::libraryName() << " " << RIFF::libraryVersion() << endl;
}

void PrintUsage() {
    cout << "rifftree - parses an arbitrary RIFF file and prints out the RIFF tree structure." << endl;
    cout << endl;
    cout << "Usage: rifftree [OPTIONS] FILE" << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << endl;
    cout << "  -v  Print version and exit." << endl;
    cout << endl;
    cout << "  -s  Print the size of each chunk." << endl;
    cout << endl;
    cout << "  --flat" << endl;
    cout << "      First chunk of file is not a list (container) chunk." << endl;
    cout << endl;
    cout << "  --first-chunk-id CKID" << endl;
    cout << "      32 bit chunk ID of very first chunk in file." << endl;
    cout << endl;
    cout << "  --big-endian" << endl;
    cout << "      File is in big endian format." << endl;
    cout << endl;
    cout << "  --little-endian" << endl;
    cout << "      File is in little endian format." << endl;
    cout << endl;
}
