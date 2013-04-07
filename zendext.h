/*
 * zendext.h
 *
 *  Created on: 19.03.2010
 *      Author: vladimir
 */

#ifndef ZENDEXT_H_
#define ZENDEXT_H_

#include "php_backtrace.h"

BACKTRACE_VISIBILITY_HIDDEN extern zend_module_entry backtrace_module_entry;

#define phpext_backtrace_ptr &backtrace_module_entry

#endif /* ZENDEXT_H_ */

