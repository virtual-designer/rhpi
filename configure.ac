#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.72])
AC_INIT([redhat-package-installer], [1.0.0], [rakinar2@onesoftnet.eu.org])
AC_CONFIG_AUX_DIR([build-aux])
AM_SILENT_RULES([yes])
AM_INIT_AUTOMAKE([gnu])
AC_CONFIG_SRCDIR([src/installer.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero dup2 strdup strerror strndup strtol strtoul])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 share/Makefile])

PKG_CHECK_MODULES([GTK], [gtk4])
PKG_CHECK_MODULES([LIBADWAITA], [libadwaita-1 >= 1])
PKG_CHECK_MODULES([POLKIT], [polkit-gobject-1])

GLIB_COMPILE_RESOURCES=`$PKG_CONFIG --variable=glib_compile_resources gio-2.0`
AC_SUBST(GLIB_COMPILE_RESOURCES)

AC_OUTPUT
