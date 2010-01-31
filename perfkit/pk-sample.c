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

#include <egg-buffer.h>

#include "pk-sample.h"

/**
 * SECTION:pk-sample
 * @title: PkSample
 * @short_description: 
 *
 * 
 */

static gboolean decode (PkSample *sample, EggBuffer *buffer);

struct _PkSample
{
	volatile gint ref_count;
};

static void
pk_sample_destroy (PkSample *sample)
{
}

/**
 * pk_sample_new:
 *
 * Creates a new instance of #PkSample.
 *
 * Returns: The newly created instance of PkSample.
 *
 * Side effects: None.
 */
static PkSample*
pk_sample_new (void)
{
	PkSample *sample;

	sample = g_slice_new0(PkSample);
	sample->ref_count = 1;

	return sample;
}

/**
 * pk_sample_new_from_data:
 * @data: a buffer of data.
 * @length: the length of the buffer.
 *
 * Creates a new #PkSample from a buffer of data.
 *
 * Returns: the #PkSample if successful; otherwise NULL.
 *
 * Side effects: None.
 */
PkSample*
pk_sample_new_from_data (const guint8 *data,
                         gsize         length)
{
	PkSample *sample;
	EggBuffer *buffer;

	sample = pk_sample_new();
	buffer = egg_buffer_new_from_data(data, length);

	if (!decode(sample, buffer)) {
		pk_sample_unref(sample);
		sample = NULL;
	}

	egg_buffer_unref(buffer);

	return sample;
}

/**
 * pk_sample_ref:
 * sample: A #PkSample
 *
 * Atomically increments the reference count of @sample by one.
 *
 * Returns: The @sample pointer with its reference count incremented.
 *
 * Side effects: None.
 */
PkSample*
pk_sample_ref (PkSample *sample)
{
	g_return_val_if_fail(sample != NULL, NULL);
	g_return_val_if_fail(sample->ref_count > 0, NULL);

	g_atomic_int_inc(&sample->ref_count);

	return sample;
}

/**
 * pk_sample_unref:
 * sample: A #PkSample
 *
 * Atomically decrements the reference count of @sample by one.
 * When the reference count reaches zero, the structures resources as well as
 * the structure are freed.
 *
 * Returns: The @sample pointer with its reference count incremented.
 *
 * Side effects: None.
 */
void
pk_sample_unref (PkSample *sample)
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(sample->ref_count > 0);

	if (g_atomic_int_dec_and_test(&sample->ref_count)) {
		pk_sample_destroy(sample);
		g_slice_free(PkSample, sample);
	}
}

GType
pk_sample_get_type (void)
{
	static GType type_id = 0;
	GType _type_id;

	if (g_once_init_enter((gsize *)&type_id)) {
		_type_id = g_boxed_type_register_static("PkSample",
		                                        (GBoxedCopyFunc)pk_sample_ref,
		                                        (GBoxedFreeFunc)pk_sample_unref);
		g_once_init_leave((gsize *)&type_id, _type_id);
	}

	return type_id;
}

static gboolean
decode (PkSample  *sample,
        EggBuffer *buffer)
{
	return TRUE;
}
