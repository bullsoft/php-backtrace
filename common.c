/*
 * common.c
 *
 *  Created on: 19.03.2010
 *      Author: vladimir
 */

#include "common.h"

ssize_t safe_write(int fd, const void* buf, size_t size)
{
	if (0 == size) return 0;

	ssize_t res;
	do {
		res = write(fd, buf, size);
	} while (-1 == res && (EAGAIN == errno || EINTR == errno));

#ifdef DEBUG
	if (-1 == res) {
		fprintf(stderr, "[%d]: write(): %s\n", getpid(), strerror(errno));
		fflush(stderr);
	}

	fprintf(stderr, "[%d]: %s\n", getpid(), (const char*)buf);
	fflush(stderr);
#endif

#if _POSIX_C_SOURCE >= 199309L || _XOPEN_SOURCE >= 500
	fdatasync(fd);
#elif _BSD_SOURCE || _XOPEN_SOURCE || _POSIX_C_SOURCE >= 200112L
	fsync(fd);
#endif

	return res;
}

