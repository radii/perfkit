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
#include <egg-time.h>
#include <time.h>

#include "pk-log.h"
#include "pk-manifest.h"
#include "pk-sample.h"
#include "pk-util.h"

/**
 * SECTION:pk-sample
 * @title: PkSample
 * @short_description: 
 *
 * 
 */

typedef struct _PkSampleField PkSampleField;

struct _PkSample
{
	volatile gint    ref_count;
	gint             source_id;
	struct timespec  ts;
	GArray          *ar;
};

struct _PkSampleField
{
	guint  field;
	GValue value;
};

static void
pk_sample_destroy (PkSample *sample)
{
	gint i;

	g_return_if_fail(sample != NULL);

	if (G_LIKELY(sample->ar)) {
		for (i = 0; i < sample->ar->len; i++) {
			g_value_unset(&(g_array_index(sample->ar, PkSampleField, i).value));
		}
	}
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
	sample->ar = g_array_new(FALSE, FALSE, sizeof(PkSampleField));

	return sample;
}

static gboolean
pk_sample_decode_timespec (PkSample   *sample,   /* IN */
                           EggBuffer  *buffer,   /* IN */
                           PkManifest *manifest) /* IN */
{
	guint field, tag;
	struct timespec sts;
	struct timespec mts;
	PkResolution res;
	guint64 u64;

	ENTRY;
	if (!egg_buffer_read_tag(buffer, &field, &tag)) {
		RETURN(FALSE);
	}
	if (field != 2 || tag != EGG_BUFFER_UINT64) {
		RETURN(FALSE);
	}
	if (!egg_buffer_read_uint64(buffer, &u64)) {
		RETURN(FALSE);
	}
	res = pk_manifest_get_resolution(manifest);
	switch (res) {
	CASE(PK_RESOLUTION_USEC);
		BREAK;
	CASE(PK_RESOLUTION_MSEC);
		u64 *= 1000;
		BREAK;
	CASE(PK_RESOLUTION_SECOND);
		u64 *= G_USEC_PER_SEC;
		BREAK;
	CASE(PK_RESOLUTION_MINUTE);
		u64 *= G_USEC_PER_SEC * 60;
		BREAK;
	CASE(PK_RESOLUTION_HOUR);
		u64 *= G_USEC_PER_SEC * (guint64)3600;
		BREAK;
	default:
		g_assert_not_reached();
	}
	timespec_from_usec(&sts, u64);
	pk_manifest_get_timespec(manifest, &mts);
	timespec_add(&sts, &mts, &sample->ts);
	RETURN(TRUE);
}

static inline gboolean
pk_sample_init_value (PkSample   *sample,
                      PkManifest *manifest,
                      guint       field,
                      GValue     *value)
{
	GType type_id;

	type_id = pk_manifest_get_row_type(manifest, field);
	if (type_id == G_TYPE_INVALID) {
		return FALSE;
	}

	g_value_init(value, type_id);
	return TRUE;
}

static gboolean
pk_sample_decode_data (PkSample   *sample,
                       EggBuffer  *buffer,
                       PkManifest *manifest)
{
	guint field, tag;
	guint data_len;
	gsize total_len,
	      offset;

	/* Make sure the buffer is at field 2, data. */
	if (!egg_buffer_read_tag(buffer, &field, &tag)) {
		return FALSE;
	}

	if (field != 3 || tag != EGG_BUFFER_DATA) {
		return FALSE;
	}

	/* Get the length of the data section. */
	if (!egg_buffer_read_uint(buffer, &data_len)) {
		return FALSE;
	}

	/* Make sure there is enough data in the buffer. */
	total_len = egg_buffer_get_length(buffer);
	offset = egg_buffer_get_pos(buffer);
	if (total_len < (offset + data_len)) {
		return FALSE;
	}

	while (offset < total_len) {
		PkSampleField item;
		GValue value = {0};
		GType type;
		gchar *str;

		/* Get the field position and type. */
		if (!egg_buffer_read_tag(buffer, &field, &tag)) {
			return FALSE;
		}

		/* Initialize the gvalue type from the manifest */
		if (!pk_sample_init_value(sample, manifest, field, &value)) {
			return FALSE;
		}

		/* Get the type for verification */
		type = G_VALUE_TYPE(&value);

		/* Extract field data */
		switch (tag) {
		case 0: /* Varint type */
			switch (type) {
			case G_TYPE_INT:
				if (!egg_buffer_read_int(buffer, &value.data[0].v_int)) {
					return FALSE;
				}
				break;
			case G_TYPE_UINT:
				if (!egg_buffer_read_uint(buffer, &value.data[0].v_uint)) {
					return FALSE;
				}
				break;
			case G_TYPE_INT64:
				if (!egg_buffer_read_int64(buffer, &value.data[0].v_int64)) {
					return FALSE;
				}
				break;
			case G_TYPE_UINT64:
				if (!egg_buffer_read_uint64(buffer, &value.data[0].v_uint64)) {
					return FALSE;
				}
				break;
			default:
				return FALSE;
			}
			break;

		case 1: /* Double type */
			switch (type) {
			case G_TYPE_DOUBLE:
				if (!egg_buffer_read_double(buffer, &value.data[0].v_double)) {
					return FALSE;
				}
				break;
			default:
				return FALSE;
			}
			break;

		case 2: /* String/data type */
			switch (type) {
			case G_TYPE_STRING:
				if (!egg_buffer_read_string(buffer, &str)) {
					return FALSE;
				}
				g_value_take_string(&value, str);
				break;
			default:
				return FALSE;
			}
			break;

		case 5: /* Float type */
			switch (type) {
			case G_TYPE_FLOAT:
				if (!egg_buffer_read_float(buffer, &value.data[0].v_float)) {
					return FALSE;
				}
				break;
			default:
				return FALSE;
			}
			break;

		default: /* Invalid type */
			return FALSE;
		}

		/* Add sample data point */
		item.field = field;
		item.value = value;
		g_array_append_val(sample->ar, item);

		/* Increment buffer position */
		offset = egg_buffer_get_pos(buffer);
	}

	return TRUE;
}

/**
 * pk_sample_decode:
 * @sample: A #PkSample.
 * @buffer: An #EggBuffer.
 * @manifest: A #PkManifest.
 *
 * Decodes the sample data within the buffer.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
pk_sample_decode (PkSample   *sample,   /* IN */
                  EggBuffer  *buffer,   /* IN */
                  PkManifest *manifest) /* IN */
{
	g_return_val_if_fail(sample != NULL, FALSE);
	g_return_val_if_fail(buffer != NULL, FALSE);

	ENTRY;
	if (!pk_sample_decode_timespec(sample, buffer, manifest)) {
		RETURN(FALSE);
	}
	if (!pk_sample_decode_data(sample, buffer, manifest)) {
		RETURN(FALSE);
	}
	RETURN(TRUE);
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
pk_sample_new_from_data (PkManifestResolver  resolver,  /* IN */
                         gpointer            user_data, /* IN */
                         const guint8       *data,      /* IN */
                         gsize               length,    /* IN */
                         gsize              *n_read)    /* IN */
{
	PkManifest *manifest = NULL;
	PkSample *sample;
	EggBuffer *buffer;
	guint field = 0;
	guint tag = 0;
	gint source_id = 0;

	g_return_val_if_fail(resolver != NULL, NULL);
	g_return_val_if_fail(data != NULL, NULL);
	g_return_val_if_fail(n_read != NULL, NULL);

	ENTRY;
	sample = pk_sample_new();
	buffer = egg_buffer_new_from_data(data, length);

	/*
	 * Resolve the manifest by the source id.
	 */
	if (!egg_buffer_read_tag(buffer, &field, &tag)) {
		GOTO(failed);
	}
	if (field != 1 || tag != EGG_BUFFER_UINT) {
		GOTO(failed);
	}
	if (!egg_buffer_read_uint(buffer, (guint *)&source_id)) {
		GOTO(failed);
	}
	if (!resolver(source_id, &manifest, user_data)) {
		GOTO(failed);
	}
	sample->source_id = source_id;

	/*
	 * Decode the rest of the sample.
	 */
	if (!pk_sample_decode(sample, buffer, manifest)) {
		pk_sample_unref(sample);
		sample = NULL;
	}

	*n_read = egg_buffer_get_pos(buffer);
	egg_buffer_unref(buffer);
	RETURN(sample);

  failed:
  	egg_buffer_unref(buffer);
  	pk_sample_unref(sample);
  	*n_read = 0;
  	RETURN(NULL);
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

/**
 * pk_sample_get_value:
 * @sample: A #PkSample
 * @row_id: The row within the manifest.
 * @value: A #GValue to initialize and set.
 *
 * Retrieves the value for a given row in the sample.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 */
gboolean
pk_sample_get_value (PkSample *sample,  // IN
                     guint     row_id,  // IN
                     GValue   *value)   // OUT
{
	PkSampleField *f;
	gint i;

	g_return_val_if_fail(sample != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);
	g_return_val_if_fail(sample->ar != NULL, FALSE);

	for (i = 0; i < sample->ar->len; i++) {
		f = &g_array_index(sample->ar, PkSampleField, i);
		if (f->field == row_id) {
			g_value_init(value, G_VALUE_TYPE(&f->value));
			g_value_copy(&f->value, value);
			return TRUE;
		}
	}

	return FALSE;
}

gint
pk_sample_get_source_id (PkSample *sample) /* IN */
{
	g_return_val_if_fail(sample != NULL, -1);
	return sample->source_id;
}

void
pk_sample_get_timespec (PkSample        *sample, /* IN */
                        struct timespec *ts)     /* OUT */
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(ts != NULL);

	ENTRY;
	*ts = sample->ts;
	EXIT;
}

void
pk_sample_get_timeval (PkSample *sample, /* IN */
                       GTimeVal *tv)     /* OUT */
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(tv != NULL);

	ENTRY;
	tv->tv_sec = sample->ts.tv_sec;
	tv->tv_usec = (sample->ts.tv_nsec / 1000);
	EXIT;
}

/**
 * pk_sample_get_type:
 *
 * Returns: The #PkSample #GType.
 */
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
