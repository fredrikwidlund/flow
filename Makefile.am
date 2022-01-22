ACLOCAL_AMFLAGS		= ${ACLOCAL_FLAGS} -I m4
EXTRA_DIST		= autogen.sh
MAINTAINERCLEANFILES	= aclocal.m4 configure Makefile.in
AM_CFLAGS		= -std=gnu2x -flto -rdynamic -Wall -Werror -pedantic -I $(top_srcdir)/src
AM_LDFLAGS		= -pthread

bin_PROGRAMS 		= flow
flow_SOURCES 		= src/main.c
flow_CFLAGS		= $(AM_CFLAGS) -static
flow_LDADD 		= libflow.la -lreactor -ljansson

lib_LTLIBRARIES		= libflow.la

FLOW_SOURCE_FILES 	= \
src/flow.c \
src/graph.c \
src/symbols.c \
src/modules.c \
src/thread.c \
src/node.c \
src/edge.c \
src/queue.c \
src/message.c \
src/log.c

FLOW_HEADER_FILES	= \
src/flow.h \
src/graph.h \
src/symbols.h \
src/modules.h \
src/thread.h \
src/node.h \
src/edge.h \
src/queue.h \
src/message.h \
src/log.h

libflow_la_SOURCES	= $(FLOW_SOURCE_FILES) $(FLOW_HEADER_FILES)
libflow_la_LIBADD	= -lreactor -ljansson -ldl -lltdl

#headerfilesdir 		= $(includedir)/flow
#headerfiles_HEADERS 	= $(FLOW_HEADER_FILES)

mainheaderdir 		= $(includedir)
mainheader_HEADERS 	= src/flow.h

maintainer-clean-local:
	rm -rf autotools m4
