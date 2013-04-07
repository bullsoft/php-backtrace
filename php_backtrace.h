#ifndef PHP_BACKTRACE_H
#define PHP_BACKTRACE_H

#define PHP_BACKTRACE_EXTNAME   "backtrace"
#define PHP_BACKTRACE_EXTVER    "0.2.1"
#define PHP_BACKTRACE_AUTHOR    "Vladimir Kolesnikov"
#define PHP_BACKTRACE_URL       "http://blog.sjinks.pro/"
#define PHP_BACKTRACE_COPYRIGHT "Copyright (c) 2009-2013"

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <main/php.h>
#include <main/php_ini.h>
#include <main/SAPI.h>
#include <Zend/zend_extensions.h>
#include <Zend/zend_builtin_functions.h>
#include <ext/standard/php_smart_str.h>

#include <signal.h>

#define BACKTRACE_BT_SIGNAL SIGCONT

#ifdef ZTS
#	include <TSRM.h>
#endif

#ifdef ZTS
#	define BACKTRACE_G(v) TSRMG(backtrace_globals_id, zend_backtrace_globals*, v)
#else
#	define BACKTRACE_G(v) (backtrace_globals.v)
#endif

#if __GNUC__ >= 4
#	define BACKTRACE_VISIBILITY_HIDDEN __attribute__((visibility("hidden")))
#	define BACKTRACE_ATTRIBUTE_MALLOC __attribute__((malloc))
#else
#	define BACKTRACE_VISIBILITY_HIDDEN
#	define BACKTRACE_ATTRIBUTE_MALLOC
#endif

BACKTRACE_VISIBILITY_HIDDEN extern zend_module_entry backtrace_module_entry;

#define phpext_backtrace_ptr &backtrace_module_entry

#if COMPILE_DL_BACKTRACE
#	define XXX_EXTENSION_ENTRY zend_extension_entry
extern ZEND_DLEXPORT zend_extension zend_extension_entry;
#else
#	define XXX_EXTENSION_ENTRY backtrace_extension_entry
BACKTRACE_VISIBILITY_HIDDEN extern zend_extension backtrace_extension_entry;
#endif

ZEND_BEGIN_MODULE_GLOBALS(backtrace)
	char* btdir;
	zend_bool safe_backtrace;
	zend_bool skip_args;
	zend_bool be_nice;
	zend_bool enabled;
ZEND_END_MODULE_GLOBALS(backtrace)

#endif
