#include "zts_support.h"

#ifdef ZTS

static tsrm_thread_end_func_t thread_end_func = NULL;
static HashTable thread_ids;
static MUTEX_T mutex;

static void backtrace_thread_end_handler(THREAD_T thread_id, void*** tsrm_ls)
{
#ifdef DEBUG
	fprintf(stderr, "[%d] Registering a new thread: %lu\n", getpid(), (unsigned long int)thread_id);
	fflush(stderr);
#endif

	if (thread_end_func != NULL) {
		thread_end_func(thread_id, tsrm_ls);
	}

	char buf[64];
	unsigned long int tid = (unsigned long int)thread_id;
	php_sprintf(buf, "%lu", tid);

	zend_hash_add_empty_element(&thread_ids, buf, strlen(buf) + 1);
}

void backtrace_zts_startup(TSRMLS_D)
{
#ifdef DEBUG
	fprintf(stderr, "[%d]: backtrace_zts_startup()\n", getpid());
	fflush(stderr);
#endif

	mutex = tsrm_mutex_alloc();

	zend_hash_init(&thread_ids, 128, NULL, NULL, 1);
	thread_end_func = (tsrm_thread_end_func_t)tsrm_set_new_thread_end_handler(backtrace_thread_end_handler);
}

void backtrace_zts_shutdown(TSRMLS_D)
{
#ifdef DEBUG
	fprintf(stderr, "[%d]: backtrace_zts_shutdown()\n", getpid());
	fflush(stderr);
#endif

	tsrm_set_new_thread_end_handler(thread_end_func);
	tsrm_mutex_free(mutex);

	thread_end_func = NULL;

	zend_hash_clean(&thread_ids);
}

void call_backtrace(int fd, zend_backtrace_globals* g, backtrace_callback_t callback)
{
#ifdef DEBUG
	fprintf(stderr, "[%d]: call_backtrace()\n", getpid());
	fflush(stderr);
#endif

	if (!callback) {
		return;
	}

	HashPosition pos;
	void** current;
	THREAD_T self = tsrm_thread_id();
	int processed_self = 0;

	tsrm_mutex_lock(mutex);

	for (
			zend_hash_internal_pointer_reset_ex(&thread_ids, &pos);
			SUCCESS == zend_hash_get_current_data_ex(&thread_ids, (void**)&current, &pos);
			zend_hash_move_forward_ex(&thread_ids, &pos)
	)
	{
		char* key;
		uint key_len;
		ulong idx;
		int type;

		type = zend_hash_get_current_key_ex(&thread_ids, &key, &key_len, &idx, 0, &pos);
		if (HASH_KEY_IS_STRING == type) {
			idx = atol(key);
		}

#ifdef DEBUG
		fprintf(stderr, "[%d]: Trying thread %lu\n", getpid(), idx);
		fflush(stderr);
#endif

		if (idx) {
#if defined(PTHREADS)
			int res = pthread_kill((pthread_t)idx, 0);
			if (res) {
				continue;
			}
#elif defined(GNUPTH)
			int res = pth_raise((pth_t)idx, 0);
			if (!res) {
				continue;
			}
#endif

#ifdef DEBUG
			fprintf(stderr, "[%d]: Processing thread %lu\n", getpid(), idx);
			fflush(stderr);
#endif

			THREAD_T thread_id = idx;

#if defined(PTHREADS)
			if (!processed_self && pthread_equal(thread_id, self)) {
				processed_self = 1;
			}
#else
			if (thread_id == self) {
				processed_self = 1;
			}
#endif

			void*** tsrm_ls = (void***)ts_resource_ex(0, &thread_id);
			callback(fd, g, tsrm_ls);
#ifdef DEBUG
			fprintf(stderr, "[%d]: Processed thread %lu\n", getpid(), idx);
			fflush(stderr);
#endif
		}
	}

	if (!processed_self) {
#ifdef DEBUG
		fprintf(stderr, "[%d]: Processing self\n", getpid());
		fflush(stderr);
#endif
		void*** tsrm_ls = (void***)ts_resource_ex(0, (THREAD_T*)0);
		callback(fd, g, tsrm_ls);
#ifdef DEBUG
		fprintf(stderr, "[%d]: Processed self\n", getpid());
		fflush(stderr);
#endif
	}

	tsrm_mutex_unlock(mutex);
}

#endif

