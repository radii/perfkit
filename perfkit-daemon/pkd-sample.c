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

#include <string.h>

#include "pkd-sample.h"

#define PKD_SAMPLE_WRITER_INDEX(w,i) G_STMT_START {         \
    if ((w)->row_count < 0xFF) {                           \
        (w)->data[(w)->pos++] = (gchar)((i) & 0xFF);       \
    } else {                                               \
        memcpy(&(w)->data[(w)->pos], &(i), sizeof(gint));  \
        (w)->pos += sizeof(gint);                          \
    }                                                      \
} G_STMT_END

struct _PkdSample
{
	volatile gint ref_count;

	gint   len;
	gchar *data;
	gchar  inline_data[64];
};

static void
pkd_sample_destroy(PkdSample *sample)
{
}

/**
 * pkd_sample_new:
 *
 * Creates a new instance of #PkdSample.
 *
 * Returns: the newly created #PkdSample.
 *
 * Side effects: None.
 */
PkdSample*
pkd_sample_new(void)
{
	PkdSample *sample;

	sample = g_slice_new0(PkdSample);
	sample->ref_count = 1;

	return sample;
}

/**
 * pkd_sample_ref:
 * @sample: A #PkdSample
 *
 * Atomically increases the reference count of @sample.
 *
 * Returns: The @sample pointer.
 *
 * Side effects: None.
 */
PkdSample*
pkd_sample_ref(PkdSample *sample)
{
	g_return_val_if_fail(sample != NULL, NULL);
	g_return_val_if_fail(sample->ref_count > 0, NULL);

	g_atomic_int_inc(&sample->ref_count);

	return sample;
}

/**
 * pkd_sample_unref:
 * @sample: A #PkdSample
 *
 * Atomically decrements the reference count of @sample.  When the reference
 * count reaches zero, the structure and its resources are freed.
 *
 * Side effects: The structure is freed if reference count reaches zero.
 */
void
pkd_sample_unref(PkdSample *sample)
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(sample->ref_count > 0);

	if (g_atomic_int_dec_and_test(&sample->ref_count)) {
		pkd_sample_destroy(sample);
		g_slice_free(PkdSample, sample);
	}
}

/**
 * pkd_sample_get_data:
 * @sample: A #PkdSample
 * @data: A location for a buffer
 * @dapkd_len: A location for the buffer length
 *
 * Retrieves the internal buffer for the sample.  The buffer should not be
 * modified or freed.
 *
 * Side effects: None.
 */
void
pkd_sample_get_data (PkdSample  *sample,
                    gchar    **data,
                    gsize     *dapkd_len)
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(data != NULL);
	g_return_if_fail(dapkd_len != NULL);

	*data = sample->data;
	*dapkd_len = sample->len;
}

GType
pkd_sample_get_type (void)
{
	static GType type_id = 0;
	GType _type_id;

	if (g_once_init_enter((gsize *)&type_id)) {
		_type_id = g_boxed_type_register_static("PkdSample",
				(GBoxedCopyFunc)pkd_sample_ref,
				(GBoxedFreeFunc)pkd_sample_unref);
		g_once_init_leave((gsize *)&type_id, (gsize)_type_id);
	}

	return type_id;
}

/**
 * pkd_sample_writer_init:
 * @writer: A #PkdSampleWriter
 * @manifest: A #PkdManifest
 * @sample: A #PkdSample
 *
 * Initializes a #PkdSampleWriter structure for writing a new sample.  After
 * calling this method you should set the various fields for the sample
 * using pkd_sample_writer_integer(), pkd_sample_writer_boolean(), or
 * similar.  When all the desired fields are set, use pkd_sample_writer_finish()
 * to move the buffer to the sample.
 *
 * Side effects: None.
 */
void
pkd_sample_writer_init (PkdSampleWriter *writer,
                       PkdManifest     *manifest,
                       PkdSample       *sample)
{
	GType type;
	gsize size = 0;
	gint i;

	writer->manifest = manifest;
	writer->sample = sample;
	writer->pos = 0;
	writer->extra = 0;
	writer->row_count = pkd_manifest_get_n_rows(manifest);
	writer->data = &writer->inline_data[0];

	/*
	 * Calculate the total size needed for our creation buffer.  The following
	 * table is used to calculate the amount of inline data.
	 *
	 * gint........: 4 bytes
	 * guint.......: 4 bytes
	 * glong.......: 4 bytes
	 * gulong......: 4 bytes
	 * gboolean....: 1 byte
	 * gchar.......: 1 byte
	 * gchararray..: sizeof gchar*
	 */
	for (i = 1; i <= writer->row_count; i++) {
		type = pkd_manifest_get_row_type(manifest, i);
		switch (type) {
		case G_TYPE_INT:
		case G_TYPE_UINT:
		case G_TYPE_LONG:
		case G_TYPE_ULONG:
			size += 4;
			break;
		case G_TYPE_CHAR:
		case G_TYPE_BOOLEAN:
			size++;
			break;
		default:
			/*
			 * NOTE:
			 *   Eventually we could support serializers for complex types
			 *   and look them up here.
			 */
			g_assert_not_reached();
		}
	}

	/*
	 * We use one-byte for row index if the row count is <= 0xFF.
	 */
	size += (writer->row_count <= 0xFF) ?
	        (writer->row_count)         :
	        (writer->row_count * sizeof(gint));

	/*
	 * Either assign a new buffer that is big enough or clear the inline
	 * buffer.
	 */
	if (size > sizeof(writer->inline_data)) {
		writer->data = g_malloc0(size);
	} else {
		memset(writer->inline_data, 0, sizeof(writer->inline_data));
	}
}

/**
 * pkd_sample_writer_boolean:
 * @writer: A #PkdSampleWriter.
 * @idx: The index within the manifest.
 * @b: The boolean value.
 *
 * Writes a boolean value for a field in the manifest.
 *
 * Side effects: None.
 */
void
pkd_sample_writer_boolean (PkdSampleWriter *writer,
                           gint             idx,
                           gboolean         b)
{
	PKD_SAMPLE_WRITER_INDEX(writer, idx);
	writer->data[writer->pos++] = b;
}

/**
 * pkd_sample_writer_string:
 * @writer: A #PkdSampleWriter
 * @idx: the manifest field index
 * @s: A string to append
 *
 * Adds the string to the sample.  @s must be a valid pointer until
 * pkd_sample_writer_finish() has been called.
 *
 * Side effects: None.
 */
void
pkd_sample_writer_string (PkdSampleWriter *writer,
                          gint             idx,
                          const gchar     *s)
{
	PKD_SAMPLE_WRITER_INDEX(writer, idx);

	/*
	 * Copy the pointer into the buffer for copying when
	 * pkd_sample_writer_finish() is called.
	 */
	memcpy(&writer->data[writer->pos], &s, sizeof(gchar*));
	writer->pos += sizeof(gchar*);

	/*
	 * Track how much extra space we need for the sample.  This
	 * can be negative.
	 */
	writer->extra += ((strlen(s) + 1) - sizeof(gchar*));
}

/**
 * pkd_sample_writer_integer:
 * @writer: A #PkdSampleWriter
 * @idx: The manifest index
 * @i: an integer to encode
 *
 * Appends @i to the buffer used for building the sample.
 */
void
pkd_sample_writer_integer (PkdSampleWriter *writer,
                           gint             idx,
                           gint             i)
{
	PKD_SAMPLE_WRITER_INDEX(writer, idx);
	memcpy(&writer->data[writer->pos], &i, sizeof(gint));
	writer->pos += sizeof(gint);
}

/**
 * pkd_sample_writer_finish:
 * @writer: A #PkdSampleWriter
 *
 * Completes the building of a #PkdSample and moves the buffer to the
 * #PkdSample instance.
 *
 * Side effects: None.
 */
void
pkd_sample_writer_finish (PkdSampleWriter *writer)
{
	PkdSample *s = writer->sample;
	gint wo = 0, so = 0, c, idx, l;
	gchar *str;
	GType type;

	s->len = writer->pos + writer->extra;
	c = writer->row_count;

	/*
	 * Allocate a buffer if needed for the samples data.
	 */
	if ((s->len + 1) <= sizeof(s->inline_data)) {
		s->data = &s->inline_data[0];
	} else {
		s->data = g_malloc0(s->len + 1);
	}

	/* First bit is id-compression */
	s->data[so++] = (writer->row_count < 0xFF);

	while (wo < s->len) {
		/*
		 * Get the manifest index.  The index is one byte if the total table
		 * count is < 0xFF.  We also copy the index to the sample.
		 */
		if (c < 0xFF) {
			idx = writer->data[wo++];
			s->data[so++] = idx;
		} else {
			memcpy(&idx, &writer->data[wo], sizeof(gint));
			memcpy(&s->data[so], &writer->data[wo], sizeof(gint));
			wo += sizeof(gint);
			so += sizeof(gint);
		}

		/*
		 * Get the row type so we can determien if we need to expand
		 * a pointer type (string) or if its a shortened (byte) type.
		 */
		type = pkd_manifest_get_row_type(writer->manifest, idx);
		switch (type) {
		case G_TYPE_INT:
		case G_TYPE_UINT:
		case G_TYPE_LONG:
		case G_TYPE_ULONG:
			memcpy(&s->data[so], &writer->data[wo], sizeof(gint));
			wo += sizeof(gint);
			so += sizeof(gint);
			break;
		case G_TYPE_STRING:
			memcpy(&str, &writer->data[wo], sizeof(gchar*));
			l = strlen(str) + 1;
			wo += sizeof(gchar*);
			memcpy(&s->data[so], str, l);
			so += l;
			break;
		case G_TYPE_BOOLEAN:
			s->data[so++] = writer->data[wo++];
			break;
		default:
			/*
			 * NOTE:
			 *   Possibly expand complex types in the future.
			 */
			g_assert_not_reached();
		}
	}
}
