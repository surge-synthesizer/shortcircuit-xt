/***************************************************************************
 *                                                                         *
 *   libgig - C++ cross-platform Gigasampler format file access library    *
 *                                                                         *
 *   Copyright (C) 2003-2017 by Christian Schoenebeck                      *
 *                              <cuse@users.sourceforge.net>               *
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

#include "helper.h"

#if /*!HAVE_VASPRINTF &&*/ defined(WIN32)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// vasprintf() does not exist on older windows versions
int vasprintf(char** ret, const char* format, va_list arg) {
    *ret = NULL;
    const int len = _vscprintf(format, arg); // counts without \0 delimiter
    if (len < 0)
        return -1;
    const size_t size = ((size_t) len) + 1;
    char* buf = (char*) malloc(size);
    if (!buf)
        return -1;
    memset(buf, 0, size);
    int res = _vsnprintf(buf, size, format, arg);
    if (res < 0) {
        free(buf);
        return -1;
    }
    *ret = buf;
    return res;
}

#endif // !HAVE_VASPRINTF && defined(WIN32)
