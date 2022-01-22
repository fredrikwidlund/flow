ACLOCAL_AMFLAGS		= ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS		= -std=gnu2x -flto -Wall -Werror -pedantic -rdynamic -fPIC -I$(top_srcdir)/src
AM_LDFLAGS		= -module

pkglib_LTLIBRARIES	=

pkglib_LTLIBRARIES	+= example.la
pkglib_LTLIBRARIES	+= generator.la
pkglib_LTLIBRARIES	+= duplicator.la
pkglib_LTLIBRARIES	+= counter.la

generator_la_LIBADD	= -lreactor -ljansson

MAINTAINERCLEANFILES 	= Makefile.in
