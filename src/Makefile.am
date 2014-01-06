AUTOMAKE_OPTIONS = foreign

distdir = ../$(PACKAGE)-$(VERSION)/src
dist_HEADERS = mrkdata_private.h fparser_private.h

BUILT_SOURCES = diag.c diag.h
EXTRA_DIST = diag.txt gen-diag
CLEANFILES = $(BUILT_SOURCES) *.core
#CLEANFILES += *.in

lib_LTLIBRARIES = libmrkdata.la libfparser.la

nobase_include_HEADERS = mrkdata.h fparser.h

if DEBUG
DEBUG_FLAGS = -g -O0 @CLANG_DEBUG@
else
DEBUG_FLAGS = -DNDEBUG -O3
endif

libmrkdata_la_SOURCES = mrkdata.c
nodist_libmrkdata_la_SOURCES = diag.c
libmrkdata_la_CFLAGS = $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99
libmrkdata_la_LDFLAGS = -version-info 1

libfparser_la_SOURCES = fparser.c
nodist_libfparser_la_SOURCES = diag.c
libfparser_la_CFLAGS = $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99
libfparser_la_LDFLAGS = -version-info 1 -lmrkcommon

SUBDIRS = . test

diag.c diag.h: diag.txt
	sh ./gen-diag mrkdata

run: all
	for i in $(bin_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;

testrun:
	for i in $(SUBDIRS); do if test "$$i" != "."; then cd $$i && $(MAKE) testrun && cd ..; fi; done;
