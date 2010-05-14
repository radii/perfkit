/* pka-context.c
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pka-context.h"

#ifndef DISABLE_TRACE
#define TRACE(_m,...)                                               \
    G_STMT_START {                                                  \
        g_log(G_LOG_DOMAIN, (1 << G_LOG_LEVEL_USER_SHIFT),          \
              _m, __VA_ARGS__);                                     \
    } G_STMT_END
#else
#define TRACE(_m,...)
#endif

#define ENTRY TRACE("ENTRY: %s():%d", G_STRFUNC, __LINE__)

#define EXIT                                                        \
    G_STMT_START {                                                  \
        TRACE(" EXIT: %s():%d", G_STRFUNC, __LINE__);               \
        return;                                                     \
    } G_STMT_END

#define RETURN(_r)                                                  \
    G_STMT_START {                                                  \
        TRACE(" EXIT: %s():%d", G_STRFUNC, __LINE__);               \
        return _r;                                                  \
    } G_STMT_END

#define GOTO(_l)                                                    \
    G_STMT_START {                                                  \
        TRACE(" GOTO: %s:%d", #_l, __LINE__);                       \
        goto _l;                                                    \
    } G_STMT_END

#define BREAK                                                       \
    TRACE("BREAK: %s:%d", G_STRFUNC, __LINE__);                     \
    break

/**
 * SECTION:pka-context
 * @title: PkaContext
 * @short_description: Security context.
 *
 * Add docs.
 */

struct _PkaContext
{
	volatile gint ref_count;
};

static void
pka_context_destroy (PkaContext *context)
{
}

/**
 * pka_context_new:
 *
 * Creates a new instance of #PkaContext.
 *
 * Returns: the newly created instance.
 */
PkaContext*
pka_context_new (void)
{
	PkaContext *context;

	context = g_slice_new0(PkaContext);
	context->ref_count = 1;

	return context;
}

/**
 * PkaContext_ref:
 * @context: A #PkaContext.
 *
 * Atomically increments the reference count of @context by one.
 *
 * Returns: A reference to @context.
 * Side effects: None.
 */
PkaContext*
pka_context_ref (PkaContext *context)
{
	g_return_val_if_fail(context != NULL, NULL);
	g_return_val_if_fail(context->ref_count > 0, NULL);

	g_atomic_int_inc(&context->ref_count);
	return context;
}

/**
 * pka_context_unref:
 * @context: A PkaContext.
 *
 * Atomically decrements the reference count of @context by one.  When the
 * reference count reaches zero, the structure will be destroyed and
 * freed.
 *
 * Returns: None.
 * Side effects: The structure will be freed when the reference count
 *   reaches zero.
 */
void
pka_context_unref (PkaContext *context)
{
	g_return_if_fail(context != NULL);
	g_return_if_fail(context->ref_count > 0);

	if (g_atomic_int_dec_and_test(&context->ref_count)) {
		pka_context_destroy(context);
		g_slice_free(PkaContext, context);
	}
}

gboolean
pka_context_is_authorized (PkaContext   *context, /* IN */
                           PkaIOControl  ioctl)   /* IN */
{
	RETURN(TRUE);
}

gboolean
pka_context_is_authenticated (PkaContext *context) /* IN */
{
	RETURN(TRUE);
}

GQuark
pka_context_error_quark (void)
{
	return g_quark_from_static_string("pka-context-error-quark");
}
