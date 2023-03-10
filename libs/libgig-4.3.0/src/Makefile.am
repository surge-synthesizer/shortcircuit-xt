# to prevent compile errors on some systems
AM_CXXFLAGS = -pedantic -Wreturn-type

pkglib_LTLIBRARIES = libgig.la libakai.la

libgigincludedir = $(includedir)/libgig
libgiginclude_HEADERS = RIFF.h DLS.h SF.h gig.h Korg.h Serialization.h
libgig_la_SOURCES = helper.cpp typeinfo.cpp RIFF.cpp DLS.cpp SF.cpp gig.cpp Korg.cpp Serialization.cpp
libgig_la_LDFLAGS = -no-undefined -version-info @LIBGIG_SHARED_VERSION_INFO@ @LIBGIG_SHLIB_VERSION_ARG@
if WIN32
libgig_la_LIBADD = -lrpcrt4
endif
if MAC
libgig_la_LDFLAGS += -framework CoreFoundation
endif

libakaiincludedir = $(includedir)/libgig
libakaiinclude_HEADERS = Akai.h
libakai_la_SOURCES = Akai.cpp
libakai_la_LDFLAGS = -no-undefined -version-info @LIBAKAI_SHARED_VERSION_INFO@ @LIBAKAI_SHLIB_VERSION_ARG@
if WIN32
libakai_la_LIBADD = -lrpcrt4
endif
if MAC
libakai_la_LDFLAGS += -framework CoreFoundation -framework IOKit
endif

EXTRA_DIST = helper.h

SUBDIRS = . tools testcases
