AUTOMAKE_OPTIONS = foreign
CLEANFILES = *.core
#CLEANFILES += *.in

noinst_PROGRAMS=testfoo testfparser

distdir = ../../$(PACKAGE)-$(VERSION)/src/test
dist_HEADERS = unittest.h

BUILT_SOURCES = ../diag.c ../diag.h

if DEBUG
DEBUG_FLAGS = -g -O0 @CLANG_DEBUG@
else
DEBUG_FLAGS = -DNDEBUG -O3
endif

nodist_testfoo_SOURCES = ../diag.c
testfoo_SOURCES = testfoo.c
testfoo_CFLAGS = $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 -I.. -I$(includedir)
testfoo_LDFLAGS = -L$(libdir) -lmrkcommon -lmrkdata -lfparser

nodist_testfparser_SOURCES = ../diag.c
testfparser_SOURCES = testfparser.c
testfparser_CFLAGS = $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 -I.. -I$(includedir)
testfparser_LDFLAGS = -L$(libdir) -lmrkcommon -lmrkdata -lfparser

../diag.c ../diag.h: ../diag.txt
	sh ../gen-diag

testrun: all
	for i in $(noinst_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;
