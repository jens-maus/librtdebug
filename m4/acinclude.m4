dnl/* vim:set ts=2 nowrap: ****************************************************
dnl
dnl librtdebug - A C++ based thread-safe Runtime Debugging Library
dnl Copyright (C) 2003-2005 by Jens Langner <Jens.Langner@light-speed.de>
dnl
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl
dnl $Id$
dnl
dnl***************************************************************************/
dnl# -*- mode: m4 -*-

dnl Autoconf Macros from gnu.org

AC_DEFUN([AC_CXX_NAMESPACES],
[AC_CACHE_CHECK(whether the compiler implements namespaces,
ac_cv_cxx_namespaces,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([namespace Outer { namespace Inner { int i = 0; }}],
                [using namespace Outer::Inner; return i;],
 ac_cv_cxx_namespaces=yes, ac_cv_cxx_namespaces=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_namespaces" = yes; then
  AC_DEFINE(HAVE_NAMESPACES,,[define if the compiler implements namespaces])
fi
])

AC_DEFUN([AC_CXX_STATIC_CAST],
[AC_CACHE_CHECK(whether the compiler supports static_cast<>,
ac_cv_cxx_static_cast,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <typeinfo>
class Base { public : Base () {} virtual void f () = 0; };
class Derived : public Base { public : Derived () {} virtual void f () {} };
int g (Derived&) { return 0; }],[
Derived d; Base& b = d; Derived& s = static_cast<Derived&> (b); return g (s);],
 ac_cv_cxx_static_cast=yes, ac_cv_cxx_static_cast=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_static_cast" = yes; then
  AC_DEFINE(HAVE_STATIC_CAST,,
            [define if the compiler supports static_cast<>])
fi
])

dnl Copyright (c) 2003 Jens Langner
AC_DEFUN(AC_ANSI_COLOR,
[
	AC_MSG_CHECKING(whether ANSI color should be used for terminal output)
	AC_ARG_ENABLE(ansi-color,
								[AC_HELP_STRING([--disable-ansi-color], [ansi-color terminal output should be disabled [default=no]])],
								[case "${enableval}" in
									yes) test_on_ansi_color=yes ;;
									no)  test_on_ansi_color=no ;;
									*)   AC_MSG_ERROR(bad value ${enableval} for --disable-ansi-color) ;;
								esac], 
								[test_on_ansi_color=no])

	if test "$test_on_ansi_color" = "yes"; then
		AC_MSG_RESULT(no)
	else
		ANSI_COLOR="ansi_color" 
		AC_DEFINE(WITH_ANSI_COLOR) 		
		AC_MSG_RESULT(yes)
	fi
	AC_DEFINE([WITH_ANSI_COLOR], [], [Use ANSI color scheme in terminal output])
	
	AC_SUBST(ANSI_COLOR) 
])

# this macro is used to get the arguments supplied
# to the configure script (./configure --enable-static-lib)
AC_DEFUN(AC_ENABLE_STATIC_LIB,
[
	AC_MSG_CHECKING(whether to link the librtdebug as a static library)
	AC_ARG_ENABLE(static-lib,
								[AC_HELP_STRING([--enable-static-lib], [turn on static linking of librtdebug [default=no]])],
								[case "${enableval}" in
									yes) test_on_enable_static_lib=yes	;;
									no)  test_on_enable_static_lib=no	;;
									*)	 AC_MSG_ERROR(bad value ${enableval} for --enable-static-lib) ;;
								esac],
								[test_on_enable_static_lib=no])

	if test "$test_on_enable_static_lib" = "yes"; then
		QTLINK_LEVEL="${QTLINK_LEVEL} staticlib"
		AC_MSG_RESULT(yes)
	else
		QTLINK_LEVEL="${QTLINK_LEVEL}"
		AC_MSG_RESULT(no)
	fi

	AC_SUBST(QTLINK_LEVEL) 
])

#
# find out the used gcc version and validate that this one is
# a working one for our compilation
# 
AC_DEFUN(AC_PROG_GCC_VERSION,[

 AC_MSG_CHECKING([for gcc version])

 dnl check if $CC exists or not
 if eval $CC -v 2>/dev/null >/dev/null; then
   dnl Check if version of gcc is sufficient
	 cc_name=`( $CC -v ) 2>&1 | tail -n 1 | cut -d ' ' -f 1`
	 cc_version=`( $CC -dumpversion ) 2>&1`
	 if test "$?" -gt 0; then
		 cc_version="not found"
	 fi
	 changequote(,)dnl	 
	 case $cc_version in
	   '')
		   cc_version="v. ?.??, bad"
			 cc_verc_fail=yes
			 ;;
		 2.95.[2-9]|2.95.[2-9][-.]*|3.[0-9]|3.[0-9].[0-9]|4.[0-9].[0-9])
			 _cc_major=`echo $cc_version | cut -d '.' -f 1`
			 _cc_minor=`echo $cc_version | cut -d '.' -f 2`
			 _cc_mini=`echo $cc_version | cut -d '.' -f 3`
			 cc_version="$cc_version, ok"
			 cc_verc_fail=no
		   ;;
		 'not found')
		   cc_verc_fail=yes
			 ;;
		 *)
		   cc_version="$cc_version, bad"
			 cc_verc_fail=yes
			 ;;
	 esac
	 changequote([, ])dnl

	 if test "$cc_verc_fail" = yes ; then
	   AC_MSG_RESULT([$cc_version])
		 AC_MSG_ERROR([gcc version check failed])
   else
 	   AC_MSG_RESULT([$cc_version])
		 GCC_VERSION=$_cc_major
   fi
 else
   AC_MSG_RESULT(FAILED)
   AC_MSG_ERROR([gcc was not found '$CC'])
 fi

 AC_SUBST(GCC_VERSION)
])

AC_DEFUN(AC_PATH_QTDIR,
[
  AC_ARG_WITH(qt,
                          [AC_HELP_STRING([--with-qt], [where the QT multithreaded library is located.])],
                          [QTDIR="$withval" ])

  if test -z "$QTDIR"; then
    AC_MSG_WARN([environment variable QTDIR is not set, you may run into problems])
  fi

])

AC_DEFUN(AC_PATH_QT_LIB,
[
  AC_REQUIRE_CPP()
  dnl AC_REQUIRE([AC_PATH_X])
  AC_ARG_WITH(qt-lib,
                          [AC_HELP_STRING([--with-qt-lib], [where the QT multithreaded library is located.])],
                      [ac_qt_libraries="$withval"], ac_qt_libraries="")

  AC_MSG_CHECKING(for QT multithreaded library)

  AC_CACHE_VAL(ac_cv_lib_qtlib, [

  qt_libdir=

  dnl No they didnt, so lets look for them...
  dnl If you need to add extra directories to check, add them here.
  if test -z "$ac_qt_libraries"; then
    if test -n "$QTDIR"; then
      qt_library_dirs="$QTDIR/lib $qt_library_dirs"
    fi

    if test -n "$QTLIB"; then
      qt_library_dirs="$QTLIB $qt_library_dirs"
    fi

    qt_library_dirs="$qt_library_dirs \
                     /usr/qt/lib \
                     /usr/local/lib/qt \
                     /usr/lib/qt3/lib \
                     /usr/lib \
                     /usr/local/lib \
                     /usr/lib/qt \
                     /usr/lib/qt/lib \
                     /usr/local/lib/qt \
                     /usr/X11/lib \
                     /usr/X11/lib/qt \
                     /usr/X11R6/lib \
                     /usr/X11R6/lib/qt \
                     /Developer/qt/lib"
  else
    qt_library_dirs="$ac_qt_libraries"
  fi

  dnl Save some global vars
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  qt_found="0"
  ac_qt_libdir=
  ac_qt_libname="-lqt-mt"
  
  LIBS="$ac_qt_libname $save_LIBS"
  for qt_dir in $qt_library_dirs; do
    dnl echo $qt_dir;
    LDFLAGS="-L$qt_dir $save_LDFLAGS"
    AC_TRY_LINK_FUNC(main, [qt_found="1"], [qt_found="0"])
    dnl AC_TRY_RUN([#include <qglobal.h>
    dnl int 
    dnl main()
    dnl {
    dnl  main();
    dnl  ;
    dnl  return QT_VERSION;
    dnl }])
    if test $qt_found = 1; then
      ac_qt_libdir="$qt_dir"
      break;
    else
      echo "tried $qt_dir" >&AC_FD_CC 
    fi
  done

  dnl Restore the saved vars
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  ac_cv_lib_qtlib="ac_qt_libname=$ac_qt_libname ac_qt_libdir=$ac_qt_libdir"
  ])

  eval "$ac_cv_lib_qtlib"

  dnl Define a shell variable for later checks
  if test -z "$ac_qt_libdir"; then
    have_qt_lib="no"
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Cannot find required QT multithreaded library in linker path.
Try --with-qt-lib to specify the path, manualy.])
  else
    have_qt_lib="yes"
    AC_MSG_RESULT([yes, lib: $ac_qt_libname in $ac_qt_libdir])
  fi

  QT_LDFLAGS="-L$ac_qt_libdir"
  QT_LIBDIR="$ac_qt_libdir"
  LIB_QT="$ac_qt_libname"
  AC_SUBST(QT_LDFLAGS)
  AC_SUBST(QT_LIBDIR)
  AC_SUBST(LIB_QT)
])
