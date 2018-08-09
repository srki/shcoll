#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# For license: see LICENSE file at top-level

AC_PREREQ([2.69])
AC_INIT([SHCOLL], [1.0], [somewhere], [], [somewhere])

AC_CONFIG_SRCDIR([autogen.sh])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects tar-pax])

AM_SILENT_RULES([yes])

AC_PROG_LN_S

# C
AC_PROG_CC_C99
AC_PROG_CC_C_O
AC_PROG_INSTALL
AM_PROG_CC_C_O

# Checks for libraries.
# for building shared libraries
LT_INIT

AC_CHECK_HEADERS([stdlib.h unistd.h string.h time.h sys/time.h sys/types.h stddef.h sys/param.h assert.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

AC_CHECK_TYPES([int64_t])
AC_CHECK_TYPES([uint64_t])
AC_CHECK_TYPES([ptrdiff_t])

# ------------------------------------------------------------------------
#
# tell us where the shmem.h header file lives
#
shmem_hdr_happy=no

AC_ARG_WITH([shmemh-dir],
            [AS_HELP_STRING([--with-shmemh-dir=DIR], [Where OpenSHMEM header lives])])

AS_IF([test "x$with_shmemh_dir" != "x"],
      [
          hdr="$with_shmemh_dir/shmem.h"
          AS_IF([test -r "$hdr"],
          [
	      SHMEM_CPPFLAGS="-I$with_shmemh_dir"
	      AC_DEFINE_UNQUOTED([SHMEM_HEADER_DIR], ["$with_shmemh_dir"], [Where OpenSHMEM header file lives])
	      AC_SUBST([SHMEM_CPPFLAGS])
	      shmem_hdr_happy=yes
	      AC_MSG_NOTICE([Using OpenSHMEM header in $with_shmemh_dir])
          ],
	  [
		AC_MSG_ERROR([Unable to find OpenSHMEM header file $hdr])
	  ]
	  )
	  ],
	  [
	    AC_MSG_ERROR([Unable to find OpenSHMEM header directory $with_shmemh_dir])
          ]
          )

AM_CONDITIONAL([HAVE_SHMEM_HEADER], [test "x$shmem_hdr_happy" != xno])
#
# ------------------------------------------------------------------------

#
# Checks for library functions.
#

# AC_CHECK_FUNCS([atexit gettimeofday gethostname uname memset strlcat sched_yield])
# AC_CHECK_LIB([m], [log10])

AM_INIT_AUTOMAKE

AC_CONFIG_TESTDIR([tests])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	tests/atlocal
        ])

AC_OUTPUT
