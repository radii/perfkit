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

/**
 * SECTION:pkd-sample
 * @title: PkdSample
 * @short_description: Samples within a data stream
 *
 * #PkdSample represents a set of data from a particular sample within a
 * data stream.  It contains as little information as possible within
 * its buffer so that it requires little size overhead.  Data values contain
 * an integer (usually a single byte) representing the data type within
 * the active #PkdManifest followed by there data.
 *
 * More information on the serialization format can be found in the
 * OVERVIEW file in the source tree.
 *
 * Various #PkdEncoder<!-- -->'s can perform extra compression based on the
 * known information in the #PkdManifest.
 *
 * Only a few data types are supported but more will be added as needed.
 */

#define PKD_SAMPLE_WRITER_INDEX(w,i) G_STMT_START {        \
    if ((w)->row_count < 0xFF) {                           \
        (w)->data[(w)->pos++] = (gchar)((i) & 0xFF);       \
    } else {                                               \
        memcpy(&(w)->data[(w)->pos], &(i), sizeof(gint));  \
        (w)->pos += sizeof(gint);                          \
    }                                                      \
} G_STMT_END

struct _PkdSample
{
	volatile gint  ref_count;

	gint   source_id;       /* Source identifier within the channel. */
	gint   len;             /* Length of data within the sample. */
	gchar *data;            /* Data buffer. Default is inline_buffer. */
	gchar  inline_data[64]; /* Default inline buffer for @data. */
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
	sample->len = 0;
	sample->data = &sample->inline_buffer[0];

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
 * @data_len: A location for the buffer length
 *
 * Retrieves the internal buffer for the sample.  The buffer should not be
 * modified or freed.
 *
 * Side effects: None.
 */
void
pkd_sample_get_data (PkdSample  *sample,
                    gchar    **data,
                    gsize     *data_len)
{
	g_return_if_fail(sample != NULL);
	g_return_if_fail(data != NULL);
	g_return_if_fail(data_len != NULL);

	*data = sample->data;
	*data_len = sample->len;
}

/**
 * pkd_sample_get_source_id:
 * @sample: A #PkdSample
 *
 * Retrieves the source index within channel.
 *
 * Returns: an integer containing the source id
 *
 * Side effects: None.
 */
gint
pkd_sample_get_source_id (PkdSample *sample)
{
	return sample->source_id;
}

/**
 * pkd_sample_set_source_id:
 * @sample: A #PkdSample
 * @source_id: The source id in the channel
 *
 * Internal method used to set the source id.
 */
void
pkd_sample_set_source_id (PkdSample *sample,
                          gint       source_id)
{
	sample->source_id = source_id;
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
	 * We use one-byte for row index if the row count is <= 0xFF.
	 */
	size += (writer->row_count < 0xFF) ?
	        (writer->row_count)        :
	        (writer->row_count * sizeof(gint));

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
	 * gchararray..: sizeof gchar* (Inline Pointer)
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
		case G_TYPE_STRING:
			/*
			 * For strings we store the pointer directly in the creation buffer
			 * and expand it when pkd_sample_writer_finish() is called.
			 */
			size += sizeof(gchar*);
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
	 * Either assign a new buffer that is big enough or clear the inline
	 * buffer.
	 */
	if (size > sizeof(writer->inline_data)) {
		writer->data = g_malloc0(size);
	} else {
		memset(writer->inline_data, 0, sizeof(writer->inline_data));
	}

	/*
	 * Mark the first byte as Manifest Row Compression.
	 */
	writer->data[writer->pos++] = (writer->row_count < 0xFF);
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
	/*
	 * Add the manifest row index.
	 */
	PKD_SAMPLE_WRITER_INDEX(writer, idx);

	/*
	 * Copy the integer value.
	 */
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
	gboolean comp;

	/*
	 * Calculate how much extra space is needed based on complex type
	 * expansion such as strings.
	 */
	s->len = writer->pos + writer->extra;
	c = writer->row_count;

	/*
	 * Allocate a buffer if needed for the samples data.
	 */
	if ((s->len + 1) <= sizeof(s->inline_data)) {
		s->data = &s->inline_data[0];
		memset(&s->inline_data[0], 0, sizeof(s->inline_data));
	} else {
		s->data = g_malloc0(s->len + 1);
	}

	/*
	 * First byte denotes id-compression.  Id compression allows us to use a
	 * single byte for the sample rather than a full integer of 4 bytes.
	 */
	s->data[so++] = comp = writer->data[wo++];

	while (wo < s->len) {
		/*
		 * Get the manifest index.  Only use one byte if we are using
		 * compressed manifest ids.
		 */
		if (comp) {
			s->data[so++] = idx = writer->data[wo++];
		} else {
			memcpy(&idx, &writer->data[wo], sizeof(gint));
			memcpy(&s->data[so], &writer->data[wo], sizeof(gint));
			wo += sizeof(gint);
			so += sizeof(gint);
		}

		/*
		 * Get the row type so we can determine if we need to expand
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
			 *
			 *   Possibly expand complex types through custom serializers in
			 *   the future.
			 */
			g_assert_not_reached();
		}
	}
}
