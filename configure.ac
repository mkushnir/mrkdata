AC_INIT(mrkdata, 0.1, markiyan.kushnir@gmail.com)
if test -z "$USE_GCC"
then
    CC="clang -Qunused-arguments -fcolor-diagnostics"
    CXX="clang++ -Qunused-arguments -fcolor-diagnostics"
    CPP="clang-cpp"
fi

AC_LANG(C)

AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR
AC_PROG_RANLIB
AC_PROG_INSTALL

LT_INIT
AM_INIT_AUTOMAKE

dnl AM_MAINTAINER_MODE

AC_CONFIG_HEADERS(config.h)

AC_CHECK_HEADERS([syslog.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([netinet/in.h])


AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD


AC_CONFIG_FILES([Makefile src/Makefile src/test/Makefile man/Makefile])

AC_PRESERVE_HELP_ORDER

AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [Enable debug build (default=no)]),
              [AM_CONDITIONAL([DEBUG], [enable_debug=yes])],
              [AM_CONDITIONAL([DEBUG], [test "$enable_debug" = "yes"])]
             )

AC_CHECK_LIB(mrkcommon, _fini, [], [AC_MSG_FAILURE(libmrkcommon.so is required. Please find it at git@github.com:mkushnir/mrkcommon.git)]) 

AC_OUTPUT
