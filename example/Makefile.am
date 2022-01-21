AUTOMAKE_OPTIONS		= subdir-objects
ACLOCAL_AMFLAGS			= ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS			= -std=gnu11 -rdynamic -fPIC -I$(top_srcdir)/src
LDADD				= -L$(top_srcdir) ../libflow.la -lreactor -ljansson -ldl -lltdl

pkglib_LTLIBRARIES		=

pkglib_LTLIBRARIES		+= sender.la
sender_la_LDFLAGS		= $(AM_LDFLAGS) -module
sender_la_LIBADD		= -lreactor
#../libflow.la

pkglib_LTLIBRARIES		+= receiver.la
receiver_la_LDFLAGS		= $(AM_LDFLAGS) -module

MAINTAINERCLEANFILES = Makefile.in
