/*
 * common.h
 *
 *  Created on: 19.03.2010
 *      Author: vladimir
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "php_backtrace.h"

BACKTRACE_VISIBILITY_HIDDEN extern ssize_t safe_write(int fd, const void* buf, size_t size);

#endif /* COMMON_H_ */

