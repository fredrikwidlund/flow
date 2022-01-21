AUTOMAKE_OPTIONS		= subdir-objects
ACLOCAL_AMFLAGS			= ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS			= -std=gnu11 -rdynamic -fPIC
AM_LDFLAGS			= -static

### libraries ###

lib_LTLIBRARIES			= libflow.la

SOURCE_FILES			= \
src/flow.c \
src/flow_module.c \
src/flow_log.c \
src/flow_node.c \
src/flow_message.c \
src/flow_queue.c

HEADER_FILES			= \
src/flow.h \
src/flow_module.h \
src/flow_log.h \
src/flow_node.h \
src/flow_message.h \
src/flow_queue.h

libflow_la_SOURCES		= $(SOURCE_FILES) $(HEADER_FILES)
libflow_la_LIBADD		= -lreactor

headerfilesdir 			= $(includedir)/flow
headerfiles_HEADERS		= $(HEADER_FILES)

pkgconfigdir 			= $(libdir)/pkgconfig
pkgconfig_DATA 			= libflow.pc

### flow program ###

bin_PROGRAMS 			= \
bin/flow

bin_flow_SOURCES 		= \
src/main.c

bin_flow_LDFLAGS 		= -pthread -static
bin_flow_LDADD 			= libflow.la -lreactor -ljansson -ldl -lltdl

### other ###

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in
maintainer-clean-local:; rm -rf autotools m4

CLEANFILES = {.,src}/*.{gcno,gcda,gcov}

indent:
	clang-format -i src/*.c example/*.c

memory: all
	@valgrind -s --leak-check=full --track-fds=yes ./bin/flow ./etc/test.json

test: all
	@./bin/flow ./etc/test.json


.phony: indent memory test
