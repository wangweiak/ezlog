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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/info.h"
#include "php_ezlog.h"
#include <time.h>

ZEND_DECLARE_MODULE_GLOBALS(ezlog)

/* True global resources - no need for thread safety here */
static int le_ezlog;


PHP_INI_BEGIN()
    STD_PHP_INI_BOOLEAN("ezlog.onmemcache",   	"0", PHP_INI_ALL, OnUpdateBool, onmemcache, zend_ezlog_globals, ezlog_globals)
    STD_PHP_INI_ENTRY("ezlog.memcachehost",         	"127.0.0.1",  PHP_INI_ALL, OnUpdateString, memcachehost, zend_ezlog_globals, ezlog_globals)
    STD_PHP_INI_ENTRY("ezlog.memcacheport",		"11211",  PHP_INI_ALL, OnUpdateLong,   memcacheport, zend_ezlog_globals, ezlog_globals)
    STD_PHP_INI_BOOLEAN("ezlog.onredis",    "0", PHP_INI_ALL, OnUpdateBool, onredis, zend_ezlog_globals, ezlog_globals)
    STD_PHP_INI_ENTRY("ezlog.redishost",            "127.0.0.1",  PHP_INI_ALL, OnUpdateString, redishost, zend_ezlog_globals, ezlog_globals)
    STD_PHP_INI_ENTRY("ezlog.redisport",            "6379",  PHP_INI_ALL, OnUpdateString, redisport, zend_ezlog_globals, ezlog_globals)
    STD_PHP_INI_BOOLEAN("ezlog.onfile",    "0", PHP_INI_ALL, OnUpdateBool, onfile, zend_ezlog_globals, ezlog_globals)
    STD_PHP_INI_ENTRY("ezlog.filepath",            "/tmp/ezlog.log",  PHP_INI_ALL, OnUpdateString, filepath, zend_ezlog_globals, ezlog_globals)
PHP_INI_END()


/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_ezlog_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_ezlog_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "ezlog", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_ezlog_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_ezlog_init_globals(zend_ezlog_globals *ezlog_globals)
{
	ezlog_globals->global_value = 0;
	ezlog_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ezlog)
{
	
	REGISTER_INI_ENTRIES();
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(ezlog)
{
	
	UNREGISTER_INI_ENTRIES();
	
	return SUCCESS;
}

char *getCurTime()
{
    time_t timep;
    struct tm *lt;
    time (&timep);
    lt = localtime(&timep);
    char *ct;
    spprintf(&ct,0,"%d/%d/%d %d:%d:%d",lt->tm_year+1900,lt->tm_mon+1,lt->tm_mday,lt->tm_hour,lt->tm_min,lt->tm_sec);
    return ct;
}

void getUri(char **uri TSRMLS_DC)
{
    *uri = (char *)SG(request_info).request_uri;
}

char *getData(char *method TSRMLS_DC)
{
    zval **carrier = NULL;
    zval **vv = NULL;
    char result[1024*1024] = {0};
    if(strcmp(method,"_POST")==0){
        (void)zend_hash_find(&EG(symbol_table),ZEND_STRS("_POST"),(void **)&carrier);
    }else{
        (void)zend_hash_find(&EG(symbol_table),ZEND_STRS("_GET"),(void **)&carrier);
    }
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(*carrier));
    int INDEX = 0;
    while(zend_hash_get_current_data(Z_ARRVAL_P(*carrier),(void **)&vv) == SUCCESS){
        if(INDEX == 0){
            char *prefix = "DATA METHOD: ";
            strcat(result,prefix);
            strcat(result,method);
        }
        char *arKey = (*((*(**carrier).value.ht).pInternalPointer)).arKey;
        char *log = NULL;
        spprintf(&log,0,"[304374456] key: %s value: %s ",arKey,Z_STRVAL_P(*vv));
        if(strlen(log)+strlen(result)+1 < 1024*1024){
            strcat(result,log);
        }
        INDEX++;
        zend_hash_move_forward(Z_ARRVAL_P(*carrier));
    }
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(*carrier));
    return result;
}

void sendToFile(char *log TSRMLS_DC)
{
    FILE *pf;
    pf = fopen(EZLOG_G(filepath),"a+");
    if(pf != NULL){
        char *enter = "\n";
        strcat(log,enter);
        fputs(log,pf);
        fclose(pf);
    }
}

void sendToRedis(char *key, char *log TSRMLS_DC)
{
    zend_hash_internal_pointer_reset(EG(class_table));
    char *class = NULL;
    int class_len = 0;
    zend_class_entry **ce = NULL;
    class_len = spprintf(&class,0,"redis");
    if(zend_hash_find(EG(class_table),"redis",class_len+1,(void *)&ce) == SUCCESS){
        zval *memobj;
        MAKE_STD_ZVAL(memobj);
        object_init_ex(memobj,*ce);
        zval *mret = NULL;
        zval *pv1;
        MAKE_STD_ZVAL(pv1);
        ZVAL_STRING(pv1,EZLOG_G(redishost),1);
        zval *pv2;
        MAKE_STD_ZVAL(pv2);
        ZVAL_STRING(pv2,EZLOG_G(redisport),1);
        zval ***params = (zval ***)emalloc(sizeof(zval **)*2);
        params[0] = &pv1;
        params[1] = &pv2;
        zval *method_name;
        MAKE_STD_ZVAL(method_name);
       	ZVAL_STRING(method_name,"connect",1);
        call_user_function_ex(&(*ce)->function_table,&memobj,method_name,&mret,2,params,1,NULL TSRMLS_CC);
        if(mret){
            zval ***paramskv = (zval ***)emalloc(sizeof(zval **)*2);
            zval *k,*v;
            zval *ret = NULL;
            MAKE_STD_ZVAL(k);
            MAKE_STD_ZVAL(v);
            ZVAL_STRING(k,key,1);
            ZVAL_STRING(v,log,1);
            paramskv[0] = &k;
            paramskv[1] = &v;
            zval *set;
            MAKE_STD_ZVAL(set);
            ZVAL_STRING(set,"set",1);
            mret = NULL;
            call_user_function_ex(&(*ce)->function_table,&memobj,set,&ret,2,paramskv,1,NULL TSRMLS_CC);
            zval *close;
            MAKE_STD_ZVAL(close);
            ZVAL_STRING(close,"close",1);
            zval *closeRet = NULL;
            call_user_function_ex(&(*ce)->function_table,&memobj,close,&closeRet,0, 0, 1, NULL TSRMLS_CC);

            zval_ptr_dtor(&k);
            zval_ptr_dtor(&v);
            zval_ptr_dtor(&set);
            zval_ptr_dtor(&close);
            zval_ptr_dtor(paramskv);
        }
        zval_ptr_dtor(&pv1);
        zval_ptr_dtor(&pv2);
        zval_ptr_dtor(&method_name);
        zval_ptr_dtor(&memobj);
        efree(params);
    }
}

void sendToMemcache(char *key, char *log TSRMLS_DC)
{
    zend_hash_internal_pointer_reset(EG(class_table));
    char *class = NULL;
    int class_len = 0;
    zend_class_entry **ce = NULL;
    class_len = spprintf(&class,0,"memcache");
    if(zend_hash_find(EG(class_table),"memcache",class_len+1,(void *)&ce) == SUCCESS){
        zval *memobj;
        MAKE_STD_ZVAL(memobj);
        object_init_ex(memobj,*ce);
        zval *pv1;
        MAKE_STD_ZVAL(pv1);
        ZVAL_STRING(pv1,EZLOG_G(memcachehost),1);
        zval *pv2;
        MAKE_STD_ZVAL(pv2);
        ZVAL_LONG(pv2,EZLOG_G(memcacheport));
        zval ***params = (zval ***)emalloc(sizeof(zval **)*2);
        params[0] = &pv1;
        params[1] = &pv2;
        
        zval *mret;
        char *addserver;
        int len_addserver = spprintf(&addserver,0,"connect");
        zend_function    *fptr;
        zval *method_name;
        MAKE_STD_ZVAL(method_name);
        ZVAL_STRINGL(method_name, addserver, len_addserver, 0);
        if(zend_hash_find(&(*ce)->function_table,addserver,len_addserver+1,(void **)&fptr) == SUCCESS){
            call_user_function_ex(&(*ce)->function_table, &memobj, method_name, &mret, 2, params, 1, NULL TSRMLS_CC);
        }
        if(mret){
            zval ***paramskv = (zval ***)emalloc(sizeof(zval **)*2);
            zval *k,*v;
            zval *ret = NULL;
            MAKE_STD_ZVAL(k);
            MAKE_STD_ZVAL(v);
            ZVAL_STRING(k,key,1);
            ZVAL_STRING(v,log,1);
            paramskv[0] = &k;
            paramskv[1] = &v;
            zval *set;
            MAKE_STD_ZVAL(set);
            ZVAL_STRING(set,"set",1);
            call_user_function_ex(&(*ce)->function_table,&memobj,set,&ret,2,paramskv,1,NULL TSRMLS_CC);
            zval *close;
            MAKE_STD_ZVAL(close);
            ZVAL_STRING(close,"close",1);
            zval *closeRet = NULL;
            call_user_function_ex(&(*ce)->function_table,&memobj,close,&closeRet,0, 0, 1, NULL TSRMLS_CC);

            zval_ptr_dtor(&k);
            zval_ptr_dtor(&v);
            zval_ptr_dtor(&set);
            zval_ptr_dtor(&close);
            efree(paramskv);
        }
        zval_ptr_dtor(&memobj);
        zval_ptr_dtor(&pv1);
        zval_ptr_dtor(&pv2);
        zval_ptr_dtor(&method_name);
        efree(params);
    }
}

void sendToMemcached(char *key, char *log TSRMLS_DC)
{
    zend_hash_internal_pointer_reset(EG(class_table));
    char *class = NULL;
    int class_len = 0;
    zend_class_entry **ce = NULL;
    class_len = spprintf(&class,0,"memcached");
    if(zend_hash_find(EG(class_table),"memcached",class_len+1,(void *)&ce) == SUCCESS){
        zval *memobj;
        MAKE_STD_ZVAL(memobj);
        object_init_ex(memobj,*ce);
        zval *pv1;
        MAKE_STD_ZVAL(pv1);
        ZVAL_STRING(pv1,EZLOG_G(memcachehost),1);
        zval *pv2;
        MAKE_STD_ZVAL(pv2);
        ZVAL_LONG(pv2,EZLOG_G(memcacheport));
        zval ***params = (zval ***)emalloc(sizeof(zval **)*2);
        params[0] = &pv1;
        params[1] = &pv2;

        zval *mret;
        char *addserver;
        int len_addserver = spprintf(&addserver,0,"addserver");
        zend_function    *fptr;
        zval *method_name;
        MAKE_STD_ZVAL(method_name);
        ZVAL_STRINGL(method_name, addserver, len_addserver, 0);
        if(zend_hash_find(&(*ce)->function_table,addserver,len_addserver+1,(void **)&fptr) == SUCCESS){
            call_user_function_ex(&(*ce)->function_table, &memobj, method_name, &mret, 2, params, 1, NULL TSRMLS_CC);
        }
        if(mret){
            zval ***paramskv = (zval ***)emalloc(sizeof(zval **)*2);
            zval *k,*v;
            zval *ret = NULL;
            MAKE_STD_ZVAL(k);
            MAKE_STD_ZVAL(v);
            ZVAL_STRING(k,key,1);
            ZVAL_STRING(v,log,1);
            paramskv[0] = &k;
            paramskv[1] = &v;
            zval *set;
            MAKE_STD_ZVAL(set);
            ZVAL_STRING(set,"set",1);
            call_user_function_ex(&(*ce)->function_table,&memobj,set,&ret,2,paramskv,1,NULL TSRMLS_CC);
            zval *close;
            MAKE_STD_ZVAL(close);
            ZVAL_STRING(close,"quit",1);
            zval *closeRet = NULL;
            call_user_function_ex(&(*ce)->function_table,&memobj,close,&closeRet,0, 0, 1, NULL TSRMLS_CC);

            zval_ptr_dtor(&k);
            zval_ptr_dtor(&v);
            zval_ptr_dtor(&set);
            zval_ptr_dtor(&close);
            efree(paramskv);
        }
        zval_ptr_dtor(&memobj);
        zval_ptr_dtor(&pv1);
        zval_ptr_dtor(&pv2);
        zval_ptr_dtor(&method_name);
        efree(params);
    }
}


/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(ezlog)
{
        char *time = getCurTime();
        char *uri = NULL;
        getUri(&uri TSRMLS_CC);
        char *log = NULL;
        char *key = NULL;
        if(uri){
            char *pmethod = "_POST";
            char pData[1024*1024+1024];
            char *Data = getData(pmethod TSRMLS_CC);
            strcpy(pData,Data);
            Data = NULL;
            char *gmethod = "_GET";
            Data = getData(gmethod TSRMLS_CC);
            spprintf(&log,0,"Time: %s Uri: %s [EZLOG] %s [EZLOG] %s",time,uri,pData,Data);
            spprintf(&key,0,"__EZLOG__%s_%s",uri,time);
            if(EZLOG_G(onredis)){
                sendToRedis(key,log TSRMLS_CC);
            }
            if(EZLOG_G(onfile)){
                sendToFile(log TSRMLS_CC);
            }
            if(EZLOG_G(onmemcache)){
                sendToMemcache(key,log TSRMLS_CC);
            }
        }
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(ezlog)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(ezlog)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "ezlog support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
	
}
/* }}} */

/* {{{ ezlog_functions[]
 *
 * Every user visible function must have an entry in ezlog_functions[].
 */
const zend_function_entry ezlog_functions[] = {
	PHP_FE(confirm_ezlog_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in ezlog_functions[] */
};
/* }}} */

/* {{{ ezlog_module_entry
 */
zend_module_entry ezlog_module_entry = {
	STANDARD_MODULE_HEADER,
	"ezlog",
	ezlog_functions,
	PHP_MINIT(ezlog),
	PHP_MSHUTDOWN(ezlog),
	PHP_RINIT(ezlog),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(ezlog),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(ezlog),
	PHP_EZLOG_VERSION,
	PHP_MODULE_GLOBALS(ezlog),
	NULL,
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_EZLOG
ZEND_GET_MODULE(ezlog)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
