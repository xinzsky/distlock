/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
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
#include "ext/standard/info.h"
#include "php_distlock.h"
#include <zookeeper.h>
#include "zoo_lock.h"

/* If you declare any globals in php_distlock.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(distlock)
*/

#define PHP_DISTLOCK_CONN   "distlock connect"
#define PHP_DISTLOCK_MUTEX  "distlock mutex"

/* True global resources - no need for thread safety here */
static int le_connect;
static int le_mutex;

/* {{{ distlock_functions[]
 *
 * Every user visible function must have an entry in distlock_functions[].
 */
const zend_function_entry distlock_functions[] = {
	PHP_FE(distlock_connect,NULL)
	PHP_FE(distlock_init,	NULL)
	PHP_FE(distlock_lock,	NULL)
	PHP_FE(distlock_trylock,	NULL)
	PHP_FE(distlock_unlock,	NULL)
	PHP_FE(distlock_free,	NULL)
	PHP_FE(distlock_disconnect,NULL)
	{NULL, NULL, NULL} 	/* Must be the last line in distlock_functions[], or PHP_FE_END */
};
/* }}} */

/* {{{ distlock_module_entry
 */
zend_module_entry distlock_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"distlock",
	distlock_functions,
	PHP_MINIT(distlock),
	PHP_MSHUTDOWN(distlock),
	PHP_RINIT(distlock),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(distlock),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(distlock),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_DISTLOCK
ZEND_GET_MODULE(distlock)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("distlock.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_distlock_globals, distlock_globals)
    STD_PHP_INI_ENTRY("distlock.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_distlock_globals, distlock_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_distlock_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_distlock_init_globals(zend_distlock_globals *distlock_globals)
{
	distlock_globals->global_value = 0;
	distlock_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(distlock)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
  le_connect = zend_register_list_destructors_ex(NULL, NULL, PHP_DISTLOCK_CONN, module_number);
  le_mutex = zend_register_list_destructors_ex(NULL, NULL, PHP_DISTLOCK_MUTEX, module_number);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(distlock)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(distlock)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(distlock)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(distlock)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "distlock support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

/* {{{ proto resource distlock_connect(string host_list, int session_timeout)
    */
PHP_FUNCTION(distlock_connect)
{
	char *host_list = NULL;
	int argc = ZEND_NUM_ARGS();
	int host_list_len;
	long session_timeout;
  zhandle_t *zh;

	if (zend_parse_parameters(argc TSRMLS_CC, "sl", &host_list, &host_list_len, &session_timeout) == FAILURE) 
	  return;
        
  if(session_timeout <= 0)
    session_timeout = 10000;

  zh = zookeeper_init(host_list,NULL,session_timeout,0,NULL,0);
  if(!zh)
    RETURN_NULL();

  ZEND_REGISTER_RESOURCE(return_value, zh, le_connect);
}
/* }}} */

/* {{{ proto resource distlock_init(resource conn_handle, string lock_path)
    */
PHP_FUNCTION(distlock_init)
{
	char *lock_path = NULL;
	int argc = ZEND_NUM_ARGS();
	int conn_handle_id = -1;
	int lock_path_len;
	zval *conn_handle = NULL;
  zhandle_t *zh;
  zkr_lock_mutex_t *mutex;

	if (zend_parse_parameters(argc TSRMLS_CC, "rs", &conn_handle, &lock_path, &lock_path_len) == FAILURE) 
		return;

	if (conn_handle) {
		ZEND_FETCH_RESOURCE(zh, zhandle_t *, &conn_handle, conn_handle_id, PHP_DISTLOCK_CONN, le_connect);
    mutex = (zkr_lock_mutex_t *)emalloc(sizeof(zkr_lock_mutex_t));
    if (zkr_lock_init(mutex, zh, lock_path, &ZOO_OPEN_ACL_UNSAFE) != 0) {
      efree(mutex);
      RETURN_NULL();
    }
    ZEND_REGISTER_RESOURCE(return_value, mutex, le_mutex);
	} else {
    RETURN_NULL();
  }
}
/* }}} */

/* {{{ proto int distlock_lock(resource mutex, int timeout)
    */
PHP_FUNCTION(distlock_lock)
{
	int argc = ZEND_NUM_ARGS();
	int mutex_id = -1;
	int timeout;
	zval *mutex = NULL;
  zkr_lock_mutex_t *lm;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &mutex, &timeout) == FAILURE) 
		return;
	
	if(timeout <= 0)
	  timeout = -1;

	if (mutex) {
		ZEND_FETCH_RESOURCE(lm, zkr_lock_mutex_t *, &mutex, mutex_id, PHP_DISTLOCK_MUTEX, le_mutex);
    if(zkr_lock_lock(lm, timeout) == 1)
      RETURN_TRUE;
	} 
	
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto int distlock_trylock(resource mutex)
    */
PHP_FUNCTION(distlock_trylock)
{
	int argc = ZEND_NUM_ARGS();
	int mutex_id = -1;
	zval *mutex = NULL;
  zkr_lock_mutex_t *lm;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &mutex) == FAILURE) 
		return;
	
	if (mutex) {
		ZEND_FETCH_RESOURCE(lm, zkr_lock_mutex_t *, &mutex, mutex_id, PHP_DISTLOCK_MUTEX, le_mutex);
    if(zkr_lock_lock(lm, 0) == 1)
      RETURN_TRUE;
	} 
	
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto int distlock_unlock(resource mutex)
    */
PHP_FUNCTION(distlock_unlock)
{
	int argc = ZEND_NUM_ARGS();
	int mutex_id = -1;
	zval *mutex = NULL;
  zkr_lock_mutex_t *lm;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &mutex) == FAILURE) 
		return;

	if (mutex) {
		ZEND_FETCH_RESOURCE(lm, zkr_lock_mutex_t *, &mutex, mutex_id, PHP_DISTLOCK_MUTEX, le_mutex);
    if(zkr_lock_unlock(lm) == 0) 
      RETURN_TRUE;
	} 
        
  RETURN_FALSE;
}
/* }}} */

/* {{{ proto int distlock_free(resource mutex)
    */
PHP_FUNCTION(distlock_free)
{
	int argc = ZEND_NUM_ARGS();
	int mutex_id = -1;
	zval *mutex = NULL;
  zkr_lock_mutex_t *lm;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &mutex) == FAILURE) 
		return;

	if (mutex) {
		ZEND_FETCH_RESOURCE(lm, zkr_lock_mutex_t *, &mutex, mutex_id, PHP_DISTLOCK_MUTEX, le_mutex);
    zkr_lock_destroy(lm);                
    efree(lm);
    RETURN_TRUE;
	} else
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto int distlock_disconnect(resource conn_handle)
    */
PHP_FUNCTION(distlock_disconnect)
{
	int argc = ZEND_NUM_ARGS();
	int conn_handle_id = -1;
	zval *conn_handle = NULL;
  zhandle_t *zh;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &conn_handle) == FAILURE) 
		return;

	if (conn_handle) {
		ZEND_FETCH_RESOURCE(zh, zhandle_t *, &conn_handle, conn_handle_id, PHP_distlock_CONN, le_connect);
    zookeeper_close(zh);
    RETURN_TRUE;
	} else
    RETURN_FALSE;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
