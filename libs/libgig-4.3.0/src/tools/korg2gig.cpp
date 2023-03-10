/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2014-2017 Christian Schoenebeck                         *
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

#if !defined(WIN32)
# include <unistd.h>
#endif

#include "../Korg.h"
#include "../gig.h"

using namespace std;

static string Revision() {
    string s = "$Revision: 3175 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

static void printVersion() {
    cout << "korg2gig revision " << Revision() << endl;
    cout << "using " << gig::libraryName() << " " << gig::libraryVersion() << endl;
}

static void printUsage() {
    cout << "korg2gig - convert sound files from KORG to GigaStudio format." << endl;
    cout << endl;
    cout << "Usage: korg2gig [-v] [-f] [--interpret-names] FILE1 [ FILE2 ... ] NEWFILE" << endl;
    cout << endl;
    cout << "  -v  Print version and exit." << endl;
    cout << endl;
    cout << "  -f  Overwrite output file if it already exists." << endl;
    cout << endl;
    cout << "  --interpret-names" << endl;
    cout << "      Try to guess left/right sample pairs and velocity splits." << endl;
    cout << endl;
}

static bool beginsWith(const string& haystack, const string& needle) {
    return haystack.substr(0, needle.size()) == needle;
}

static bool endsWith(const string& haystack, const string& needle) {
    if (haystack.size() < needle.size()) return false;
    return haystack.substr(haystack.size() - needle.size(), needle.size()) == needle;
}

static bool fileExists(const string& filename) {
    FILE* hFile = fopen(filename.c_str(), "r");
    if (!hFile) return false;
    fclose(hFile);
    return true;
}

// this could also be replaced by fopen(name, "w") to simply truncate the file to zero
static void deleteFile(const string& filename) {
    #if defined(WIN32)
    DeleteFile(filename.c_str());
    #else
    unlink(filename.c_str());
    #endif
}

/**
 * Intermediate container class which helps to group .KMP ("multi sample")
 * instruments  together, so that only one .gig instrument will be created per
 * respective identified .KMP instrument group.
 */
class InstrGroup : public vector<Korg::KMPInstrument*> {
public:
    bool isStereo; ///< True if a stereo pair of sampls was found, will result in a "stereo dimension" being created in the .gig instrument.
    bool hasVelocitySplits; ///< True if velocity split informations were identified in the .KMP ("multi sample") instrument name, a velocity dimension will be created accordingly.
    string baseName; ///< This is the .KMP ("multi sample") instrument name with the numeric velecity range hints and stereo pair hints being stripped of, resulting in a shorter left hand string that all KMPinstruments have in common of this group.

    typedef vector<Korg::KMPInstrument*> base_t;

    InstrGroup () :
        vector<Korg::KMPInstrument*>(),
        isStereo(false), hasVelocitySplits(false)
    {
    }

    InstrGroup (const InstrGroup& x) :
        vector<Korg::KMPInstrument*>(x)
    {
        isStereo          = x.isStereo;
        hasVelocitySplits = x.hasVelocitySplits;
        baseName          = x.baseName;
    }

    InstrGroup& operator= (const InstrGroup& x) {
        base_t::operator=(x);
        isStereo          = x.isStereo;
        hasVelocitySplits = x.hasVelocitySplits;
        baseName          = x.baseName;
        return *this;
    }
};

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

inline bool isDigit(const char& c) {
    return c >= '0' && c <= '9';
}

inline int digitToInt(const char& c) {
    return c - '0';
}

/**
 * I.e. for @a s = "FOO 003-127" it sets @a from = 3, @a to = 127 and
 * returns "FOO ".
 *
 * @param s - input string
 * @param from - (output) numeric left part of range
 * @param to - (output) numeric right part of range
 * @returns input string without the numeric range string component
 */
inline string parseNumberRangeAtEnd(const string& s, int& from, int& to) {
    enum _State {
        EXPECT_NUMBER1,
        EXPECT_NUMBER1_OR_MINUS,
        EXPECT_NUMBER2,
        EXPECT_NUMBER2_OR_END
    };
    int state = EXPECT_NUMBER1;
    from = to = -1;
    int dec = 10;
    for (int i = s.size() - 1; i >= 0; --i) {
        switch (state) {
            case EXPECT_NUMBER1:
                if (isDigit(s[i])) {
                    to = digitToInt(s[i]);
                    state++;
                } else return s; // unexpected, error
                break;
            case EXPECT_NUMBER1_OR_MINUS:
                if (s[i] == '-') {
                    state++;
                } else if (isDigit(s[i])) {
                    to += digitToInt(s[i]) * dec;
                    dec *= 10;
                } else return s; // unexpected, error
                break;
            case EXPECT_NUMBER2:
                if (isDigit(s[i])) {
                    from = digitToInt(s[i]);
                    dec = 10;
                    state++;
                } else return s; // unexpected, error
                break;
            case EXPECT_NUMBER2_OR_END:
                if (isDigit(s[i])) {
                    from += digitToInt(s[i]) * dec;
                    dec *= 10;
                } else return s.substr(0, i+1);
                break;
        }
    }
    return (state == EXPECT_NUMBER2_OR_END) ? "" : s;
}

inline void stripLeftOrRightMarkerAtEnd(string& s);

/**
 * I.e. for @a s = "FOO 003-127 -R" it returns "FOO" and sets @a from = 3 and
 * @a to = 127. In case no number range was found (that is on parse errors),
 * the returned string will equal the input string @a s instead.
 */
inline string parseNumberRange(const string& s, int& from, int& to) {
    string w = s;
    stripWhiteSpace(w);
    stripLeftOrRightMarkerAtEnd(w);
    string result = parseNumberRangeAtEnd(w, from, to);
    if (result == w) return s; // parse error occurred, return original input s
    stripWhiteSpace(result);
    return result;
}

/// I.e. for @a s = "FOO 003-127" it returns "FOO".
inline string stripNumberRangeAtEnd(string s) {
    stripWhiteSpace(s);
    int from, to;
    s = parseNumberRangeAtEnd(s, from, to);
    stripWhiteSpace(s);
    return s;
}

inline bool hasLeftOrRightMarker(string s) {
    stripWhiteSpace(s);
    return endsWith(s, "-L") || endsWith(s, "-R");
}

inline bool isLeftMarker(string s) {
    stripWhiteSpace(s);
    return endsWith(s, "-L");
}

inline void stripLeftOrRightMarkerAtEnd(string& s) {
    if (hasLeftOrRightMarker(s)) {
        s = s.substr(0, s.size() - 2);
        stripWhiteSpace(s);
    }
}

/// I.e. for @a name = "FOO 003-127 -R" it returns "FOO".
static string interpretBaseName(string name) {
    // if there is "-L" or "-R" at the end, then remove it
    stripLeftOrRightMarkerAtEnd(name);
    // if there is a velocity split range indicator like "008-110", then remove it
    string s = stripNumberRangeAtEnd(name);
    return (s.empty()) ? name : s;
}

static bool hasNumberRangeIndicator(string name) {
    int from, to;
    string baseName = parseNumberRange(name, from, to);
    return baseName != name;
}

inline InstrGroup* findGroupWithBaseName(vector<InstrGroup>& groups, const string& baseName) {
    if (baseName.empty()) return NULL;
    for (int i = 0; i < groups.size(); ++i)
        if (groups[i].baseName == baseName)
            return &groups[i];
    return NULL;
}

/**
 * If requested, try to group individual input instruments by looking at their
 * instrument names (guessing left/right samples and velocity splits by name).
 */
static vector<InstrGroup> groupInstruments(
    const vector<Korg::KMPInstrument*>& instruments, bool bInterpretNames)
{
    vector<InstrGroup> result;
    if (!bInterpretNames) {
        // no grouping requested, so make each input instrument its own group
        for (vector<Korg::KMPInstrument*>::const_iterator it = instruments.begin();
             it != instruments.end(); ++it)
        {
            InstrGroup g;
            g.baseName = (*it)->Name();
            g.push_back(*it);
            result.push_back(g);
        }
    } else {
        // try grouping the input instruments by instrument name interpretation
        for (vector<Korg::KMPInstrument*>::const_iterator it = instruments.begin();
             it != instruments.end(); ++it)
        {
            const string baseName = interpretBaseName((*it)->Name());
            InstrGroup* g = findGroupWithBaseName(result, baseName);
            if (!g) {
                result.push_back(InstrGroup());
                g = &result.back();
                g->baseName = baseName;
            }
            g->push_back(*it);
            g->isStereo |= hasLeftOrRightMarker( (*it)->Name() );
            g->hasVelocitySplits |= hasNumberRangeIndicator( (*it)->Name() );
        }
    }
    return result;
}

static set<DLS::range_t> collectVelocitySplits(const InstrGroup& group, DLS::range_t keyRange) {
    set<DLS::range_t> velocityRanges;
    for (int i = 0; i < group.size(); ++i) {
        Korg::KMPInstrument* instr = group[i];
        uint16_t iLowerKey = 0;
        for (int k = 0; k < instr->GetRegionCount(); ++k) {
            Korg::KMPRegion* rgn = instr->GetRegion(k);
            DLS::range_t keyRange2 = { iLowerKey, rgn->TopKey };
            iLowerKey = rgn->TopKey + 1; // for next loop cycle
            if (keyRange == keyRange2) {
                int from, to;
                string baseName = parseNumberRange(instr->Name(), from, to);
                if (baseName != instr->Name()) { // number range like "003-120" found in instrument name ...
                    DLS::range_t velRange = { uint16_t(from), uint16_t(to) };
                    velocityRanges.insert(velRange);
                }
            }
        }
    }
    return velocityRanges;
}

/**
 * Ensure that the given list of ranges covers the full range between 0 .. 127.
 *
 * @param orig - (input) original set of ranges (read only)
 * @param corrected - (output) corrected set of ranges (will be cleared and refilled)
 * @return map that pairs respective original range with respective corrected range
 */
static map<DLS::range_t,DLS::range_t> killGapsInRanges(const set<DLS::range_t>& orig, set<DLS::range_t>& corrected) {
    map<DLS::range_t,DLS::range_t> result;
    corrected.clear();
    if (orig.empty()) return result;

    int iLow = 0;
    int i = 0;
    for (set<DLS::range_t>::const_iterator it = orig.begin(); it != orig.end(); ++it, ++i) {
        DLS::range_t r = *it;
        r.low = iLow;
        iLow = r.high + 1;
        if (i == orig.size() - 1) r.high = 127;
        corrected.insert(r);
        result[*it] = r;
    }
    return result;
}

static void printRanges(const set<DLS::range_t>& ranges) {
    cout << "{ ";
    for (set<DLS::range_t>::const_iterator it = ranges.begin(); it != ranges.end(); ++it) {
        if (it != ranges.begin()) cout << ", ";
        cout << (int)it->low << ".." << (int)it->high;
    }
    cout << " }" << flush;
}

static vector<Korg::KSFSample*>     ksfSamples;      // input .KSF files
static vector<Korg::KMPInstrument*> kmpInstruments;  // input .KMP files
static gig::File* g_gig = NULL;                      // output .gig file

static Korg::KSFSample* findKSFSampleWithFileName(const string& name) {
    for (int i = 0; i < ksfSamples.size(); ++i)
        if (ksfSamples[i]->FileName() == name)
            return ksfSamples[i];
    return NULL;
}

static map<Korg::KSFSample*,gig::Sample*> sampleRelations;

/**
 * First pass: if the respective gig sample does not exist yet, it will be
 * created, meta informations and size will be copied from the original Korg
 * sample. However the actual sample data will not yet be copied here yet.
 * Because the .gig file needs to be saved and resized in file size accordingly
 * to the total size of all samples. For efficiency reasons the resize opertion
 * is first just here schedued for all samples, then in a second pass the actual
 * data written in writeAllSampleData();
 */
static gig::Sample* findOrcreateGigSampleForKSFSample(Korg::KSFSample* ksfSample, gig::Group* gigSampleGroup, const Korg::KMPRegion* kmpRegion = NULL) {
    if (ksfSample->SamplePoints <= 0) {
        cout << "Skipping KSF sample '" << ksfSample->FileName() << "' (because of zero length)." << endl;
        return NULL; // writing a gig sample with zero size to a gig file would throw an exception
    }

    gig::Sample* gigSample = sampleRelations[ksfSample];
    if (gigSample) return gigSample;

    // no such gig file yet, create it ...

    // add new gig sample to gig output file
    gigSample = g_gig->AddSample();
    gigSampleGroup->AddSample(gigSample);
    sampleRelations[ksfSample] = gigSample;
    // convert Korg Sample -> gig Sample
    gigSample->pInfo->Name      = ksfSample->Name;
    gigSample->Channels         = ksfSample->Channels;
    gigSample->SamplesPerSecond = ksfSample->SampleRate;
    gigSample->BitDepth         = ksfSample->BitDepth;
    gigSample->FrameSize        = ksfSample->Channels * ksfSample->BitDepth / 8;
    //gigSample->SamplesTotal     = ksfSample->SamplePoints;
    if (kmpRegion) {
        gigSample->MIDIUnityNote = kmpRegion->OriginalKey;
    }
    if (ksfSample->LoopEnd) {
        gigSample->Loops         = 1; // the gig format currently supports only one loop per sample
        gigSample->LoopType      = gig::loop_type_normal;
        gigSample->LoopStart     = ksfSample->LoopStart;
        gigSample->LoopEnd       = ksfSample->LoopEnd;
        gigSample->LoopSize      = gigSample->LoopEnd - gigSample->LoopStart;
        gigSample->LoopPlayCount = 0; // infinite
    }
    // schedule for resize (will be performed when gig->Save() is called)
    gigSample->Resize(ksfSample->SamplePoints);

    return gigSample;
}

/**
 * Second pass: see comment of findOrcreateGigSampleForKSFSample().
 */
static void writeAllSampleData() {
    for (map<Korg::KSFSample*,gig::Sample*>::iterator it = sampleRelations.begin();
         it != sampleRelations.end(); ++it)
    {
        Korg::KSFSample* ksfSample = it->first;
        gig::Sample* gigSample     = it->second;
        if (!gigSample) continue; // can be NULL if the source KORG file had zero length (attempting to write a .gig file with zero size sample would throw an exception)
        Korg::buffer_t src = ksfSample->LoadSampleData();
        gigSample->SetPos(0);
        gigSample->Write(src.pStart, ksfSample->SamplePoints);
        ksfSample->ReleaseSampleData();
    }
}

static gig::Sample* findOrCreateGigSampleForKSFRegion(const Korg::KMPRegion* kmpRegion) {
    int from, to;
    const string baseName = parseNumberRange(kmpRegion->GetInstrument()->Name(), from, to);

    gig::Group* gigSampleGroup = g_gig->GetGroup(baseName);
    if (!gigSampleGroup) {
        gigSampleGroup = g_gig->AddGroup();
        gigSampleGroup->Name = baseName;
    }

    if (kmpRegion->SampleFileName == "SKIPPEDSAMPL" ||
        (beginsWith(kmpRegion->SampleFileName, "INTERNAL") && !endsWith(kmpRegion->SampleFileName, ".KSF")))
    {
        return NULL;
    }

    Korg::KSFSample* ksfSample = findKSFSampleWithFileName(kmpRegion->FullSampleFileName());
    if (!ksfSample)
        throw Korg::Exception("Internal error: Could not resolve KSFSample object");
    gig::Sample* gigSample = sampleRelations[ksfSample];
    if (gigSample) return gigSample;
    return findOrcreateGigSampleForKSFSample(
        ksfSample, gigSampleGroup, kmpRegion
    );
}

static void loadKorgFile(const string& filename, bool bReferenced = false) {
    try {
        if (endsWith(filename, ".KMP")) {
            cout << "Loading KORG Multi Sample file '" << filename << "' ... " << flush;
            Korg::KMPInstrument* instr = new Korg::KMPInstrument(filename);
            cout << "OK\n";
            kmpInstruments.push_back(instr);

            for (int i = 0; i < instr->GetRegionCount(); ++i) {
                Korg::KMPRegion* rgn = instr->GetRegion(i);
                if (rgn->SampleFileName == "SKIPPEDSAMPL") {
                    cout << "WARNING: 'SKIPPEDSAMPL' as sample reference found!\n";
                    continue;
                } else if (beginsWith(rgn->SampleFileName, "INTERNAL") &&
                           !endsWith(rgn->SampleFileName, ".KSF")) {
                    cout << "WARNING: One of the KORG instrument's internal samples was referenced as sample!\n";
                    continue;
                }
                // check if the sample referenced by this region was already
                // loaded, if not then load it ...
                if (!findKSFSampleWithFileName(rgn->FullSampleFileName()))
                    loadKorgFile(rgn->FullSampleFileName(), true);
            }
        } else if (endsWith(filename, ".KSF")) {
            cout << "Loading " << (bReferenced ? "referenced " : "") << "KORG Sample file '" << filename << "' ... " << flush;
            Korg::KSFSample* smpl = new Korg::KSFSample(filename);
            cout << "OK\n";
            ksfSamples.push_back(smpl);
        } else if (endsWith(filename, ".PCG")) {
            cerr << "Error with input file '" << filename << "':" << endl;
            cerr << "There is no support for .PCG files in this version of korg2gig yet." << endl;
            exit(EXIT_FAILURE);
        } else {
            cerr << "Unknown file type (file name postfix) for input file '" << filename << "'" << endl;
            exit(EXIT_FAILURE);
        }
    } catch (RIFF::Exception e) {
        cerr << "Failed opening input file '" << filename << "':" << endl;
        e.PrintMessage();
        exit(EXIT_FAILURE);
    } catch (...) {
        cerr << "Failed opening input file '" << filename << "':" << endl;
        cerr << "Unknown exception occurred while trying to access input file." << endl;
        exit(EXIT_FAILURE);
    }
}

static void cleanup() {
    if (g_gig) {
        delete g_gig;
        g_gig = NULL;
    }
    for (int i = 0; i < ksfSamples.size(); ++i) delete ksfSamples[i];
    ksfSamples.clear();
    for (int i = 0; i < kmpInstruments.size(); ++i) delete kmpInstruments[i];
    kmpInstruments.clear();
}

inline int getDimensionIndex(gig::Region* region, gig::dimension_t type) {
    for (int d = 0; d < region->Dimensions; ++d)
        if (region->pDimensionDefinitions[d].dimension == type)
            return d;
    return -1;
}

int main(int argc, char *argv[]) {
    bool bInterpretNames = false;
    bool bForce = false;

    // validate & parse arguments provided to this program
    int iArg;
    for (iArg = 1; iArg < argc; ++iArg) {
        const string opt = argv[iArg];
        if (opt == "--") { // common for all command line tools: separator between initial option arguments and i.e. subsequent file arguments
            iArg++;
            break;
        }
        if (opt.substr(0, 1) != "-") break;

        if (opt == "-v") {
            printVersion();
            return EXIT_SUCCESS;
        } else if (opt == "-f") {
            bForce = true;
        } else if (opt == "--interpret-names") {
            bInterpretNames = true;
        } else {
            cerr << "Unknown option '" << opt << "'" << endl;
            cerr << endl;
            printUsage();
            return EXIT_FAILURE;
        }
    }
    if (argc < 3) {
        printUsage();
        return EXIT_FAILURE;
    }

    set<string> inFileNames;
    string outFileName;

    // all options have been processed, all subsequent args should be files
    for (; iArg < argc; ++iArg) {
        if (iArg == argc - 1) {
            outFileName = argv[iArg];
        } else {
            inFileNames.insert(argv[iArg]);
        }
    }
    if (inFileNames.empty() || outFileName.empty()) {
        cerr << "You must provide at least one input file (Korg format) and one output file (.gig format)!" << endl;
        return EXIT_FAILURE;
    }

    // check if output file already exists
    if (fileExists(outFileName)) {
        if (bForce) deleteFile(outFileName);
        else {
            cerr << "Output file '" << outFileName << "' already exists. Use -f to overwrite it." << endl;
            return EXIT_FAILURE;
        }
    }

    // open all input (KORG) files
    for (set<string>::const_iterator it = inFileNames.begin();
         it != inFileNames.end(); ++it)
    {
        loadKorgFile(*it);
    }

    // create and assemble a new .gig file as output
    try {
        // start with an empty .gig file
        g_gig = new gig::File();

        // if requested, try to group individual input instruments by looking
        // at their names, and create only one output instrument per group
        // (guessing left/right samples and velocity splits by name)
        vector<InstrGroup> instrGroups = groupInstruments(kmpInstruments, bInterpretNames);

        // create the individual instruments in the gig file
        for (int i = 0; i < instrGroups.size(); ++i) {
            InstrGroup& group = instrGroups[i];
            cout << "Adding gig instrument '" << group.baseName << "'" << endl;

            gig::Instrument* instr = g_gig->AddInstrument();
            instr->pInfo->Name = group.baseName;

            map<DLS::range_t, gig::Region*> regions; // the gig instrument regions to be created, sorted by key ranges

            // apply korg regions to gig regions
            for (InstrGroup::iterator itInstr = group.begin();
                 itInstr != group.end(); ++itInstr)
            {
                Korg::KMPInstrument* kmpInstr = *itInstr;
                cout << "    |---> KMP multi sample '" << kmpInstr->Name() << "'" << endl;

                uint16_t iLowKey = 0;
                for (int k = 0; k < kmpInstr->GetRegionCount(); ++k) {
                    Korg::KMPRegion* kmpRegion = kmpInstr->GetRegion(k);
                    DLS::range_t keyRange = { iLowKey, kmpRegion->TopKey };
                    iLowKey = kmpRegion->TopKey + 1; // for next loop cycle
                    gig::Region* gigRegion = regions[keyRange];

                    // if there is no gig region for that key range yet, create it
                    if (!gigRegion) {
                        gigRegion = instr->AddRegion();
                        gigRegion->SetKeyRange(keyRange.low, keyRange.high);
                        regions[keyRange] = gigRegion;
                    }

                    uint8_t iDimBits[8] = {}; // used to select the target gig::DimensionRegion for current source KMPRegion to be applied

                    bool isStereoSplit = bInterpretNames && hasLeftOrRightMarker(kmpInstr->Name());
                    if (isStereoSplit) { // stereo dimension split required for this region ...
                        int iStereoDimensionIndex = getDimensionIndex(gigRegion, gig::dimension_samplechannel);

                        // create stereo dimension if it doesn't exist already ...
                        if (iStereoDimensionIndex < 0) {
                            gig::dimension_def_t dim;
                            dim.dimension = gig::dimension_samplechannel;
                            dim.bits  = 1; // 2^(1) = 2
                            dim.zones = 2; // stereo = 2 audio channels = 2 split zones
                            cout << "Adding stereo dimension." << endl;
                            gigRegion->AddDimension(&dim);

                            iStereoDimensionIndex = getDimensionIndex(gigRegion, gig::dimension_samplechannel);
                        }

                        if (iStereoDimensionIndex < 0)
                            throw gig::Exception("Internal error: Could not resolve target stereo dimension bit");

                        // select dimension bit for this stereo dimension split
                        iDimBits[iStereoDimensionIndex] = isLeftMarker(kmpInstr->Name()) ? 0 : 1;
                    }

                    int iVelocityDimensionIndex = -1;
                    DLS::range_t velRange = { 0 , 127 };
                    bool isVelocitySplit = bInterpretNames && hasNumberRangeIndicator(kmpInstr->Name());
                    if (isVelocitySplit) { // a velocity split is required for this region ...
                        // get the set of velocity split zones defined for
                        // this particular output instrument and key range
                        set<DLS::range_t> origVelocitySplits = collectVelocitySplits(group, keyRange);
                        // if there are any gaps, that is if the list of velocity
                        // split ranges do not cover the full range completely
                        // between 0 .. 127, then auto correct it by increasing
                        // the split zones accordingly
                        map<DLS::range_t,DLS::range_t> velocityCorrectionMap; // maps original defined velocity range (key) -> corrected velocity range (value)
                        {
                            set<DLS::range_t> correctedSplits;
                            velocityCorrectionMap = killGapsInRanges(origVelocitySplits, correctedSplits);
                            if (correctedSplits != origVelocitySplits) {
                                cout << "WARNING: Velocity splits did not cover the full range 0..127, auto adjusted it from " << flush;
                                printRanges(origVelocitySplits);
                                cout << " to " << flush;
                                printRanges(correctedSplits);
                                cout << endl;
                            }
                        }

                        // only create a velocity dimension if there was really
                        // more than one velocity split zone defined
                        if (origVelocitySplits.size() <= 1) {
                            cerr << "WARNING: Velocity split mentioned, but with only one zone, thus ignoring velocity split.\n";
                        } else {
                            // get the velocity range for current KORG region
                            {
                                int from, to;
                                if (parseNumberRange(kmpInstr->Name(), from, to) == kmpInstr->Name())
                                    throw Korg::Exception("Internal error: parsing velocity range failed");
                                velRange.low  = from;
                                velRange.high = to;
                                if (velocityCorrectionMap.find(velRange) == velocityCorrectionMap.end())
                                    throw Korg::Exception("Internal error: inconsistency in velocity split correction map");
                                velRange = velocityCorrectionMap[velRange]; // corrected
                            }

                            // create velocity split dimension if it doesn't exist already ...
                            iVelocityDimensionIndex = getDimensionIndex(gigRegion, gig::dimension_velocity);
                            if (iVelocityDimensionIndex < 0) {
                                gig::dimension_def_t dim;
                                dim.dimension = gig::dimension_velocity;
                                dim.zones = origVelocitySplits.size();
                                // Find the number of bits required to hold the
                                // specified amount of zones.
                                int zoneBits = dim.zones - 1;
                                for (dim.bits = 0; zoneBits > 1; dim.bits += 2, zoneBits >>= 2);
                                dim.bits += zoneBits;
                                cout << "Adding velocity dimension: zones=" << (int)dim.zones << ", bits=" << (int)dim.bits << endl;
                                gigRegion->AddDimension(&dim);

                                iVelocityDimensionIndex = getDimensionIndex(gigRegion, gig::dimension_velocity);
                            }

                            if (iVelocityDimensionIndex < 0)
                                throw gig::Exception("Internal error: Could not resolve target velocity dimension bit");

                            // find the velocity zone for this one
                            int iVelocitySplitZone = -1;
                            {
                                int i = 0;
                                for (map<DLS::range_t,DLS::range_t>::const_iterator itVelSplit = velocityCorrectionMap.begin();
                                    itVelSplit != velocityCorrectionMap.end(); ++itVelSplit, ++i)
                                {
                                    if (itVelSplit->second == velRange) { // already corrected before, thus second, not first
                                        iVelocitySplitZone = i;
                                        break;
                                    }
                                }
                                if (iVelocitySplitZone == -1)
                                    throw gig::Exception("Internal error: Could not resolve target velocity dimension zone");
                            }

                            // select dimension bit for this stereo dimension split
                            iDimBits[iVelocityDimensionIndex] = iVelocitySplitZone;
                        }
                    }

                    // resolve target gig::DimensionRegion for the left/right and velocity split zone detected above
                    gig::DimensionRegion* dimRgn = gigRegion->GetDimensionRegionByBit(iDimBits);
                    if (!dimRgn)
                        throw gig::Exception("Internal error: Could not resolve Dimension Region");

                    // if this is a velocity split, apply the precise velocity split range values
                    if (isVelocitySplit) {
                        dimRgn->VelocityUpperLimit = velRange.high; // gig v2
                        dimRgn->DimensionUpperLimits[iVelocityDimensionIndex] = velRange.high; // gig v3 and above
                    }

                    dimRgn->FineTune = kmpRegion->Tune;

                    // assign the respective gig sample to this dimension region
                    gig::Sample* gigSample = findOrCreateGigSampleForKSFRegion(kmpRegion);
                    dimRgn->pSample = gigSample; // might be NULL (if Korg sample had zero size, or if the original instrument's internal samples were used)
                    if (gigSample) {
                        dimRgn->UnityNote = gigSample->MIDIUnityNote;
                        if (gigSample->Loops) {
                            DLS::sample_loop_t loop;
                            loop.Size = sizeof(loop);
                            loop.LoopType   = gig::loop_type_normal;
                            loop.LoopStart  = gigSample->LoopStart;
                            loop.LoopLength = gigSample->LoopEnd - gigSample->LoopStart;
                            dimRgn->AddSampleLoop(&loop);
                        }
                    }
                }
            }
        }

        // add the .KSF samples that are not referenced by any instrument
        for (int i = 0; i < ksfSamples.size(); ++i) {
            Korg::KSFSample* ksfSample = ksfSamples[i];
            gig::Sample* gigSample = sampleRelations[ksfSample];
            if (!gigSample) {
                // put those "orphaned" samples into a sample group called
                // "Not referenced", showing the user in gigedit that this
                // sample is not used (referenced) by any gig instrument
                const string sNotReferenced = "Not referenced";
                gig::Group* gigSampleGroup = g_gig->GetGroup(sNotReferenced);
                if (!gigSampleGroup) {
                    gigSampleGroup = g_gig->AddGroup();
                    gigSampleGroup->Name = sNotReferenced;
                }
                // create the gig sample
                gigSample = findOrcreateGigSampleForKSFSample(ksfSample, gigSampleGroup);
            }
        }

        // save result to disk (as .gig file)
        cout << "Saving converted (.gig) file to '" << outFileName << "' ... " << flush;
        g_gig->Save(outFileName); 
        cout << "OK\n";
        cout << "Writing all samples to converted (.gig) file  ... " << flush;
        writeAllSampleData();
        cout << "OK\n";
    } catch (RIFF::Exception e) {
        cerr << "Failed generating output file:" << endl;
        e.PrintMessage();
        cleanup();
        return EXIT_FAILURE;
    } catch (...) {
        cerr << "Unknown exception while trying to assemble output file." << endl;
        cleanup();
        return EXIT_FAILURE;
    }

    cleanup();
    return EXIT_SUCCESS;
}
