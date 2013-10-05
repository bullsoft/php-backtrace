#include "backtrace.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "zts_support.h"
#include "backtrace_methods.h"
#include "common.h"

volatile sig_atomic_t got_signal = 0;
ZEND_DECLARE_MODULE_GLOBALS(backtrace);

void signal_handler(int sig, siginfo_t* si, void* unused)
{
	TSRMLS_FETCH();

#ifdef DEBUG
	fprintf(stderr, "In signal handler, PID is %d\n", getpid());
	fflush(stderr);
#endif

	if (!BACKTRACE_G(be_nice)) {
		got_signal = 0;
#ifndef ZTS
		if (EG(current_execute_data)) {
#endif
			do_backtrace(TSRMLS_C);
#ifndef ZTS
		}
#endif
	}
	else {
		got_signal = 1;
	}

#ifdef DEBUG
	fprintf(stderr, "Quit signal handler, PID is %d\n", getpid());
	fflush(stderr);
#endif
}

static void make_backtrace(int fd, zend_backtrace_globals* g TSRMLS_DC)
{
	smart_str s;
	s.c = NULL;

	smart_str_appends(&s, "Script: ");
	smart_str_appends(&s, SG(request_info).path_translated ? SG(request_info).path_translated : "???");
	smart_str_appendc(&s, '\n');

	smart_str_0(&s);
	safe_write(fd, s.c, s.len);

	smart_str_free(&s);
	s.c = NULL;

	if (EG(active) && EG(current_execute_data)) {
		if (g->safe_backtrace) {
			safe_backtrace(fd TSRMLS_CC);
		}
		else {
			debug_backtrace(fd, g->skip_args TSRMLS_CC);
		}
	}

	smart_str_appends(&s, "Dump succeeded.\n\n\n");
	smart_str_0(&s);
	safe_write(fd, s.c, s.len);

	smart_str_free(&s);
	s.c = NULL;

	close(fd);
}

void do_backtrace(TSRMLS_D)
{
	char buf[2048];
	size_t len = strlen(BACKTRACE_G(btdir));
	if (len > 2000) {
#ifdef DEBUG
		fprintf(stderr, "[%d]: len is too big\n", getpid());
		fflush(stderr);
#endif
		return;
	}

	memmove(buf, BACKTRACE_G(btdir), len);
	if (buf[len-1] != '/' && buf[len-1] != '\\') {
		buf[len] = '/';
		++len;
	}

#ifdef ZTS
	len += sprintf(
		buf + len,
		"%d_%ld.trace",
		getpid(),
		(long int)tsrm_thread_id()
	);
#else
	len += sprintf(
		buf + len,
		"%d.trace",
		getpid()
	);
#endif

	buf[len] = 0;

	int fd = open(buf, O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (-1 == fd) {
		// Do nothing for now
#ifdef DEBUG
		fprintf(stderr, "[%d]: open(): %s\n", getpid(), strerror(errno));
		fflush(stderr);
#endif
	}

#ifdef DEBUG
	fprintf(stderr, "[%d]: %d = open(%s)\n", getpid(), fd, buf);
	fflush(stderr);
#endif

	{
		smart_str s = { NULL, 0, 0 };

		long int x = (long int)getpid();
		smart_str_appends(&s, "PID: ");
		smart_str_append_long(&s, x);
		smart_str_appendc(&s, '\n');

		x = (long int)getuid();
		smart_str_appends(&s, "UID: ");
		smart_str_append_long(&s, x);
		smart_str_appendc(&s, '\n');

		x = (long int)getgid();
		smart_str_appends(&s, "GID: ");
		smart_str_append_long(&s, x);
		smart_str_appendc(&s, '\n');

		smart_str_0(&s);
		safe_write(fd, s.c, s.len);

		smart_str_free(&s);
	}

#ifdef ZTS
	zend_backtrace_globals* g = (zend_backtrace_globals*)(*((void***)tsrm_ls))[TSRM_UNSHUFFLE_RSRC_ID(backtrace_globals_id)];
#else
	zend_backtrace_globals* g = &backtrace_globals;
#endif

#ifdef ZTS
	call_backtrace(fd, g, make_backtrace);
#else
	make_backtrace(fd, g TSRMLS_CC);
#endif

	close(fd);

#ifdef DEBUG
	fprintf(stderr, "[%d]: close(%d)\n", getpid(), fd);
	fflush(stderr);
#endif

}

