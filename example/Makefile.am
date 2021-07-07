AUTOMAKE_OPTIONS		= subdir-objects
ACLOCAL_AMFLAGS			= ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS			= -std=gnu11 -rdynamic -fPIC -I$(top_srcdir)/src
LDADD				= -L$(top_srcdir) ../libflow.la -lreactor -ldynamic -ljansson -ldl -lltdl

pkglib_LTLIBRARIES		=

pkglib_LTLIBRARIES		+= sender.la
sender_la_LDFLAGS		= $(AM_LDFLAGS) -module
sender_la_LIBADD		= -lreactor -ldynamic ../libflow.la

pkglib_LTLIBRARIES		+= receiver.la
receiver_la_LDFLAGS		= $(AM_LDFLAGS) -module

bin_PROGRAMS 			= \
count

count_SOURCES			= \
count.c	\
sender_static.c \
receiver_static.c

count_LDFLAGS			= -static

MAINTAINERCLEANFILES = Makefile.in
