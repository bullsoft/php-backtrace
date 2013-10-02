/*
 * zendext.c
 *
 *  Created on: 19.03.2010
 *      Author: vladimir
 */

#include "backtrace.h"
#include <ext/standard/info.h>
#include "zts_support.h"

static void (*old_execute_internal)(zend_execute_data* execute_data_ptr, int return_value_used TSRMLS_DC);

#if !COMPILE_DL_BACKTRACE
zend_extension backtrace_extension_entry;
#endif

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("backtrace.directory",        "/tmp",  PHP_INI_ALL, OnUpdateString, btdir,          zend_backtrace_globals, backtrace_globals)
	STD_PHP_INI_BOOLEAN("backtrace.enabled",        "1",     PHP_INI_ALL, OnUpdateBool,   enabled,        zend_backtrace_globals, backtrace_globals)
	STD_PHP_INI_BOOLEAN("backtrace.safe_backtrace", "1",     PHP_INI_ALL, OnUpdateBool,   safe_backtrace, zend_backtrace_globals, backtrace_globals)
	STD_PHP_INI_BOOLEAN("backtrace.skip_args",      "0",     PHP_INI_ALL, OnUpdateBool,   skip_args,      zend_backtrace_globals, backtrace_globals)
	STD_PHP_INI_BOOLEAN("backtrace.be_nice",        "1",     PHP_INI_ALL, OnUpdateBool,   be_nice,        zend_backtrace_globals, backtrace_globals)
PHP_INI_END()

static PHP_MINIT_FUNCTION(backtrace)
{
#ifdef DEBUG
	fprintf(stderr, "[%d]: backtrace module startup\n", getpid());
	fflush(stderr);
#endif

	REGISTER_INI_ENTRIES();

	if (BACKTRACE_G(enabled)) {
		int res;
		struct sigaction sa;

		sigfillset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART | SA_SIGINFO;
		sa.sa_sigaction = signal_handler;

		res = sigaction(BACKTRACE_BT_SIGNAL, &sa, NULL);
		if (-1 == res) {
			zend_error(E_CORE_ERROR, "sigaction() failed: %s", strerror(errno));
			return FAILURE;
		}
	}

#if !COMPILE_DL_BACKTRACE
	{
		zend_extension extension = backtrace_extension_entry;
		extension.handle = NULL;
		zend_llist_add_element(&zend_extensions, &extension);
	}
#endif

	return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(backtrace)
{
#ifdef DEBUG
	fprintf(stderr, "[%d]: backtrace module shutdown\n", getpid());
	fflush(stderr);
#endif

	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

static PHP_RINIT_FUNCTION(backtrace)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, BACKTRACE_BT_SIGNAL);
#if defined(ZTS) && defined(PTHREADS)
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
#else
	sigprocmask(SIG_UNBLOCK, &set, NULL);
#endif

#ifdef DEBUG
	fprintf(stderr, "Request init, PID is %d\n", getpid());
	fflush(stderr);
#endif

	return SUCCESS;
}

static PHP_GINIT_FUNCTION(backtrace)
{
#ifdef DEBUG
	fprintf(stderr, "backtrace module ginit\n");
	fflush(stderr);
#endif

	backtrace_globals->btdir          = NULL;
	backtrace_globals->safe_backtrace = 1;
	backtrace_globals->skip_args      = 0;
	backtrace_globals->be_nice        = 1;
}

static PHP_MINFO_FUNCTION(backtrace)
{
	int signal = BACKTRACE_BT_SIGNAL;
	char buf[64];

	php_sprintf(buf, "%d", signal);

	php_info_print_table_start();
	php_info_print_table_row(2, "Backtrace Module", "enabled");
	php_info_print_table_row(2, "version", PHP_BACKTRACE_EXTVER);
	php_info_print_table_row(2, "Signal", buf);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

zend_module_entry backtrace_module_entry = {
	STANDARD_MODULE_HEADER,
	PHP_BACKTRACE_EXTNAME,
	NULL,
	PHP_MINIT(backtrace),
	PHP_MSHUTDOWN(backtrace),
	PHP_RINIT(backtrace),
	NULL,
	PHP_MINFO(backtrace),
	PHP_BACKTRACE_EXTVER,
	PHP_MODULE_GLOBALS(backtrace),
	PHP_GINIT(backtrace),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

static void backtrace_execute_internal(zend_execute_data* execute_data_ptr, int return_value_used TSRMLS_DC)
{
	if (1 == got_signal) {
		do_backtrace(TSRMLS_C);
		got_signal = 0;
	}

	old_execute_internal(execute_data_ptr, return_value_used TSRMLS_CC);

	if (1 == got_signal) {
		do_backtrace(TSRMLS_C);
		got_signal = 0;
	}
}

static int backtrace_zend_startup(zend_extension* extension)
{
#ifdef DEBUG
	fprintf(stderr, "backtrace_zend_startup()\n");
#	ifdef ZTS
	fprintf(stderr, "ZTS support enabled\n");
#	endif
	fflush(stderr);
#endif

#if COMPILE_DL_BACKTRACE
	if (FAILURE == zend_startup_module(&backtrace_module_entry)) {
		return FAILURE;
	}
#endif

	old_execute_internal = zend_execute_internal;
	if (NULL == old_execute_internal) {
		old_execute_internal = execute_internal;
	}

	zend_execute_internal = backtrace_execute_internal;

#ifdef ZTS
	{
		TSRMLS_FETCH();
		backtrace_zts_startup(TSRMLS_C);
	}
#endif

	return SUCCESS;
}

static void backtrace_zend_shutdown(zend_extension* extension)
{
	struct sigaction sa;
	int res;
#ifdef ZTS
	TSRMLS_FETCH();
#endif

	zend_execute_internal = (old_execute_internal == execute_internal) ? NULL : old_execute_internal;

#ifdef ZTS
	backtrace_zts_shutdown(TSRMLS_C);
#endif

	sigemptyset(&sa.sa_mask);
	sa.sa_flags   = SA_RESTART;
	sa.sa_handler = SIG_DFL;
	res = sigaction(BACKTRACE_BT_SIGNAL, &sa, NULL);
	if (-1 == res) {
		zend_error(E_CORE_WARNING, "sigaction() failed: %s", strerror(errno));
	}
}

#if COMPILE_DL_BACKTRACE
ZEND_DLEXPORT
#endif
zend_extension XXX_EXTENSION_ENTRY = {
	PHP_BACKTRACE_EXTNAME,
	PHP_BACKTRACE_EXTVER,
	PHP_BACKTRACE_AUTHOR,
	PHP_BACKTRACE_URL,
	PHP_BACKTRACE_COPYRIGHT,

	backtrace_zend_startup,  /* Startup */
	backtrace_zend_shutdown, /* Shutdown */
	NULL,                    /* Activate */
	NULL,                    /* Deactivate */

	NULL,    /* Message handler */
	NULL,    /* Op Array Handler */

	NULL,   /* Statement handler */
	NULL,   /* fcall begin handler */
	NULL,   /* fcall end handler */

	NULL, /* Op Array Constructor */
	NULL, /* Op Array Destructor */

	STANDARD_ZEND_EXTENSION_PROPERTIES
};

#ifndef ZEND_EXT_API
#	define ZEND_EXT_API ZEND_DLEXPORT
#endif

#if COMPILE_DL_BACKTRACE
ZEND_EXTENSION();
#endif
