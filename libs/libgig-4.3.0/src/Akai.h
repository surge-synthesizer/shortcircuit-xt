/*
  libakai - C++ cross-platform akai sample disk reader
  Copyright (C) 2002-2003 Sébastien Métrot

  Linux port by Christian Schoenebeck <cuse@users.sourceforge.net> 2003-2019
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
// Akai.h
// Info for the akai disk & file format found here:
// http://www.abel.co.uk/~maxim/akai/akaiinfo.htm
//
#ifndef __akai_h__
#define __akai_h__

// for use with autoconf
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if !defined(_CARBON_) && !defined(__APPLE__) && !defined(WIN32)
# define LINUX 1
#endif

#include <stdint.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <list>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
# include <sys/fcntl.h>
#endif
#if defined(_CARBON_) || defined(__APPLE__) || LINUX
# include <sys/ioctl.h>
# include <unistd.h>
#elif defined(WIN32)
# include <windows.h>
  typedef unsigned int   uint;
#endif
#if LINUX
# include <linux/cdrom.h>
#endif

typedef std::string String;
typedef std::streampos streampos;

class AkaiVolume;
class AkaiPartition;
class AkaiDisk;


/* current state of the Akai stream */
typedef enum {
  akai_stream_ready       = 0,
  akai_stream_end_reached = 1,
  akai_stream_closed      = 2
} akai_stream_state_t;

/* stream position dependent to these relations */
typedef enum {
  akai_stream_start  = 0,
  akai_stream_curpos = 1,
  akai_stream_end    = 2
} akai_stream_whence_t;


/* We need to cache IO access to reduce IO system calls which else would slow
   down things tremendously. For that we differ between the following two
   cache sizes:

   CD_FRAMESIZE       for CDROM access
   DISK_CLUSTER_SIZE  for normal IO access (e.g. from hard disk)

   Not yet sure if these are the optimum sizes.
 */

#ifndef CD_FRAMESIZE
#  define CD_FRAMESIZE 2048 /* frame size for Yellow Book, Form 1 */
#endif

#define DISK_CLUSTER_SIZE 61440 /* 60 kB */

typedef std::string String;

/** @brief Accessing AKAI image either from file or a drive (i.e. CDROM).
 *
 * This class implements a hardware abstraction layer, providing an abstract
 * streaming API to read from AKAI data images, no matter if the AKAI image is
 * already available as image file or whether the respective hardware drive
 * needs to be accessed directly (i.e. CDROM drive, ZIP drive). So the main task
 * of this class is isolating operating system dependent file/hardware access.
 */
class DiskImage
{
public:
  DiskImage(const char* path); ///< Open an image from a file.
  DiskImage(int disk); ///< Open an image from a device number (0='a:', 1='b:', etc...).

  bool WriteImage(const char* path); ///< Extract Akai data track and write it into a regular file.

  virtual ~DiskImage();

  virtual akai_stream_state_t GetState() const;
  virtual int GetPos() const;
  virtual int SetPos(int Where, akai_stream_whence_t Whence = akai_stream_start);

  virtual int Available (uint WordSize = 1);
  virtual int Read (void* pData, uint WordCount, uint WordSize); ///< Returns number of successfully read words.

  void     ReadInt8(uint8_t* pData, uint WordCount);
  void     ReadInt16(uint16_t* pData, uint WordCount);
  void     ReadInt32(uint32_t* pData, uint WordCount);
  int      ReadInt8(uint8_t* pData);    ///< Returns number of successfully read 8 Bit words.
  int      ReadInt16(uint16_t* pData);  ///< Returns number of successfully read 16 Bit words.
  int      ReadInt32(uint32_t* pData);  ///< Returns number of successfully read 32 Bit words.
  uint8_t  ReadInt8();
  uint16_t ReadInt16();
  uint32_t ReadInt32();


  virtual uint GetError() const
  {
    return 0;
  }

  /*virtual const nglChar* GetErrorStr(uint err) const
  {
    return _T("No Error");
  }*/

protected:
#ifdef WIN32
  HANDLE mFile;
#elif defined _CARBON_ || defined(__APPLE__) || LINUX
  int mFile;
#endif
  bool mRegularFile;
  int mPos;
  int mCluster;
  int mClusterSize;
  int mSize; /* in bytes */
  /* start and end of the data track we chose (if we're reading from CDROM) */
  int mStartFrame;
  int mEndFrame;
  char* mpCache;

  void OpenStream(const char* path);
  inline void swapBytes_16(void* Word);
  inline void swapBytes_32(void* Word);

private:
  void Init();
};


class Resource
{
public:
  Resource()
  {
    mRefCount = 0;
  }
  virtual ~Resource()
  {
    //NGL_ASSERT(mRefCount==0);
  }
  uint Acquire()
  {
    return mRefCount++;
  }
  uint Release()
  {
    uint res;
    //NGL_ASSERT(mRefCount!=0);
    mRefCount--;
    res = mRefCount;
    if (!mRefCount)
      delete this;
    return res;
  }
private:
  uint mRefCount;
};

class AkaiDirEntry
{
public:
  String mName;
  uint16_t mType;
  int mSize;
  uint16_t mStart;
  int mIndex;
};

// AkaiDiskElement:
class AkaiDiskElement : public Resource
{
public:
  AkaiDiskElement(uint Offset = 0)
  {
    mOffset = Offset;
  }

  uint GetOffset()
  {
    return mOffset;
  }

protected:
  void SetOffset(uint Offset)
  {
    mOffset = Offset;
  }

  void AkaiToAscii(char * buffer, int length);
  int ReadFAT(DiskImage* pDisk, AkaiPartition* pPartition, int block);
  bool ReadDirEntry(DiskImage* pDisk, AkaiPartition* pPartition, AkaiDirEntry& rEntry, int block, int pos);
private:
  uint mOffset;
};

class AkaiSampleLoop
{
public:
  //    4     unsigned    Loop 1 marker
  uint32_t mMarker;
  //    2     unsigned    Loop 1 fine length   (65536ths)
  uint16_t mFineLength;
  //    4     unsigned    Loop 1 coarse length (words)
  uint32_t mCoarseLength;
  //    2     unsigned    Loop 1 time          (msec. or 9999=infinite)
  uint16_t mTime;
private:
  friend class AkaiSample;
  bool Load(DiskImage* pDisk);
};

class AkaiSample : public AkaiDiskElement
{
public:
  AkaiDirEntry GetDirEntry();
  // Length   Format      Description
  // --------------------------------------------------------------
  //    1                 3
  //    1                 Not important: 0 for 22050Hz, 1 for 44100Hz
  //    1     unsigned    MIDI root note (C3=60)
  uint8_t mMidiRootNote;
  //   12     AKAII       Filename
  String mName;
  //    1                 128
  //    1     unsigned    Number of active loops
  uint8_t mActiveLoops;
  //    1     unsigned char       First active loop (0 for none)
  uint8_t mFirstActiveLoop;
  //    1                         0
  //    1     unsigned    Loop mode: 0=in release 1=until release
  //                                 2=none       3=play to end
  uint8_t mLoopMode;
  //    1     signed      Cents tune -50...+50
  int8_t mTuneCents;
  //    1     signed      Semi tune  -50...+50
  int8_t mTuneSemitones;
  //    4                 0,8,2,0
  //
  //    4     unsigned    Number of sample words
  uint32_t mNumberOfSamples;
  //    4     unsigned    Start marker
  uint32_t mStartMarker;
  //    4     unsigned    End marker
  uint32_t mEndMarker;
  //
  //    4     unsigned    Loop 1 marker
  //    2     unsigned    Loop 1 fine length   (65536ths)
  //    4     unsigned    Loop 1 coarse length (words)
  //    2     unsigned    Loop 1 time          (msec. or 9999=infinite)
  //
  //   84     [as above]  Loops 2 to 8
  //
  AkaiSampleLoop mLoops[8];
  //    4                 0,0,255,255
  //    2     unsigned    Sampling frequency
  uint16_t mSamplingFrequency;
  //    1     signed char         Loop tune offset -50...+50
  int8_t mLoopTuneOffset;
  //   10                 0,0,0...

  int16_t* mpSamples;

  bool LoadSampleData(); ///< Load sample into RAM
  void ReleaseSampleData(); ///< release the samples once you used them if you don't want to be bothered to
  int SetPos(int Where, akai_stream_whence_t Whence = akai_stream_start); ///< Use this method and Read() if you don't want to load the sample into RAM, thus for disk streaming.
  int Read(void* pBuffer, uint SampleCount); ///< Use this method and SetPos() if you don't want to load the sample into RAM, thus for disk streaming.
  bool LoadHeader();
private:
  AkaiSample(DiskImage* pDisk, AkaiVolume* pParent, const AkaiDirEntry& DirEntry);
  virtual ~AkaiSample();

  friend class AkaiVolume;

  AkaiVolume* mpParent;
  DiskImage* mpDisk;
  AkaiDirEntry mDirEntry;
  bool mHeaderOK;
  int mPos;
  int mImageOffset; // actual position in the image where sample starts
};

class AkaiKeygroupSample : public AkaiDiskElement
{
public:
  //     35-46   sample 1 name               10,10,10... AKAII character set
  String mName;
  //     47      low vel                     0           0..127
  uint8_t mLowLevel;
  //     48      high vel                    127         0..127
  uint8_t mHighLevel;
  //     49      tune cents                  0           -128..127 (-50..50 cents)
  int8_t mTuneCents;
  //     50      tune semitones              0           -50..50
  int8_t mTuneSemitones;
  //     51      loudness                    0           -50..+50
  int8_t mLoudness;
  //     52      filter                      0           -50..+50
  int8_t mFilter;
  //     53      pan                         0           -50..+50
  int8_t mPan;
  //     54      loop mode                   0           0=AS_SAMPLE 1=LOOP_IN_REL 
  //                                                     2=LOOP_UNTIL_REL 3=NO_LOOP 
  //                                                     4=PLAY_TO_END
  uint8_t mLoopMode;
  //     55      (internal use)              255
  //     56      (internal use)              255
  //     57-58   (internal use)              44,1
  //
private:
  friend class AkaiKeygroup;
  bool Load(DiskImage* pDisk);
};

class AkaiEnveloppe
{
public:
  //     13      env1 attack                 0           0..99
  uint8_t mAttack;
  //     14      env1 decay                  30          0..99
  uint8_t mDecay;
  //     15      env1 sustain                99          0..99
  uint8_t mSustain;
  //     16      env1 release                45          0..99
  uint8_t mRelease;
  //     17      env1 vel>attack             0           -50..50
  int8_t mVelocityToAttack;
  //     18      env1 vel>release            0           -50..50 
  int8_t mVelocityToRelease;
  //     19      env1 offvel>release         0           -50..50
  int8_t mOffVelocityToRelease;
  //     20      env1 key>dec&rel            0           -50..50
  int8_t mKeyToDecayAndRelease;
private:
  friend class AkaiKeygroup;
  bool Load(DiskImage* pDisk);
};

class AkaiKeygroup
{
public:
  //    byte     description                 default     range/comments
  //   ---------------------------------------------------------------------------
  //     1       keygroup ID                 2
  //     2-3     next keygroup address       44,1        300,450,600,750.. (16-bit)         
  //     4       low key                     24          24..127
  uint8_t mLowKey;
  //     5       high key                    127         24..127
  uint8_t mHighKey;
  //     6       tune cents                  0           -128..127 (-50..50 cents)
  int8_t mTuneCents;
  //     7       tune semitones              0           -50..50
  int8_t mTuneSemitones;
  //     8       filter                      99          0..99
  uint8_t mFilter;
  //     9       key>filter                  12          0..24 semitone/oct
  uint8_t mKeyToFilter;
  //     10      vel>filt                    0           -50..50
  uint8_t mVelocityToFilter;
  //     11      pres>filt                   0           -50..50
  uint8_t mPressureToFilter;
  //     12      env2>filt                   0           -50..50
  uint8_t mEnveloppe2ToFilter;

  //     13      env1 attack                 0           0..99
  //     14      env1 decay                  30          0..99
  //     15      env1 sustain                99          0..99
  //     16      env1 release                45          0..99
  //     17      env1 vel>attack             0           -50..50
  //     18      env1 vel>release            0           -50..50 
  //     19      env1 offvel>release         0           -50..50
  //     20      env1 key>dec&rel            0           -50..50
  //     21      env2 attack                 0           0..99
  //     22      env2 decay                  50          0..99
  //     23      env2 sustain                99          0..99
  //     24      env2 release                45          0..99
  //     25      env2 vel>attack             0           -50..50
  //     26      env2 vel>release            0           -50..50
  //     27      env2 offvel>release         0           -50..50
  //     28      env2 key>dec&rel            0           -50..50
  AkaiEnveloppe mEnveloppes[2];

  //     29      vel>env2>filter             0           -50..50
  int8_t mVelocityToEnveloppe2ToFilter;
  //     30      env2>pitch                  0           -50..50
  int8_t mEnveloppe2ToPitch;
  //     31      vel zone crossfade          1           0=OFF 1=ON
  bool mVelocityZoneCrossfade;
  //     32      vel zones used              4           
  uint mVelocityZoneUsed;
  //     33      (internal use)              255         
  //     34      (internal use)              255         
  //

  //     35-46   sample 1 name               10,10,10... AKAII character set
  //     47      low vel                     0           0..127
  //     48      high vel                    127         0..127
  //     49      tune cents                  0           -128..127 (-50..50 cents)
  //     50      tune semitones              0           -50..50
  //     51      loudness                    0           -50..+50
  //     52      filter                      0           -50..+50
  //     53      pan                         0           -50..+50
  //     54      loop mode                   0           0=AS_SAMPLE 1=LOOP_IN_REL 
  //                                                     2=LOOP_UNTIL_REL 3=NO_LOOP 
  //                                                     4=PLAY_TO_END
  //     55      (internal use)              255
  //     56      (internal use)              255
  //     57-58   (internal use)              44,1
  //
  //     59-82   [repeat 35-58 for sample 2]
  //
  //     83-106  [repeat 35-58 for sample 3]
  //
  //     107-130 [repeat 35-58 for sample 4]
  //
  AkaiKeygroupSample mSamples[4];
  
  //     131     beat detune                 0           -50..50
  int8_t mBeatDetune;
  //     132     hold attack until loop      0           0=OFF 1=ON
  bool mHoldAttackUntilLoop;
  //     133-136 sample 1-4 key tracking     0           0=TRACK 1=FIXED
  bool mSampleKeyTracking[4];
  //     137-140 sample 1-4 aux out offset   0           0..7
  uint8_t mSampleAuxOutOffset[4];
  //     141-148 vel>sample start            0           -9999..9999 (16-bit signed)
  int16_t mVelocityToSampleStart[4];
  //     149     vel>volume offset           0           -50..50
  int8_t mVelocityToVolumeOffset[4];
  //     150     (not used)

private:
  friend class AkaiProgram;
  bool Load(DiskImage* pDisk);
};

/** @brief AKAI instrument definition
 *
 * Represents exactly one sample based instrument on the AKAI media.
 */
class AkaiProgram : public AkaiDiskElement
{
public:
  AkaiDirEntry GetDirEntry();
  // Samples:
  uint ListSamples(std::list<String>& rSamples);
  AkaiSample* GetSample(uint Index);
  AkaiSample* GetSample(const String& rName);

  //    byte     description                 default     range/comments
  //   ---------------------------------------------------------------------------
  //     1       program ID                  1
  //     2-3     first keygroup address      150,0       
  //     4-15    program name                10,10,10... AKAII character set
  String mName;
  //     16      MIDI program number         0           0..127
  uint8_t mMidiProgramNumber;
  //     17      MIDI channel                0           0..15, 255=OMNI
  uint8_t mMidiChannel;
  //     18      polyphony                   15          1..16
  uint8_t mPolyphony;
  //     19      priority                    1           0=LOW 1=NORM 2=HIGH 3=HOLD
  uint8_t mPriority;
  //     20      low key                     24          24..127
  uint8_t mLowKey;
  //     21      high key                    127         24..127
  uint8_t mHighKey;
  //     22      octave shift                0           -2..2
  int8_t mOctaveShift;
  //     23      aux output select           255         0..7, 255=OFF
  uint8_t mAuxOutputSelect;
  //     24      mix output level            99          0..99
  uint8_t mMixOutputSelect;
  //     25      mix output pan              0           -50..50
  int8_t mMixPan;
  //     26      volume                      80          0..99
  uint8_t mVolume;
  //     27      vel>volume                  20          -50..50
  int8_t mVelocityToVolume;
  //     28      key>volume                  0           -50..50
  int8_t mKeyToVolume;
  //     29      pres>volume                 0           -50..50
  int8_t mPressureToVolume;
  //     30      pan lfo rate                50          0..99
  uint8_t mPanLFORate;
  //     31      pan lfo depth               0           0..99
  uint8_t mPanLFODepth;
  //     32      pan lfo delay               0           0..99
  uint8_t mPanLFODelay;
  //     33      key>pan                     0           -50..50
  int8_t mKeyToPan;
  //     34      lfo rate                    50          0..99
  uint8_t mLFORate;
  //     35      lfo depth                   0           0..99
  uint8_t mLFODepth;
  //     36      lfo delay                   0           0..99
  uint8_t mLFODelay;
  //     37      mod>lfo depth               30          0..99
  uint8_t mModulationToLFODepth;
  //     38      pres>lfo depth              0           0..99
  uint8_t mPressureToLFODepth;
  //     39      vel>lfo depth               0           0..99
  uint8_t mVelocityToLFODepth;
  //     40      bend>pitch                  2           0..12 semitones
  uint8_t mBendToPitch;
  //     41      pres>pitch                  0           -12..12 semitones
  int8_t mPressureToPitch;
  //     42      keygroup crossfade          0           0=OFF 1=ON
  bool mKeygroupCrossfade;
  //     43      number of keygroups         1           1..99
  uint8_t mNumberOfKeygroups;
  //     44      (internal use)              0           program number
  //     45-56   key temperament C,C#,D...   0           -25..25 cents
  int8_t mKeyTemperament[11];
  //     57      fx output                   0           0=OFF 1=ON
  bool mFXOutput;
  //     58      mod>pan                     0           -50..50
  int8_t mModulationToPan;
  //     59      stereo coherence            0           0=OFF 1=ON
  bool mStereoCoherence;
  //     60      lfo desync                  1           0=OFF 1=ON
  bool mLFODesync;
  //     61      pitch law                   0           0=LINEAR
  uint8_t mPitchLaw;
  //     62      voice re-assign             0           0=OLDEST 1=QUIETEST
  uint8_t mVoiceReassign;
  //     63      softped>volume              10          0..99
  uint8_t mSoftpedToVolume;
  //     64      softped>attack              10          0..99
  uint8_t mSoftpedToAttack;
  //     65      softped>filt                10          0..99
  uint8_t mSoftpedToFilter;
  //     66      tune cents                  0           -128..127 (-50..50 cents)
  int8_t mSoftpedToTuneCents;
  //     67      tune semitones              0           -50..50
  int8_t mSoftpedToTuneSemitones;
  //     68      key>lfo rate                0           -50..50
  int8_t mKeyToLFORate;
  //     69      key>lfo depth               0           -50..50
  int8_t mKeyToLFODepth;
  //     70      key>lfo delay               0           -50..50
  int8_t mKeyToLFODelay;
  //     71      voice output scale          1           0=-6dB 1=0dB 2=+12dB
  uint8_t mVoiceOutputScale;
  //     72      stereo output scale         0           0=0dB 1=+6dB
  uint8_t mStereoOutputScale;
  //     73-150  (not used)

  AkaiKeygroup* mpKeygroups;

  bool Load();
  AkaiVolume* GetParent()
  {
    return mpParent;
  }

private:
  AkaiProgram(DiskImage* pDisk, AkaiVolume* pParent, const AkaiDirEntry& DirEntry);
  virtual ~AkaiProgram();

  friend class AkaiVolume;

  std::list<AkaiSample*> mpSamples;
  AkaiVolume* mpParent;
  DiskImage* mpDisk;
  AkaiDirEntry mDirEntry;
};

/** @brief Subdivision of an AKAI disk partition.
 *
 * An AKAI volume is a further subdivision of an AKAI disk partition.
 *
 * An AKAI volume actually provides access to the list of instruments (programs)
 * and samples. Samples referenced by an instrument (program) are always part of
 * the same volume.
 */
class AkaiVolume : public AkaiDiskElement
{
public:
  AkaiDirEntry GetDirEntry();
  // Programs:
  uint ListPrograms(std::list<AkaiDirEntry>& rPrograms);
  AkaiProgram* GetProgram(uint Index);
  AkaiProgram* GetProgram(const String& rName);

  // Samples:
  uint ListSamples(std::list<AkaiDirEntry>& rSamples);
  AkaiSample* GetSample(uint Index);
  AkaiSample* GetSample(const String& rName);

  AkaiPartition* GetParent()
  {
    return mpParent;
  }

  bool IsEmpty();
private:
  AkaiVolume(DiskImage* pDisk, AkaiPartition* pParent, const AkaiDirEntry& DirEntry);
  virtual ~AkaiVolume();
  uint ReadDir();

  friend class AkaiPartition;

  String mName;
  std::list<AkaiProgram*> mpPrograms;
  std::list<AkaiSample*> mpSamples;
  DiskImage* mpDisk;
  AkaiPartition* mpParent;
  AkaiDirEntry mDirEntry;
};

/** @brief Encapsulates one disk partition of an AKAI disk.
 *
 * An object of this class represents exactly one disk partition of an AKAI disk
 * media or of an AKAI disk image file. This is similar to a hard disk partition
 * on other operating systems, just in AKAI's own custom format.
 *
 * Each AKAI disk partition is further subdivided into so called "volumes".
 */
class AkaiPartition : public AkaiDiskElement
{
public:
  // Samples:
  uint ListVolumes(std::list<AkaiDirEntry>& rVolumes);
  AkaiVolume* GetVolume(uint Index);
  AkaiVolume* GetVolume(const String& rName);

  AkaiDisk* GetParent()
  {
    return mpParent;
  }

  bool IsEmpty();
private:
  AkaiPartition(DiskImage* pDisk, AkaiDisk* pParent);
  virtual ~AkaiPartition();

  friend class AkaiDisk;

  String mName;
  std::list<AkaiVolume*> mpVolumes;
  AkaiDisk* mpParent;
  DiskImage* mpDisk;
};

/** @brief Toplevel AKAI image interpreter.
 *
 * This class takes an AKAI disk image as constructor argument and provides
 * access to the individual partitions of that AKAI disk/image. The concept is
 * similar to hard disc layout for other operating systems, which are also
 * divided into individual partitions as topmost instance on the mass data
 * media.
 */
class AkaiDisk : public AkaiDiskElement
{
public:
  AkaiDisk(DiskImage* pDisk);
  virtual ~AkaiDisk();

  // Partitions:
  uint GetPartitionCount();
  AkaiPartition* GetPartition(uint count);

private:
  DiskImage* mpDisk;
  std::list<AkaiPartition*> mpPartitions;
};

#define AKAI_FILE_ENTRY_SIZE            24
#define AKAI_DIR_ENTRY_OFFSET           0xca
#define AKAI_DIR_ENTRY_SIZE             16
#define AKAI_ROOT_ENTRY_OFFSET          0x0

#define AKAI_PARTITION_END_MARK         0x8000
#define AKAI_BLOCK_SIZE                 0x2000

#define AKAI_FAT_OFFSET                 0x70a

#define AKAI_MAX_FILE_ENTRIES_S1000     125  //should be 128 ??
#define AKAI_MAX_FILE_ENTRIES_S3000     509  // should be 512 ??
#define AKAI_MAX_DIR_ENTRIES            100
#define AKAI_TYPE_DIR_S1000             1
#define AKAI_TYPE_DIR_S3000             3

#define AKAI_PROGRAM_ID   1
#define AKAI_KEYGROUP_ID  2
#define AKAI_SAMPLE_ID    3

#endif // __akai_h__
