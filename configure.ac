AC_INIT([flow],[1.0.0],[fredrik.widlund@gmail.com])

AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-define])

: ${CFLAGS="-g -O3 -Wall -Wextra -Wpedantic -flto"}

AM_PROG_AR
LT_INIT
AM_PROG_CC_C_O

AC_PREFIX_DEFAULT(/usr)
AC_CONFIG_FILES([Makefile example/Makefile libflow.pc])
AC_OUTPUT
