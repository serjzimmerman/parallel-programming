AC_INIT([mpi-sort-comparison], [0.0.1], [tsimmerman.ss@phystech.edu])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AX_BOOST_BASE([1.80],, [AC_MSG_ERROR([Boost is required.])])
AX_CXX_COMPILE_STDCXX([20], [ext], [mandatory])
AC_PROG_CC
AC_PROG_CXX
AC_CONFIG_SRCDIR([src/sort.cpp])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
