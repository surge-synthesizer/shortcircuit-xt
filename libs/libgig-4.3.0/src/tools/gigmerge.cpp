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
# include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

#include "../gig.h"

using namespace std;

static vector<RIFF::File*> g_riffs;
static vector<gig::File*> g_gigs;

static void printUsage() {
    cout << "gigmerge - merges several Gigasampler files to one Gigasampler file." << endl;
    cout << endl;
    cout << "Usage: gigmerge [-v] FILE1 FILE2 [ ... ] NEWFILE" << endl;
    cout << endl;
    cout << "   -v  Print version and exit." << endl;
    cout << endl;
    cout << "Note: There is no check for duplicate (equivalent) samples yet." << endl;
    cout << endl;
}

static string programRevision() {
    string s = "$Revision: 2573 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

static void printVersion() {
    cout << "gigmerge revision " << programRevision() << endl;
    cout << "using " << gig::libraryName() << " " << gig::libraryVersion() << endl;
}

static void cleanup() {
    for (int i = 0; i < g_gigs.size(); ++i) delete g_gigs[i];
    for (int i = 0; i < g_riffs.size(); ++i) delete g_riffs[i];
    g_gigs.clear();
    g_riffs.clear();
}

int main(int argc, char *argv[]) {
    // validate arguments provided to this program
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
    if (argc <= 3) {
        printUsage();
        return EXIT_FAILURE;
    }
    
    // open all input .gig files
    const int iInputFiles = argc - 2;
    int i;
    try {
        for (i = 0; i < iInputFiles; ++i) {
            RIFF::File* riff = new RIFF::File(argv[i+1]);
            g_riffs.push_back(riff);

            gig::File* gig = new gig::File(riff);
            g_gigs.push_back(gig);   
        }
    } catch (RIFF::Exception e) {
        cerr << "Failed opening input file " << (i+1) << endl;
        e.PrintMessage();
        return EXIT_FAILURE;
    } catch (...) {
        cerr << "Failed opening input file " << (i+1) << endl;
        cerr << "Unknown exception while trying to access input file." << endl;
        return EXIT_FAILURE;
    }
    
    gig::File* outGig = NULL;
    
    // create a new .gig file as output
    try {
        outGig = new gig::File();
        outGig->SetFileName(argv[argc-1]); // required, because AddContentOf() performs auto save
        for (int i = 0; i < g_gigs.size(); ++i) {
            outGig->AddContentOf(g_gigs[i]);
        }
        outGig->Save();
    } catch (RIFF::Exception e) {
        cerr << "Failed generating output file:" << endl;
        e.PrintMessage();
        return EXIT_FAILURE;
    } catch (...) {
        cerr << "Unknown exception while trying to assemble output file." << endl;
        return EXIT_FAILURE;
    }
    
    // close all input files
    cleanup();
    
    return EXIT_SUCCESS;
}
