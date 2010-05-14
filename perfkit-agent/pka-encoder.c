/* pka-encoder.c
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

#include "pka-encoder.h"
#include "pka-log.h"

/**
 * SECTION:pka-encoder
 * @title: PkaEncoder
 * @short_description: Sample and Manifest encoding
 *
 * The #PkaEncoder interface provides a way to encode both manifests and
 * samples into buffers which can be transported to clients.  The default
 * encoder uses a protocol buffers inspired format.  We try to maintain
 * compat with protocol buffers, but we wont guarantee that.
 *
 * It is possible to implement new encoders that compress or encrypt data
 * if desired.
 */

static inline guint64
pka_resolution_apply (PkaResolution res,
                      guint64       u)
{
	switch (res) {
	case PKA_RESOLUTION_PRECISE:
		return u;
	case PKA_RESOLUTION_USEC:
		return u / (G_GINT64_CONSTANT(100));
	case PKA_RESOLUTION_MSEC:
		return u / (G_GINT64_CONSTANT(10000));
	case PKA_RESOLUTION_SECOND:
		return u / (G_GINT64_CONSTANT(10000000));
	case PKA_RESOLUTION_MINUTE:
		return u / (G_GINT64_CONSTANT(600000000));
	case PKA_RESOLUTION_HOUR:
		return u / (G_GINT64_CONSTANT(864000000000));
	default:
		g_assert_not_reached();
	}
}

static gboolean
pka_encoder_real_encode_samples (PkaManifest *manifest,
                                 PkaSample  **samples,
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

	ENTRY;
	buf = egg_buffer_new();
	pka_manifest_get_timeval(manifest, &mtv);

	for (i = 0; i < n_samples; i++) {
		/*
		 * Add the relative time since the manifest loosing un-needed
		 * precision for the variable integer width compression.
		 */
		pka_sample_get_timeval(samples[i], &tv);
		g_time_val_diff(&tv, &mtv, &rel);
		rel = pka_resolution_apply(pka_manifest_get_resolution(manifest), rel);
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
		pka_sample_get_data(samples[i], &tbuf, &tlen);
		egg_buffer_write_tag(buf, 2, EGG_BUFFER_DATA);
		egg_buffer_write_data(buf, tbuf, tlen);
	}

	egg_buffer_get_buffer(buf, &tbuf, &tlen);
	*data = g_malloc(tlen);
	*data_len = tlen;
	memcpy(*data, tbuf, tlen);
	egg_buffer_unref(buf);

	RETURN(TRUE);
}

/**
 * pka_encoder_encode_samples:
 * @encoder: A #PkaEncoder.
 * @manifest: The current #PkaManifest.
 * @samples An array of #PkaSample.
 * @n_samples: The number of #PkaSample in @samples.
 * @data: A location for a data buffer.
 * @data_len: A location for the data buffer length.
 *
 * Encodes the samples into a buffer.  The resulting buffer is stored in
 * @data and the length of the buffer in @data_len.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 */
gboolean
pka_encoder_encode_samples  (PkaEncoder   *encoder,
                             PkaManifest  *manifest,
                             PkaSample   **samples,
                             gint          n_samples,
                             gchar       **data,
                             gsize        *data_len)
{
	ENTRY;
	if (encoder) {
		RETURN(PKA_ENCODER_GET_INTERFACE(encoder)->
				encode_samples(encoder, manifest, samples, n_samples,
				               data, data_len));
	}
	RETURN(pka_encoder_real_encode_samples(manifest, samples, n_samples,
	                                       data, data_len));
}

static gboolean
pka_encoder_real_encode_manifest (PkaManifest  *manifest,
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

	ENTRY;
	buf = egg_buffer_new();

	/*
	 * Field 1: Timestamp. (usec since Jan 1, 01)
	 */
	pka_manifest_get_timeval(manifest, &tv);
	g_time_val_to_ticks(&tv, &ticks);
	egg_buffer_write_tag(buf, 1, EGG_BUFFER_UINT64);
	egg_buffer_write_int64(buf, ticks);

	/*
	 * Desired sample resolution.  This allows us to save considerable
	 * width in the relative-timestamp per sample.
	 */
	egg_buffer_write_tag(buf, 2, EGG_BUFFER_ENUM);
	egg_buffer_write_uint(buf, pka_manifest_get_resolution(manifest));

	/*
	 * Source index offset within the channel.
	 */
	egg_buffer_write_tag(buf, 3, EGG_BUFFER_UINT);
	egg_buffer_write_uint(buf, pka_manifest_get_source_id(manifest));

	/*
	 * Create a new buffer for the repeated data series.
	 */
	ebuf = egg_buffer_new();

	/*
	 * Write the manifest data description.  This is a set of embedded
	 * messages within the message.
	 */
	rows = pka_manifest_get_n_rows(manifest);
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
		egg_buffer_write_uint(mbuf, pka_manifest_get_row_type(manifest, i));

		/*
		 * Write the row name.
		 */
		egg_buffer_write_tag(mbuf, 3, EGG_BUFFER_STRING);
		egg_buffer_write_string(mbuf, pka_manifest_get_row_name(manifest, i));

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
	RETURN(TRUE);
}

/**
 * pka_encoder_encode_manifest:
 * @encoder: A #PkaEncoder
 * @manifest: A #PkaManifest
 * @data: A location for a data buffer
 * @data_len: A location for the data buffer length
 *
 * Encodes the manifest into a buffer.  If the encoder is %NULL, the default
 * of copying the buffers will be performed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 */
gboolean
pka_encoder_encode_manifest (PkaEncoder   *encoder,
                             PkaManifest  *manifest,
                             gchar       **data,
                             gsize        *data_len)
{
	ENTRY;
	if (encoder) {
		RETURN(PKA_ENCODER_GET_INTERFACE(encoder)->
				encode_manifest(encoder, manifest, data, data_len));
	}
	RETURN(pka_encoder_real_encode_manifest(manifest, data, data_len));
}

/**
 * pka_encoder_get_id:
 * @encoder: A #PkaEncoder.
 *
 * Retrieves the unique identifier of the encoder.
 *
 * Returns: The identifier.
 * Side effects: None.
 */
gint
pka_encoder_get_id (PkaEncoder *encoder) /* IN */
{
	G_LOCK_DEFINE(encoder_seq);
	static gint encoder_seq = 0;
	gpointer id;

	ENTRY;
	G_LOCK(encoder_seq);
	if (!(id = g_object_get_data(G_OBJECT(encoder), "pka-encoder-id"))) {
		id = GINT_TO_POINTER(encoder_seq++);
		g_object_set_data(G_OBJECT(encoder), "pka-encoder-id", id);
	}
	G_UNLOCK(encoder_seq);
	RETURN(GPOINTER_TO_INT(id));
}

/**
 * pka_encoder_get_type:
 *
 * Returns: the #PkaEncoder #GType.
 */
GType
pka_encoder_get_type (void)
{
	static GType type_id = 0;

	if (g_once_init_enter((gsize *)&type_id)) {
		const GTypeInfo g_type_info = { 
			sizeof (PkaEncoderIface),
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

		GType _type_id = g_type_register_static (G_TYPE_INTERFACE, "PkaEncoder",
		                                         &g_type_info, 0); 
		g_type_interface_add_prerequisite (_type_id, G_TYPE_OBJECT);
		g_once_init_leave((gsize *)&type_id, _type_id);
	}   
	return type_id;
}
