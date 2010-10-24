/* g-ring.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "g-ring.h"

#ifndef g_malloc0_n
#define g_malloc0_n(x,y) g_malloc0(x * y)
#endif

#define get_element(r,i) ((r)->data + ((r)->elt_size * i))

typedef struct _GRealRing GRealRing;

struct _GRealRing
{
	guint8          *data;      /* Pointer to real data. */
	guint            len;       /* Length of data allocation. */
	guint            pos;       /* Position in ring. */
	guint            elt_size;  /* Size of each element. */
	gboolean         looped;    /* Have we wrapped around at least once. */
	GDestroyNotify   destroy;   /* Destroy element callback. */
	volatile gint    ref_count; /* Reference count. */
};

/**
 * g_ring_sized_new:
 * @element_size: The size per element.
 * @reserved_size: The number of elements to allocate.
 *
 * Creates a new instance of #GRing with the given number of elements.
 *
 * Returns: None.
 * Side effects: None.
 */
GRing*
g_ring_sized_new (guint          element_size,    /* IN */
                  guint          reserved_size,   /* IN */
                  GDestroyNotify element_destroy) /* IN */
{
	GRealRing *real_ring;

	real_ring = g_slice_new0(GRealRing);
	real_ring->elt_size = element_size;
	real_ring->len = reserved_size;
	real_ring->data = g_malloc0_n(reserved_size, element_size);
	real_ring->destroy = element_destroy;
	real_ring->ref_count = 1;

	return (GRing *)real_ring;
}

/**
 * g_ring_append_vals:
 * @ring: A #GRing.
 * @data: A pointer to the array of values.
 * @len: The number of values.
 *
 * Appends @len values located at @data.
 *
 * Returns: None.
 * Side effects: None.
 */
void
g_ring_append_vals (GRing         *ring, /* IN */
                    gconstpointer  data, /* IN */
                    guint          len)  /* IN */
{
	GRealRing *real_ring = (GRealRing *)ring;
	gpointer idx;
	gint x;
	gint i;

	g_return_if_fail(real_ring != NULL);

	for (i = 0; i < len; i++) {
		x = ring->pos - i;
		x = (x >= 0) ? x : ring->len + x;
		idx = ring->data + (real_ring->elt_size * x);
		if (real_ring->destroy && real_ring->looped == TRUE) {
			real_ring->destroy(idx);
		}
		memcpy(idx, data, real_ring->elt_size);
		ring->pos++;
		if (ring->pos >= ring->len) {
			real_ring->looped = TRUE;
		}
		ring->pos %= ring->len;
		data += real_ring->elt_size;
	}
}

/**
 * g_ring_foreach:
 * @ring: A #GRing.
 * @func: A #GFunc.
 * @user_data: user data for @func.
 *
 * Calls @func for every item in the #GRing starting from the most recently
 * inserted element to the least recently inserted.
 *
 * Returns: None.
 * Side effects: None.
 */
void
g_ring_foreach (GRing    *ring,      /* IN */
                GFunc     func,      /* IN */
                gpointer  user_data) /* IN */
{
	GRealRing *real_ring = (GRealRing *)ring;
	gint i;

	g_return_if_fail(real_ring != NULL);

	for (i = real_ring->pos - 1; i >= 0; i--) {
		func(get_element(real_ring, i), user_data);
	}
	for (i = ring->len - 1; i >= real_ring->pos; i--) {
		func(get_element(real_ring, i), user_data);
	}
}

/**
 * g_ring_destroy:
 * @ring: A #GRing.
 *
 * Cleans up after a #GRing that is no longer in use.
 *
 * Returns: None.
 * Side effects: None.
 */
void
g_ring_destroy (GRing *ring) /* IN */
{
	GRealRing *real_ring = (GRealRing *)ring;

	g_return_if_fail(ring != NULL);
	g_return_if_fail(real_ring->ref_count == 0);

	g_free(real_ring->data);
}

/**
 * g_ring_ref:
 * @ring: A #GRing.
 *
 * Atomically increments the reference count of @ring by one.
 *
 * Returns: @ring.
 * Side effects: None.
 */
GRing*
g_ring_ref (GRing *ring) /* IN */
{
	GRealRing *real_ring = (GRealRing *)ring;

	g_return_val_if_fail(ring != NULL, NULL);
	g_return_val_if_fail(real_ring->ref_count > 0, NULL);

	g_atomic_int_inc(&real_ring->ref_count);
	return ring;
}

/**
 * g_ring_unref:
 * @ring: A #GRing.
 *
 * Atomically decrements the reference count of @ring by one.  When the
 * reference count reaches zero, the structure is freed.
 *
 * Returns: None.
 * Side effects: None.
 */
void
g_ring_unref (GRing *ring) /* IN */
{
	GRealRing *real_ring = (GRealRing *)ring;

	g_return_if_fail(ring != NULL);
	g_return_if_fail(real_ring->ref_count > 0);

	if (g_atomic_int_dec_and_test(&real_ring->ref_count)) {
		g_ring_destroy(ring);
	}
}

/**
 * g_ring_get_type:
 *
 * Retrieves the #GType identifier for #GRing.
 *
 * Returns: The #GType for #GRing.
 * Side effects: The type is registered on first call.
 */
GType
g_ring_get_type (void)
{
	static gsize initialized = FALSE;
	static GType type_id = 0;

	if (g_once_init_enter(&initialized)) {
		type_id = g_boxed_type_register_static("GRing",
		                                       (GBoxedCopyFunc)g_ring_ref,
		                                       (GBoxedFreeFunc)g_ring_unref);
		g_once_init_leave(&initialized, TRUE);
	}
	return type_id;
}
