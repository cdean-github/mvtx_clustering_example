AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = \
  -I$(includedir) \
  -I$(OFFLINE_MAIN)/include \
  -isystem$(ROOTSYS)/include

AM_LDFLAGS = \
  -L$(libdir) \
  -L$(OFFLINE_MAIN)/lib \
  -L$(OFFLINE_MAIN)/lib64

pkginclude_HEADERS = \
  VertexCompare.h \
  silicon_detector_analyser.h

lib_LTLIBRARIES = \
  libVertexCompare.la

libVertexCompare_la_SOURCES = \
  VertexCompare.cc \
  silicon_detector_analyser.cc

libVertexCompare_la_LIBADD = \
  -lphool \
  -lSubsysReco \
  -lglobalvertex \
  -lintt \
  -lmvtx \
  -lffarawobjects

BUILT_SOURCES = testexternals.cc

noinst_PROGRAMS = \
  testexternals

testexternals_SOURCES = testexternals.cc
testexternals_LDADD   = libVertexCompare.la

testexternals.cc:
	echo "//*** this is a generated file. Do not commit, do not edit" > $@
	echo "int main()" >> $@
	echo "{" >> $@
	echo "  return 0;" >> $@
	echo "}" >> $@

clean-local:
	rm -f $(BUILT_SOURCES)
