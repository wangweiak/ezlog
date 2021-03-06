/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_EZLOG_H
#define PHP_EZLOG_H

extern zend_module_entry ezlog_module_entry;
#define phpext_ezlog_ptr &ezlog_module_entry

#define PHP_EZLOG_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_EZLOG_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_EZLOG_API __attribute__ ((visibility("default")))
#else
#	define PHP_EZLOG_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif


ZEND_BEGIN_MODULE_GLOBALS(ezlog)
	zend_bool onmemcache;
	zend_bool onfile;
        zend_bool onredis;
        char *memcachehost;
        unsigned long memcacheport;
        char *filepath;
        char *redishost;
        char *redisport;
ZEND_END_MODULE_GLOBALS(ezlog)

char *getCurTime();
void getUri(char **uri TSRMLS_DC);
char *getData(char * TSRMLS_DC);
/* In every utility function you add that needs to use variables 
   in php_ezlog_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as EZLOG_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define EZLOG_G(v) TSRMG(ezlog_globals_id, zend_ezlog_globals *, v)
#else
#define EZLOG_G(v) (ezlog_globals.v)
#endif

extern ZEND_DECLARE_MODULE_GLOBALS(ezlog);

#endif	/* PHP_EZLOG_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
