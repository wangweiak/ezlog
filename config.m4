dnl $Id$
dnl config.m4 for extension ezlog

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(ezlog, for ezlog support,
Make sure that the comment is aligned:
[  --with-ezlog             Include ezlog support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(ezlog, whether to enable ezlog support,
dnl Make sure that the comment is aligned:
dnl [  --enable-ezlog           Enable ezlog support])

if test "$PHP_EZLOG" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-ezlog -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/ezlog.h"  # you most likely want to change this
  dnl if test -r $PHP_EZLOG/$SEARCH_FOR; then # path given as parameter
  dnl   EZLOG_DIR=$PHP_EZLOG
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for ezlog files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       EZLOG_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$EZLOG_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the ezlog distribution])
  dnl fi

  dnl # --with-ezlog -> add include path
  dnl PHP_ADD_INCLUDE($EZLOG_DIR/include)

  dnl # --with-ezlog -> check for lib and symbol presence
  dnl LIBNAME=ezlog # you may want to change this
  dnl LIBSYMBOL=ezlog # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $EZLOG_DIR/$PHP_LIBDIR, EZLOG_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_EZLOGLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong ezlog lib version or lib not found])
  dnl ],[
  dnl   -L$EZLOG_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(EZLOG_SHARED_LIBADD)

  PHP_NEW_EXTENSION(ezlog, ezlog.c, $ext_shared)
fi
