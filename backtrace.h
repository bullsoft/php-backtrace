/*
 * backtrace.h
 *
 *  Created on: 19.03.2010
 *      Author: vladimir
 */

#ifndef BACKTRACE_H_
#define BACKTRACE_H_

#include "php_backtrace.h"

BACKTRACE_VISIBILITY_HIDDEN extern ZEND_DECLARE_MODULE_GLOBALS(backtrace);

BACKTRACE_VISIBILITY_HIDDEN extern volatile sig_atomic_t got_signal;
BACKTRACE_VISIBILITY_HIDDEN extern void signal_handler(int sig, siginfo_t* si, void* unused);
BACKTRACE_VISIBILITY_HIDDEN extern void do_backtrace(TSRMLS_D);

#endif /* BACKTRACE_H_ */

