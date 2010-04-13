/* pka-zlib.c
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

#include "pka-zlib.h"

static void pka_encoder_init (PkaEncoderIface *iface);

G_DEFINE_TYPE_EXTENDED (PkaZlib,
                        pka_zlib,
                        G_TYPE_OBJECT,
                        0,
                        G_IMPLEMENT_INTERFACE (PKA_TYPE_ENCODER,
                                               pka_encoder_init))

/**
 * SECTION:pka-zlib
 * @title: PkaZlib
 * @short_description: 
 *
 * 
 */

const PkaStaticEncoderInfo pka_encoder_plugin = {
	.uid         = "ZLib",
	.name        = "ZLib Compression",
	.description = "This encoder provides zlib based compression "
	               "for data streams.",
	.version     = "0.1.0",
	.factory     = pka_zlib_new,
};

struct _PkaZlibPrivate
{
	gpointer dummy;
};

/**
 * pka_zlib_new:
 *
 * Creates a new instance of #PkaZlib.
 *
 * Return value: the newly created #PkaZlib instance.
 */
PkaEncoder*
pka_zlib_new (void)
{
	return g_object_new (PKA_TYPE_ZLIB, NULL);
}

static gboolean
pka_zlib_encode_samples (PkaEncoder    *encoder,
                         PkaManifest   *manifest,
                         PkaSample    **samples,
                         gint          sample_len,
                         gchar       **data,
                         gsize        *dapka_len)
{
	g_return_val_if_fail(encoder != NULL, FALSE);
	g_return_val_if_fail(manifest != NULL, FALSE);
	g_return_val_if_fail(samples != NULL, FALSE);
	g_return_val_if_fail(sample_len > 0, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(dapka_len != NULL, FALSE);

	/*
	 * XXX: Stub for encoding samples.
	 */

	return FALSE;
}

static gboolean
pka_zlib_encode_manifest (PkaEncoder   *encoder,
                         PkaManifest  *manifest,
                         gchar      **data,
                         gsize       *dapka_len)
{
	g_return_val_if_fail(encoder != NULL, FALSE);
	g_return_val_if_fail(manifest != NULL, FALSE);
	g_return_val_if_fail(data != NULL, FALSE);
	g_return_val_if_fail(dapka_len != NULL, FALSE);

	/*
	 * XXX: Stub for encoding manifests.
	 */

	return FALSE;
}

static void
pka_zlib_finalize (GObject *object)
{
	PkaZlibPrivate *priv;

	g_return_if_fail (PKA_IS_ZLIB (object));

	priv = PKA_ZLIB (object)->priv;

	G_OBJECT_CLASS(pka_zlib_parent_class)->finalize(object);
}

static void
pka_zlib_class_init (PkaZlibClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pka_zlib_finalize;
	g_type_class_add_private(object_class, sizeof(PkaZlibPrivate));
}

static void
pka_zlib_init (PkaZlib *zlib)
{
	zlib->priv = G_TYPE_INSTANCE_GET_PRIVATE(zlib,
	                                         PKA_TYPE_ZLIB,
	                                         PkaZlibPrivate);
}

static void
pka_encoder_init (PkaEncoderIface *iface)
{
	g_assert(iface);

	iface->encode_samples = pka_zlib_encode_samples;
	iface->encode_manifest = pka_zlib_encode_manifest;
}
