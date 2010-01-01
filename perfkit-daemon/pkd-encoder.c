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

#include <string.h>

#include "pkd-encoder.h"

/**
 * SECTION:pkd-encoder
 * @title: PkdEncoder
 * @short_description: Sample and Manifest encoding
 *
 * The #PkdEncoder interface provides a way to encode both manifests and
 * samples into buffers which can be transported to clients.  The default
 * encoder implementation is fairly strait forward and passes native
 * samples to the client with little encapsulation.
 *
 * It is possible to implement new encoders that compress or encrypt data
 * if desired.
 *
 * The OVERVIEW file in the source tree provides more information on the
 * encoding format for the default encoder.
 */

static gboolean
pkd_encoder_real_encode_samples (PkdSample **samples,
                                 gint        n_samples,
                                 gchar     **data,
                                 gsize      *data_len)
{
	gchar *buf = NULL;
	gsize s = 0, buflen = 0;
	gint i;

	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(data_len != NULL, FALSE);

	for (i = 0; i < n_samples; i++) {
		/*
		 * Sample Length.
		 */
		s += sizeof(gint);

		/*
		 * Source Id.
		 */
		s++;

		/*
		 * Sample Data.
		 */
		pkd_sample_get_data(samples[i], &buf, &buflen);
		s += buflen;
	}

	*data_len = s;
	(*data) = g_malloc(s);
	s = 0;

	for (i = 0; i < n_samples; i++) {
		pkd_sample_get_data(samples[i], &buf, &buflen);

		/*
		 * Increment buflen by one to include our source id.
		 */
		buflen++;

		/*
		 * Sample Length.
		 */
		memcpy(&((*data)[s]), &buflen, sizeof(gint));
		s += sizeof(gint);

		/*
		 * Source Identifier.
		 */
		(*data)[s++] = (gchar)(pkd_sample_get_source_id(samples[i]) & 0xFF);

		/*
		 * Sample Data.
		 */
		memcpy(&((*data)[s]), buf, buflen);
		s += (buflen - 1); /* Don't include our previous increment */
	}

	return TRUE;
}

/**
 * pkd_encoder_encode_samples:
 * @encoder: A #PkdEncoder
 * @samples An array of #PkdSample
 * @n_samples: The number of #PkdSample in @samples.
 * @data: A location for a data buffer
 * @data_len: A location for the data buffer length
 *
 * 
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 */
gboolean
pkd_encoder_encode_samples  (PkdEncoder  *encoder,
                             PkdSample  **samples,
                             gint         n_samples,
                             gchar      **data,
                             gsize       *data_len)
{
	if (encoder && PKD_ENCODER_GET_INTERFACE(encoder)->encode_samples)
		return PKD_ENCODER_GET_INTERFACE(encoder)->encode_samples
			(encoder, samples, n_samples, data, data_len);
	return pkd_encoder_real_encode_samples(samples, n_samples, data, data_len);
}

static gboolean
pkd_encoder_real_encode_manifest (PkdManifest  *manifest,
                                  gchar       **data,
                                  gsize        *data_len)
{
	const gchar *name;
	gint rows, i, l;
	GType type;
	gsize s = 0, o = 0;
	gboolean p;

	g_return_val_if_fail(manifest != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(data_len != NULL, FALSE);

	/*
	 * Step 1:
	 *
	 *   Determine total buffer length.
	 *
	 */

	rows = pkd_manifest_get_n_rows(manifest);

	/*
	 * Include the Source ID.
	 */
	s++;

	/*
	 * Determine if we have compressed IDs.
	 */
	p = (rows <= 0xFF);
	s++;

	/*
	 * Iterate the types and names and determine our desired buffer size.
	 */
	for (i = 1; i <= rows; i++) {
		name = pkd_manifest_get_row_name(manifest, i);
		type = pkd_manifest_get_row_type(manifest, i);
		s += (rows <= 0xFF) ? 1 : sizeof(gint);  /* Row ID */
		s++;                                     /* Row Type */
		s += strlen(name) + 1;                   /* Row Name */
	}

	/*
	 * Step 2:
	 *
	 *   Move data into destination buffer.
	 *
	 */

	/*
	 * Allocate destination buffer.
	 */
	*data = g_malloc(s);
	*data_len = s;

	/*
	 * Add the Source ID.
	 */
	(*data)[o++] = (gchar)(pkd_manifest_get_source_id(manifest) & 0xFF);

	/*
	 * Mark if we have compressed Row IDs.
	 */
	(*data)[o++] = p;

	/*
	 * Iterate the types and names and drop into the buffer.
	 */
	for (i = 1; i <= rows; i++) {
		/*
		 * Drop the Row ID into the buffer.
		 */
		if (p) {
			(*data)[o++] = (gchar)(i & 0xFF);
		} else {
			memcpy(&((*data)[o]), &i, sizeof(gint));
			o += sizeof(gint);
		}

		/*
		 * Drop the Type ID into the buffer.
		 */
		(*data)[o++] = (gchar)(pkd_manifest_get_row_type(manifest, i) & 0xFF);

		/*
		 * Copy the string into the buffer.
		 */
		name = pkd_manifest_get_row_name(manifest, i);
		l = strlen(name) + 1;
		memcpy(&((*data)[o]), name, l);
		o += l;
	}

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
	if (encoder && PKD_ENCODER_GET_INTERFACE(encoder)->encode_manifest)
		return PKD_ENCODER_GET_INTERFACE(encoder)->encode_manifest
			(encoder, manifest, data, data_len);
	return pkd_encoder_real_encode_manifest(manifest, data, data_len);
}

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
