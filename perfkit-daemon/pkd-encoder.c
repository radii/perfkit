/* pkd-encoder.c
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#include "pkd-encoder.h"

/**
 * SECTION:pkd-encoder
 * @title: PkdEncoder
 * @short_description: Sample and Manifest encoding
 *
 * The #PkdEncoder interface provides a way to encode both manifests and
 * samples into buffers which can be transported to clients.  The default
 * encoder uses a protocol buffers inspired format.  We try to maintain
 * compat with protocol buffers, but we wont guarantee that.
 *
 * It is possible to implement new encoders that compress or encrypt data
 * if desired.
 */

static inline guint64
pkd_resolution_apply (PkdResolution res,
                      guint64       u)
{
	switch (res) {
	case PKD_RESOLUTION_PRECISE:
		return u;
	case PKD_RESOLUTION_USEC:
		return u / (G_GINT64_CONSTANT(100));
	case PKD_RESOLUTION_MSEC:
		return u / (G_GINT64_CONSTANT(10000));
	case PKD_RESOLUTION_SECOND:
		return u / (G_GINT64_CONSTANT(10000000));
	case PKD_RESOLUTION_MINUTE:
		return u / (G_GINT64_CONSTANT(600000000));
	case PKD_RESOLUTION_HOUR:
		return u / (G_GINT64_CONSTANT(864000000000));
	default:
		g_assert_not_reached();
	}
}

static gboolean
pkd_encoder_real_encode_samples (PkdManifest *manifest,
                                 PkdSample  **samples,
                                 gint         n_samples,
                                 gchar      **data,
                                 gsize       *data_len)
{
	EggBuffer *buf;
	GTimeVal mtv, tv;
	gint64 rel;
	const guint8 *tbuf;
	gsize tlen;
	gint i;

	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(data_len != NULL, FALSE);

	buf = egg_buffer_new();
	pkd_manifest_get_timeval(manifest, &mtv);

	for (i = 0; i < n_samples; i++) {
		/*
		 * Add the relative time since the manifest loosing un-needed
		 * precision for the variable integer width compression.
		 */
		pkd_sample_get_timeval(samples[i], &tv);
		g_time_val_diff(&tv, &mtv, &rel);
		rel = pkd_resolution_apply(pkd_manifest_get_resolution(manifest), rel);
		egg_buffer_write_tag(buf, 1, EGG_BUFFER_UINT64);
		egg_buffer_write_uint64(buf, rel);

		/*
		 * The sample is a protobuf inspired blob but is not protobuf compat.
		 *
		 * This was so that we could save significant message and repeated type
		 * overhead that is not needed for introspection since we have the
		 * manifest to provide us that data.
		 *
		 * Therefore, we simply treat the sample as an opaque buffer for the
		 * other side to unwrap.
		 */
		pkd_sample_get_data(samples[i], &tbuf, &tlen);
		egg_buffer_write_tag(buf, 2, EGG_BUFFER_DATA);
		egg_buffer_write_data(buf, tbuf, tlen);
	}

	egg_buffer_get_buffer(buf, &tbuf, &tlen);
	*data = g_malloc(tlen);
	*data_len = tlen;
	memcpy(*data, tbuf, tlen);

	egg_buffer_unref(buf);

	return TRUE;
}

/**
 * pkd_encoder_encode_samples:
 * @encoder: A #PkdEncoder.
 * @manifest: The current #PkdManifest.
 * @samples An array of #PkdSample.
 * @n_samples: The number of #PkdSample in @samples.
 * @data: A location for a data buffer.
 * @data_len: A location for the data buffer length.
 *
 * Encodes the samples into a buffer.  The resulting buffer is stored in
 * @data and the length of the buffer in @data_len.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 */
gboolean
pkd_encoder_encode_samples  (PkdEncoder   *encoder,
                             PkdManifest  *manifest,
                             PkdSample   **samples,
                             gint          n_samples,
                             gchar       **data,
                             gsize        *data_len)
{
	if (encoder)
		return PKD_ENCODER_GET_INTERFACE(encoder)->encode_samples(encoder,
		                                                          manifest,
		                                                          samples,
		                                                          n_samples,
		                                                          data,
		                                                          data_len);
	return pkd_encoder_real_encode_samples(manifest,
	                                       samples,
	                                       n_samples,
	                                       data,
	                                       data_len);
}

static gboolean
pkd_encoder_real_encode_manifest (PkdManifest  *manifest,
                                  gchar       **data,
                                  gsize        *data_len)
{
	EggBuffer *buf, *mbuf, *ebuf;
	GTimeVal tv;
	gint64 ticks;
	const guint8 *tbuf;
	gsize tlen;
	gint i, rows;

	g_return_val_if_fail(manifest != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(data_len != NULL, FALSE);

	buf = egg_buffer_new();

	/*
	 * Field 1: Timestamp. (usec since Jan 1, 01)
	 */
	pkd_manifest_get_timeval(manifest, &tv);
	g_time_val_to_ticks(&tv, &ticks);
	egg_buffer_write_tag(buf, 1, EGG_BUFFER_UINT64);
	egg_buffer_write_int64(buf, ticks);

	/*
	 * Desired sample resolution.  This allows us to save considerable
	 * width in the relative-timestamp per sample.
	 */
	egg_buffer_write_tag(buf, 2, EGG_BUFFER_ENUM);
	egg_buffer_write_uint(buf, pkd_manifest_get_resolution(manifest));

	/*
	 * Source index offset within the channel.
	 */
	egg_buffer_write_tag(buf, 3, EGG_BUFFER_UINT);
	egg_buffer_write_uint(buf, pkd_manifest_get_source_id(manifest));

	/*
	 * Create a new buffer for the repeated data series.
	 */
	ebuf = egg_buffer_new();

	/*
	 * Write the manifest data description.  This is a set of embedded
	 * messages within the message.
	 */
	rows = pkd_manifest_get_n_rows(manifest);
	for (i = 1; i <= rows; i++) {
		mbuf = egg_buffer_new();

		/*
		 * Write the row identifier.
		 */
		egg_buffer_write_tag(mbuf, 1, EGG_BUFFER_UINT);
		egg_buffer_write_uint(mbuf, i);

		/*
		 * Write the row type.
		 */
		egg_buffer_write_tag(mbuf, 2, EGG_BUFFER_ENUM);
		egg_buffer_write_uint(mbuf, pkd_manifest_get_row_type(manifest, i));

		/*
		 * Write the row name.
		 */
		egg_buffer_write_tag(mbuf, 3, EGG_BUFFER_STRING);
		egg_buffer_write_string(mbuf, pkd_manifest_get_row_name(manifest, i));

		/*
		 * Embed the message as a data blob.
		 */
		egg_buffer_get_buffer(mbuf, &tbuf, &tlen);
		egg_buffer_write_data(ebuf, tbuf, tlen);

		egg_buffer_unref(mbuf);
	}

	/*
	 * Add the repeated message length and data.
	 */
	egg_buffer_get_buffer(ebuf, &tbuf, &tlen);
	egg_buffer_write_tag(buf, 4, EGG_BUFFER_REPEATED);
	egg_buffer_write_data(buf, tbuf, tlen);
	egg_buffer_unref(ebuf);

	/*
	 * Copy the buffer to the destination.
	 */
	egg_buffer_get_buffer(buf, &tbuf, &tlen);
	*data = g_malloc(tlen);
	*data_len = tlen;
	memcpy(*data, tbuf, tlen);

	egg_buffer_unref(buf);

	return TRUE;
}

/**
 * pkd_encoder_encode_manifest:
 * @encoder: A #PkdEncoder
 * @manifest: A #PkdManifest
 * @data: A location for a data buffer
 * @data_len: A location for the data buffer length
 *
 * Encodes the manifest into a buffer.  If the encoder is %NULL, the default
 * of copying the buffers will be performed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 */
gboolean
pkd_encoder_encode_manifest (PkdEncoder   *encoder,
                             PkdManifest  *manifest,
                             gchar       **data,
                             gsize        *data_len)
{
	if (encoder) {
		return PKD_ENCODER_GET_INTERFACE(encoder)->
			encode_manifest(encoder, manifest, data, data_len);
	}

	return pkd_encoder_real_encode_manifest(manifest, data, data_len);
}

/**
 * pkd_encoder_get_type:
 *
 * Returns: the #PkdEncoder #GType.
 */
GType
pkd_encoder_get_type (void)
{
	static GType type_id = 0;

	if (g_once_init_enter((gsize *)&type_id)) {
		const GTypeInfo g_type_info = { 
			sizeof (PkdEncoderIface),
			NULL, /* base_init      */
			NULL, /* base_finalize  */
			NULL, /* class_init     */
			NULL, /* class_finalize */
			NULL, /* class_data     */
			0,    /* instance_size  */
			0,    /* n_preallocs    */
			NULL, /* instance_init  */
			NULL  /* value_table    */
		};  

		GType _type_id = g_type_register_static (G_TYPE_INTERFACE, "PkdEncoder",
		                                         &g_type_info, 0); 
		g_type_interface_add_prerequisite (_type_id, G_TYPE_OBJECT);
		g_once_init_leave((gsize *)&type_id, _type_id);
	}   

	return type_id;
}
