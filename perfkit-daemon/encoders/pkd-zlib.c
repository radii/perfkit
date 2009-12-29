/* pkd-zlib.c
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

#include <zlib.h>

#include "pkd-zlib.h"

static void pkd_encoder_init (PkdEncoderIface *iface);

G_DEFINE_TYPE_EXTENDED (PkdZlib,
                        pkd_zlib,
                        G_TYPE_OBJECT,
                        0,
                        G_IMPLEMENT_INTERFACE (PKD_TYPE_ENCODER,
                                               pkd_encoder_init))

/**
 * SECTION:pkd-zlib
 * @title: PkdZlib
 * @short_description: 
 *
 * 
 */

const PkdStaticEncoderInfo pkd_encoder_plugin = {
	.uid         = "ZLib",
	.name        = "ZLib Compression",
	.description = "This encoder provides zlib based compression "
	               "for data streams.",
	.version     = "0.1.0",
	.factory     = pkd_zlib_new,
};

struct _PkdZlibPrivate
{
	gpointer dummy;
};

/**
 * pkd_zlib_new:
 *
 * Creates a new instance of #PkdZlib.
 *
 * Return value: the newly created #PkdZlib instance.
 */
PkdEncoder*
pkd_zlib_new (void)
{
	return g_object_new (PKD_TYPE_ZLIB, NULL);
}

static gboolean
pkd_zlib_encode_samples (PkdEncoder  *encoder,
                        PkdSample  **samples,
                        gint        sample_len,
                        gchar     **data,
                        gsize      *dapkd_len)
{
	g_return_val_if_fail(encoder != NULL, FALSE);
	g_return_val_if_fail(samples != NULL, FALSE);
	g_return_val_if_fail(sample_len > 0, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(dapkd_len != NULL, FALSE);

	/*
	 * XXX: Stub for encoding samples.
	 */

	return FALSE;
}

static gboolean
pkd_zlib_encode_manifest (PkdEncoder   *encoder,
                         PkdManifest  *manifest,
                         gchar      **data,
                         gsize       *dapkd_len)
{
	g_return_val_if_fail(encoder != NULL, FALSE);
	g_return_val_if_fail(manifest != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(dapkd_len != NULL, FALSE);

	/*
	 * XXX: Stub for encoding manifests.
	 */

	return FALSE;
}

static void
pkd_zlib_finalize (GObject *object)
{
	PkdZlibPrivate *priv;

	g_return_if_fail (PKD_IS_ZLIB (object));

	priv = PKD_ZLIB (object)->priv;

	G_OBJECT_CLASS(pkd_zlib_parent_class)->finalize(object);
}

static void
pkd_zlib_class_init (PkdZlibClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkd_zlib_finalize;
	g_type_class_add_private(object_class, sizeof(PkdZlibPrivate));
}

static void
pkd_zlib_init (PkdZlib *zlib)
{
	zlib->priv = G_TYPE_INSTANCE_GET_PRIVATE(zlib,
	                                         PKD_TYPE_ZLIB,
	                                         PkdZlibPrivate);
}

static void
pkd_encoder_init (PkdEncoderIface *iface)
{
	g_assert(iface);

	iface->encode_samples = pkd_zlib_encode_samples;
	iface->encode_manifest = pkd_zlib_encode_manifest;
}
