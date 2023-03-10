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
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <set>
#include <vector>

#ifdef WIN32
# define DIR_SEPARATOR '\\'
#else
# define DIR_SEPARATOR '/'
#endif

#include "../gig.h"

using namespace std;

/**
 * How the Gigasampler samples shall be converted from stereo to mono.
 */
enum conversion_method_t {
    CONVERT_MIX_CHANNELS,  ///< Mix left & right audio channel together.
    CONVERT_LEFT_CHANNEL,  ///< Only pick left audio channel for mono output.
    CONVERT_RIGHT_CHANNEL  ///< Only pick right audio channel for mono output.
};

static set<string> g_files;

static conversion_method_t g_conversionMethod = CONVERT_MIX_CHANNELS;

static void printUsage() {
    cout << "gig2mono - converts Gigasampler files from stereo to mono." << endl;
    cout << endl;
    cout << "Usage: gig2mono [-r] [--left | --right | --mix] FILE_OR_DIR1 [ FILE_OR_DIR2 ... ]" << endl;
    cout << "       gig2mono -v" << endl;
    cout << endl;
    cout << "   -r       Recurse through subdirectories." << endl;
    cout << endl;
    cout << "   --mix    Convert by mixing left & right audio channels (default)." << endl;
    cout << endl;
    cout << "   --left   Convert by using left audio channel data." << endl;
    cout << endl;
    cout << "   --right  Convert by using right audio channel data." << endl;
    cout << endl;
    cout << "   -v       Print version and exit." << endl;
    cout << endl;
}

static string programRevision() {
    string s = "$Revision: 3048 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

static void printVersion() {
    cout << "gig2mono revision " << programRevision() << endl;
    cout << "using " << gig::libraryName() << " " << gig::libraryVersion() << endl;
}

static bool isGigFileName(const string path) {
    const string t = ".gig";
    return path.substr(path.length() - t.length()) == t;
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

static bool convertFileToMono(const string path) {
    try {
        RIFF::File riff(path);
        gig::File gig(&riff);
        
        gig::buffer_t decompressionBuffer;
        unsigned long decompressionBufferSize = 0; // in sample points, not bytes
        vector< vector<uint8_t> > audioBuffers;
        vector<uint8_t> stereoBuffer;
        
        // count the total amount of samples
        int nSamples = 0;
        for (gig::Sample* pSample = gig.GetFirstSample(); pSample; pSample = gig.GetNextSample())
            nSamples++;

        audioBuffers.resize(nSamples);

        // read all original (stereo) sample audio data, convert it to mono and
        // keep the mono audio data in tempoary buffers in RAM
        bool bModified = false;
        int i = 0;
        for (gig::Sample* pSample = gig.GetFirstSample(); pSample; pSample = gig.GetNextSample(), ++i) {
            if (pSample->Channels != 2)
                continue; // ignore samples not being stereo

            // ensure audio conversion buffer is large enough for this sample
            const long neededStereoSize = pSample->SamplesTotal * pSample->FrameSize;
            const long neededMonoSize   = neededStereoSize / 2;
            if (stereoBuffer.size() < neededStereoSize)
                stereoBuffer.resize(neededStereoSize);
            audioBuffers[i].resize(neededMonoSize);

            // read sample's current (stereo) audio data
            if (pSample->Compressed) {
                if (decompressionBufferSize < pSample->SamplesTotal) {
                    decompressionBufferSize = pSample->SamplesTotal;
                    gig::Sample::DestroyDecompressionBuffer(decompressionBuffer);
                    decompressionBuffer = gig::Sample::CreateDecompressionBuffer(pSample->SamplesTotal);
                }
                pSample->Read(&stereoBuffer[0], pSample->SamplesTotal, &decompressionBuffer);
            } else {
                pSample->Read(&stereoBuffer[0], pSample->SamplesTotal);
            }

            const int stereoFrameSize = pSample->FrameSize;
            const int monoFrameSize   = stereoFrameSize / 2;

            // This condition must be satisfied for the conversion for() loops
            // below to not end up beyond buffer boundaries. This check should
            // only fail on corrupt .gig files.
            if (stereoBuffer.size() % stereoFrameSize != 0) {
                cerr << "Internal error: Invalid stereo data length (" << int(stereoBuffer.size()) << " bytes)" << endl;
                return false; // error
            }

            // convert stereo -> mono
            switch (g_conversionMethod) {
                case CONVERT_LEFT_CHANNEL: {
                    const int n = pSample->SamplesTotal;
                    for (int m = 0; m < n; ++m) {
                        for (int k = 0; k < monoFrameSize; ++k) {
                            audioBuffers[i][m * monoFrameSize + k] = stereoBuffer[m * stereoFrameSize + k];
                        }
                    }
                    break;
                }

                case CONVERT_RIGHT_CHANNEL: {
                    const int n = pSample->SamplesTotal;
                    for (int m = 0; m < n; ++m) {
                        for (int k = 0; k < monoFrameSize; ++k) {
                            audioBuffers[i][m * monoFrameSize + k] = stereoBuffer[m * stereoFrameSize + k + monoFrameSize];
                        }
                    }
                    break;
                }

                case CONVERT_MIX_CHANNELS:
                    switch (pSample->BitDepth) {
                        case 16: {
                            int16_t* in  = (int16_t*) &stereoBuffer[0];
                            int16_t* out = (int16_t*) &audioBuffers[i][0];
                            const int n = pSample->SamplesTotal;
                            for (int m = 0; m < n; ++m) {
                                const float l = in[m*2];
                                const float r = in[m*2+1];
                                out[m] = int16_t((l + r) * 0.5f);
                            }
                            break;
                        }

                        case 24: {
                            int8_t* p = (int8_t*) &stereoBuffer[0];
                            int16_t* out = (int16_t*) &audioBuffers[i][0];
                            const int n = pSample->SamplesTotal;
                            for (int t = 0, m = 0; m < n; t += 6, ++m) {
                                #if WORDS_BIGENDIAN
                                const float l = p[t + 0] << 8 | p[t + 1] << 16 | p[t + 2] << 24;
                                const float r = p[t + 3] << 8 | p[t + 4] << 16 | p[t + 5] << 24;
                                #else
                                // 24bit read optimization: 
                                // a misaligned 32bit read and subquent 8 bit shift is faster (on x86)  than reading 3 single bytes and shifting them
                                const float l = (*((int32_t *)(&p[t])))   << 8;
                                const float r = (*((int32_t *)(&p[t+3]))) << 8;
                                #endif
                                out[m] = int16_t((l + r) * 0.5f);
                            }
                            break;
                        }

                        default:
                            cerr << "Internal error: Invalid sample bit depth (" << int(pSample->BitDepth) << " bit)" << endl;
                            return false; // error
                    }
                    break;
                    
                default:
                    cerr << "Internal error: Invalid conversion method (#" << int(g_conversionMethod) << ")" << endl;
                    return false; // error
            }

            // update sample's meta informations for mono
            pSample->Channels = 1;
            if (pSample->FrameSize) pSample->FrameSize /= 2;
            // libgig does not support saving of compressed samples
            pSample->Compressed = false;

            // schedule sample for physical resize operation
            pSample->Resize(pSample->SamplesTotal);

            bModified = true;
        }

        if (!bModified) {
            cout << "[ignored: not stereo] " << flush;
            return true; // success
        }

        // remove all stereo dimensions (if any)
        for (gig::Instrument* instr = gig.GetFirstInstrument(); instr; instr = gig.GetNextInstrument()) {
            for (gig::Region* rgn = instr->GetFirstRegion(); rgn; rgn = instr->GetNextRegion()) {
                for (int k = 0; k < 8; ++k) {
                    if (rgn->pDimensionDefinitions[k].dimension == gig::dimension_samplechannel) {
                        rgn->DeleteDimension(&rgn->pDimensionDefinitions[k]);
                        goto nextRegion;
                    }
                }
                nextRegion:;
            }
        }

        // Make sure sample chunks are resized to the required size on disk,
        // so we can directly write the audio data directly to the file on disk
        // as next step. This is required because in worst case the original
        // stereo sample audio data might be compressed and might be smaller in
        // size than the final (always uncompressed) mono audio data size.
        gig.Save();

        // now convert and write the samples' mono audio data directly to the file
        i = 0;
        for (gig::Sample* pSample = gig.GetFirstSample(); pSample; pSample = gig.GetNextSample(), ++i) {
            if (audioBuffers[i].empty())
                continue; // was not a stereo sample before, so ignore this one

            const int n = audioBuffers[i].size() / pSample->FrameSize;
            pSample->Write(&audioBuffers[i][0], n);
        }
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
        } else if (opt == "--left") {
            nOptions++;
            g_conversionMethod = CONVERT_LEFT_CHANNEL;
        } else if (opt == "--right") {
            nOptions++;
            g_conversionMethod = CONVERT_RIGHT_CHANNEL;
        } else if (opt == "--mix") {
            nOptions++;
            g_conversionMethod = CONVERT_MIX_CHANNELS;
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
            bool bSuccess = convertFileToMono(*it);
            if (!bSuccess) return EXIT_FAILURE;
            cout << "OK" << endl;
        }
    }

    return EXIT_SUCCESS;
}
