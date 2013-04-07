/*
 * backtrace_methods.h
 *
 *  Created on: 19.03.2010
 *      Author: vladimir
 */

#ifndef BACKTRACE_METHODS_H_
#define BACKTRACE_METHODS_H_

#include "php_backtrace.h"

BACKTRACE_VISIBILITY_HIDDEN extern void safe_backtrace(int fd TSRMLS_DC);
BACKTRACE_VISIBILITY_HIDDEN extern void debug_backtrace(int fd, int skip_args TSRMLS_DC);

#endif /* BACKTRACE_METHODS_H_ */

