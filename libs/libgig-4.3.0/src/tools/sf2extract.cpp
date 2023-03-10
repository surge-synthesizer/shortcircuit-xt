/***************************************************************************
 *                                                                         *
 *   sf2extract tool: Copyright (C) 2015 - 2017 by Christian Schoenebeck   *
 *                                          <cuse@users.sf.net>            *
 *                                                                         *
 *   SF2 C++ Library: Copyright (C) 2009-2010 by Grigor Iliev              *
 *                                               <grigor@grigoriliev.com>  *
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
#include <string.h>
#include <string>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "../SF.h"

#ifdef _MSC_VER
#define S_ISDIR(x) (S_IFDIR & (x))
#define S_IWUSR S_IWRITE
#define S_IXUSR S_IEXEC
#endif

#if POSIX
# include <dlfcn.h>
#endif

// only libsndfile is available for Windows, so we use that for writing the sound files
#ifdef WIN32
# define HAVE_SNDFILE 1
#endif // WIN32

// abort compilation here if neither libsndfile nor libaudiofile are available
#if !HAVE_SNDFILE && !HAVE_AUDIOFILE
# error "Neither libsndfile nor libaudiofile seem to be available!"
# error "(HAVE_SNDFILE and HAVE_AUDIOFILE are both false)"
#endif

// we prefer libsndfile before libaudiofile
#if HAVE_SNDFILE
# include <sndfile.h>
#else
# include <audiofile.h>
#endif // HAVE_SNDFILE

using namespace std;

typedef map<unsigned int, bool> OrderMap;

void ExtractSamples(sf2::File* gig, char* destdir, OrderMap& selection);
int writeWav(sf2::Sample* sample, const char* filename, void* samples, long totalFrameCount, int bitdepth);

string ToString(int i) {
    const int SZ = 64;
    static char strbuf[SZ];
    snprintf(strbuf, SZ, "%d", i);
    string s = strbuf;
    return s;
}

string Revision() {
    string s = "$Revision: 3175 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

void PrintVersion() {
    cout << "sf2extract revision " << Revision() << endl;
    cout << "using " << sf2::libraryName() << " " << sf2::libraryVersion();
    #if HAVE_SNDFILE
    char versionBuffer[128];
    sf_command(NULL, SFC_GET_LIB_VERSION, versionBuffer, 128);
    cout << ", " << versionBuffer;
    #else // use libaudiofile
    cout << "\nbuilt against libaudiofile "
         << LIBAUDIOFILE_MAJOR_VERSION << "." << LIBAUDIOFILE_MINOR_VERSION;
    # ifdef LIBAUDIOFILE_MICRO_VERSION
    cout << "." << LIBAUDIOFILE_MICRO_VERSION;
    # endif // LIBAUDIOFILE_MICRO_VERSION
    #endif // HAVE_SNDFILE
    cout << endl;
}

static void PrintUsage() {
    cout << "sf2extract - extracts samples from a SoundFont version 2 file." << endl;
    cout << endl;
    cout << "Usage: sf2extract [-v] SF2FILE DESTDIR [SAMPLENR] [ [SAMPLENR] ...]" << endl;
    cout << endl;
    cout << "   SF2FILE  Input SoundFont (.sf2) file." << endl;
    cout << endl;
    cout << "   DESTDIR  Destination directory where all .wav files shall be written to." << endl;
    cout << endl;
    cout << "   SAMPLENR Index (/indices) of Sample(s) which should be extracted." << endl;
    cout << "            If no sample indices are given, all samples will be extracted" << endl;
    cout << "            (use sf2dump to look for available samples)." << endl;
    cout << endl;
    cout << "   -v       Print version and exit." << endl;
    cout << endl;
}

#if !HAVE_SNDFILE // use libaudiofile
void* hAFlib; // handle to libaudiofile
void openAFlib(void);
void closeAFlib(void);
// pointers to libaudiofile functions
AFfilesetup(*_afNewFileSetup)(void);
void(*_afFreeFileSetup)(AFfilesetup);
void(*_afInitChannels)(AFfilesetup,int,int);
void(*_afInitSampleFormat)(AFfilesetup,int,int,int);
void(*_afInitFileFormat)(AFfilesetup,int);
void(*_afInitRate)(AFfilesetup,int,double);
int(*_afWriteFrames)(AFfilehandle,int,const void*,int);
AFfilehandle(*_afOpenFile)(const char*,const char*,AFfilesetup);
int(*_afCloseFile)(AFfilehandle file);
#endif // !HAVE_SNDFILE

int main(int argc, char *argv[]) {
     OrderMap selectedSamples;
     if (argc >= 2) {
        if (argv[1][0] == '-') {
            switch (argv[1][1]) {
                case 'v':
                    PrintVersion();
                    return EXIT_SUCCESS;
            }
        }
    }
    if (argc < 3) {
        PrintUsage();
        return EXIT_FAILURE;
    }
    FILE* hFile = fopen(argv[1], "r");
    if (!hFile) {
        cout << "Invalid input file argument!" << endl;
        return EXIT_FAILURE;
    }
    fclose(hFile);
    struct stat buf;
    if (stat(argv[2], &buf) == -1) {
        cout << "Unable to open DESTDIR: ";
        switch (errno) {
            case EACCES:  cout << "Permission denied." << endl;
                          break;
            case ENOENT:  cout << "Directory does not exist, or name is an empty string." << endl;
                          break;
            case ENOMEM:  cout << "Insufficient memory to complete the operation." << endl;
                          break;
            case ENOTDIR: cout << "Is not a directory." << endl;
                          break;
            default:      cout << "Unknown error" << endl;
        }
        return EXIT_FAILURE;
    } else if (!S_ISDIR(buf.st_mode)) {
        cout << "Unable to open DESTDIR: Is not a directory." << endl;
        return EXIT_FAILURE;
    } else if (!(S_IWUSR & buf.st_mode) || !(S_IXUSR & buf.st_mode)) {
        cout << "Unable to open DESTDIR: Permission denied." << endl;
        return EXIT_FAILURE;
    }
    try {
        RIFF::File* riff = new RIFF::File(argv[1]);
        sf2::File*  sf   = new sf2::File(riff);

        // assemble list of selected samples (by sample index)
        if (argc > 3) { // extracting specific samples ...
            for (int i = 3; i < argc; i++) {
                unsigned int index = atoi(argv[i]);
                selectedSamples[index-1] = true;
            }
        } else { // extract all samples ...
            for (int i = 0; i < sf->GetSampleCount(); ++i)
                selectedSamples[i] = true;
        }

        cout << "Extracting samples from \"" << argv[1] << "\" to directory \"" << argv[2] << "\"." << endl << flush;
        ExtractSamples(sf, argv[2], selectedSamples);
        cout << "Extraction finished." << endl << flush;
        delete sf;
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

/// Removes spaces from the beginning and end of @a s.
inline void stripWhiteSpace(string& s) {
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
}

static bool endsWith(const string& haystack, const string& needle) {
    if (haystack.size() < needle.size()) return false;
    return haystack.substr(haystack.size() - needle.size(), needle.size()) == needle;
}

inline bool hasLeftOrRightMarker(string s) {
    stripWhiteSpace(s);
    return endsWith(s, "-L") || endsWith(s, "-R") ||
           endsWith(s, "_L") || endsWith(s, "_R") ||
           endsWith(s, " L") || endsWith(s, " R");
}

inline void stripLeftOrRightMarkerAtEnd(string& s) {
    if (hasLeftOrRightMarker(s)) {
        s = s.substr(0, s.size() - 2);
        stripWhiteSpace(s);
    }
}

void ExtractSamples(sf2::File* sf, char* destdir, OrderMap& selection) {
    if (sf->GetSampleCount() <= 0) {
        cerr << "No samples found in file.\n";
        return;
    }

#if !HAVE_SNDFILE // use libaudiofile
    hAFlib = NULL;
    openAFlib();
#endif // !HAVE_SNDFILE
    uint8_t* pWave  = NULL;
    int* pIntWave = NULL;
    long BufferSize = 0;
    sf2::Sample::buffer_t tmpBuf;

    for (int iSample = 0; iSample < sf->GetSampleCount(); ++iSample) {
        sf2::Sample* pSample = sf->GetSample(iSample);
        if (selection[iSample] == false)
            continue;
        sf2::Sample* pSample2 =
            (pSample->GetChannelCount() > 1) ?
                sf->GetSample(pSample->SampleLink) : NULL;

        string name = pSample->Name;
        if (pSample2)
            stripLeftOrRightMarkerAtEnd(name);

        long totalFrameCount = pSample->GetTotalFrameCount();
        if (pSample2 && pSample2->GetTotalFrameCount() > totalFrameCount)
            totalFrameCount = pSample2->GetTotalFrameCount();

        string filename = destdir;
        if (filename[filename.size() - 1] != '/') filename += "/";
        filename += ToString(iSample+1);
        filename += "_";
        if (name == "") {
            name = "(NO NAME)";
            filename += "NONAME";
        } else {
            filename += name;
            name.insert(0, "\"");
            name += "\"";
        }
        filename += ".wav";
        const int bitdepth =
            pSample->GetFrameSize() / pSample->GetChannelCount() * 8;
        cout << "Extracting Sample " << (iSample+1) << ") " << name << " ("
             << bitdepth << "Bits, " << pSample->SampleRate << "Hz, " << pSample->GetChannelCount() << " Channels, " << totalFrameCount << " Samples";
        if (pSample->HasLoops()) {
            cout << ", LoopStart " << pSample->StartLoop
                 << ", LoopEnd "   << pSample->EndLoop;
        }
        cout << ")..." << flush;

        // some sanity checks whether the stereo sample pair matches
        if (pSample2) {
            if (pSample->GetFrameSize()    != pSample2->GetFrameSize() ||
                pSample->GetChannelCount() != pSample2->GetChannelCount())
            {
                cerr << "Error: stereo sample pair does not match [ignoring]\n";
                continue;
            }
        }

        const long neededsize = (bitdepth == 24) ?
            totalFrameCount * pSample->GetChannelCount() * sizeof(int) :
            totalFrameCount * pSample->GetFrameSize();
        if (BufferSize < neededsize) {
            if (pWave) delete[] pWave;
            pWave = new uint8_t[neededsize];
            BufferSize = neededsize;
        }
        pIntWave = (int*)pWave;
        memset(pWave, 0, neededsize);

        pSample->SetPos(0);
        unsigned long nRead = pSample->Read(pWave, pSample->GetTotalFrameCount());
        if (nRead <= 0)
            throw RIFF::Exception("Could not load sample data.");

        if (pSample2) {
            if (tmpBuf.Size < neededsize) {
                if (tmpBuf.pStart) delete[] (uint8_t*)tmpBuf.pStart;
                tmpBuf.pStart = new uint8_t[neededsize];
                tmpBuf.Size   = neededsize;
            }
            pSample2->SetPos(0);
            unsigned long nRead = pSample2->ReadNoClear(pWave, pSample2->GetTotalFrameCount(), tmpBuf);
            if (nRead <= 0)
                throw RIFF::Exception("Could not load sample data.");
        }

        // Both libsndfile and libaudiofile uses int for 24 bit
        // samples. libgig however returns 3 bytes per sample, so
        // we have to convert the wave data before writing.
        if (bitdepth == 24) {
            int n = totalFrameCount * pSample->GetChannelCount();
            for (int i = n - 1 ; i >= 0 ; i--) {
#if HAVE_SNDFILE
                pIntWave[i] = pWave[i * 3] << 8 | pWave[i * 3 + 1] << 16 | pWave[i * 3 + 2] << 24;
#else
                pIntWave[i] = pWave[i * 3] | pWave[i * 3 + 1] << 8 | pWave[i * 3 + 2] << 16;
#endif
            }
        }

        int res = writeWav(pSample, filename.c_str(), pWave, totalFrameCount, bitdepth);
        if (res < 0) cout << "Couldn't write sample data." << endl;
        else cout << "ok" << endl;

        // make sure we don't extract this stereo sample pair a 2nd time
        if (pSample2)
            selection[pSample->SampleLink] = false;
    }
    if (pWave) delete[] pWave;
    if (tmpBuf.pStart) delete[] (uint8_t*)tmpBuf.pStart;
#if !HAVE_SNDFILE // use libaudiofile
    closeAFlib();
#endif // !HAVE_SNDFILE
}

int writeWav(sf2::Sample* sample, const char* filename, void* samples, long totalFrameCount, int bitdepth) {
#if HAVE_SNDFILE
    SNDFILE* hfile;
    SF_INFO  sfinfo;
    SF_INSTRUMENT instr;
    int format = SF_FORMAT_WAV;
    switch (bitdepth) {
        case 8:
            format |= SF_FORMAT_PCM_S8;
            break;
        case 16:
            format |= SF_FORMAT_PCM_16;
            break;
        case 24:
            format |= SF_FORMAT_PCM_24;
            break;
        case 32:
            format |= SF_FORMAT_PCM_32;
            break;
        default:
            cerr << "Error: Bithdepth " << ToString(bitdepth) << " not supported by libsndfile, ignoring sample!\n" << flush;
            return -1;
    }
    memset(&sfinfo, 0, sizeof (sfinfo));
    memset(&instr, 0, sizeof (instr));
    sfinfo.samplerate = sample->SampleRate;
    sfinfo.frames     = totalFrameCount;
    sfinfo.channels   = sample->GetChannelCount();
    sfinfo.format     = format;
    if (!(hfile = sf_open(filename, SFM_WRITE, &sfinfo))) {
        cerr << "Error: Unable to open output file \'" << filename << "\'.\n" << flush;
        return -1;
    }
    instr.basenote = sample->OriginalPitch;
    instr.detune = sample->PitchCorrection;
    if (sample->HasLoops()) {
        instr.loop_count = 1;
        instr.loops[0].mode  = SF_LOOP_FORWARD;
        instr.loops[0].start = sample->StartLoop;
        instr.loops[0].end   = sample->EndLoop;
        instr.loops[0].count = 1;
    }
    sf_command(hfile, SFC_SET_INSTRUMENT, &instr, sizeof(instr));
    sf_count_t res = (bitdepth == 24) ?
        sf_write_int(hfile, static_cast<int*>(samples), sample->GetChannelCount() * totalFrameCount) :
        sf_write_short(hfile, static_cast<short*>(samples), sample->GetChannelCount() * totalFrameCount);
    if (res != sample->GetChannelCount() * totalFrameCount) {
        cerr << sf_strerror(hfile) << endl << flush;
        sf_close(hfile);
        return -1;
    }
    sf_close(hfile);
#else // use libaudiofile
    AFfilesetup setup = _afNewFileSetup();
    _afInitFileFormat(setup, AF_FILE_WAVE);
    _afInitChannels(setup, AF_DEFAULT_TRACK, sample->GetChannelCount());
    _afInitSampleFormat(setup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, bitdepth);
    _afInitRate(setup, AF_DEFAULT_TRACK, sample->SampleRate);
    if (setup == AF_NULL_FILESETUP) return -1;
    AFfilehandle hFile = _afOpenFile(filename, "w", setup);
    if (hFile == AF_NULL_FILEHANDLE) return -1;
    if (_afWriteFrames(hFile, AF_DEFAULT_TRACK, samples, totalFrameCount) < 0) return -1;
    _afCloseFile(hFile);
    _afFreeFileSetup(setup);
#endif // HAVE_SNDFILE

    return 0; // success
}

#if !HAVE_SNDFILE // use libaudiofile
void openAFlib() {
    hAFlib = dlopen("libaudiofile.so", RTLD_NOW);
    if (!hAFlib) {
        cout << "Unable to load library libaudiofile.so: " << dlerror() << endl;
        return;
    }
    _afNewFileSetup     = (AFfilesetup(*)(void)) dlsym(hAFlib, "afNewFileSetup");
    _afFreeFileSetup    = (void(*)(AFfilesetup)) dlsym(hAFlib, "afFreeFileSetup");
    _afInitChannels     = (void(*)(AFfilesetup,int,int)) dlsym(hAFlib, "afInitChannels");
    _afInitSampleFormat = (void(*)(AFfilesetup,int,int,int)) dlsym(hAFlib, "afInitSampleFormat");
    _afInitFileFormat   = (void(*)(AFfilesetup,int)) dlsym(hAFlib, "afInitFileFormat");
    _afInitRate         = (void(*)(AFfilesetup,int,double)) dlsym(hAFlib, "afInitRate");
    _afWriteFrames      = (int(*)(AFfilehandle,int,const void*,int)) dlsym(hAFlib, "afWriteFrames");
    _afOpenFile         = (AFfilehandle(*)(const char*,const char*,AFfilesetup)) dlsym(hAFlib, "afOpenFile");
    _afCloseFile        = (int(*)(AFfilehandle file)) dlsym(hAFlib, "afCloseFile");
    if (dlerror()) cout << "Failed to load function from libaudiofile.so: " << dlerror() << endl;
}

void closeAFlib() {
    if (hAFlib) dlclose(hAFlib);
}
#endif // !HAVE_SNDFILE


