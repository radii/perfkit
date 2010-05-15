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
#include "pka-log.h"

/**
 * SECTION:pka-context
 * @title: PkaContext
 * @short_description: Security context.
 *
 * 
 */

static guint context_seq = 1;

struct _PkaContext
{
	volatile gint ref_count;
	guint id;
};

static void
pka_context_destroy (PkaContext *context)
{
}

/**
 * pka_context_default:
 *
 * Retrieves the default context for the process used for internal purposes
 * only.
 *
 * Returns: A #PkaContext which has not had its reference count incremented.
 * Side effects: None.
 */
PkaContext*
pka_context_default (void)
{
	static PkaContext *default_context = NULL;
	PkaContext *context;

	if (G_UNLIKELY(g_once_init_enter((gsize *)&default_context))) {
		context = pka_context_new();
		context->id = 0;
		g_once_init_leave((gsize *)&default_context, (gsize)context);
	}
	return default_context;
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

	ENTRY;
	context = g_slice_new0(PkaContext);
	context->ref_count = 1;
	context->id = g_atomic_int_exchange_and_add((gint *)&context_seq, 1);
	RETURN(context);
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

/**
 * pka_context_get_id:
 * @context: A #PkaContext.
 *
 * Retrieves the identifier for the context.
 *
 * Returns: The context identifier.
 * Side effects: None.
 */
guint
pka_context_get_id (PkaContext *context) /* IN */
{
	g_return_val_if_fail(context != NULL, G_MAXUINT);
	return context->id;
}

/**
 * pka_context_is_authorized:
 * @context: A #PkaContext.
 *
 * Checks to see if the context is authorized to make the #PkaIOControl call.
 *
 * Returns: %TRUE if the context is authorized; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_context_is_authorized (PkaContext   *context, /* IN */
                           PkaIOControl  ioctl)   /* IN */
{
	ENTRY;
	RETURN(TRUE);
}

/**
 * pka_context_is_authenticated:
 * @context: A #PkaContext.
 *
 * Checks to see if the context has been authenticated.
 *
 * Returns: %TRUE if the context is authenticated; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_context_is_authenticated (PkaContext *context) /* IN */
{
	ENTRY;
	RETURN(TRUE);
}

/**
 * pka_context_error_quark:
 *
 * Retrieves the #PkaContext error domain #GQuark.
 *
 * Returns: A #GQuark.
 * Side effects: None.
 */
GQuark
pka_context_error_quark (void)
{
	return g_quark_from_static_string("pka-context-error-quark");
}
