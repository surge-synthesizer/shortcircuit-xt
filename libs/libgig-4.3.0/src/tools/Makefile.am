# to prevent compile errors on some systems
AM_CXXFLAGS = -pedantic -Wreturn-type

# link gigextract and akaiextract either against libaudiofile or libsndfile
# (if both exist on the system, prefer libsndfile)
if HAVE_SNDFILE
audiofileaccess_libs = $(SNDFILE_LIBS)
audiofileaccess_flags = $(SNDFILE_CFLAGS)
else
audiofileaccess_libs = $(AUDIOFILE_LIBS) -ldl
audiofileaccess_flags = $(AUDIOFILE_CFLAGS)
endif

bin_PROGRAMS = rifftree dlsdump gigdump gigextract gigmerge gig2mono gig2stereo sf2dump sf2extract korgdump korg2gig akaidump akaiextract

rifftree_SOURCES = rifftree.cpp
rifftree_LDADD = $(top_builddir)/src/libgig.la

dlsdump_SOURCES = dlsdump.cpp
dlsdump_LDADD = $(top_builddir)/src/libgig.la

gigdump_SOURCES = gigdump.cpp
gigdump_LDADD = $(top_builddir)/src/libgig.la

gigextract_SOURCES = gigextract.cpp
gigextract_LDADD = $(top_builddir)/src/libgig.la $(audiofileaccess_libs)
gigextract_CFLAGS = $(audiofileaccess_flags)
gigextract_CXXFLAGS = $(audiofileaccess_flags)

gigmerge_SOURCES = gigmerge.cpp
gigmerge_LDADD = $(top_builddir)/src/libgig.la

gig2mono_SOURCES = gig2mono.cpp
gig2mono_LDADD = $(top_builddir)/src/libgig.la

gig2stereo_SOURCES = gig2stereo.cpp
gig2stereo_LDADD = $(top_builddir)/src/libgig.la

sf2dump_SOURCES = sf2dump.cpp
sf2dump_LDADD = $(top_builddir)/src/libgig.la

sf2extract_SOURCES = sf2extract.cpp
sf2extract_LDADD = $(top_builddir)/src/libgig.la $(audiofileaccess_libs)
sf2extract_CFLAGS = $(audiofileaccess_flags)
sf2extract_CXXFLAGS = $(audiofileaccess_flags)

korgdump_SOURCES = korgdump.cpp
korgdump_LDADD = $(top_builddir)/src/libgig.la

korg2gig_SOURCES = korg2gig.cpp
korg2gig_LDADD = $(top_builddir)/src/libgig.la

akaidump_SOURCES = akaidump.cpp
akaidump_LDADD = $(top_builddir)/src/libakai.la

akaiextract_SOURCES = akaiextract.cpp
akaiextract_LDADD = $(top_builddir)/src/libakai.la $(audiofileaccess_libs)
akaiextract_CFLAGS = $(audiofileaccess_flags)
akaiextract_CXXFLAGS = $(audiofileaccess_flags)
