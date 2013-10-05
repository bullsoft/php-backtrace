#include "backtrace_methods.h"
#include "common.h"

BACKTRACE_ATTRIBUTE_MALLOC static smart_str* arg_to_string(zval** arg, char* comma TSRMLS_DC)
{
	smart_str* res = (smart_str*)emalloc(sizeof(smart_str));
	res->c = NULL;

	if (arg && *arg) {
		switch (Z_TYPE_PP(arg)) {
			case IS_BOOL:
				smart_str_appends(res, "bool(");
				smart_str_appends(res, Z_LVAL_PP(arg) ? "true" : "false");
				smart_str_appendc(res, ')');
				break;

			case IS_NULL:
				smart_str_appends(res, "NULL");
				break;

			case IS_LONG:
				smart_str_appends(res, "long(");
				smart_str_append_long(res, Z_LVAL_PP(arg));
				smart_str_appendc(res, ')');
				break;

			case IS_DOUBLE: {
				zval* copy = *arg;
				zval_copy_ctor(copy);
				smart_str_appends(res, "double(");
				zend_locale_sprintf_double(copy ZEND_FILE_LINE_CC);
				smart_str_appendl(res, Z_STRVAL_P(copy), Z_STRLEN_P(copy));
				smart_str_appendc(res, ')');
				zval_ptr_dtor(&copy);
				break;
			}

			case IS_STRING:
				smart_str_appends(res, "string(\"");
				if (Z_STRVAL_PP(arg)) {
					char* s = Z_STRVAL_PP(arg);
					int len = Z_STRLEN_PP(arg) <= 100 ? Z_STRLEN_PP(arg) : 100;
					smart_str_appendl(res, s, len);
					if (Z_STRLEN_PP(arg) > 100) {
						smart_str_appends(res, "...");
					}
				}

				smart_str_appends(res, "\")");
				break;

			case IS_ARRAY:
#ifdef IS_CONSTANT_ARRAY
			case IS_CONSTANT_ARRAY:
#endif
				smart_str_appends(res, "array(");
				smart_str_append_long(res, zend_hash_num_elements(Z_ARRVAL_PP(arg)));

				smart_str_appends(res, " elements)");
				break;

			case IS_OBJECT: {
#if PHP_VERSION_ID >= 50400
				const
#endif
				char* class_name = NULL;
				zend_uint class_name_len;
				HashTable* ht;
#ifdef Z_OBJDEBUG_PP
				int is_temp;
				ht = Z_OBJDEBUG_PP(arg, is_temp);
#else
				ht = Z_OBJPROP_PP(arg);
#endif
				zend_object_handle handle = Z_OBJ_HANDLE_PP(arg);
				Z_OBJ_HANDLER(**arg, get_class_name)(*arg, &class_name, &class_name_len, 0 TSRMLS_CC);
				smart_str_appends(res, "object(");
				smart_str_appends(res, class_name);
				smart_str_appends(res, " #");
				smart_str_append_long(res, handle);
				smart_str_appendc(res, ' ');
				smart_str_append_long(res, ht ? zend_hash_num_elements(ht) : 0);
				smart_str_appendc(res, ')');
				efree((char*)class_name);

#ifdef Z_OBJDEBUG_PP
				if (is_temp) {
					zend_hash_destroy(ht);
					efree(ht);
				}
#endif
				break;
			}

			case IS_RESOURCE: {
#if PHP_VERSION_ID >= 50400
				const
#endif
				char* tn = zend_rsrc_list_get_rsrc_type(Z_LVAL_PP(arg) TSRMLS_CC);
				smart_str_appends(res, "resource(");
				smart_str_append_long(res, Z_LVAL_PP(arg));
				smart_str_appendc(res, ' ');
				smart_str_appends(res, tn ? tn : "unknown");
				smart_str_appendc(res, ')');
				break;
			}

			default: {
				zval* copy = *arg;
				zval_copy_ctor(copy);
				convert_to_string(copy);
				smart_str_appends(res, "<UNKNOWN>(");
				smart_str_appendl(res, Z_STRVAL_P(copy), Z_STRLEN_P(copy));
				zval_ptr_dtor(&copy);
				smart_str_appendc(res, ')');

				break;
			}
		}

		smart_str_appends(res, comma);
	}
	else {
		smart_str_appends(res, "???, ");
	}

	smart_str_0(res);
	return res;
}

void safe_backtrace(int fd TSRMLS_DC)
{
	struct _zend_execute_data* d = EG(current_execute_data);
	int i = 1;
	smart_str s;

	s.c = NULL;

	while (d) {
		smart_str_append_long(&s, i);
		smart_str_appends(&s, ". ");
		zend_function* func = d->function_state.function;

		if (func && func->common.function_name) {
			if (d->object && IS_OBJECT == Z_TYPE_P(d->object)) {
				if (func->common.scope && func->common.scope->name) {
					smart_str_appends(&s, func->common.scope->name);
					smart_str_appends(&s, "->");
				}
				else {
					smart_str_appends(&s, "<Unknown Class>->");
				}
			}
			else if (func->common.scope) {
				if (func->common.scope->name) {
					smart_str_appends(&s, func->common.scope->name);
					smart_str_appends(&s, "::");
				}
				else {
					smart_str_appends(&s, "<Unknown Class>::");
				}
			}

			smart_str_appends(&s, func->common.function_name);
			smart_str_appends(&s, ", ");
		}
		else {
			if (!d->opline || d->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
				smart_str_appends(&s, "(unknown), ");
			}
			else {
				switch (
#if PHP_VERSION_ID >= 50400
					d->opline->extended_value
#else
					d->opline->op2.u.constant.value.lval
#endif
				) {
					case ZEND_EVAL:
						smart_str_appends(&s, "eval, ");
						break;

					case ZEND_INCLUDE:
						smart_str_appends(&s, "include, ");
						break;

					case ZEND_REQUIRE:
						smart_str_appends(&s, "require, ");
						break;

					case ZEND_INCLUDE_ONCE:
						smart_str_appends(&s, "include_once, ");
						break;

					case ZEND_REQUIRE_ONCE:
						smart_str_appends(&s, "require_once, ");
						break;

					default:
						smart_str_appends(&s, "(unknown), ");
						break;
				}
			}
		}

		zend_op_array* op = d->op_array;
		if (op && op->filename) {
			smart_str_appends(&s, op->filename);
			smart_str_appends(&s, ", ");
		}
		else {
			smart_str_appends(&s, "(unknown), ");
		}

		struct _zend_op* opline = d->opline;
		if (opline) {
			smart_str_append_long(&s, opline->lineno);
		}
		else {
			smart_str_appendc(&s, '?');
		}

		smart_str_appendc(&s, '\n');
		smart_str_0(&s);

		safe_write(fd, s.c, s.len);

		smart_str_free(&s);
		s.c = NULL;

		d = d->prev_execute_data;
		++i;
	}
}

void debug_backtrace(int fd, int skip_args TSRMLS_DC)
{
	static ulong hfile  = (ulong)-1;
	static ulong hline  = (ulong)-1;
	static ulong hfunc  = (ulong)-1;
	static ulong htype  = (ulong)-1;
	static ulong hclass = (ulong)-1;
	static ulong hargs  = (ulong)-1;

	char* b_file;
	long int b_line;
	char* b_func;
	char* b_type;
	char* b_class;
	zval* args;

	smart_str s;
	zval** v;
	zval backtrace;

	if ((ulong)-1 == hfile) {
		hfile  = zend_get_hash_value("file", sizeof("file"));
		hline  = zend_get_hash_value("line", sizeof("line"));
		hfunc  = zend_get_hash_value("function", sizeof("function"));
		htype  = zend_get_hash_value("type", sizeof("type"));
		hclass = zend_get_hash_value("class", sizeof("class"));
		hargs  = zend_get_hash_value("args", sizeof("args"));
	}

	s.c = NULL;
	INIT_ZVAL(backtrace);

	zend_fetch_debug_backtrace(
		&backtrace,
		0,
		0
#if PHP_VERSION_ID >= 50400
		, 0
#endif
		TSRMLS_CC
	);

	smart_str_appends(&s, "Backtrace succeeded.\n");
	smart_str_0(&s);
	safe_write(fd, s.c, s.len);

	smart_str_free(&s);
	s.c = NULL;

	HashPosition pos;
	zval** current;
	int i = 0;

	for (
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL(backtrace), &pos);
			SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL(backtrace), (void**)&current, &pos);
			zend_hash_move_forward_ex(Z_ARRVAL(backtrace), &pos)
	)
	{
		++i;
		smart_str_append_long(&s, i);
		smart_str_appendl(&s, ". ", 2);

		b_file  = (FAILURE == zend_hash_quick_find(Z_ARRVAL_PP(current), "file",     sizeof("file"),     hfile,  (void**)&v)) ? NULL : (Z_TYPE_PP(v) == IS_STRING ? estrdup(Z_STRVAL_PP(v)) : NULL);
		b_line  = (FAILURE == zend_hash_quick_find(Z_ARRVAL_PP(current), "line",     sizeof("line"),     hline,  (void**)&v)) ? 0    : (Z_TYPE_PP(v) == IS_LONG   ? Z_LVAL_PP(v)            : 0);
		b_func  = (FAILURE == zend_hash_quick_find(Z_ARRVAL_PP(current), "function", sizeof("function"), hfunc,  (void**)&v)) ? NULL : (Z_TYPE_PP(v) == IS_STRING ? estrdup(Z_STRVAL_PP(v)) : NULL);
		b_type  = (FAILURE == zend_hash_quick_find(Z_ARRVAL_PP(current), "type",     sizeof("type"),     htype,  (void**)&v)) ? NULL : (Z_TYPE_PP(v) == IS_STRING ? estrdup(Z_STRVAL_PP(v)) : NULL);
		b_class = (FAILURE == zend_hash_quick_find(Z_ARRVAL_PP(current), "class",    sizeof("class"),    hclass, (void**)&v)) ? NULL : (Z_TYPE_PP(v) == IS_STRING ? estrdup(Z_STRVAL_PP(v)) : NULL);
		args    = (skip_args || FAILURE == zend_hash_quick_find(Z_ARRVAL_PP(current), "args", sizeof("args"), hargs, (void**)&v)) ? NULL : (Z_TYPE_PP(v) == IS_ARRAY  ? *v                  : NULL);

		if (b_class && b_type) {
			smart_str_appends(&s, b_class);
			smart_str_appends(&s, b_type);
			efree(b_class);
			efree(b_type);
		}

		if (b_func) {
			smart_str_appends(&s, b_func);
			efree(b_func);
		}
		else {
			smart_str_appendl(&s, "<unknown function>", sizeof("<unknown function>") - 1);
		}

		smart_str_appendc(&s, '(');

		if (args) {
			HashPosition a_pos;
			zval** a_current;
			int num = zend_hash_num_elements(Z_ARRVAL_P(args));
			int cur = 0;

			for (
					zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(args), &a_pos);
					SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_P(args), (void**)&a_current, &a_pos);
					zend_hash_move_forward_ex(Z_ARRVAL_P(args), &a_pos)
			)
			{
				++cur;

				if (!a_current || !*a_current) {
					smart_str_appends(&s, "<ERROR>");
					break;
				}

				char* comma    = (cur == num) ? "" : ", ";
				smart_str* arg = arg_to_string(a_current, comma TSRMLS_CC);
				smart_str_append(&s, arg);
				smart_str_free(arg);
				efree(arg);
			}
		}

		smart_str_appends(&s, "), ");
		if (b_file) {
			smart_str_appends(&s, b_file);
			efree(b_file);
		}
		else {
			smart_str_appends(&s, "(unknown)");
		}

		smart_str_appendc(&s, ':');
		smart_str_append_long(&s, b_line);
		smart_str_appendc(&s, '\n');

		smart_str_0(&s);

		safe_write(fd, s.c, s.len);

		smart_str_free(&s);
		s.c = NULL;
	}

	zval_dtor(&backtrace);
}
