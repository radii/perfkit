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

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Sample"

#include <egg-buffer.h>
#include <egg-time.h>
#include <string.h>

#include "pka-log.h"
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
	volatile gint  ref_count;
	GTimeVal       tv;              /* Time of sample. */
	gint           source_id;       /* Source identifier within the channel. */
	EggBuffer     *buf;             /* Protocol buffer style data blob. */
};

/**
 * pka_sample_destroy:
 * @sample: A #PkaSample.
 *
 * Destroys the resources allocated by @sample.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_sample_destroy (PkaSample *sample) /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_unref(sample->buf);
	EXIT;
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

	ENTRY;
	sample = g_slice_new0(PkaSample);
	sample->ref_count = 1;
	sample->source_id = -1;
	sample->buf = egg_buffer_new();
	g_get_current_time(&sample->tv);
	RETURN(sample);
}

/**
 * pka_sample_ref:
 * @sample: A #PkaSample.
 *
 * Atomically increases the reference count of @sample by one.
 *
 * Returns: @sample.
 * Side effects: None.
 */
PkaSample*
pka_sample_ref (PkaSample *sample) /* IN */
{
	g_return_val_if_fail(sample != NULL, NULL);
	g_return_val_if_fail(sample->ref_count > 0, NULL);

	ENTRY;
	g_atomic_int_inc(&sample->ref_count);
	RETURN(sample);
}

/**
 * pka_sample_unref:
 * @sample: A #PkaSample.
 *
 * Atomically decrements the reference count of @sample by one.  When the
 * reference count reaches zero, the structure and its resources are freed.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_unref (PkaSample *sample) /* IN */
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(sample->ref_count > 0);

	ENTRY;
	if (g_atomic_int_dec_and_test(&sample->ref_count)) {
		pka_sample_destroy(sample);
		g_slice_free(PkaSample, sample);
	}
	EXIT;
}

/**
 * pka_sample_get_data:
 * @sample: A #PkaSample.
 * @data: A location for a buffer.
 * @data_len: A location for the buffer length.
 *
 * Retrieves the internal buffer for the sample.  The buffer should not be
 * modified or freed.
 *
 * Side effects: None.
 */
void
pka_sample_get_data (PkaSample     *sample,   /* IN */
                     const guint8 **data,     /* OUT */
                     gsize         *data_len) /* OUT */
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(data != NULL);
	g_return_if_fail(data_len != NULL);

	ENTRY;
	egg_buffer_get_buffer(sample->buf, data, data_len);
	EXIT;
}

/**
 * pka_sample_get_source_id:
 * @sample: A #PkaSample.
 *
 * Retrieves the id of the #PkaSource which created the sample.
 *
 * Returns: the source id.
 * Side effects: None.
 */
gint
pka_sample_get_source_id (PkaSample *sample) /* IN */
{
	g_return_val_if_fail(sample != NULL, -1);
	return sample->source_id;
}

/**
 * pka_sample_set_source:
 * @sample: A #PkaSample.
 * @source_id: The #PkaSource identifier.
 *
 * Internal method used to set the source id.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_set_source_id (PkaSample *sample,    /* IN */
                          gint       source_id) /* IN */
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(sample->source_id == -1);

	ENTRY;
	sample->source_id = source_id;
	EXIT;
}

/**
 * pka_sample_get_timeval:
 * @sample: A #PkaSample
 * @tv: A #GTimeVal
 *
 * Retrieves the #GTimeVal for when the sample occurred.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_get_timeval (PkaSample *sample, /* IN */
                        GTimeVal  *tv)     /* IN */
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
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_set_timeval (PkaSample *sample, /* IN */
                        GTimeVal  *tv)     /* IN */
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(tv != NULL);

	ENTRY;
	sample->tv = *tv;
	EXIT;
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
pka_sample_append_boolean (PkaSample *sample, /* IN */
                           gint       field,  /* IN */
                           gboolean   b)      /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_BOOLEAN);
	egg_buffer_write_boolean(sample->buf, b);
	EXIT;
}

/**
 * pka_sample_append_double:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @d: A #gdouble to append to the buffer.
 *
 * Appends the double value @d to the buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_append_double (PkaSample *sample, /* IN */
                          gint       field,  /* IN */
                          gdouble    d)      /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_DOUBLE);
	egg_buffer_write_double(sample->buf, d);
	EXIT;
}

/**
 * pka_sample_append_float:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @f: A #gfloat to append to the buffer.
 *
 * Appends the float value @f to the buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_append_float (PkaSample *sample, /* IN */
                         gint       field,  /* IN */
                         gfloat     f)      /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_FLOAT);
	egg_buffer_write_float(sample->buf, f);
	EXIT;
}

/**
 * pka_sample_append_int:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @i: A #gint to append to the buffer.
 *
 * Appends the integer value @i to the buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_append_int (PkaSample *sample, /* IN */
                       gint       field,  /* IN */
                       gint       i)      /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_INT);
	egg_buffer_write_int(sample->buf, i);
	EXIT;
}

/**
 * pka_sample_append_int64:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @i: A #gint64 to append to the buffer.
 *
 * Appends the 64-bit integer value @i to the buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_append_int64 (PkaSample *sample, /* IN */
                         gint       field,  /* IN */
                         gint64     i)      /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_INT64);
	egg_buffer_write_int64(sample->buf, i);
	EXIT;
}

/**
 * pka_sample_append_string:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @s: A string to append to the buffer.
 *
 * Appends the string @s to the buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_append_string (PkaSample   *sample, /* IN */
                          gint         field,  /* IN */
                          const gchar *s)      /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_STRING);
	egg_buffer_write_string(sample->buf, s);
	EXIT;
}

/**
 * pka_sample_append_uint:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @u: A #guint to append to the buffer.
 *
 * Appends the unsigned integer @u to the buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_append_uint (PkaSample *sample, /* IN */
                        gint       field,  /* IN */
                        guint      u)      /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_UINT);
	egg_buffer_write_uint(sample->buf, u);
	EXIT;
}

/**
 * pka_sample_append_uint64:
 * @sample: A #PkaSample.
 * @field: The field within the manifest.
 * @u: A #guint64 to append to the buffer.
 *
 * Appends the 64-bit unsigned integer @u to the buffer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_append_uint64 (PkaSample *sample, /* IN */
                          gint       field,  /* IN */
                          guint64    u)      /* IN */
{
	g_return_if_fail(sample != NULL);

	ENTRY;
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_UINT64);
	egg_buffer_write_uint64(sample->buf, u);
	EXIT;
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
 * Returns: None.
 * Side effects: None.
 */
void
pka_sample_append_timeval (PkaSample *sample, /* IN */
                           gint       field,  /* IN */
                           GTimeVal  *tv)     /* IN */
{
	gint64 ticks;

	g_return_if_fail(sample != NULL);

	ENTRY;
	g_time_val_to_ticks(tv, &ticks);
	egg_buffer_write_tag(sample->buf, field, EGG_BUFFER_UINT64);
	egg_buffer_write_uint64(sample->buf, ticks);
	EXIT;
}

/**
 * pka_sample_get_type:
 *
 * Retrieves the #GType for #PkaSample.
 *
 * Returns: a #GType.
 * Side effects: Registers the type on first call.
 */
GType
pka_sample_get_type (void)
{
	static gsize initialized = FALSE;
	static GType type_id = G_TYPE_INVALID;

	if (g_once_init_enter(&initialized)) {
		type_id = g_boxed_type_register_static(
				"PkaSample",
				(GBoxedCopyFunc)pka_sample_ref,
				(GBoxedFreeFunc)pka_sample_unref);
		g_once_init_leave(&initialized, TRUE);
	}
	return type_id;
}
