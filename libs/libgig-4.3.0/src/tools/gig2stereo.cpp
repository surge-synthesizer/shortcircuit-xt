/***************************************************************************
 *                                                                         *
 *   libgig - C++ cross-platform Gigasampler format file access library    *
 *                                                                         *
 *   Copyright (C) 2003-2017 by Christian Schoenebeck                      *
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
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <utility>

#ifdef WIN32
# define DIR_SEPARATOR '\\'
#else
# define DIR_SEPARATOR '/'
#endif

#include "../gig.h"

using namespace std;

static set<string> g_files;

static void printUsage() {
    cout << "gig2stereo - converts Gigasampler files from mono sample pairs to" << endl;
    cout << "             true stereo interleaved samples." << endl;
    cout << endl;
    cout << "Usage: gig2stereo [-r] [--keep] [--verbose [LEVEL]] [--force-replace] FILE_OR_DIR1 [ FILE_OR_DIR2 ... ]" << endl;
    cout << "       gig2stereo -v" << endl;
    cout << endl;
    cout << "   --force-replace  Replace all old mono references by the new stereo ones." << endl;
    cout << endl;
    cout << "   --incompatible   Also match sample pairs that seem to be incompatible." << endl;
    cout << endl;
    cout << "   --keep           Keep orphaned mono samples after conversion." << endl;
    cout << endl;
    cout << "   -r               Recurse through subdirectories." << endl;
    cout << endl;
    cout << "   -v               Print version and exit." << endl;
    cout << endl;
    cout << "   --verbose        Print additional information while converting." << endl;
    cout << endl;
    cout << "Read `man gig2stereo' for details." << endl;
    cout << endl;
}

static string programRevision() {
    string s = "$Revision: 3175 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

static void printVersion() {
    cout << "gig2stereo revision " << programRevision() << endl;
    cout << "using " << gig::libraryName() << " " << gig::libraryVersion() << endl;
}

static bool isGigFileName(const string path) {
    const string t = ".gig";
    return path.substr(path.length() - t.length()) == t;
}

static bool endsWith(const string& haystack, const string& needle, bool caseSensitive) {
    if (haystack.size() < needle.size()) return false;
    const string sub = haystack.substr(haystack.size() - needle.size(), needle.size());
    return (caseSensitive) ? (sub == needle) : (!strcasecmp(sub.c_str(), needle.c_str()));
}

/// Removes spaces from the beginning and end of @a s.
static string stripWhiteSpace(string s) {
    // strip white space at the beginning
    for (int i = 0; i < s.size(); ++i) {
        if (s[i] != ' ') {
            s = s.substr(i);
            break;
        }
    }
    // strip white space at the end
    for (int i = s.size() - 1; i >= 0; --i) {
        if (s[i] != ' ') {
            s = s.substr(0, i+1);
            break;
        }
    }
    return s;
}

static void collectFilesOfDir(string path, bool bRecurse, bool* pbError = NULL) {
    DIR* d = opendir(path.c_str());
    if (!d) {
        if (pbError) *pbError = true;
        cerr << strerror(errno) << " : '" << path << "'" << endl;
        return;
    }

    for (struct dirent* e = readdir(d); e; e = readdir(d)) {
        if (string(e->d_name) == "." || string(e->d_name) == "..")
            continue;

        const string fullName = path + DIR_SEPARATOR + e->d_name;

        struct stat s;
        if (stat(fullName.c_str(), &s)) {
            if (pbError) *pbError = true;
            cerr << strerror(errno) << " : '" << fullName << "'" << endl;
            continue;
        }

        if (S_ISREG(s.st_mode) && isGigFileName(fullName)) {
            g_files.insert(fullName);
        } else if (S_ISDIR(s.st_mode) && bRecurse) {
            collectFilesOfDir(fullName, bRecurse, pbError);
        }
    }

    closedir(d);
}

static void collectFiles(string path, bool bRecurse, bool* pbError = NULL) {
    struct stat s;
    if (stat(path.c_str(), &s)) {
        if (pbError) *pbError = true;
        cerr << strerror(errno) << " : '" << path << "'" << endl;
        return;
    }
    if (S_ISREG(s.st_mode) && isGigFileName(path)) {
        g_files.insert(path);
    } else if (S_ISDIR(s.st_mode)) {
        collectFilesOfDir(path, bRecurse, pbError);
    } else {
        if (pbError) *pbError = true;
        cerr << "Neither a regular (.gig) file nor directory : '" << path << "'" << endl;
    }
}

static string stripAudioChannelFromName(const string& s) {
    if (endsWith(s, "-L", false) || endsWith(s, "-R", false) ||
        endsWith(s, "_L", false) || endsWith(s, "_R", false) ||
        endsWith(s, " L", false) || endsWith(s, " R", false) )
    {
        return s.substr(0, s.size() - 2);
    }
    if (endsWith(s, "-LEFT", false) || endsWith(s, "_LEFT", false) || endsWith(s, " LEFT", false) ) {
        return s.substr(0, s.size() - 5);
    }
    if (endsWith(s, "-RIGHT", false) || endsWith(s, "_RIGHT", false) || endsWith(s, " RIGHT", false) ) {
        return s.substr(0, s.size() - 6);
    }
    return s;
}

#define OPTIONAL_SKIP_CHECK() \
    if (skipIncompatible) { \
        cerr << " Skipping!\n"; \
        continue; /* skip to convert this sample pair */ \
    } else { \
        cerr << " Merging anyway (upon request)!\n"; \
    }

/**
 * Converts .gig file given by @a path towards using true stereo interleaved
 * samples.
 *
 * @param path - path and file name of .gig file to be converted
 * @param keep - if true: do not delete the old mono samples, even if they are
 *               not referenced at all anymore after conversion
 * @param forceReplace - By default certain references of the old mono samples
 *                       are not replaced by the new true stereo samples,
 *                       usually because the respective mono reference is been
 *                       used in an instrument context that seems to be entirely
 *                       mono, not stereo. By enabling this argument all those
 *                       old references will be replaced by the new stereo
 *                       sample reference instead as well.
 * @param matchIncompatible - if set to true, only mono sample pairs will be
 *                            qualified to be merged to true stereo samples if
 *                            their main sample characteristics match, if set
 *                            to true this sanity check will be skipped
 * @param verbosity - verbosity level, defines whether and how much additional
 *                    information to be printed to the console while doing the
 *                    conversion (0 .. 2)
 */
static bool convertFileToStereo(const string path, bool keep, bool forceReplace, bool skipIncompatible, int verbose) {
    try {
        // open .gig file
        RIFF::File riff(path);
        gig::File gig(&riff);

        typedef pair<gig::DimensionRegion*, gig::DimensionRegion*> DimRgnPair;
        typedef pair<gig::Sample*, gig::Sample*> SamplePair;

        // find and collect all dimension regions with a 2 mono samples pair
        if (verbose) cout << "Searching mono sample pairs ... " << flush;
        if (verbose >= 2) cout << endl;
        map<SamplePair, vector<DimRgnPair> > samplePairs;
        map<gig::Sample*, SamplePair> samplePairsByL;
        map<gig::Sample*, SamplePair> samplePairsByR;
        for (gig::Instrument* instr = gig.GetFirstInstrument(); instr; instr = gig.GetNextInstrument()) {
            for (gig::Region* rgn = instr->GetFirstRegion(); rgn; rgn = instr->GetNextRegion()) {
                int bits = 0;
                for (int d = 0; d < 8; ++d) {
                    if (rgn->pDimensionDefinitions[d].dimension == gig::dimension_samplechannel) {
                        for (int dr = 0; dr < rgn->DimensionRegions && rgn->pDimensionRegions[dr]; ++dr) {
                            if (!rgn->pDimensionRegions[dr]->pSample) continue; // no sample assigned, ignore
                            if (rgn->pDimensionRegions[dr]->pSample->Channels != 1) continue; // seems already to be a true stereo sample, ignore

                            const int channel = (dr & (1 << bits)) ? 1 : 0;
                            if (channel == 1) continue; // ignore right audio channel dimension region here

                            // so current dimension region is for left channel, so now get its matching right channel dimension region ...
                            const int drR = dr | (1 << bits);
                            if (drR >= rgn->DimensionRegions || !rgn->pDimensionRegions[drR]) {
                                cerr << "WARNING: Could not find matching right channel for left channel dimension region!" << endl;
                                continue;
                            }
                            if (!rgn->pDimensionRegions[drR]->pSample) continue; // only a sample for left channel, but none for right channel, ignore
                            if (rgn->pDimensionRegions[drR]->pSample->Channels != 1) {
                                cerr << "WARNING: Right channel seems already to be a true stereo sample!" << endl;
                                continue;
                            }
                            if (rgn->pDimensionRegions[dr]->pSample == rgn->pDimensionRegions[drR]->pSample) continue; // both channels use same sample, thus actually a mono thing here, ignore

                            // found a valid mono pair, remember it
                            SamplePair samplePair(
                                rgn->pDimensionRegions[dr]->pSample,
                                rgn->pDimensionRegions[drR]->pSample
                            );
                            SamplePair samplePairInversed(
                                rgn->pDimensionRegions[drR]->pSample,
                                rgn->pDimensionRegions[dr]->pSample
                            );
                            DimRgnPair dimRgnpair(
                                rgn->pDimensionRegions[dr],
                                rgn->pDimensionRegions[drR]
                            );
                            if (samplePairs.count(samplePairInversed)) {
                                cerr << "WARNING: Sample pair ['" << rgn->pDimensionRegions[dr]->pSample->pInfo->Name << "', '" << rgn->pDimensionRegions[drR]->pSample->pInfo->Name << "'] already referenced in inverse channel order!\n";
                            } else {
                                if (samplePairsByL.count(rgn->pDimensionRegions[drR]->pSample)) {
                                    cerr << "WARNING: Right mono sample '" << rgn->pDimensionRegions[drR]->pSample->pInfo->Name << "' already referenced as left sample!\n";
                                }
                                if (samplePairsByR.count(rgn->pDimensionRegions[dr]->pSample)) {
                                    cerr << "WARNING: Left mono sample '" << rgn->pDimensionRegions[dr]->pSample->pInfo->Name << "' already referenced as right sample!\n";
                                }
                            }
                            samplePairs[samplePair].push_back(dimRgnpair);
                            if (verbose >= 2) cout << "Found mono sample pair ['" << rgn->pDimensionRegions[dr]->pSample->pInfo->Name << "', '" << rgn->pDimensionRegions[drR]->pSample->pInfo->Name << "'].\n" << flush;
                        }
                        goto nextRegion;
                    }
                    bits += rgn->pDimensionDefinitions[d].bits;
                }
                nextRegion:;
            }
        }
        if (verbose) cout << samplePairs.size() << " mono pairs found.\n" << flush;

        // create a true stereo (interleaved) sample for each one of the found
        // mono sample pairs
        if (verbose) cout << "Creating true stereo samples ... " << flush;
        if (verbose >= 2) cout << endl;
        map<SamplePair, vector<DimRgnPair> > samplePairsFiltered;
        map<SamplePair, gig::Sample*> targetStereoSamples;
        for (map<SamplePair, vector<DimRgnPair> >::iterator it = samplePairs.begin();
             it != samplePairs.end(); ++it)
        {
            gig::Sample* pSampleL = it->first.first;
            gig::Sample* pSampleR = it->first.second;

            // bunch of sanity checks which ensure that both mono samples are
            // using the same base parameter characteristics
            if (pSampleL->SamplesPerSecond != pSampleR->SamplesPerSecond) {
                cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different sample rate! Skipping!\n";
                continue;
            }
            if (pSampleL->FrameSize != pSampleR->FrameSize) {
                cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different frame size! Skipping!\n";
                continue;
            }
            if (pSampleL->MIDIUnityNote != pSampleR->MIDIUnityNote) {
                cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different unity notes!";
                OPTIONAL_SKIP_CHECK();
            }
            if (pSampleL->FineTune != pSampleR->FineTune) {
                cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different fine tune!";
                OPTIONAL_SKIP_CHECK();
            }
            if (pSampleL->Loops != pSampleR->Loops) {
                cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different loop amount! Skipping!\n";
                continue; // skip to convert this sample pair
            }
            if (pSampleL->Loops == 1 && pSampleR->Loops == 1) {
                if (pSampleL->LoopType != pSampleR->LoopType) {
                    cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different loop type!";
                    OPTIONAL_SKIP_CHECK();
                }
                if (pSampleL->LoopStart != pSampleR->LoopStart) {
                    cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different loop start!";
                    OPTIONAL_SKIP_CHECK();
                }
                if (pSampleL->LoopEnd != pSampleR->LoopEnd) {
                    cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different loop end!";
                    OPTIONAL_SKIP_CHECK();
                }
                if (pSampleL->LoopSize != pSampleR->LoopSize) {
                    cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different loop size!";
                    OPTIONAL_SKIP_CHECK();
                }
                if (pSampleL->LoopPlayCount != pSampleR->LoopPlayCount) {
                    cerr << "WARNING: Sample pair ['" << pSampleL->pInfo->Name << "', '" << pSampleR->pInfo->Name << "'] with different loop play count!";
                    OPTIONAL_SKIP_CHECK();
                }
            }

            // create the stereo sample
            gig::Sample* pStereoSample = gig.AddSample();

            // add the new stereo sample to an appropriate sample group
            const string groupName = stripWhiteSpace(stripAudioChannelFromName(pSampleL->GetGroup()->Name)) + " STEREO";
            gig::Group* pGroup = NULL;
            for (gig::Group* gr = gig.GetFirstGroup(); gr; gr = gig.GetNextGroup()) {
                if (gr->Name == groupName) {
                    pGroup = gr;
                    break;
                }
            }
            if (!pGroup) {
                pGroup = gig.AddGroup();
                pGroup->Name = groupName;
            }
            pGroup->AddSample(pStereoSample);

            pStereoSample->pInfo->Name      = stripWhiteSpace(stripAudioChannelFromName(pSampleL->pInfo->Name));
            pStereoSample->Channels         = 2;
            pStereoSample->SamplesPerSecond = pSampleL->SamplesPerSecond;
            pStereoSample->BitDepth         = pSampleL->BitDepth;
            pStereoSample->FrameSize        = pSampleL->FrameSize * 2;
            pStereoSample->MIDIUnityNote    = pSampleL->MIDIUnityNote;
            pStereoSample->Loops            = pSampleL->Loops;
            pStereoSample->LoopType         = pSampleL->LoopType;
            pStereoSample->LoopStart        = pSampleL->LoopStart;
            pStereoSample->LoopEnd          = pSampleL->LoopEnd;
            pStereoSample->LoopSize         = pSampleL->LoopSize;
            pStereoSample->LoopPlayCount    = pSampleL->LoopPlayCount;

            // libgig does not support saving of compressed samples
            pStereoSample->Compressed = false;

            // schedule new sample for resize (will be performed when gig.Save() is called)
            const long newStereoSamplesTotal = max(pSampleL->SamplesTotal, pSampleR->SamplesTotal);
            if (verbose >= 2) cout << "Resize new stereo sample '" << pStereoSample->pInfo->Name << "' to " << newStereoSamplesTotal << " sample points.\n" << flush;
            pStereoSample->Resize(newStereoSamplesTotal);

            samplePairsFiltered[it->first] = it->second;
            targetStereoSamples[it->first] = pStereoSample;
        }
        if (verbose) cout << "Done.\n" << flush;

        // increase the .gig file size appropriately
        if (verbose) cout << "Increase file size ... " << flush;
        gig.Save();
        if (verbose) cout << "Done.\n" << flush;

        vector<uint8_t> bufferMono[2];
        vector<uint8_t> stereoBuffer;
        gig::buffer_t decompressionBuffer;
        unsigned long decompressionBufferSize = 0; // in sample points, not bytes

        // do the actual sample conversion of each mono -> stereo sample by
        // reading/writing directly from/to disk
        if (verbose) cout << "Convert mono pair data to true stereo data ... " << flush;
        if (verbose >= 2) cout << endl;
        set<gig::Sample*> allAffectedMonoSamples;
        for (map<SamplePair, vector<DimRgnPair> >::iterator it = samplePairsFiltered.begin();
             it != samplePairsFiltered.end(); ++it)
        {
            gig::Sample* pSampleL = it->first.first;
            gig::Sample* pSampleR = it->first.second;
            gig::Sample* pSampleMono[2] = { pSampleL, pSampleR };
            gig::Sample* pStereoSample = targetStereoSamples[it->first];

            if ((pSampleMono[0]->SamplesTotal != pStereoSample->GetSize() && 
                 pSampleMono[1]->SamplesTotal != pStereoSample->GetSize()) || pStereoSample->GetSize() <= 0)
            {
                cerr << "Stereo length " << pStereoSample->GetSize() << ", mono L size " << pSampleL->SamplesTotal << ", mono R size " << pSampleR->SamplesTotal << "\n" << flush;
                cerr << "CRITICAL ERROR: Ill sample sizes detected! Aborting.\n";
                exit(-1);
            }

            allAffectedMonoSamples.insert(pSampleL);
            allAffectedMonoSamples.insert(pSampleR);

            const long neededStereoSize = pStereoSample->GetSize() * pStereoSample->FrameSize;
            if (stereoBuffer.size() < neededStereoSize)
                stereoBuffer.resize(neededStereoSize);

            if (bufferMono[0].size() < neededStereoSize / 2) {
                bufferMono[0].resize(neededStereoSize / 2);
                bufferMono[1].resize(neededStereoSize / 2);
            }
            memset(&bufferMono[0][0], 0, bufferMono[0].size());
            memset(&bufferMono[1][0], 0, bufferMono[1].size());

            // read mono samples from disk
            for (int iChan = 0; iChan < 2; ++iChan) {
                pSampleMono[iChan]->SetPos(0);
                unsigned long pointsRead = 0;
                if (pSampleMono[iChan]->Compressed) {
                    if (decompressionBufferSize < pSampleMono[iChan]->SamplesTotal) {
                        decompressionBufferSize = pSampleMono[iChan]->SamplesTotal;
                        gig::Sample::DestroyDecompressionBuffer(decompressionBuffer);
                        decompressionBuffer = gig::Sample::CreateDecompressionBuffer(pSampleMono[iChan]->SamplesTotal);
                    }
                    pointsRead = pSampleMono[iChan]->Read(&bufferMono[iChan][0], pSampleMono[iChan]->SamplesTotal, &decompressionBuffer);
                } else {
                    pointsRead = pSampleMono[iChan]->Read(&bufferMono[iChan][0], pSampleMono[iChan]->SamplesTotal);
                }
                if (pointsRead != pSampleMono[iChan]->SamplesTotal) {
                    cerr << "Read " << pointsRead << " sample points from mono sample " << ((iChan == 0) ? "L" : "R") << ".\n";
                    cerr << "CRITICAL ERROR: Reading entire mono sample data from disk failed! Aborting.\n";
                    exit(-1);
                }
            }

            if (pSampleL->FrameSize != pSampleR->FrameSize || pStereoSample->FrameSize / 2 != pSampleL->FrameSize || pStereoSample->FrameSize < 1) {
                cerr << "Stereo frame size " << pStereoSample->FrameSize << ", mono L frame size " << pSampleL->FrameSize << ", mono R frame size " << pSampleL->FrameSize << "\n" << flush;
                cerr << "CRITICAL ERROR: Ill frame sizes detected! Aborting.\n";
                exit(-1);
            }

            // convert from the 2 mono buffers to the stereo buffer
            for (int s = 0; s < pStereoSample->GetSize(); ++s) {
                memcpy(&stereoBuffer[s * pStereoSample->FrameSize], &bufferMono[0][s * pSampleL->FrameSize], pSampleL->FrameSize);
                memcpy(&stereoBuffer[s * pStereoSample->FrameSize + pSampleL->FrameSize], &bufferMono[1][s * pSampleR->FrameSize], pSampleR->FrameSize);
            }

            // write converted stereo sample data directly to disk
            if (verbose >= 2) cout << "Writing stereo sample '" << pStereoSample->pInfo->Name << "' (" << pStereoSample->GetSize() << " sample points).\n" << flush;
            pStereoSample->SetPos(0);
            unsigned long pointsWritten = pStereoSample->Write(&stereoBuffer[0], pStereoSample->GetSize());
            if (verbose >= 2) cout << "Written " << pointsWritten << " sample points.\n" << flush;
        }
        if (verbose) cout << "Done.\n" << flush;

        // decompression buffer not needed anymore
        gig::Sample::DestroyDecompressionBuffer(decompressionBuffer);

        // replace the old mono sample references by the new stereo sample
        // references for all instruments of the .gig file
        if (verbose) cout << "Replace old mono sample references by new stereo sample references ... " << flush;
        for (map<SamplePair, vector<DimRgnPair> >::iterator it = samplePairsFiltered.begin();
             it != samplePairsFiltered.end(); ++it)
        {
            if (forceReplace) {
                // replace EVERY occurence of the old mono sample references by
                // the new true stereo ones
                gig::Sample* pMonoSamples[2] = { it->first.first, it->first.second };
                gig::Sample* pStereoSample = targetStereoSamples[it->first];
                for (int iChan = 0; iChan < 2; ++iChan) {
                    gig::Sample* pMonoSample = pMonoSamples[iChan];
                    // iterate through all instruments and replace all references of that sample
                    for (gig::Instrument* instr = gig.GetFirstInstrument(); instr; instr = gig.GetNextInstrument()) {
                        for (gig::Region* rgn = instr->GetFirstRegion(); rgn; rgn = instr->GetNextRegion()) {
                            for (int dr = 0; dr < rgn->DimensionRegions; ++dr) {
                                if (!rgn->pDimensionRegions[dr])
                                    continue;
                                if (rgn->pDimensionRegions[dr]->pSample == pMonoSample) {
                                    rgn->pDimensionRegions[dr]->pSample = pStereoSample;
                                }
                            }
                        }
                    }
                }
            } else {
                // only replace old mono sample references of instruments if
                // they really seem to be used in a stereo context
                const vector<DimRgnPair>& dimRgnPairs = it->second;
                gig::Sample* pStereoSample = targetStereoSamples[it->first];
                for (int drp = 0; drp < dimRgnPairs.size(); ++drp) {
                    dimRgnPairs[drp].first->pSample  = pStereoSample;
                    dimRgnPairs[drp].second->pSample = pStereoSample;
                }
            }
        }
        if (verbose) cout << "Done.\n" << flush;

        if (!keep) {
            if (verbose) cout << "Removing orphaned mono samples ... " << flush;
            int iDeletedSamples = 0;
            // remove all old 2 mono sample pairs that are not referenced anymore
            for (map<SamplePair, vector<DimRgnPair> >::iterator it = samplePairsFiltered.begin();
                 it != samplePairsFiltered.end(); ++it)
            {
                gig::Sample* pMonoSamples[2] = { it->first.first, it->first.second };
                for (int iChan = 0; iChan < 2; ++iChan) {
                    gig::Sample* pMonoSample = pMonoSamples[iChan];

                    // iterate through all instruments and check if that
                    // old mono sample is still referenced somewhere
                    for (gig::Instrument* instr = gig.GetFirstInstrument(); instr; instr = gig.GetNextInstrument()) {
                        for (gig::Region* rgn = instr->GetFirstRegion(); rgn; rgn = instr->GetNextRegion()) {
                            for (int dr = 0; dr < rgn->DimensionRegions; ++dr) {
                                if (!rgn->pDimensionRegions[dr])
                                    continue;
                                if (rgn->pDimensionRegions[dr]->pSample == pMonoSample) {
                                    std::cout << "NOTE: Mono sample '" + pMonoSample->pInfo->Name + "' is still referenced by instrument '" << instr->pInfo->Name << "'. Thus not deleting sample.\n";
                                    goto nextSample; // still referenced, don't delete sample
                                }
                            }
                        }
                    }

                    // if this point is reached, then pMonoSample is not referenced
                    // by any instrument anymore, so delete this old mono sample now
                    {
                        gig::Group* pGroup = pMonoSample->GetGroup();
                        gig.DeleteSample(pMonoSample);
                        iDeletedSamples++;

                        // if deleted sample's group is now empty, delete the group
                        if (!pGroup->GetFirstSample()) gig.DeleteGroup(pGroup);
                    }

                    nextSample:;
                }
            }
            if (verbose) {
                cout << "Done (deleted " << iDeletedSamples << " of " << allAffectedMonoSamples.size() << " mono samples).\n" << flush;
                if (iDeletedSamples < allAffectedMonoSamples.size())
                    cout << "(Use --force-replace to replace all old mono references.)\n" << flush;
            }
        }

        // save sample reference changes (and drop deleted mono samples)
        if (verbose) cout << "Saving final sample references to disk and shrink file ... " << flush;
        gig.Save();
        if (verbose) cout << "Done.\n" << flush;

    } catch (RIFF::Exception e) {
        cerr << "Failed converting file:" << endl;
        e.PrintMessage();
        return false; // error
    } catch (...) {
        cerr << "Unknown exception occurred while trying to convert file." << endl;
        return false; // error
    }
    return true; // success
}

int main(int argc, char *argv[]) {
    int nOptions = 0;
    bool bOptionRecurse = false;
    bool bOptionKeep = false;
    bool bOptionForceReplace = false;
    bool bOptionMatchIncompatible = false;
    int iVerbose = 0;

    // validate & parse arguments provided to this program
    if (argc <= 1) {
        printUsage();
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; ++i) {
        const string opt = argv[i];
        if (opt == "--") { // common for all command line tools: separator between initial option arguments and i.e. subsequent file arguments
            nOptions++;
            break;
        }
        if (opt.substr(0, 1) != "-") break;
        
        if (opt == "-v") {
            printVersion();
            return EXIT_SUCCESS;
        } else if (opt == "-r") {
            nOptions++;
            bOptionRecurse = true;
        } else if (opt == "--keep") {
            nOptions++;
            bOptionKeep = true;
        } else if (opt == "--force-replace") {
            nOptions++;
            bOptionForceReplace = true;    
        } else if (opt == "--incompatible") {
            nOptions++;
            bOptionMatchIncompatible = true;
        } else if (opt == "--verbose") {
            nOptions++;
            iVerbose = 1;
            if (i+1 < argc) {
                char* s = argv[i+1];
                char* endptr = NULL;
                errno = 0;
                long val = strtol(s, &endptr, 10);
                if (errno != EINVAL && errno != ERANGE && endptr != s) {
                    if (val < 0) {
                        cerr << "Numeric argument for --verbose must not be negative!\n";
                        return -1;
                    }
                    iVerbose = val;
                    nOptions++;
                    i++;
                }
            }
            
        }
    }
    const int nFileArguments = argc - nOptions - 1;
    if (nFileArguments < 1) {
        printUsage();
        return EXIT_FAILURE;
    }

    bool bError = false;

    // assemble list of .gig file names to be converted
    for (int i = nOptions + 1; i < argc; ++i) {
        collectFiles(argv[i], bOptionRecurse, &bError);
    }
    if (g_files.size() < 1) {
        cerr << "No file given to be converted!" << endl;
        return EXIT_FAILURE;
    }
    if (bError) {
        cerr << "Aborted due to error. No file has been changed." << endl;
        return EXIT_FAILURE;
    }

    // convert each file in the assembled list of file names
    {
        int i = 0;
        for (set<string>::const_iterator it = g_files.begin(); it != g_files.end(); ++it, ++i) {
            cout << "Converting file " << (i+1) << "/" << int(g_files.size()) << ": " << *it << " ... " << flush;
            if (iVerbose) cout << endl;
            bool bSuccess = convertFileToStereo(
                *it, bOptionKeep, bOptionForceReplace, !bOptionMatchIncompatible,
                iVerbose
            );
            if (!bSuccess) return EXIT_FAILURE;
            cout << "OK" << endl;
        }
    }

    return EXIT_SUCCESS;
}
