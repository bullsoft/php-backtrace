#ifndef ZTS_SUPPORT_H_
#define ZTS_SUPPORT_H_

#include "php_backtrace.h"

#ifdef ZTS

typedef void (*backtrace_callback_t)(int fd, zend_backtrace_globals* g TSRMLS_DC);

BACKTRACE_VISIBILITY_HIDDEN extern void backtrace_zts_startup(TSRMLS_D);
BACKTRACE_VISIBILITY_HIDDEN extern void backtrace_zts_shutdown(TSRMLS_D);
BACKTRACE_VISIBILITY_HIDDEN extern void call_backtrace(int fd, zend_backtrace_globals* g, backtrace_callback_t callback);

#endif

#endif /* ZTS_SUPPORT_H_ */

