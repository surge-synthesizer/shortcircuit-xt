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
// Akai.cpp

#include "Akai.h"

#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
# include <unistd.h>
# include <fcntl.h>
#endif
#if defined(_CARBON_) || defined(__APPLE__)
# if defined (__GNUC__) && (__GNUC__ >= 4)
#  include <sys/disk.h>
# else
#  include <dev/disk.h>
# endif
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#if defined(_CARBON_) || defined(__APPLE__)
#include <paths.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMediaBSDClient.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(_CARBON_) || defined(__APPLE__)

// These definitions were taken from mount_cd9660.c
// There are some similar definitions in IOCDTypes.h
// however there seems to be some dissagreement in
// the definition of CDTOC.length
struct _CDMSF {
    u_char   minute;
    u_char   second;
    u_char   frame;
};

#define MSF_TO_LBA(msf)                \
     (((((msf).minute * 60UL) + (msf).second) * 75UL) + (msf).frame - 150)

struct _CDTOC_Desc {
    u_char        session;
    u_char        ctrl_adr;  /* typed to be machine and compiler independent */
    u_char        tno;
    u_char        point;
    struct _CDMSF address;
    u_char        zero;
    struct _CDMSF p;
};

struct _CDTOC {
    u_short            length;  /* in native cpu endian */
    u_char             first_session;
    u_char             last_session;
    struct _CDTOC_Desc trackdesc[1];
};

// Most of the following Mac CDROM IO functions were taken from Apple's IOKit
// examples (BSD style license). ReadTOC() function was taken from the Bochs x86
// Emulator (LGPL). Most probably they have taken it however also from some
// other BSD style licensed example code as well ...

// Returns an iterator across all CD media (class IOCDMedia). Caller is responsible for releasing
// the iterator when iteration is complete.
static kern_return_t FindEjectableCDMedia(io_iterator_t *mediaIterator) {
    kern_return_t kernResult;
    CFMutableDictionaryRef classesToMatch;

    // CD media are instances of class kIOCDMediaClass
    classesToMatch = IOServiceMatching(kIOCDMediaClass);
    if (classesToMatch == NULL) {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    } else {
        CFDictionarySetValue(classesToMatch, CFSTR(kIOMediaEjectableKey), kCFBooleanTrue);
        // Each IOMedia object has a property with key kIOMediaEjectableKey which is true if the
        // media is indeed ejectable. So add this property to the CFDictionary we're matching on.
    }

    kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, mediaIterator);
    
    return kernResult;
}

// Given an iterator across a set of CD media, return the BSD path to the
// next one. If no CD media was found the path name is set to an empty string.
static kern_return_t GetBSDPath(io_iterator_t mediaIterator, char *bsdPath, CFIndex maxPathSize) {
    io_object_t nextMedia;
    kern_return_t kernResult = KERN_FAILURE;
    
    *bsdPath = '\0';
    
    nextMedia = IOIteratorNext(mediaIterator);
    if (nextMedia) {
        CFTypeRef bsdPathAsCFString;
        
        bsdPathAsCFString = IORegistryEntryCreateCFProperty(nextMedia,
                                                            CFSTR(kIOBSDNameKey),
                                                            kCFAllocatorDefault,
                                                            0);
        if (bsdPathAsCFString) {
            strlcpy(bsdPath, _PATH_DEV, maxPathSize);
            
            // Add "r" before the BSD node name from the I/O Registry to specify the raw disk
            // node. The raw disk nodes receive I/O requests directly and do not go through
            // the buffer cache.
            
            strlcat(bsdPath, "r", maxPathSize);
            
            size_t devPathLength = strlen(bsdPath);
            
            if (CFStringGetCString((CFStringRef)bsdPathAsCFString,
                                   bsdPath + devPathLength,
                                   maxPathSize - devPathLength,
                                   kCFStringEncodingUTF8)) {
                printf("BSD path: %s\n", bsdPath);
                kernResult = KERN_SUCCESS;
            }
            
            CFRelease(bsdPathAsCFString);
        }
    
        IOObjectRelease(nextMedia);
    }
    
    return kernResult;
}

// Given the path to a CD drive, open the drive.
// Return the file descriptor associated with the device.
/*static int OpenDrive(const char *bsdPath) {
    int fileDescriptor;
    
    // This open() call will fail with a permissions error if the sample has been changed to
    // look for non-removable media. This is because device nodes for fixed-media devices are
    // owned by root instead of the current console user.

    fileDescriptor = open(bsdPath, O_RDONLY);
    
    if (fileDescriptor == -1) {
        printf("Error opening device %s: ", bsdPath);
        perror(NULL);
    }
    
    return fileDescriptor;
}*/

// Given the file descriptor for a whole-media CD device, read a sector from the drive.
// Return true if successful, otherwise false.
/*static Boolean ReadSector(int fileDescriptor) {
    char *buffer;
    ssize_t numBytes;
    u_int32_t blockSize;
    
    // This ioctl call retrieves the preferred block size for the media. It is functionally
    // equivalent to getting the value of the whole media object's "Preferred Block Size"
    // property from the IORegistry.
    if (ioctl(fileDescriptor, DKIOCGETBLOCKSIZE, &blockSize)) {
        perror("Error getting preferred block size");
        
        // Set a reasonable default if we can't get the actual preferred block size. A real
        // app would probably want to bail at this point.
        blockSize = kCDSectorSizeCDDA;
    }
    
    printf("Media has block size of %d bytes.\n", blockSize);
    
    // Allocate a buffer of the preferred block size. In a real application, performance
    // can be improved by reading as many blocks at once as you can.
    buffer = (char*) malloc(blockSize);
    
    // Do the read. Note that we use read() here, not fread(), since this is a raw device
    // node.
    numBytes = read(fileDescriptor, buffer, blockSize);
        
    // Free our buffer. Of course, a real app would do something useful with the data first.
    free(buffer);
    
    return numBytes == blockSize ? true : false;
}*/

// Given the file descriptor for a device, close that device.
/*static void CloseDrive(int fileDescriptor) {
    close(fileDescriptor);
}*/

// path is the BSD path to a raw device such as /dev/rdisk1
static struct _CDTOC * ReadTOC(const char *devpath) {
    struct _CDTOC * toc_p = NULL;
    io_iterator_t iterator = 0;
    io_registry_entry_t service = 0;
    CFDictionaryRef properties = 0;
    CFDataRef data = 0;
    mach_port_t port = 0;
    const char *devname;

    if ((devname = strrchr(devpath, '/')) != NULL) {
        ++devname;
    } else {
        devname = (char *) devpath;
    }

    if (IOMasterPort(bootstrap_port, &port) != KERN_SUCCESS) {
        fprintf(stderr, "IOMasterPort failed\n");
        goto Exit;
    }

    if (IOServiceGetMatchingServices(port, IOBSDNameMatching(port, 0, devname),
                                     &iterator) != KERN_SUCCESS) {
        fprintf(stderr, "IOServiceGetMatchingServices failed\n");
        goto Exit;
    }

    service = IOIteratorNext(iterator);

    IOObjectRelease(iterator);

    iterator = 0;

    while (service && !IOObjectConformsTo(service, "IOCDMedia")) {
        if (IORegistryEntryGetParentIterator(service, kIOServicePlane,
                                             &iterator) != KERN_SUCCESS)
        {
            fprintf(stderr, "IORegistryEntryGetParentIterator failed\n");
            goto Exit;
        }
 
        IOObjectRelease(service);
        service = IOIteratorNext(iterator);
        IOObjectRelease(iterator);
    }

    if (!service) {
        fprintf(stderr, "CD media not found\n");
        goto Exit;
    }

    if (IORegistryEntryCreateCFProperties(service, (__CFDictionary **) &properties,
                                          kCFAllocatorDefault,
                                          kNilOptions) != KERN_SUCCESS)
    {
        fprintf(stderr, "IORegistryEntryGetParentIterator failed\n");
        goto Exit;
    }

    data = (CFDataRef) CFDictionaryGetValue(properties, CFSTR(kIOCDMediaTOCKey));
    if (data == NULL) {
        fprintf(stderr, "CFDictionaryGetValue failed\n");
        goto Exit;
    } else {
        CFRange range;
        CFIndex buflen;

        buflen = CFDataGetLength(data) + 1;
        range = CFRangeMake(0, buflen);
        toc_p = (struct _CDTOC *) malloc(buflen);
        if (toc_p == NULL) {
            fprintf(stderr, "Out of memory\n");
            goto Exit;
        } else {
            CFDataGetBytes(data, range, (unsigned char *) toc_p);
        }

       /*
        fprintf(stderr, "Table of contents\n length %d first %d last %d\n",
                toc_p->length, toc_p->first_session, toc_p->last_session);
        */

        CFRelease(properties);
    }

    Exit:

    if (service) {
        IOObjectRelease(service);
    }
 
    return toc_p;
}

#endif // defined(_CARBON_) || defined(__APPLE__)

//////////////////////////////////
// AkaiSample:
AkaiSample::AkaiSample(DiskImage* pDisk, AkaiVolume* pParent, const AkaiDirEntry& DirEntry)
  : AkaiDiskElement(pDisk->GetPos())
{
  mpParent = pParent;
  mpDisk = pDisk;
  mDirEntry = DirEntry;
  mpSamples = NULL;
  mHeaderOK = false;
  mPos = 0;

  //
  LoadHeader();
}

AkaiSample::~AkaiSample()
{
  if (mpSamples)
    free(mpSamples);
}

AkaiDirEntry AkaiSample::GetDirEntry()
{
  return mDirEntry;
}

bool AkaiSample::LoadSampleData()
{
  if (!LoadHeader())
    return false;
  if (mpSamples)
    return true;

  mpDisk->SetPos(mImageOffset);
  mpSamples = (int16_t*) malloc(mNumberOfSamples * sizeof(int16_t));
  if (!mpSamples)
    return false;

  mpDisk->ReadInt16((uint16_t*)mpSamples, mNumberOfSamples);
  return true;
}

void AkaiSample::ReleaseSampleData()
{
  if (!mpSamples)
    return;
  free(mpSamples);
  mpSamples = NULL;
}

int AkaiSample::SetPos(int Where, akai_stream_whence_t Whence)
{
  if (!mHeaderOK) return -1;
  int w = Where;
  switch (Whence)
  {
  case akai_stream_start:
    mPos = w;
    break;
  /*case eStreamRewind:
    w = -w;*/
  case akai_stream_curpos:
    mPos += w;
    break;
  case akai_stream_end:
    mPos = mNumberOfSamples - w;
    break;
  }
  if (mPos > mNumberOfSamples) mPos = mNumberOfSamples;
  if (mPos < 0) mPos = 0;
  return mPos;
}

int AkaiSample::Read(void* pBuffer, uint SampleCount)
{
  if (!mHeaderOK) return 0;

  if (mPos + SampleCount > mNumberOfSamples) SampleCount = mNumberOfSamples - mPos;

  mpDisk->SetPos(mImageOffset + mPos * 2); // FIXME: assumes 16 bit sample depth
  mpDisk->ReadInt16((uint16_t*)pBuffer, SampleCount);
  return SampleCount;
}

bool AkaiSample::LoadHeader()
{
  if (mHeaderOK)
    return true;

  mpDisk->SetPos(mpParent->GetParent()->GetOffset() + mDirEntry.mStart * AKAI_BLOCK_SIZE );

  // Length   Format      Description
  // --------------------------------------------------------------
  //    1                 3
  if (mpDisk->ReadInt8() != AKAI_SAMPLE_ID)
    return false;
  //    1                 Not important: 0 for 22050Hz, 1 for 44100Hz
  mpDisk->ReadInt8();
  //    1     unsigned    MIDI root note (C3=60)
  mMidiRootNote = mpDisk->ReadInt8();
  //   12     AKAII       Filename
  char buffer[13];
  mpDisk->Read(buffer, 12, 1);
  AkaiToAscii(buffer, 12);
  mName = buffer;

  //    1                 128
  mpDisk->ReadInt8();
  //    1     unsigned    Number of active loops
  mActiveLoops = mpDisk->ReadInt8();
  //    1     unsigned char       First active loop (0 for none)
  mFirstActiveLoop = mpDisk->ReadInt8();
  //    1                         0
  mpDisk->ReadInt8();
  //    1     unsigned    Loop mode: 0=in release 1=until release
  //                                 2=none       3=play to end
  mLoopMode = mpDisk->ReadInt8();
  //    1     signed      Cents tune -50...+50
  mTuneCents = mpDisk->ReadInt8();
  //    1     signed      Semi tune  -50...+50
  mTuneSemitones = mpDisk->ReadInt8();
  //    4                 0,8,2,0
  mpDisk->ReadInt8();
  mpDisk->ReadInt8();
  mpDisk->ReadInt8();
  mpDisk->ReadInt8();
  //
  //    4     unsigned    Number of sample words
  mNumberOfSamples = mpDisk->ReadInt32();
  //    4     unsigned    Start marker
  mStartMarker = mpDisk->ReadInt32();
  //    4     unsigned    End marker
  mEndMarker = mpDisk->ReadInt32();
  //
  //    4     unsigned    Loop 1 marker
  //    2     unsigned    Loop 1 fine length   (65536ths)
  //    4     unsigned    Loop 1 coarse length (words)
  //    2     unsigned    Loop 1 time          (msec. or 9999=infinite)
  //
  //   84     [as above]  Loops 2 to 8
  //
  int i;
  for (i=0; i<8; i++)
    mLoops[i].Load(mpDisk);
  //    4                 0,0,255,255
  mpDisk->ReadInt32();
  //    2     unsigned    Sampling frequency
  mSamplingFrequency = mpDisk->ReadInt16();
  //    1     signed char         Loop tune offset -50...+50
  mLoopTuneOffset = mpDisk->ReadInt8();

  mImageOffset = mpParent->GetParent()->GetOffset() + mDirEntry.mStart * AKAI_BLOCK_SIZE + 150; // 150 is the size of the header

  //FIXME: no header validation yet implemented
  return (mHeaderOK = true);
}

bool AkaiSampleLoop::Load(DiskImage* pDisk)
{
  //    4     unsigned    Loop 1 marker
  mMarker = pDisk->ReadInt32();
  //    2     unsigned    Loop 1 fine length   (65536ths)
  mFineLength = pDisk->ReadInt16();
  //    4     unsigned    Loop 1 coarse length (words)
  mCoarseLength = pDisk->ReadInt32();
  //    2     unsigned    Loop 1 time          (msec. or 9999=infinite)
  mTime = pDisk->ReadInt16();
  return true;
}

//////////////////////////////////
// AkaiProgram:
AkaiProgram::AkaiProgram(DiskImage* pDisk, AkaiVolume* pParent, const AkaiDirEntry& DirEntry)
  : AkaiDiskElement(pDisk->GetPos())
{
  mpParent = pParent; 
  mpDisk = pDisk;
  mDirEntry = DirEntry;
  mpKeygroups = NULL;
  Load();
}

AkaiProgram::~AkaiProgram()
{
  if (mpKeygroups)
    delete[] mpKeygroups;
}

AkaiDirEntry AkaiProgram::GetDirEntry()
{
  return mDirEntry;
}

bool AkaiProgram::Load()
{
  // Read each of the program parameters
  uint temppos = mpDisk->GetPos();
  mpDisk->SetPos(mpParent->GetParent()->GetOffset() + mDirEntry.mStart * AKAI_BLOCK_SIZE );
  //    byte     description                 default     range/comments
  //   ---------------------------------------------------------------------------
  //     1       program ID                  1
  uint8_t ProgID = mpDisk->ReadInt8();
  if (ProgID != AKAI_PROGRAM_ID)
  {
    mpDisk->SetPos(temppos);
    return false;
  }
  //     2-3     first keygroup address      150,0       
  /*uint16_t KeygroupAddress =*/ mpDisk->ReadInt16();
  //     4-15    program name                10,10,10... AKAII character set
  char buffer[13];
  mpDisk->Read(buffer, 12, 1);
  AkaiToAscii(buffer, 12);
  mName = buffer;
  //     16      MIDI program number         0           0..127
  mMidiProgramNumber = mpDisk->ReadInt8();
  //     17      MIDI channel                0           0..15, 255=OMNI
  mMidiChannel = mpDisk->ReadInt8();
  //     18      polyphony                   15          1..16
  mPolyphony = mpDisk->ReadInt8();
  //     19      priority                    1           0=LOW 1=NORM 2=HIGH 3=HOLD
  mPriority = mpDisk->ReadInt8();
  //     20      low key                     24          24..127
  mLowKey = mpDisk->ReadInt8();
  //     21      high key                    127         24..127
  mHighKey = mpDisk->ReadInt8();
  //     22      octave shift                0           -2..2
  mOctaveShift = mpDisk->ReadInt8();
  //     23      aux output select           255         0..7, 255=OFF
  mAuxOutputSelect = mpDisk->ReadInt8();
  //     24      mix output level            99          0..99
  mMixOutputSelect = mpDisk->ReadInt8();
  //     25      mix output pan              0           -50..50
  mMixPan = mpDisk->ReadInt8();
  //     26      volume                      80          0..99
  mVolume = mpDisk->ReadInt8();
  //     27      vel>volume                  20          -50..50
  mVelocityToVolume = mpDisk->ReadInt8();
  //     28      key>volume                  0           -50..50
  mKeyToVolume = mpDisk->ReadInt8();
  //     29      pres>volume                 0           -50..50
  mPressureToVolume = mpDisk->ReadInt8();
  //     30      pan lfo rate                50          0..99
  mPanLFORate = mpDisk->ReadInt8();
  //     31      pan lfo depth               0           0..99
  mPanLFODepth = mpDisk->ReadInt8();
  //     32      pan lfo delay               0           0..99
  mPanLFODelay = mpDisk->ReadInt8();
  //     33      key>pan                     0           -50..50
  mKeyToPan = mpDisk->ReadInt8();
  //     34      lfo rate                    50          0..99
  mLFORate = mpDisk->ReadInt8();
  //     35      lfo depth                   0           0..99
  mLFODepth = mpDisk->ReadInt8();
  //     36      lfo delay                   0           0..99
  mLFODelay = mpDisk->ReadInt8();
  //     37      mod>lfo depth               30          0..99
  mModulationToLFODepth = mpDisk->ReadInt8();
  //     38      pres>lfo depth              0           0..99
  mPressureToLFODepth = mpDisk->ReadInt8();
  //     39      vel>lfo depth               0           0..99
  mVelocityToLFODepth = mpDisk->ReadInt8();
  //     40      bend>pitch                  2           0..12 semitones
  mBendToPitch = mpDisk->ReadInt8();
  //     41      pres>pitch                  0           -12..12 semitones
  mPressureToPitch = mpDisk->ReadInt8();
  //     42      keygroup crossfade          0           0=OFF 1=ON
  mKeygroupCrossfade = mpDisk->ReadInt8()?true:false;
  //     43      number of keygroups         1           1..99
  mNumberOfKeygroups = mpDisk->ReadInt8();
  //     44      (internal use)              0           program number
   mpDisk->ReadInt8();
  //     45-56   key temperament C,C#,D...   0           -25..25 cents
  uint i;
  for (i = 0; i<11; i++)
    mKeyTemperament[i] =  mpDisk->ReadInt8();
  //     57      fx output                   0           0=OFF 1=ON
  mFXOutput = mpDisk->ReadInt8()?true:false;
  //     58      mod>pan                     0           -50..50
  mModulationToPan = mpDisk->ReadInt8();
  //     59      stereo coherence            0           0=OFF 1=ON
  mStereoCoherence = mpDisk->ReadInt8()?true:false;
  //     60      lfo desync                  1           0=OFF 1=ON
  mLFODesync = mpDisk->ReadInt8()?true:false;
  //     61      pitch law                   0           0=LINEAR
  mPitchLaw = mpDisk->ReadInt8();
  //     62      voice re-assign             0           0=OLDEST 1=QUIETEST
  mVoiceReassign = mpDisk->ReadInt8();
  //     63      softped>volume              10          0..99
  mSoftpedToVolume = mpDisk->ReadInt8();
  //     64      softped>attack              10          0..99
  mSoftpedToAttack = mpDisk->ReadInt8();
  //     65      softped>filt                10          0..99
  mSoftpedToFilter = mpDisk->ReadInt8();
  //     66      tune cents                  0           -128..127 (-50..50 cents)
  mSoftpedToTuneCents = mpDisk->ReadInt8();
  //     67      tune semitones              0           -50..50
  mSoftpedToTuneSemitones = mpDisk->ReadInt8();
  //     68      key>lfo rate                0           -50..50
  mKeyToLFORate = mpDisk->ReadInt8();
  //     69      key>lfo depth               0           -50..50
  mKeyToLFODepth = mpDisk->ReadInt8();
  //     70      key>lfo delay               0           -50..50
  mKeyToLFODelay = mpDisk->ReadInt8();
  //     71      voice output scale          1           0=-6dB 1=0dB 2=+12dB
  mVoiceOutputScale = mpDisk->ReadInt8();
  //     72      stereo output scale         0           0=0dB 1=+6dB
  mStereoOutputScale = mpDisk->ReadInt8();
  //     73-150  (not used)

  // Read the key groups:
  if (mpKeygroups)
    delete[] mpKeygroups;
  mpKeygroups = new AkaiKeygroup[mNumberOfKeygroups];
  for (i = 0; i < mNumberOfKeygroups; i++)
  {
    // Go where the key group is on the disk:
    mpDisk->SetPos(mpParent->GetParent()->GetOffset() + mDirEntry.mStart * AKAI_BLOCK_SIZE + 150 * (i+1));
    if (!mpKeygroups[i].Load(mpDisk))
    {
      mpDisk->SetPos(temppos);
      return false;
    }
  }

  mpDisk->SetPos(temppos);
  return true;
}

uint AkaiProgram::ListSamples(std::list<String>& rSamples)
{
  return 0;
}

AkaiSample* AkaiProgram::GetSample(uint Index)
{
  return NULL;
}

AkaiSample* AkaiProgram::GetSample(const String& rName)
{
  return NULL;
}

// AkaiKeygroup:
bool AkaiKeygroup::Load(DiskImage* pDisk)
{
  uint i;
  //    byte     description                 default     range/comments
  //   ---------------------------------------------------------------------------
  //     1       keygroup ID                 2
  if (pDisk->ReadInt8() != AKAI_KEYGROUP_ID)
    return false;
  //     2-3     next keygroup address       44,1        300,450,600,750.. (16-bit)
  /*uint16_t NextKeygroupAddress =*/ pDisk->ReadInt16();
  //     4       low key                     24          24..127
  mLowKey = pDisk->ReadInt8();
  //     5       high key                    127         24..127
  mHighKey = pDisk->ReadInt8();
  //     6       tune cents                  0           -128..127 (-50..50 cents)
  mTuneCents = pDisk->ReadInt8();
  //     7       tune semitones              0           -50..50
  mTuneSemitones = pDisk->ReadInt8();
  //     8       filter                      99          0..99
  mFilter = pDisk->ReadInt8();
  //     9       key>filter                  12          0..24 semitone/oct
  mKeyToFilter = pDisk->ReadInt8();
  //     10      vel>filt                    0           -50..50
  mVelocityToFilter = pDisk->ReadInt8();
  //     11      pres>filt                   0           -50..50
  mPressureToFilter = pDisk->ReadInt8();
  //     12      env2>filt                   0           -50..50
  mEnveloppe2ToFilter = pDisk->ReadInt8();

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
  for (i=0; i<2; i++)
    mEnveloppes[i].Load(pDisk);

  //     29      vel>env2>filter             0           -50..50
  mVelocityToEnveloppe2ToFilter = pDisk->ReadInt8();
  //     30      env2>pitch                  0           -50..50
  mEnveloppe2ToPitch = pDisk->ReadInt8();
  //     31      vel zone crossfade          1           0=OFF 1=ON
  mVelocityZoneCrossfade = pDisk->ReadInt8()?true:false;
  //     32      vel zones used              4
  mVelocityZoneUsed = pDisk->ReadInt8();
  //     33      (internal use)              255
  pDisk->ReadInt8();
  //     34      (internal use)              255
  pDisk->ReadInt8();
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
  for (i=0; i<4; i++)
    mSamples[i].Load(pDisk);
  
  //     131     beat detune                 0           -50..50
  mBeatDetune = pDisk->ReadInt8();
  //     132     hold attack until loop      0           0=OFF 1=ON
  mHoldAttackUntilLoop = pDisk->ReadInt8()?true:false;
  //     133-136 sample 1-4 key tracking     0           0=TRACK 1=FIXED
  for (i=0; i<4; i++)
    mSampleKeyTracking[i] = pDisk->ReadInt8()?true:false;
  //     137-140 sample 1-4 aux out offset   0           0..7
  for (i=0; i<4; i++)
    mSampleAuxOutOffset[i] = pDisk->ReadInt8();
  //     141-148 vel>sample start            0           -9999..9999 (16-bit signed)
  for (i=0; i<4; i++)
    mVelocityToSampleStart[i] = pDisk->ReadInt8();
  //     149     vel>volume offset           0           -50..50
  for (i=0; i<4; i++)
    mVelocityToVolumeOffset[i] = pDisk->ReadInt8();
  //     150     (not used)

  return true;
}

bool AkaiEnveloppe::Load(DiskImage* pDisk)
{
  //     13      env1 attack                 0           0..99
  mAttack = pDisk->ReadInt8();
  //     14      env1 decay                  30          0..99
  mDecay = pDisk->ReadInt8();
  //     15      env1 sustain                99          0..99
  mSustain = pDisk->ReadInt8();
  //     16      env1 release                45          0..99
  mRelease = pDisk->ReadInt8();
  //     17      env1 vel>attack             0           -50..50
  mVelocityToAttack = pDisk->ReadInt8();
  //     18      env1 vel>release            0           -50..50 
  mVelocityToRelease = pDisk->ReadInt8();
  //     19      env1 offvel>release         0           -50..50
  mOffVelocityToRelease = pDisk->ReadInt8();
  //     20      env1 key>dec&rel            0           -50..50
  mKeyToDecayAndRelease = pDisk->ReadInt8();
  return true;
}

bool AkaiKeygroupSample::Load(DiskImage* pDisk)
{
  //     35-46   sample 1 name               10,10,10... AKAII character set
  char buffer[13];
  pDisk->Read(buffer, 12, 1);
  AkaiToAscii(buffer, 12);
  mName = buffer;

  //     47      low vel                     0           0..127
  mLowLevel = pDisk->ReadInt8();
  //     48      high vel                    127         0..127
  /*uint8_t mHighLevel =*/ pDisk->ReadInt8();
  //     49      tune cents                  0           -128..127 (-50..50 cents)
  /*int8_t mTuneCents =*/ pDisk->ReadInt8();
  //     50      tune semitones              0           -50..50
  /*int8_t mTuneSemitones =*/ pDisk->ReadInt8();
  //     51      loudness                    0           -50..+50
  /*int8_t mLoudness =*/ pDisk->ReadInt8();
  //     52      filter                      0           -50..+50
  /*int8_t mFilter =*/ pDisk->ReadInt8();
  //     53      pan                         0           -50..+50
  /*int8_t mPan =*/ pDisk->ReadInt8();
  //     54      loop mode                   0           0=AS_SAMPLE 1=LOOP_IN_REL 
  //                                                     2=LOOP_UNTIL_REL 3=NO_LOOP 
  //                                                     4=PLAY_TO_END
  /*uint8_t mLoopMode =*/ pDisk->ReadInt8();
  //     55      (internal use)              255
  pDisk->ReadInt8();
  //     56      (internal use)              255
  pDisk->ReadInt8();
  //     57-58   (internal use)              44,1
  pDisk->ReadInt16();
  //

  return true;
}

//////////////////////////////////
// AkaiVolume:
AkaiVolume::AkaiVolume(DiskImage* pDisk, AkaiPartition* pParent, const AkaiDirEntry& DirEntry)
  : AkaiDiskElement()
{
  mpDisk = pDisk;
  mpParent = pParent;
  mDirEntry = DirEntry;

  if (mDirEntry.mType != AKAI_TYPE_DIR_S1000 && mDirEntry.mType != AKAI_TYPE_DIR_S3000)
  {
    printf("Creating Unknown Volume type! %d\n",mDirEntry.mType);
#ifdef WIN32
    OutputDebugString("Creating Unknown Volume type!\n");
#endif
  }
}

AkaiVolume::~AkaiVolume()
{
  {
    std::list<AkaiProgram*>::iterator it;
    std::list<AkaiProgram*>::iterator end = mpPrograms.end();
    for (it = mpPrograms.begin(); it != end; it++)
      if (*it)
        (*it)->Release();
  }

  {
    std::list<AkaiSample*>::iterator it;
    std::list<AkaiSample*>::iterator end = mpSamples.end();
    for (it = mpSamples.begin(); it != end; it++)
      if (*it)
        (*it)->Release();
  }
}

uint AkaiVolume::ReadDir()
{
  uint i;
  if (mpPrograms.empty())
  {
    uint maxfiles = ReadFAT(mpDisk, mpParent,mDirEntry.mStart) ? AKAI_MAX_FILE_ENTRIES_S1000 : AKAI_MAX_FILE_ENTRIES_S3000;
    for (i = 0; i < maxfiles; i++) 
    {
      AkaiDirEntry DirEntry;
      ReadDirEntry(mpDisk, mpParent, DirEntry, mDirEntry.mStart, i); 
      DirEntry.mIndex = i;
      if (DirEntry.mType == 'p') 
      {
        AkaiProgram* pProgram = new AkaiProgram(mpDisk, this, DirEntry);
        pProgram->Acquire();
        mpPrograms.push_back(pProgram);
      }
      else if (DirEntry.mType == 's') 
      {
        AkaiSample* pSample = new AkaiSample(mpDisk, this, DirEntry);
        pSample->Acquire();
        mpSamples.push_back(pSample);
      }
    }
  }
  return (uint)(mpPrograms.size() + mpSamples.size());
}

uint AkaiVolume::ListPrograms(std::list<AkaiDirEntry>& rPrograms)
{
  rPrograms.clear();
  ReadDir();

  std::list<AkaiProgram*>::iterator it;
  std::list<AkaiProgram*>::iterator end = mpPrograms.end();
  for (it = mpPrograms.begin(); it != end; it++)
    if (*it)
      rPrograms.push_back((*it)->GetDirEntry());
  return (uint)rPrograms.size();
}

AkaiProgram* AkaiVolume::GetProgram(uint Index)
{
  uint i = 0;

  if (mpPrograms.empty())
  {
    std::list<AkaiDirEntry> dummy;
    ListPrograms(dummy);
  }

  std::list<AkaiProgram*>::iterator it;
  std::list<AkaiProgram*>::iterator end = mpPrograms.end();
  for (it = mpPrograms.begin(); it != end; it++)
  {
    if (*it && i == Index)
    {
      (*it)->Acquire();
      return *it;
    }
    i++;
  }
  return NULL;
}

AkaiProgram* AkaiVolume::GetProgram(const String& rName)
{
  if (mpPrograms.empty())
  {
    std::list<AkaiDirEntry> dummy;
    ListPrograms(dummy);
  }

  std::list<AkaiProgram*>::iterator it;
  std::list<AkaiProgram*>::iterator end = mpPrograms.end();
  for (it = mpPrograms.begin(); it != end; it++)
  {
    if (*it && rName == (*it)->GetDirEntry().mName)
    {
      (*it)->Acquire();
      return *it;
    }
  }
  return NULL;
}

uint AkaiVolume::ListSamples(std::list<AkaiDirEntry>& rSamples)
{
  rSamples.clear();
  ReadDir();

  std::list<AkaiSample*>::iterator it;
  std::list<AkaiSample*>::iterator end = mpSamples.end();
  for (it = mpSamples.begin(); it != end; it++)
    if (*it)
      rSamples.push_back((*it)->GetDirEntry());
  return (uint)rSamples.size();
}

AkaiSample* AkaiVolume::GetSample(uint Index)
{
  uint i = 0;

  if (mpSamples.empty())
  {
    std::list<AkaiDirEntry> dummy;
    ListSamples(dummy);
  }

  std::list<AkaiSample*>::iterator it;
  std::list<AkaiSample*>::iterator end = mpSamples.end();
  for (it = mpSamples.begin(); it != end; it++)
  {
    if (*it && i == Index)
    {
      (*it)->Acquire();
      return *it;
    }
    i++;
  }
  return NULL;
}

AkaiSample* AkaiVolume::GetSample(const String& rName)
{
  if (mpSamples.empty())
  {
    std::list<AkaiDirEntry> dummy;
    ListSamples(dummy);
  }

  std::list<AkaiSample*>::iterator it;
  std::list<AkaiSample*>::iterator end = mpSamples.end();
  for (it = mpSamples.begin(); it != end; it++)
  {
    if (*it && rName == (*it)->GetDirEntry().mName)
    {
      (*it)->Acquire();
      return *it;
    }
  }
  return NULL;
}

AkaiDirEntry AkaiVolume::GetDirEntry()
{
  return mDirEntry;
}

bool AkaiVolume::IsEmpty()
{
  return ReadDir() == 0;
}


//////////////////////////////////
// AkaiPartition:
AkaiPartition::AkaiPartition(DiskImage* pDisk, AkaiDisk* pParent)
{
  mpDisk = pDisk;
  mpParent = pParent;
}

AkaiPartition::~AkaiPartition()
{
  std::list<AkaiVolume*>::iterator it;
  std::list<AkaiVolume*>::iterator end = mpVolumes.end();
  for (it = mpVolumes.begin(); it != end; it++)
    if (*it)
      (*it)->Release();
}

uint AkaiPartition::ListVolumes(std::list<AkaiDirEntry>& rVolumes)
{
  rVolumes.clear();
  uint i;
  if (mpVolumes.empty())
  {
    for (i = 0; i < AKAI_MAX_DIR_ENTRIES; i++)
    {
      AkaiDirEntry DirEntry;
      ReadDirEntry(mpDisk, this, DirEntry, AKAI_ROOT_ENTRY_OFFSET, i);
      DirEntry.mIndex = i;
      if (DirEntry.mType == AKAI_TYPE_DIR_S1000 || DirEntry.mType == AKAI_TYPE_DIR_S3000)
      {
        AkaiVolume* pVolume = new AkaiVolume(mpDisk, this, DirEntry);
        pVolume->Acquire();
        if (!pVolume->IsEmpty())
        {
          mpVolumes.push_back(pVolume);
          rVolumes.push_back(DirEntry);
        }
        else
          pVolume->Release();
      }
    }
  }
  else
  {
    std::list<AkaiVolume*>::iterator it;
    std::list<AkaiVolume*>::iterator end = mpVolumes.end();
    for (it = mpVolumes.begin(); it != end; it++)
      if (*it)
        rVolumes.push_back((*it)->GetDirEntry());
  }
  return (uint)rVolumes.size();
}

AkaiVolume* AkaiPartition::GetVolume(uint Index)
{
  uint i = 0;

  if (mpVolumes.empty())
  {
    std::list<AkaiDirEntry> dummy;
    ListVolumes(dummy);
  }

  std::list<AkaiVolume*>::iterator it;
  std::list<AkaiVolume*>::iterator end = mpVolumes.end();
  for (it = mpVolumes.begin(); it != end; it++)
  {
    if (*it && i == Index)
    {
      (*it)->Acquire();
      return *it;
    }
    i++;
  }
  return NULL;
}

AkaiVolume* AkaiPartition::GetVolume(const String& rName)
{
  if (mpVolumes.empty())
  {
    std::list<AkaiDirEntry> dummy;
    ListVolumes(dummy);
  }

  std::list<AkaiVolume*>::iterator it;
  std::list<AkaiVolume*>::iterator end = mpVolumes.end();
  for (it = mpVolumes.begin(); it != end; it++)
  {
    if (*it && rName == (*it)->GetDirEntry().mName)
    {
      (*it)->Acquire();
      return *it;
    }
  }
  return NULL;
}

bool AkaiPartition::IsEmpty()
{
  std::list<AkaiDirEntry> Volumes;
  return ListVolumes(Volumes) == 0;
}


//////////////////////////////////
// AkaiDisk:
AkaiDisk::AkaiDisk(DiskImage* pDisk)
{
  mpDisk = pDisk;
}

AkaiDisk::~AkaiDisk()
{
  std::list<AkaiPartition*>::iterator it;
  std::list<AkaiPartition*>::iterator end = mpPartitions.end();
  for (it = mpPartitions.begin(); it != end ; it++)
    if (*it)
      (*it)->Release();
}

uint AkaiDisk::GetPartitionCount()
{
  if (!mpPartitions.empty())
    return (uint)mpPartitions.size();

  uint offset = 0;
  uint16_t size = 0;
  while (size != AKAI_PARTITION_END_MARK && size != 0x0fff && size != 0xffff
         && size<30720 && mpPartitions.size()<9)
  {
    // printf("size: %x\t",size);
    AkaiPartition* pPartition = new AkaiPartition(mpDisk,this);
    pPartition->Acquire();
    pPartition->SetOffset(offset);

    if (!pPartition->IsEmpty())
    {
      mpPartitions.push_back(pPartition); // Add this partitions' offset to the list.
    }

    mpDisk->SetPos(offset);
    if (!mpDisk->ReadInt16(&size))
      return (uint)mpPartitions.size();
    uint t = size;
    offset +=  AKAI_BLOCK_SIZE * t;
//		printf("new offset %d / size %d\n",offset,size);
  }

  return (uint)mpPartitions.size();
}

AkaiPartition* AkaiDisk::GetPartition(uint count)
{
  std::list<AkaiPartition*>::iterator it;
  std::list<AkaiPartition*>::iterator end = mpPartitions.end();
  uint i = 0;
  for (i = 0, it = mpPartitions.begin(); it != end && i != count; i++) it++;

  if (i != count || it == end)
    return NULL;

  (*it)->Acquire();
  return *it;
}

/////////////////////////
// AkaiDiskElement

int AkaiDiskElement::ReadFAT(DiskImage* pDisk, AkaiPartition* pPartition, int block)
{ 
  int16_t value = 0;
  pDisk->SetPos(pPartition->GetOffset()+AKAI_FAT_OFFSET + block*2); 
  pDisk->Read(&value, 2,1); 
  return value;
}


bool AkaiDiskElement::ReadDirEntry(DiskImage* pDisk, AkaiPartition* pPartition, AkaiDirEntry& rEntry, int block, int pos)
{
  char buffer[13];

  if (block == AKAI_ROOT_ENTRY_OFFSET)
  {
    pDisk->SetPos(pPartition->GetOffset()+AKAI_DIR_ENTRY_OFFSET + pos * AKAI_DIR_ENTRY_SIZE);
    pDisk->Read(buffer, 12, 1);
    AkaiToAscii(buffer, 12);
    rEntry.mName = buffer;

    pDisk->ReadInt16(&rEntry.mType);
    pDisk->ReadInt16(&rEntry.mStart);
    rEntry.mSize = 0;
    return true;
  }
  else
  {
    if (pos < 341)
    {
      pDisk->SetPos(block * AKAI_BLOCK_SIZE + pos * AKAI_FILE_ENTRY_SIZE + pPartition->GetOffset());
    }
    else 
    {
      int temp; 
      temp = ReadFAT(pDisk, pPartition, block);
      pDisk->SetPos(pPartition->GetOffset()+temp * AKAI_BLOCK_SIZE + (pos - 341) * AKAI_FILE_ENTRY_SIZE);
    }

    pDisk->Read(buffer, 12, 1);
    AkaiToAscii(buffer, 12);
    rEntry.mName = buffer; 

    uint8_t t1,t2,t3;
    pDisk->SetPos(4,akai_stream_curpos);
    pDisk->Read(&t1, 1,1);
    rEntry.mType = t1; 

    pDisk->Read(&t1, 1,1);
    pDisk->Read(&t2, 1,1);
    pDisk->Read(&t3, 1,1);
    rEntry.mSize = (unsigned char)t1 | ((unsigned char)t2 <<8) | ((unsigned char)t3<<16); 

    pDisk->ReadInt16(&rEntry.mStart,1);
    return true; 
  } // not root block
}

void AkaiDiskElement::AkaiToAscii(char * buffer, int length) 
{ 
  int i;

  for (i = 0; i < length; i++)
  {
    if (buffer[i]>=0 && buffer[i]<=9)
      buffer[i] +=48;
    else if (buffer[i]==10)
      buffer[i] = 32;
    else if (buffer[i]>=11 && buffer[i]<=36)
      buffer[i] = 64+(buffer[i]-10);
    else
      buffer[i] = 32;
  }
  buffer[length] = '\0';
  while (length-- > 0 && buffer[length] == 32)
  {
    // This block intentionaly left blank :)
  }
  buffer[length+1] = '\0';
}


//////////////////////////////////
// DiskImage:

DiskImage::DiskImage(const char* path)
{
  Init();
  OpenStream(path);
}

#ifdef _CARBON_
kern_return_t FindEjectableCDMedia( io_iterator_t *mediaIterator )
{
  kern_return_t  kernResult;
  mach_port_t   masterPort;
  CFMutableDictionaryRef classesToMatch;

  kernResult = IOMasterPort( MACH_PORT_NULL, &masterPort );
  if ( KERN_SUCCESS != kernResult )
  {
    printf( "IOMasterPort returned %d\n", kernResult );
  }

  classesToMatch = IOServiceMatching( kIOCDMediaClass );
  if ( classesToMatch == NULL )
  {
      printf( "IOServiceMatching returned a NULL dictionary.\n" );
  }
  else
  {
    CFDictionarySetValue( classesToMatch, CFSTR( kIOMediaEjectableKey ), kCFBooleanTrue );
  }

  kernResult = IOServiceGetMatchingServices( masterPort, classesToMatch, mediaIterator );
  if ( KERN_SUCCESS != kernResult )
  {
      printf( "IOServiceGetMatchingServices returned %d\n", kernResult );
  }

  return kernResult;
}

kern_return_t GetBSDPath( io_iterator_t mediaIterator, char *bsdPath, CFIndex maxPathSize )
{
  io_object_t  nextMedia;
  kern_return_t kernResult = KERN_FAILURE;

  *bsdPath = '\0';

  nextMedia = IOIteratorNext( mediaIterator );
  if ( nextMedia )
  {
    CFTypeRef bsdPathAsCFString;

    bsdPathAsCFString = IORegistryEntryCreateCFProperty( nextMedia,
                                                         CFSTR( kIOBSDNameKey ), 
                                                         kCFAllocatorDefault,
                                                         0 );
    if ( bsdPathAsCFString )
    {
      size_t devPathLength;

      strcpy( bsdPath, _PATH_DEV );
      strcat( bsdPath, "r" );

      devPathLength = strlen( bsdPath );
      
      if ( CFStringGetCString( (__CFString*)bsdPathAsCFString,
                               bsdPath + devPathLength,
                               maxPathSize - devPathLength,
                               kCFStringEncodingASCII ) )
      {
          printf( "BSD path: %s\n", bsdPath );
          kernResult = KERN_SUCCESS;
      }

      CFRelease( bsdPathAsCFString );
    }
    IOObjectRelease( nextMedia );
  }
  
  return kernResult;
}

struct _CDMSF 
{
  u_char   minute;
  u_char   second;
  u_char   frame;
};

/* converting from minute-second to logical block entity */
#define MSF_TO_LBA(msf) (((((msf).minute * 60UL) + (msf).second) * 75UL) + (msf).frame - 150)

struct _CDTOC_Desc
{
  u_char        session;
  u_char        ctrl_adr; /* typed to be machine and compiler independent */
  u_char        tno;
  u_char        point;
  struct _CDMSF  address;
  u_char        zero;
  struct _CDMSF  p;
};

struct _CDTOC 
{
  u_short            length; /* in native cpu endian */
  u_char             first_session;
  u_char             last_session;
  struct _CDTOC_Desc  trackdesc[1];
};

static struct _CDTOC * ReadTOC (const char * devpath )
{ 
  struct _CDTOC * toc_p = NULL;
  io_iterator_t iterator = 0;
  io_registry_entry_t service = 0;
  CFDictionaryRef properties = 0;
  CFDataRef data = 0;
  mach_port_t port = 0; 
  char * devname;
  if (( devname = strrchr( devpath, '/' )) != NULL ) 
  {
    ++devname;
  } 
  else
  {
    devname = ( char *) devpath;
  }
  
  if ( IOMasterPort(bootstrap_port, &port ) != KERN_SUCCESS )
  {
    printf(  "IOMasterPort failed\n" ); goto Exit ;
  }
  if ( IOServiceGetMatchingServices( port, IOBSDNameMatching( port, 0, devname ),
                                     &iterator ) != KERN_SUCCESS ) 
  {
    printf( "IOServiceGetMatchingServices failed\n" ); goto Exit ;
  }

    char buffer[1024];
    IOObjectGetClass(iterator,buffer);
    printf("Service: %s\n",buffer);
     
  
  IOIteratorReset (iterator);
  service = IOIteratorNext( iterator );

  IOObjectRelease( iterator );

  iterator = 0; 
  while ( service && !IOObjectConformsTo( service, "IOCDMedia" ))
  { 
    char buffer[1024];
    IOObjectGetClass(service,buffer);
    printf("Service: %s\n",buffer);

    if ( IORegistryEntryGetParentIterator( service, kIOServicePlane, &iterator ) != KERN_SUCCESS ) 
    {
      printf( "IORegistryEntryGetParentIterator failed\n" );
      goto Exit ;
    }

    IOObjectRelease( service );
    service = IOIteratorNext( iterator );
    IOObjectRelease( iterator );

  }
  if ( service == NULL ) 
  {
    printf(  "CD media not found\n" ); goto Exit ;
  }
  if ( IORegistryEntryCreateCFProperties( service, (__CFDictionary **) &properties,
                                          kCFAllocatorDefault,
                                          kNilOptions ) != KERN_SUCCESS ) 
  {
    printf( "IORegistryEntryGetParentIterator failed\n" ); goto Exit ;
  }

  data = (CFDataRef) CFDictionaryGetValue( properties, CFSTR( "TOC" ) ); 
  if ( data == NULL ) 
  {
    printf(  "CFDictionaryGetValue failed\n" ); goto Exit ;
  } 
  else
  {

    CFRange range;
    CFIndex buflen;

    buflen = CFDataGetLength( data ) + 1;
    range = CFRangeMake( 0, buflen );
    toc_p = ( struct _CDTOC *) malloc( buflen );
    if ( toc_p == NULL )
    {
      printf( "Out of memory\n" ); goto Exit ;
    } 
    else 
    {
      CFDataGetBytes( data, range, ( unsigned char *) toc_p );
    } 

    /*
    fprintf( stderr, "Table of contents\n length %d first %d last %d\n",
            toc_p->length, toc_p->first_session, toc_p->last_session );
      */

    CFRelease( properties );

  }
Exit :
  if ( service )
  {
    IOObjectRelease( service );
  }

  return toc_p;
}
#endif // _CARBON_

DiskImage::DiskImage(int disk)
{
  Init();
#ifdef _WIN32_
  char buffer[1024];
  sprintf(buffer,"%c:\\",'A'+disk);
  mSize = GetFileSize(buffer,NULL);

  sprintf(buffer,"\\\\.\\%c:",'a'+disk);
  mFile = CreateFile(buffer, GENERIC_READ,0,NULL,OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS,NULL);

  DWORD junk;
  DISK_GEOMETRY dg;
  DISK_GEOMETRY* pdg = &dg;
  BOOL res = DeviceIoControl(mFile,
    IOCTL_DISK_GET_DRIVE_GEOMETRY,
    NULL, 0,
    pdg, sizeof(*pdg),
    &junk,
    NULL);

  if (res)
  {
    mSize = dg.BytesPerSector * dg.SectorsPerTrack * dg.TracksPerCylinder * dg.Cylinders.LowPart;
    mClusterSize = dg.BytesPerSector;
  }
#elif defined(_CARBON_) || defined(__APPLE__)
  kern_return_t  kernResult;
  io_iterator_t mediaIterator;
  char  bsdPath[ MAXPATHLEN ];
  kernResult = FindEjectableCDMedia( &mediaIterator );
  kernResult = GetBSDPath( mediaIterator, bsdPath, sizeof( bsdPath ) );
  if ( bsdPath[ 0 ] != '\0' )
  {
        strcpy(bsdPath,"/dev/rdisk1s0");

      struct _CDTOC * toc = ReadTOC( bsdPath );
      if ( toc )
      {
        size_t toc_entries = ( toc->length - 2 ) / sizeof (struct _CDTOC_Desc );

        int start_sector = -1;
        int data_track = -1;
        // Iterate through the list backward. Pick the first data track and
        // get the address of the immediately previous (or following depending
        // on how you look at it).  The difference in the sector numbers
        // is returned as the sized of the data track.
        for (ssize_t i=toc_entries - 1; i>=0; i-- )
        {
          if ( start_sector != -1 )
          {
            start_sector = MSF_TO_LBA(toc->trackdesc[i].p) - start_sector;
            break ;

          }
          if (( toc->trackdesc[i].ctrl_adr >> 4) != 1 )
            continue ;
          if ( toc->trackdesc[i].ctrl_adr & 0x04 )
          {
            data_track = toc->trackdesc[i].point;
            start_sector = MSF_TO_LBA(toc->trackdesc[i].p);
          }
        }

        free( toc );
        if ( start_sector == -1 )
        {
          start_sector = 0;
        }
            }
//      mSize = start_sector;
//          printf("size %d\n",mSize);


    mFile = open(bsdPath,O_RDONLY);
    if (!mFile)
      printf("Error while opening file: %s\n",bsdPath);
    else
    {
      printf("opened file: %s\n",bsdPath);

            mSize = (int) lseek(mFile,1000000000,SEEK_SET);
            printf("size %d\n",mSize);
            lseek(mFile,0,SEEK_SET);

            mSize = 700 * 1024 * 1024;

    }
  }
  if ( mediaIterator )
  {
    IOObjectRelease( mediaIterator );
  }
#elif LINUX
  OpenStream("/dev/cdrom");
#endif
}

void DiskImage::Init()
{
  mFile        = 0;
  mPos         = 0;
  mCluster     = (uint)-1;
  mStartFrame  = -1;
  mEndFrame    = -1;
#ifdef WIN32
  mpCache = (char*) VirtualAlloc(NULL,mClusterSize,MEM_COMMIT,PAGE_READWRITE);
#else
  mpCache = NULL; // we allocate the cache later when we know what type of media we access
#endif
}

DiskImage::~DiskImage()
{
#ifdef WIN32
  if (mFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(mFile);
  }
#elif defined _CARBON_ || defined(__APPLE__) || LINUX
  if (mFile)
  {
    close(mFile);
  }
#endif
  if (mpCache)
  {
#ifdef WIN32
    VirtualFree(mpCache, 0, MEM_RELEASE);
#elif defined(_CARBON_) || defined(__APPLE__) || LINUX
    free(mpCache);
#endif
  }
}

akai_stream_state_t DiskImage::GetState() const
{
  if (!mFile)       return akai_stream_closed;
  if (mPos > mSize) return akai_stream_end_reached;
  return akai_stream_ready;
}

int DiskImage::GetPos() const
{
  return mPos;
}

int DiskImage::SetPos(int Where, akai_stream_whence_t Whence)
{
//  printf("setpos %d\n",Where);
  int w = Where;
  switch (Whence)
  {
  case akai_stream_start:
    mPos = w;
    break;
  /*case eStreamRewind:
    w = -w;*/
  case akai_stream_curpos:
    mPos += w;
    break;
  case akai_stream_end:
    mPos = mSize - w;
    break;
  }
//  if (mPos > mSize)
//    mPos = mSize;
  if (mPos < 0)
    mPos = 0;
  return mPos;
}

int DiskImage::Available (uint WordSize)
{
  return (mSize-mPos)/WordSize;
}

int DiskImage::Read(void* pData, uint WordCount, uint WordSize)
{
  int readbytes = 0;
  int sizetoread = WordCount * WordSize;

  while (sizetoread > 0) {
    if (mSize <= mPos) return readbytes / WordSize;
    int requestedCluster = (mRegularFile) ? mPos / mClusterSize
                                          : mPos / mClusterSize + mStartFrame;
    if (mCluster != requestedCluster) { // read the requested cluster into cache
      mCluster = requestedCluster;
#ifdef WIN32
      if (mCluster * mClusterSize != SetFilePointer(mFile, mCluster * mClusterSize, NULL, FILE_BEGIN)) {
        printf("ERROR: couldn't seek device!\n");
#if 0 // FIXME: endian correction is missing correct detection
        if ((readbytes > 0) && (mEndian != eEndianNative)) {
          switch (WordSize) {
            case 2: bswap_16_s ((uint16*)pData, readbytes); break;
            case 4: bswap_32_s ((uint32*)pData, readbytes); break;
            case 8: bswap_64_s ((uint64*)pData, readbytes); break;
          }
        }
#endif
        return readbytes / WordSize;
      }
      DWORD size;
      ReadFile(mFile, mpCache, mClusterSize, &size, NULL);
#elif defined(_CARBON_) || defined(__APPLE__) || LINUX
      if (mCluster * mClusterSize != lseek(mFile, mCluster * mClusterSize, SEEK_SET))
        return readbytes / WordSize;
//          printf("trying to read %d bytes from device!\n",mClusterSize);
      /*int size =*/ read(mFile, mpCache, mClusterSize);
//          printf("read %d bytes from device!\n",size);
#endif
    }
//      printf("read %d bytes at pos %d\n",WordCount*WordSize,mPos);

    int currentReadSize = sizetoread;
    int posInCluster = mPos % mClusterSize;
    if (currentReadSize > mClusterSize - posInCluster) // restrict to this current cached cluster.
      currentReadSize = mClusterSize - posInCluster;

    memcpy((uint8_t*)pData + readbytes, mpCache + posInCluster, currentReadSize);

    mPos       += currentReadSize;
    readbytes  += currentReadSize;
    sizetoread -= currentReadSize;
//      printf("new pos %d\n",mPos);
  }

#if 0 // FIXME: endian correction is missing correct detection
  if ((readbytes > 0) && (mEndian != eEndianNative))
    switch (WordSize)
    {
      case 2: bswap_16_s ((uint16_t*)pData, readbytes); break;
      case 4: bswap_32_s ((uint32_t*)pData, readbytes); break;
      case 8: bswap_64_s ((uint64_t*)pData, readbytes); break;
    }
#endif

  return readbytes / WordSize;
}

void DiskImage::ReadInt8(uint8_t* pData, uint WordCount) {
  Read(pData, WordCount, 1);
}

void DiskImage::ReadInt16(uint16_t* pData, uint WordCount) {
  int i;
  for (i = 0; i < WordCount; i++) {
    *(pData + i) = ReadInt16();
  }
}

void DiskImage::ReadInt32(uint32_t* pData, uint WordCount) {
  int i;
  for (i = 0; i < WordCount; i++) {
    *(pData + i) = ReadInt32();
  }
}

int DiskImage::ReadInt8(uint8_t* pData) {
  return Read(pData, 1, 1);
}

int DiskImage::ReadInt16(uint16_t* pData) {
  int result = Read(pData, 1, 2);
#if WORDS_BIGENDIAN
  swapBytes_16(pData);
#endif
  return result;
}

int DiskImage::ReadInt32(uint32_t* pData) {
  int result = Read(pData, 1, 4);
#if WORDS_BIGENDIAN
  swapBytes_32(pData);
#endif
  return result;
}

uint8_t DiskImage::ReadInt8()
{
  uint8_t word;
  Read(&word,1,1);
  return word;
}

uint16_t DiskImage::ReadInt16()
{
  uint16_t word;
  Read(&word,1,2);
#if WORDS_BIGENDIAN
  swapBytes_16(&word);
#endif
  return word;
}

uint32_t DiskImage::ReadInt32()
{
  uint32_t word;
  Read(&word,1,4);
#if WORDS_BIGENDIAN
  swapBytes_32(&word);
#endif
  return word;
}

void DiskImage::OpenStream(const char* path) {
#ifdef WIN32
  mFile = CreateFile(path, GENERIC_READ,0,NULL,OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS,NULL);
  BY_HANDLE_FILE_INFORMATION FileInfo;
  GetFileInformationByHandle(mFile,&FileInfo);
  mSize = FileInfo.nFileSizeLow;
#elif defined(_CARBON_) || defined(__APPLE__) || LINUX
  struct stat filestat;
  stat(path,&filestat);
  mFile = open(path, O_RDONLY | O_NONBLOCK);
  if (mFile <= 0) {
    printf("Can't open %s\n", path);
    mFile = 0;
    return;
  }
  // TODO: we should also check here if 'path' is a link or something
  if (S_ISREG(filestat.st_mode)) { // regular file
    printf("Using regular Akai image file.\n");
    mRegularFile = true;
    mSize        = (int) filestat.st_size;
    mClusterSize = DISK_CLUSTER_SIZE;
    mpCache = (char*) malloc(mClusterSize);
  } else { // CDROM
#if defined(_CARBON_) || defined(__APPLE__)
    printf("Can't open %s: not a regular file\n", path);
#else // Linux ...
    mRegularFile = false;
    mClusterSize = CD_FRAMESIZE;
    mpCache = (char*) malloc(mClusterSize);

    struct cdrom_tochdr   tochdr;
    struct cdrom_tocentry tocentry;
    int prev_addr = 0;
    if (ioctl(mFile, CDROMREADTOCHDR, (unsigned long)&tochdr) < 0) {
      printf("Trying to read TOC of %s failed\n", path);
      close(mFile);
      mFile = 0;
      return;
    }
    printf("Total tracks: %d\n", tochdr.cdth_trk1);

    /* we access the CD with logical blocks as entity */
    tocentry.cdte_format = CDROM_LBA;

    int firstDataTrack = -1;
    int start, end, length;

    /* we pick up the lowest data track by iterating backwards, starting with Lead Out */
    for (int t = tochdr.cdth_trk1; t >= 0; t--) {
      tocentry.cdte_track = (t == tochdr.cdth_trk1) ? CDROM_LEADOUT : t + 1;
      if (ioctl(mFile, CDROMREADTOCENTRY, (unsigned long)&tocentry) < 0){
        printf("Failed to read TOC entry for track %d\n", tocentry.cdte_track);
        close(mFile);
        mFile = 0;
        return;
      }
      if (tocentry.cdte_track == CDROM_LEADOUT) {
          printf("Lead Out: Start(LBA)=%d\n", tocentry.cdte_addr.lba);
      }
      else {
        printf("Track %d: Start(LBA)=%d End(LBA)=%d Length(Blocks)=%d ",
               tocentry.cdte_track, tocentry.cdte_addr.lba, prev_addr - 1, prev_addr - tocentry.cdte_addr.lba);
        if (tocentry.cdte_ctrl & CDROM_DATA_TRACK) {
          printf("Type: Data\n");
          firstDataTrack = tocentry.cdte_track;
          start  = tocentry.cdte_addr.lba;
          end    = prev_addr - 1;
          length = prev_addr - tocentry.cdte_addr.lba;
        }
        else printf("Type: Audio\n");
      }
      prev_addr = tocentry.cdte_addr.lba;
    }

    if (firstDataTrack == -1) {
      printf("Sorry, no data track found on %s\n", path);
      close(mFile);
      mFile = 0;
      return;
    }

    printf("Ok, I'll pick track %d\n", firstDataTrack);
    mStartFrame = start;
    mEndFrame   = end;
    mSize       = length * CD_FRAMESIZE;
#endif
  } // CDROM
#endif
}

bool DiskImage::WriteImage(const char* path)
{
#if defined(_CARBON_) || defined(__APPLE__) || LINUX
  const uint bufferSize = 524288; // 512 kB
  int fOut = open(path, O_WRONLY | O_NONBLOCK | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR | S_IRGRP);
  if (mFile <= 0) {
    printf("Can't open output file %s\n", path);
    return false;
  }
  uint8_t* pBuffer = new uint8_t[bufferSize];
  SetPos(0);
  while (Available() > 0) {
    int readBytes = Read(pBuffer,bufferSize,1);
    if (readBytes > 0) write(fOut,pBuffer,readBytes);
  }
  delete[] pBuffer;
  close(fOut);
  return true;
#endif // _CARBON_ || LINUX
  return false;
}

inline void DiskImage::swapBytes_16(void* Word) {
  uint8_t byteCache;
  byteCache = *((uint8_t*) Word);
  *((uint8_t*) Word)     = *((uint8_t*) Word + 1);
  *((uint8_t*) Word + 1) = byteCache;
}

inline void DiskImage::swapBytes_32(void* Word) {
  uint8_t byteCache;
  byteCache = *((uint8_t*) Word);
  *((uint8_t*) Word)     = *((uint8_t*) Word + 3);
  *((uint8_t*) Word + 3) = byteCache;
  byteCache = *((uint8_t*) Word + 1);
  *((uint8_t*) Word + 1) = *((uint8_t*) Word + 2);
  *((uint8_t*) Word + 2) = byteCache;
}
