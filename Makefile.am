AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -pedantic -g -O3 -flto -rdynamic -Isrc -Imodule -fPIC
AM_LDFLAGS = -module

EXTRA_DIST = \
README.rst

bin_PROGRAMS = \
bin/flow

FLOW_SOURCE_FILES = \
src/main.c \
src/flow.c \
src/message.c

FLOW_HEADER_FILES = \
src/flow.h \
src/message.h

bin_flow_SOURCES = $(FLOW_SOURCE_FILES) $(FLOW_HEADER_FILES)
bin_flow_LDFLAGS = -pthread
bin_flow_LDADD = -lreactor -ldynamic -ljansson -ldl -lltdl

pkglib_LTLIBRARIES = \
module/test.la \
module/test2.la

module_test_la_LIBADD = -lreactor -ldynamic

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in
maintainer-clean-local:; rm -rf autotools m4

CLEANFILES = {.,src}/*.{gcno,gcda,gcov}

indent:
	clang-format -i src/*.c module/*.c

.phony: indent