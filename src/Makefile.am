# For license: see LICENSE file at top-level

CC                      = oshcc

SOURCES                 = alltoall.c \
				alltoalls.c \
				barrier.c \
				broadcast.c \
				collect.c \
				fcollect.c \
				reduction.c
SOURCES                += util/bithacks.c \
				util/broadcast-size.c \
				util/rotate.c \
				util/scan.c \
				util/trees.c

lib_LTLIBRARIES         = libshcoll.la
libshcoll_la_SOURCES    = $(SOURCES)
libshcoll_la_CFLAGS     = $(AM_CFLAGS)

lib_LIBRARIES           = libshcoll.a
libshcoll_a_SOURCES     = $(SOURCES)

nobase_include_HEADERS  = shcoll.h \
				shcoll/alltoall.h \
				shcoll/alltoalls.h \
				shcoll/barrier.h \
				shcoll/broadcast.h \
				shcoll/collect.h \
				shcoll/common.h \
				shcoll/compat.h \
				shcoll/fcollect.h \
				shcoll/reduction.h

