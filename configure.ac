# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([ppht_semver],[m4_esyscmd_s([perl git-semver])])dnl

AC_PREREQ([2.69])
LT_PREREQ([2.4.6])

AC_COPYRIGHT([Library copyright (C) 2020-2022 by Rob Menke.  Please see the
accompanying LICENSE.txt file for details.])

AC_INIT([PPHT],[ppht_semver],[ppht-bugs@the-wabe.com],[],[http://www.the-wabe.com/ppht/])

AC_CONFIG_AUX_DIR([build-aux])
AC_REQUIRE_AUX_FILE([tap-driver.sh])

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([include/ppht.hpp])

AM_INIT_AUTOMAKE([-Wall foreign nostdinc subdir-objects])

LT_INIT

LT_LANG([C++])
AX_CXX_COMPILE_STDCXX([17],[noext],[mandatory])

AX_CODE_COVERAGE

AC_CONFIG_FILES([Makefile test/Makefile Doxyfile])

AC_OUTPUT
