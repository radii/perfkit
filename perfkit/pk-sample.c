/* pk-sample.c
 * 
 * Copyright (C) 2009 Christian Hergert
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

#include "pk-sample.h"

/**
 * SECTION:pk-sample
 * @title: PkSample
 * @short_description: Source samples
 *
 * #PkSample is an abstraction for data samples which are recorded from a
 * #PkSource.
 */

struct _PkSample
{
	volatile gint  ref_count;
	GArray        *data;
};

static void
pk_sample_destroy (PkSample *sample)
{
	g_array_unref (sample->data);
}

static void
pk_sample_free (PkSample *sample)
{
	g_slice_free (PkSample, sample);
}

/**
 * pk_sample_new:
 *
 * Creates a new instance of #PkSample.
 *
 * Return value: the newly created #PkSample instance.
 *
 * Side effects: None
 */
PkSample*
pk_sample_new (void)
{
	PkSample *sample;

	sample = g_slice_new0 (PkSample);
	sample->ref_count = 1;
	sample->data = g_array_new (TRUE, TRUE, sizeof (gchar));

	return sample;
}

/**
 * pk_sample_sized_new:
 * @n_bytes: the number of bytes to allocate
 *
 * Creates a new instance of #PkSample.  The buffer is initialized to
 * the number of bytes specified by @n_bytes.
 *
 * Return value: the newly created #PkSample instance.
 *
 * Side effects: None
 */
PkSample*
pk_sample_sized_new (gsize n_bytes)
{
	PkSample *sample;

	sample = g_slice_new0 (PkSample);
	sample->ref_count = 1;
	sample->data = g_array_sized_new (TRUE, TRUE, sizeof (gchar), n_bytes);

	return sample;
}

/**
 * pk_sample_ref:
 * @sample: A #PkSample
 *
 * Atomically increments the reference count of @sample by one.
 *
 * Return value: the #PkSample reference.
 *
 * Side effects: The reference count of @sample is incremented.
 */
PkSample*
pk_sample_ref (PkSample *sample)
{
	g_return_val_if_fail (sample != NULL, NULL);
	g_return_val_if_fail (sample->ref_count > 0, NULL);

	g_atomic_int_inc (&sample->ref_count);
	return sample;
}

/**
 * pk_sample_unref:
 * @sample: A #PkSample
 *
 * Atomically decrements the reference count of @sample by one.
 * When the reference count reaches zero, the structure is destroyed and freed.
 *
 * Side effects: The ref count of @sample is decremented, and potentially the
 *   instance is destroyed and freed.
 */
void
pk_sample_unref (PkSample *sample)
{
	g_return_if_fail (sample != NULL);
	g_return_if_fail (sample->ref_count > 0);

	if (g_atomic_int_dec_and_test (&sample->ref_count)) {
		pk_sample_destroy (sample);
		pk_sample_free (sample);
	}
}

GType
pk_sample_get_type (void)
{
	static GType gtype = 0;
	GType        init_gtype;

	if (g_once_init_enter ((gsize*)&gtype)) {
		init_gtype = g_boxed_type_register_static ("PkSample",
		                                           (GBoxedCopyFunc)pk_sample_ref,
		                                           (GBoxedFreeFunc)pk_sample_unref);
		g_once_init_leave ((gsize*)&gtype, (gsize)init_gtype);
	}

	return gtype;
}

/**
 * pk_sample_get_array:
 * @sample: A #PkSample
 *
 * Retrieves the internal data buffer for @sample.  If the array is to be
 * stored somewhere, it should be referenced with g_array_ref().
 *
 * Return value: a #GArray containing the buffer.
 *
 * Side effects: None
 */
GArray*
pk_sample_get_array (PkSample *sample)
{
	g_return_val_if_fail (sample != NULL, NULL);
	return sample->data;
}

PkSample*
pk_sample_new_from_data (const gchar *data,
                         gsize        len)
{
	PkSample *sample;

	sample = g_slice_new0 (PkSample);
	sample->ref_count = 1;
	sample->data = g_array_sized_new (FALSE, FALSE, sizeof (gchar), len);
	g_array_append_vals (sample->data, data, len);

	return sample;

}
