AC_INIT([KISS SMPP Client], [0.5], [bothadjt@gmail.com], [ksmppc], [http://djb.co.za/])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 -Wall no-define])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AC_CONFIG_MACRO_DIR([m4])

BOOST_REQUIRE([1.54.0])
BOOST_SYSTEM
BOOST_ASIO
BOOST_DATE_TIME
BOOST_FILESYSTEM
BOOST_REGEX
BOOST_THREAD
BOOST_DATE_TIME
BOOST_PROGRAM_OPTIONS

AC_ARG_WITH([boost-include-path],
            [AS_HELP_STRING([--with-boost-include-path], [location of the boost headers, defaults to /usr/include])],
            [BOOST_CFLAGS="-I$withval"],
            [BOOST_CFLAGS='-I/usr/include'])
AC_SUBST([BOOST_CFLAGS]) 

AC_ARG_WITH([boost-lib-path],
            [AS_HELP_STRING([--with-boost-lib-path], [location of the boost libraries])],
            [BOOST_LIBS="-L$withval -lboost_system -lboost_thread -lboost_filesystem -lboost_regex -lboost_date_time"],
            [BOOST_LIBS='-lboost_system -lboost_thread -lboost_filesystem -lboost_regex -lboost_date_time -lboost_program_options'])
AC_SUBST([BOOST_LIBS])

PTHREAD_LIB="-lpthread"
AC_ARG_WITH([pthread],
            [AS_HELP_STRING([--without-pthread], [ignore pthread support @<:@default: automatic@:>@])],
            [PTHREAD_LIB="-lpthread"])
AC_SUBST([PTHREAD_LIB])

PKG_CHECK_MODULES([DEPS], [smpp_pdu-1 >= 0.5 kisscpp-1 >= 0.5])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

