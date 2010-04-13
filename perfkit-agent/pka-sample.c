/* pka-sample.c
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
#include <egg-time.h>
#include <string.h>

#include "pka-sample.h"

/**
 * SECTION:pka-sample
 * @title: PkaSample
 * @short_description: Samples within a data stream
 *
 * #PkaSample represents a set of data from a particular sample within a
 * data stream.  The internal format of this is a Google Protocol Buffer.
 *
 * Use the helper methods to help build your data buffer.
 */

struct _PkaSample
{
	volatile gint ref_count;

	GTimeVal    tv;              /* Time of sample. */
	gint        source_id;       /* Source identifier within the channel. */
	EggBuffer  *buf;
};

static void
pka_sample_destroy(PkaSample *sample)
{
	egg_buffer_unref(sample->buf);
}

/**
 * pka_sample_new:
 *
 * Creates a new instance of #PkaSample.
 *
 * Returns: the newly created #PkaSample.
 *
 * Side effects: None.
 */
PkaSample*
pka_sample_new (void)
{
	PkaSample *sample;

	sample = g_slice_new0(PkaSample);
	sample->ref_count = 1;
	sample->buf = egg_buffer_new();
	g_get_current_time(&sample->tv);

	return sample;
}

/**
 * pka_sample_ref:
 * @sample: A #PkaSample
 *
 * Atomically increases the reference count of @sample.
 *
 * Returns: The @sample pointer.
 *
 * Side effects: None.
 */
PkaSample*
pka_sample_ref (PkaSample *sample)
{
	g_return_val_if_fail(sample != NULL, NULL);
	g_return_val_if_fail(sample->ref_count > 0, NULL);

	g_atomic_int_inc(&sample->ref_count);

	return sample;
}

/**
 * pka_sample_unref:
 * @sample: A #PkaSample
 *
 * Atomically decrements the reference count of @sample.  When the reference
 * count reaches zero, the structure and its resources are freed.
 *
 * Side effects: The structure is freed if reference count reaches zero.
 */
void
pka_sample_unref (PkaSample *sample)
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(sample->ref_count > 0);

	if (g_atomic_int_dec_and_test(&sample->ref_count)) {
		pka_sample_destroy(sample);
		g_slice_free(PkaSample, sample);
	}
}

/**
 * pka_sample_get_data:
 * @sample: A #PkaSample
 * @data: A location for a buffer
 * @data_len: A location for the buffer length
 *
 * Retrieves the internal buffer for the sample.  The buffer should not be
 * modified or freed.
 *
 * Side effects: None.
 */
void
pka_sample_get_data (PkaSample     *sample,
                     const guint8 **data,
                     gsize         *data_len)
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(data != NULL);
	g_return_if_fail(data_len != NULL);

	egg_buffer_get_buffer(sample->buf, data, data_len);
}

/**
 * pka_sample_get_source_id:
 * @sample: A #PkaSample
 *
 * Retrieves the source index within channel.
 *
 * Returns: an integer containing the source id
 *
 * Side effects: None.
 */
gint
pka_sample_get_source_id (PkaSample *sample)
{
	return sample->source_id;
}

/**
 * pka_sample_set_source_id:
 * @sample: A #PkaSample
 * @source_id: The source id in the channel
 *
 * Internal method used to set the source id.
 */
void
pka_sample_set_source_id (PkaSample *sample,
                          gint       source_id)
{
	sample->source_id = source_id;
}

/**
 * pka_sample_get_timeval:
 * @sample: A #PkaSample
 * @tv: A #GTimeVal
 *
 * Retrieves the #GTimeVal for when the sample occurred.
 */
void
pka_sample_get_timeval (PkaSample *sample,
                        GTimeVal  *tv)
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(tv != NULL);

	*tv = sample->tv;
}

/**
 * pka_sample_set_timeval:
 * @sample: A #PkaSample
 * @tv: A #GTimeVal
 *
 * Sets the timeval for when @sample occurred.
 */
void
pka_sample_set_timeval (PkaSample *sample,
                        GTimeVal  *tv)
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(tv != NULL);

	sample->tv = *tv;
}

/**
 * pka_sample_append_boolean:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @b: A #gboolean to append to the buffer.
 *
 * Appends the boolean value @b to the buffer.
 *
 * Side effects: None.
 */
void
pka_sample_append_boolean (PkaSample *sample,
                           gint       field,
                           gboolean   b)
{
	g_return_if_fail(sample != NULL);

	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_BOOLEAN);
	egg_buffer_write_boolean(sample->buf, b);
}

/**
 * pka_sample_append_double:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @d: A #gdouble to append to the buffer.
 *
 * Appends the double value @d to the buffer.
 *
 * Side effects: None.
 */
void
pka_sample_append_double (PkaSample *sample,
                          gint       field,
                          gdouble    d)
{
	g_return_if_fail(sample != NULL);

	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_DOUBLE);
	egg_buffer_write_double(sample->buf, d);
}

/**
 * pka_sample_append_float:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @f: A #gfloat to append to the buffer.
 *
 * Appends the float value @f to the buffer.
 *
 * Side effects: None.
 */
void
pka_sample_append_float (PkaSample *sample,
                         gint       field,
                         gfloat     f)
{
	g_return_if_fail(sample != NULL);

	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_FLOAT);
	egg_buffer_write_float(sample->buf, f);
}

/**
 * pka_sample_append_int:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @i: A #gint to append to the buffer.
 *
 * Appends the integer value @i to the buffer.
 *
 * Side effects: None.
 */
void
pka_sample_append_int (PkaSample *sample,
                       gint       field,
                       gint       i)
{
	g_return_if_fail(sample != NULL);

	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_INT);
	egg_buffer_write_int(sample->buf, i);
}

/**
 * pka_sample_append_int64:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @i: A #gint64 to append to the buffer.
 *
 * Appends the 64-bit integer value @i to the buffer.
 *
 * Side effects: None.
 */
void
pka_sample_append_int64 (PkaSample *sample,
                         gint       field,
                         gint64     i)
{
	g_return_if_fail(sample != NULL);

	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_INT64);
	egg_buffer_write_int64(sample->buf, i);
}

/**
 * pka_sample_append_string:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @s: A string to append to the buffer.
 *
 * Appends the string @s to the buffer.
 *
 * Side effects: None.
 */
void
pka_sample_append_string (PkaSample   *sample,
                          gint         field,
                          const gchar *s)
{
	g_return_if_fail(sample != NULL);

	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_STRING);
	egg_buffer_write_string(sample->buf, s);
}

/**
 * pka_sample_append_uint:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @u: A #guint to append to the buffer.
 *
 * Appends the unsigned integer @u to the buffer.
 *
 * Side effects: None.
 */
void
pka_sample_append_uint (PkaSample *sample,
                        gint       field,
                        guint      u)
{
	g_return_if_fail(sample != NULL);

	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_UINT);
	egg_buffer_write_uint(sample->buf, u);
}

/**
 * pka_sample_append_uint64:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @u: A #guint64 to append to the buffer.
 *
 * Appends the 64-bit unsigned integer @u to the buffer.
 *
 * Side effects: None.
 */
void
pka_sample_append_uint64 (PkaSample *sample,
                          gint       field,
                          guint64    u)
{
	g_return_if_fail(sample != NULL);

	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_UINT64);
	egg_buffer_write_uint64(sample->buf, u);
}

/**
 * pka_sample_append_timeval:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @tv: A #GTimeVal to append to the buffer.
 *
 * Encodes @tv into the number of 100-nanosecond ticks since the Gregorian
 * Epoch (January 1, 01).  The value is stored as a 64-bit unsigned and is
 * guaranteed to take up no more than 10-bytes for the timeval alone.
 *
 * The field will take at least one addition byte (as always).
 *
 * Side effects: None.
 */
void
pka_sample_append_timeval (PkaSample *sample,
                           gint       field,
                           GTimeVal  *tv)
{
	gint64 ticks;

	g_return_if_fail(sample != NULL);

	g_time_val_to_ticks(tv, &ticks);
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_UINT64);
	egg_buffer_write_uint64(sample->buf, ticks);
}

/**
 * pka_sample_get_type:
 *
 * Returns: the #PkaSample #GType.
 */
GType
pka_sample_get_type (void)
{
	static GType type_id = 0;
	GType _type_id;

	if (g_once_init_enter((gsize *)&type_id)) {
		_type_id = g_boxed_type_register_static("PkaSample",
				(GBoxedCopyFunc)pka_sample_ref,
				(GBoxedFreeFunc)pka_sample_unref);
		g_once_init_leave((gsize *)&type_id, (gsize)_type_id);
	}

	return type_id;
}
