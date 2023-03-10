/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2014 Christian Schoenebeck                              *
 *                      <cuse@users.sourceforge.net>                       *
 *                                                                         *
 *   This library is part of libgig.                                       *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef LIBGIG_KORG_H
#define LIBGIG_KORG_H

#include "RIFF.h"
#include "gig.h" // for struct buffer_t
#include <vector>

/**
 * @brief KORG sound format specific classes and definitions
 *
 * Classes in this namespace provide access to Korg's sample based instrument
 * files which are used by Korg Trinity, Triton, OASYS, M3 and Kronos
 * synthesizer keyboards.
 *
 * At the moment these classes only support read support, but no write support
 * yet.
 *
 * Sample based instruments are spread in KORG's format over individual files:
 *
 * - @e .KSF @e Sample @e File (KSFSample): contains exactly one audio sample
 *   (mono). So each audio sample is stored in its own .KSF file. It also stores
 *   some basic meta informations for the sample, i.e. loop points.
 *
 * - @e .KMP @e Multi @e Sample @e File (KMPInstrument): groups individual .KSF
 *   sample files to one logical group of samples. This file just references the
 *   actual .KSF files by file name. It also provides some articulation
 *   informations, for example it maps the individual samples to regions on the
 *   keyboard, but it does not even provide support for velocity splits or
 *   layers.
 *
 * The upper two file types are used by KORG for many years and their internal
 * file format has remained nearly unchanged over that long period, and has also
 * remained consistent over many different synthesizer keyboard models and
 * series. Due to this however, the articulation informations stored in those
 * two files are too primitive for being used directly on modern keyboards.
 * That's why the following file type exists as well:
 *
 * - @e .PCG @e Program @e File: contains a complete bank list of "programs"
 *   (sounds / virtual instruments) and "combinations" (combi sounds), along
 *   with all their detailed articulation informations and references to the
 *   .KSF sample files. The precise internal format of this file type differs
 *   quite a lot between individual keyboard models, series and even between
 *   different OS versions of the same keyboard model. The individual sound
 *   definitions in this file references the individual (separate) sound files,
 *   defines velocity splits, groups individual mono samples to stereo samples
 *   and stores all synthesis model specific sound settings like envelope
 *   generator settings, LFO settings, MIDI controllers, filter settings, etc.
 *
 * Unfortunately this library does not provide support for .PCG files yet.
 */
namespace Korg {

    typedef std::string String;

    class KMPRegion;
    class KMPInstrument;

    typedef gig::buffer_t buffer_t;

    /**
     * @brief .KSF audio sample file
     *
     * Implements access to KORG audio sample files with ".KSF" file name
     * extension. As of to date, there are only mono samples in .KSF format.
     * If you ever encounter a stereo sample, please report it!
     */
    class KSFSample {
    public:
        String Name; ///< Sample name for drums (since this name is always stored with 16 bytes, this name must never be longer than 16 characters).
        uint8_t DefaultBank; ///< 0..3
        uint32_t Start;
        uint32_t Start2;
        uint32_t LoopStart;
        uint32_t LoopEnd;
        uint32_t SampleRate; ///< i.e. 44100
        uint8_t  Attributes; ///< Bit field of flags, better call IsCompressed(), CompressionID() and Use2ndStart() instead of accessing this variable directly.
        int8_t   LoopTune; ///< -99..+99
        uint8_t  Channels; ///< Number of audio channels (seems to be always 1, thus Mono for all Korg sound files ATM).
        uint8_t  BitDepth; ///< i.e. 8 or 16
        uint32_t SamplePoints; ///< Currently the library expects all Korg samples to be Mono, thus the value here might be incorrect in case you ever find a Korg sample in Stereo. If you got a Stereo one, please report it!

        KSFSample(const String& filename);
        virtual ~KSFSample();
        int FrameSize() const;
        bool IsCompressed() const;
        uint8_t CompressionID() const;
        bool Use2ndStart() const;
        String FileName() const;
        buffer_t GetCache() const;
        buffer_t LoadSampleData();
        buffer_t LoadSampleData(unsigned long SampleCount);
        buffer_t LoadSampleDataWithNullSamplesExtension(uint NullSamplesCount);
        buffer_t LoadSampleDataWithNullSamplesExtension(unsigned long SampleCount, uint NullSamplesCount);
        void ReleaseSampleData();
        unsigned long SetPos(unsigned long SampleCount, RIFF::stream_whence_t Whence = RIFF::stream_start);
        unsigned long GetPos() const;
        unsigned long Read(void* pBuffer, unsigned long SampleCount);
    private:
        RIFF::File* riff;
        buffer_t RAMCache; ///< Buffers sample data in RAM.
    };

    /**
     * @brief Region of a .KMP multi sample file
     *
     * Encapsulates one region on the keyboard which is part of a KORG ".KMP"
     * file (KMPInstrument). Each regions defines a mapping between one (mono)
     * sample and one consecutive area on the keyboard.
     */
    class KMPRegion {
    public:
        bool Transpose;
        uint8_t OriginalKey; ///< Note of sample's original pitch, a.k.a. "root key" (0..127).
        uint8_t TopKey; ///< The end of this region on the keyboard (0..127). The start of this region is given by TopKey+1 of the previous region.
        int8_t Tune; ///< -99..+99 cents
        int8_t Level; ///< -99..+99 cents
        int8_t Pan; ///< -64..+63
        int8_t FilterCutoff; ///< -50..0
        String SampleFileName; ///< Base file name of sample file (12 bytes). Call FullSampleFileName() for getting the file name with path, which you might then pass to a KSFSample constructor to load the respective sample. There are two special names: "SKIPPEDSAMPL" means the sample was skipped during loading on the original KORG instrument, whereas "INTERNALnnnn" means internal sample (of the original KORG instrument) with number nnnn is used. In both cases, you obviously have no other chance than skipping them.
        
        String FullSampleFileName() const;
        KMPInstrument* GetInstrument() const;
    protected:
        KMPRegion(KMPInstrument* parent, RIFF::Chunk* rlp1);
        virtual ~KMPRegion();
        friend class KMPInstrument;
    private:
        KMPInstrument* parent;
        RIFF::Chunk* rlp1;
    };

    /**
     * @brief .KMP multi sample file
     *
     * Implements access to so called KORG "multi sample" files with ".KMP" file
     * name extension. A KMPInstrument combines individual KSFSamples (.KSF
     * files) to a logical group of samples. It also maps the individual (mono)
     * samples to consecutive areas (KMPRegion) on the keyboard.
     *
     * A KMPInstrument only supports very simple articulation informations.
     * Too simple for decent sounds. That's why this kind of file is usually not
     * used directly on keyboards. Instead a keyboard uses a separate .PCG file
     * for its more complex and very synthesis model specific and OS version
     * specific articulation definitions.
     *
     * There is no support for .PCG files in this library yet though.
     */
    class KMPInstrument {
    public:
        String Name16; ///< Human readable name of the instrument for display purposes (not the file name). Since this name is always stored with 16 bytes, this name must never be longer than 16 characters. This information is stored in all .KMP files (@see Name()).
        String Name24; ///< Longer Human readable name (24 characters) of the instrument for display purposes (not the file name). This longer name might not be stored in all versions of .KMP files (@see Name()).
        uint8_t Attributes; ///< Bit field of attribute flags (ATM only bit 0 is used, better call Use2ndStart() instead of accessing this variable directly though).

        KMPInstrument(const String& filename);
        virtual ~KMPInstrument();
        KMPRegion* GetRegion(int index);
        int GetRegionCount() const;
        bool Use2ndStart() const;
        String FileName() const;
        String Name() const;
    private:
        RIFF::File* riff;
        std::vector<KMPRegion*> regions;
    };

    /**
     * @brief Korg format specific exception
     *
     * Will be thrown whenever a Korg format specific error occurs while trying
     * to access such a file in this format. Note: In your application you
     * should better catch for RIFF::Exception rather than this one, except you
     * explicitly want to catch and handle Korg::Exception and RIFF::Exception
     * independently from each other, which usually shouldn't be necessary
     * though.
     */
    class Exception : public RIFF::Exception {
    public:
        Exception(String Message);
        void PrintMessage();
    };

    String libraryName();
    String libraryVersion();

} // namespace Korg

#endif // LIBGIG_KORG_H
