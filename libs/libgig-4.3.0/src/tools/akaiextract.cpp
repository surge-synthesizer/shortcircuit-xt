/*
  libakai - C++ cross-platform akai sample disk reader
  Copyright (C) 2002-2003 Sébastien Métrot

  Linux port by Christian Schoenebeck <cuse@users.sourceforge.net> 2003-2015

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
// akaiextract.cpp : Defines the entry point for the console application.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
# define _WIN32_WINNT 0x0500
# include <windows.h>
# include <conio.h>
#else
# include <errno.h>
# include <dlfcn.h>
#endif

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

// for testing the disk streaming methods
#define USE_DISK_STREAMING 1

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>

#include "../Akai.h"

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

int writeWav(AkaiSample* sample, const char* filename, void* samples, long samplecount, int channels = 1, int bitdepth = 16, long rate = 44100);
void ConvertAkaiToAscii(char * buffer, int length); // debugging purpose only

using namespace std;

template<class T> inline std::string ToString(T o) {
    std::stringstream ss;
    ss << o;
    return ss.str();
}

void PrintLastError(char* file, int line)
{
#ifdef WIN32
  LPVOID lpMsgBuf;
  FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0,
      NULL
  );
  // Display the string.
  char buf[2048];
  sprintf(buf,"%s(%d): %s",file, line, (LPCTSTR)lpMsgBuf);
  printf("%s",buf);
  OutputDebugString(buf);
  // Free the buffer.
  LocalFree( lpMsgBuf );
#endif
}

#define SHOWERROR PrintLastError(__FILE__,__LINE__)

static void printUsage() {
#ifdef WIN32
    const wchar_t* msg =
        L"akaiextract <source-drive-letter>: <destination-dir>\n"
        "by S\351bastien M\351trot (meeloo@meeloo.net)\n\n"
        "Reads an AKAI media (i.e. CDROM, ZIP disk) and extracts all its audio\n"
        "samples as .wav files to the given output directory. If the given output\n"
        "directory does not exist yet, then it will be created.\n\n"
        "Available types of your drives:\n";
    DWORD n = wcslen(msg);
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg, n, &n, NULL);
    char rootpath[4]="a:\\";
    for (char drive ='a'; drive <= 'z'; drive++) {
        rootpath[0] = drive;
        int type = GetDriveType(rootpath);
        if (type != DRIVE_NO_ROOT_DIR) {
            printf("Drive %s is a ", rootpath);
            switch(type) {
                case DRIVE_UNKNOWN: printf("Unknown\n"); break;
                case DRIVE_NO_ROOT_DIR: printf("Unavailable\n");
                case DRIVE_REMOVABLE: printf("removable disk\n"); break;
                case DRIVE_FIXED: printf("fixed disk\n"); break;
                case DRIVE_REMOTE: printf("remote (network) drive\n"); break;
                case DRIVE_CDROM: printf("CD-ROM drive\n"); break;
                case DRIVE_RAMDISK: printf("RAM disk\n"); break;
            }
        }
    }
    printf("\n");
#elif defined _CARBON_
    setlocale(LC_CTYPE, "");
    printf("%ls",
        L"akaiextract [<source-path>] <destination-dir>\n"
        "by S\351bastien M\351trot (meeloo@meeloo.net)\n\n"
        "Reads an AKAI media (i.e. CDROM, ZIP disk) and extracts all its audio\n"
        "samples as .wav files to the given output directory. If the given output\n"
        "directory does not exist yet, then it will be created.\n\n"
        "<source-path> - path of the source AKAI image file, if omitted CDROM\n"
        "                drive ('/dev/rdisk1s0') will be accessed\n\n"
        "<destination-dir> - target directory where samples will be written to\n\n"
    );
#else
    setlocale(LC_CTYPE, "");
    printf("%ls",
        L"akaiextract <source-path> <destination-dir>\n"
        "by S\351bastien M\351trot (meeloo@meeloo.net)\n"
        "Linux port by Christian Schoenebeck\n\n"
        "Reads an AKAI media (i.e. CDROM, ZIP disk) and extracts all its audio\n"
        "samples as .wav files to the given output directory. If the given output\n"
        "directory does not exist yet, then it will be created.\n\n"
        "<source-path> - path of the source drive or image file (i.e. /dev/cdrom)\n\n"
        "<destination-dir> - target directory where samples will be written to\n\n"
    );
#endif
}

int main(int argc, char** argv) {
#if defined _CARBON_
    if (argc < 2) {
        printUsage();
        return -1;
    }
#else
    if (argc < 3) {
        printUsage();
        return -1;
    }
#endif

    // open input source
    DiskImage* pImage = NULL;
#ifdef WIN32
    char drive = toupper(*(argv[1]))-'A';
    printf("opening drive %c:\n",drive+'a'); 
    pImage = new DiskImage(drive);
#elif defined _CARBON_
    if (argc == 2) {
        printf("Opening AKAI media at '/dev/rdisk1s0'\n");
        pImage = new DiskImage((int)0); // arbitrary int, argument will be ignored
    } else {
        printf("Opening source AKAI image file at '%s'\n", argv[1]);
        pImage = new DiskImage(argv[1]);
    }
#else
    printf("Opening AKAI media or image file at '%s'\n", argv[1]);
    pImage = new DiskImage(argv[1]);
#endif

    // determine output directory path
#if defined _CARBON_
    const char* outPath = (argc == 2) ? argv[1] : argv[2];
#else
    const char* outPath = argv[2];
#endif

  AkaiDisk* pAkai = new AkaiDisk(pImage);

  printf("Reading Akai file entries (this may take a while)...\n");
  printf("Partitions: %d\n", pAkai->GetPartitionCount());
  printf("Partitions list:\n");
  long totalSamplesSize = 0, totalSamples = 0;
  uint i;
  for (i = 0; i < pAkai->GetPartitionCount(); i++)
  {
    printf("%2.2d: Partition %c\n",i,'A'+i);
    AkaiPartition* pPartition = pAkai->GetPartition(i);
    if (pPartition)
    {
      std::list<AkaiDirEntry> Volumes;
      pPartition->ListVolumes(Volumes);
      std::list<AkaiDirEntry>::iterator it;
      std::list<AkaiDirEntry>::iterator end = Volumes.end();
      int vol = 0;
      for (it = Volumes.begin(); it != end; it++)
      {
        AkaiDirEntry DirEntry = *it;
        printf("  %d (%d) '%.12s'  [start=%d - type=%d]\n", vol,DirEntry.mIndex, DirEntry.mName.c_str(), DirEntry.mStart, DirEntry.mType);
        AkaiVolume* pVolume = pPartition->GetVolume(vol);
        if (pVolume)
        {
          std::list<AkaiDirEntry> Programs;
          pVolume->ListPrograms(Programs);
          std::list<AkaiDirEntry>::iterator it;
          std::list<AkaiDirEntry>::iterator end = Programs.end();

          uint prog = 0;
          for (it = Programs.begin(); it != end; it++)
          {
            AkaiDirEntry DirEntry = *it;
            printf("    Program %d (%d) '%.12s' [start=%d size=%d - type=%c]\n", prog,DirEntry.mIndex, DirEntry.mName.c_str(), DirEntry.mStart, DirEntry.mSize, DirEntry.mType);
            AkaiProgram* pProgram = pVolume->GetProgram(prog);
            printf("        Number of key groups: %d\n",pProgram->mNumberOfKeygroups);
            pProgram->Release();
            prog++;
          }

          std::list<AkaiDirEntry> Samples;
          pVolume->ListSamples(Samples);
          end = Samples.end();

          uint samp = 0;
          for (it = Samples.begin(); it != end; it++)
          {
            AkaiDirEntry DirEntry = *it;
            printf("    Sample %d (%d) '%.12s' [start=%d size=%d - type=%c]\n", samp,DirEntry.mIndex, DirEntry.mName.c_str(), DirEntry.mStart, DirEntry.mSize, DirEntry.mType);
            AkaiSample* pSample= pVolume->GetSample(samp);
            printf("        Number of samples: %d (%d Hz)\n",pSample->mNumberOfSamples,pSample->mSamplingFrequency);
            totalSamplesSize += pSample->mNumberOfSamples;
            totalSamples++;
            pSample->Release();
            samp++;
          }

          pVolume->Release();
        }

        vol++;
      }

      pPartition->Release();
    }
  }

#if 0
  printf("Writing Akai track to hard disk...");
  fflush(stdout);
  bool success = pImage->WriteImage("/tmp/some.akai");
  if (success) printf("ok\n");
  else printf("error\n");
#endif

#if !HAVE_SNDFILE // use libaudiofile
  hAFlib = NULL;
  openAFlib();
#endif // !HAVE_SNDFILE

  totalSamplesSize *= 2; // due to 16 bit samples
  printf("There are %ld samples on this disk with a total size of %ld Bytes. ",
         totalSamples, totalSamplesSize);
  printf("Do you want to extract them (.wav files will be written to '%s')? [y/n]\n", outPath);
  char c = getchar();
  if (c == 'y' || c == 'Y') {
      bool errorOccured = false;
      DIR* dir = opendir(outPath);
      if (!dir) {
          if (errno == ENOENT) {
              struct stat filestat;
#ifdef WIN32
              if (stat(outPath, &filestat) < 0) {
#else
              if (lstat(outPath, &filestat) < 0) {
#endif
                  printf("Creating output directory '%s'...", outPath);
                  fflush(stdout);
#ifdef WIN32
                  if (mkdir(outPath) < 0) {
#else
                  if (mkdir(outPath, 0770) < 0) {
#endif
                      perror("failed");
                      errorOccured = true;
                  }
                  else printf("ok\n");
              }
              else {
#if !defined(WIN32)
                  if (!S_ISLNK(filestat.st_mode)) {
#endif
                      printf("Cannot create output directory '%s': ", outPath);
                      printf("a file of that name already exists\n");
                      errorOccured = true;
#if !defined(WIN32)
                  }
#endif
              }
          }
          else {
              perror("error while opening output directory");
              errorOccured = true;
          }
      }
      if (dir) closedir(dir);
      if (!errorOccured) {
          printf("Starting extraction...\n");
          long currentsample = 1;
          uint i;
          for (i = 0; i < pAkai->GetPartitionCount() && !errorOccured; i++) {
              AkaiPartition* pPartition = pAkai->GetPartition(i);
              if (pPartition) {
                  std::list<AkaiDirEntry> Volumes;
                  pPartition->ListVolumes(Volumes);
                  std::list<AkaiDirEntry>::iterator it;
                  std::list<AkaiDirEntry>::iterator end = Volumes.end();
                  int vol = 0;
                  for (it = Volumes.begin(); it != end && !errorOccured; it++) {
                      AkaiDirEntry DirEntry = *it;
                      AkaiVolume* pVolume = pPartition->GetVolume(vol);
                      if (pVolume) {
                          std::list<AkaiDirEntry> Samples;
                          pVolume->ListSamples(Samples);
                          std::list<AkaiDirEntry>::iterator it;
                          std::list<AkaiDirEntry>::iterator end = Samples.end();
                          uint samp = 0;
                          for (it = Samples.begin(); it != end && !errorOccured; it++) {
                              AkaiDirEntry DirEntry = *it;
                              AkaiSample* pSample = pVolume->GetSample(samp);
                              printf("Extracting Sample (%ld/%ld) %s...",
                                     currentsample++,
                                     totalSamples,
                                     DirEntry.mName.c_str());
                              fflush(stdout);
#if USE_DISK_STREAMING
                              if (pSample->LoadHeader()) {
                                  uint16_t* pSampleBuf = new uint16_t[pSample->mNumberOfSamples];
                                  pSample->Read(pSampleBuf, pSample->mNumberOfSamples);

                                  String filename = outPath + String("/") + DirEntry.mName + ".wav";
                                  int res = writeWav(pSample,
                                                     filename.c_str(),
                                                     pSampleBuf,
                                                     pSample->mNumberOfSamples);
                                  if (res < 0) {
                                      printf("couldn't write sample data\n");
                                      errorOccured = true;
                                  }
                                  else printf("ok\n");
                                  delete[] pSampleBuf;
                              }
                              else {
                                  printf("failed to load sample data\n");
                                  errorOccured = true;
                              }
#else // no disk streaming
                              if (pSample->LoadSampleData()) {

                                  String filename = outPath + String("/") + DirEntry.mName + ".wav";
                                  int res = writeWav(pSample,
                                                     filename.c_str(),
                                                     pSample->mpSamples,
                                                     pSample->mNumberOfSamples);
                                  if (res < 0) {
                                      printf("couldn't write sample data\n");
                                      errorOccured = true;
                                  }
                                  else printf("ok\n");
                                  pSample->ReleaseSampleData();
                              }
                              else {
                                  printf("failed to load sample data\n");
                                  errorOccured = true;
                              }
#endif // USE_DISK_STREAMING
                              pSample->Release();
                              samp++;
                          }
                          pVolume->Release();
                      }
                      vol++;
                  }
                  pPartition->Release();
              }
          }
      }
      if (errorOccured) printf("Extraction failed\n");
  }

#if !HAVE_SNDFILE // use libaudiofile
  closeAFlib();
#endif // !HAVE_SNDFILE

#if 0
  // this is just for debugging purpose - it converts the whole image to ascii
  printf("Converting...\n");
  FILE* f = fopen("akai_converted_to_ascii.iso", "w");
  if (f) {
    char* buf = (char*) malloc(AKAI_BLOCK_SIZE);
    pImage->SetPos(0);
    while (pImage->Available(1)) {
        int readbytes = pImage->Read(buf,1,AKAI_BLOCK_SIZE);
	if (readbytes != AKAI_BLOCK_SIZE) printf("Block incomplete (read: %d)\n",readbytes);
	ConvertAkaiToAscii(buf, readbytes);
	fwrite(buf, AKAI_BLOCK_SIZE, 1, f);
    }
    free(buf);
    fclose(f);
  }
  else printf("Could not open file\n");
#endif

  delete pAkai;
  delete pImage;
#if WIN32
  while(!_kbhit());
#endif
}

// only for debugging
void ConvertAkaiToAscii(char * buffer, int length)
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

int writeWav(AkaiSample* sample, const char* filename, void* samples, long samplecount, int channels, int bitdepth, long rate) {
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
    sfinfo.samplerate = rate;
    sfinfo.frames     = samplecount;
    sfinfo.channels   = channels;
    sfinfo.format     = format;
    if (!(hfile = sf_open(filename, SFM_WRITE, &sfinfo))) {
        cerr << "Error: Unable to open output file \'" << filename << "\'.\n" << flush;
        return -1;
    }
    instr.basenote = sample->mMidiRootNote;
    instr.detune = sample->mTuneCents;
    for (int i = 0; i < sample->mActiveLoops; ++i) {
        instr.loop_count = sample->mActiveLoops;
        instr.loops[i].mode = SF_LOOP_FORWARD;
        instr.loops[i].start = sample->mLoops[i].mMarker;
        instr.loops[i].end   = sample->mLoops[i].mCoarseLength;
        instr.loops[i].count = 0; // infinite
    }
    sf_command(hfile, SFC_SET_INSTRUMENT, &instr, sizeof(instr));
    sf_count_t res = bitdepth == 24 ?
        sf_write_int(hfile, static_cast<int*>(samples), channels * samplecount) :
        sf_write_short(hfile, static_cast<short*>(samples), channels * samplecount);
    if (res != channels * samplecount) {
        cerr << sf_strerror(hfile) << endl << flush;
        sf_close(hfile);
        return -1;
    }
    sf_close(hfile);
#else // use libaudiofile
    AFfilesetup setup = _afNewFileSetup();
    _afInitFileFormat(setup, AF_FILE_WAVE);
    _afInitChannels(setup, AF_DEFAULT_TRACK, channels);
    _afInitSampleFormat(setup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, bitdepth);
    _afInitRate(setup, AF_DEFAULT_TRACK, rate);
    if (setup == AF_NULL_FILESETUP) return -1;
    AFfilehandle hFile = _afOpenFile(filename, "w", setup);
    if (hFile == AF_NULL_FILEHANDLE) return -1;
    if (_afWriteFrames(hFile, AF_DEFAULT_TRACK, samples, samplecount) < 0) return -1;
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
