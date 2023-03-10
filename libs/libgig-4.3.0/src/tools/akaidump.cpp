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
// akaidump.cpp : Defines the entry point for the console application.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <conio.h>
#endif

#include <stdio.h>
#include "../Akai.h"

static void PrintLastError(char* file, int line)
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
        L"akaidump <source-drive-letter>: <destination-filename>\n"
        "by S\351bastien M\351trot (meeloo@meeloo.net)\n\n"
        "Reads an AKAI media (i.e. CDROM, ZIP disk) and writes it as AKAI\n"
        "disk image file to your hard disk drive for more convenient and\n"
        "faster further usage.\n\n"
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
        L"akaidump [<source-path>] <destination-filename>\n"
        "by S\351bastien M\351trot (meeloo@meeloo.net)\n\n"
        "Reads an AKAI media (i.e. CDROM, ZIP disk) and writes it as AKAI\n"
        "disk image file to your hard disk drive for more convenient and\n"
        "faster further usage.\n\n"
        "<source-path> - path of the source AKAI image file, if omitted CDROM\n"
        "                drive ('/dev/rdisk1s0') will be accessed\n\n"
        "<destination-filename> - target filename to write the AKAI disk image to\n\n"
    );
#else
    setlocale(LC_CTYPE, "");
    printf("%ls",
        L"akaidump <source-path> <destination-filename>\n"
        "by S\351bastien M\351trot (meeloo@meeloo.net)\n"
        "Linux port by Christian Schoenebeck\n\n"
        "Reads an AKAI media (i.e. CDROM, ZIP disk) and writes it as AKAI\n"
        "disk image file to your hard disk drive for more convenient and\n"
        "faster further usage.\n\n"
        "<source-path> - path of the source drive, i.e. /dev/cdrom\n\n"
        "<destination-filename> - target filename to write the AKAI disk image to\n\n"
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

    // open output file for writing the AKAI disk image
#if defined _CARBON_
    const char* outPath = (argc == 2) ? argv[1] : argv[2];
#else
    const char* outPath = argv[2];
#endif
    printf("Writing AKAI image file to '%s'\n", outPath);
    FILE* file = fopen(outPath, "wb");
    if (!file) {
        printf("Couldn't open output file '%s'\n", outPath);
        return -1;
    }

    uint i;
    uint size = pImage->Available(1);
    uint mbytes = 0;
    uint writenbytes = 0;
    printf("Dumping %d bytes (%d Mb)\n", size, size / (1024*1024)); 
    for (i = 0; i < size; ) {
        char buffer[2048];
        uint32_t readbytes = 0;
        readbytes = pImage->Read(buffer, 2048, 1);
        writenbytes += fwrite(buffer, readbytes, 1, file);
        i += readbytes;
    
        if (i / (1024*1024) > mbytes) {
            mbytes = i / (1024*1024);
            printf("\rDumped %d Mbytes.",mbytes);
        }
    }
    printf("\n%d bytes read, %d bytes written\nOK\n", i, writenbytes);

    fclose(file);

    delete pImage;

//  while(!_kbhit());
    return 0;
}
