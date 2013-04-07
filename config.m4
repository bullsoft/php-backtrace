PHP_ARG_ENABLE(
	[backtrace],
	[whether to enable the "backtrace" extension],
	[  --enable-backtrace      Enable "backtrace" extension support]
)

if test $PHP_BACKTRACE != "no"; then
	AC_CHECK_HEADERS([fcntl.h signal.h sys/types.h sys/stat.h])
	PHP_NEW_EXTENSION(backtrace, [backtrace.c zts_support.c common.c backtrace_methods.c zendext.c], $ext_shared, , [-Wall -Wextra -Wno-unused-parameter -std=gnu99 -D_GNU_SOURCE])
	PHP_SUBST(BACKTRACE_SHARED_LIBADD)
fi

