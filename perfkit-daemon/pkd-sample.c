/* pkd-sample.c
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

#include "pkd-sample.h"

/**
 * SECTION:pkd-sample
 * @title: PkdSample
 * @short_description: Source samples
 *
 * #PkdSample is an abstraction for data samples which are recorded from a
 * #PkdSource.
 */

struct _PkdSample
{
	volatile gint  ref_count;
	GArray        *data;
};

static void
pkd_sample_destroy (PkdSample *sample)
{
}

static void
pkd_sample_free (PkdSample *sample)
{
	g_slice_free (PkdSample, sample);
}

/**
 * pkd_sample_new:
 *
 * Creates a new instance of #PkdSample.
 *
 * Return value: the newly created #PkdSample instance.
 */
PkdSample*
pkd_sample_new (void)
{
	PkdSample *sample;

	sample = g_slice_new0 (PkdSample);
	sample->ref_count = 1;
	sample->data = g_array_new (TRUE, TRUE, sizeof (gchar));

	return sample;
}

/**
 * pkd_sample_sized_new:
 * @n_bytes: the number of bytes to allocate
 *
 * Creates a new instance of #PkdSample.  The buffer is initialized to
 * the number of bytes specified by @n_bytes.
 *
 * Return value: the newly created #PkdSample instance.
 */
PkdSample*
pkd_sample_sized_new (gsize n_bytes)
{
	PkdSample *sample;

	sample = g_slice_new0 (PkdSample);
	sample->ref_count = 1;
	sample->data = g_array_sized_new (TRUE, TRUE, sizeof (gchar), n_bytes);

	return sample;
}

/**
 * pkd_sample_ref:
 * @sample: A #PkdSample
 *
 * Atomically increments the reference count of @sample by one.
 *
 * Return value: the #PkdSample reference.
 */
PkdSample*
pkd_sample_ref (PkdSample *sample)
{
	g_return_val_if_fail (sample != NULL, NULL);
	g_return_val_if_fail (sample->ref_count > 0, NULL);

	g_atomic_int_inc (&sample->ref_count);
	return sample;
}

/**
 * pkd_sample_unref:
 * @sample: A #PkdSample
 *
 * Atomically decrements the reference count of @sample by one.
 * When the reference count reaches zero, the structure is destroyed and freed.
 */
void
pkd_sample_unref (PkdSample *sample)
{
	g_return_if_fail (sample != NULL);
	g_return_if_fail (sample->ref_count > 0);

	if (g_atomic_int_dec_and_test (&sample->ref_count)) {
		pkd_sample_destroy (sample);
		pkd_sample_free (sample);
	}
}

GType
pkd_sample_get_type (void)
{
	static GType gtype = 0;

	if (g_once_init_enter ((gsize*)&gtype)) {
		GType init_gtype;

		init_gtype = g_boxed_type_register_static ("PkdSample",
		                                           (GBoxedCopyFunc)pkd_sample_ref,
		                                           (GBoxedFreeFunc)pkd_sample_unref);
		g_once_init_leave ((gsize*)&gtype, (gsize)init_gtype);
	}

	return gtype;
}

/**
 * pkd_sample_write_int:
 * @sample: A #PkdSample
 * @v_int: the integer to write
 *
 * Writes @v_int to the buffer.
 */
void
pkd_sample_write_int (PkdSample *sample,
                      gint       v_int)
{
	g_return_if_fail (sample != NULL);
	g_array_append_vals (sample->data, &v_int, sizeof (gint));
}

/**
 * pkd_sample_write_char:
 * @sample: A #PkdSample
 * @v_char: a #gchar
 *
 * Writes a single byte onto the sample buffer.
 */
void
pkd_sample_write_char (PkdSample *sample,
                       gchar      v_char)
{
	g_return_if_fail (sample != NULL);
	g_array_append_val (sample->data, v_char);
}

/**
 * pkd_sample_get_buffer:
 * @sample: A #PkdSample
 *
 * Retrieves the internal data buffer for @sample.  If the buffer is to be
 * stored somewhere, it should be referenced with g_array_ref().
 *
 * Return value: a #GArray containing the buffer.
 */
GArray*
pkd_sample_get_buffer (PkdSample *sample)
{
	g_return_val_if_fail (sample != NULL, NULL);
	return sample->data;
}
