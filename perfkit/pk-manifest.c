/* pk-manifest.c
 *
 * Copyright (C) 2010 Christian Hergert
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pk-manifest.h"

/**
 * SECTION:pk-manifest
 * @title: PkManifest
 * @short_description: 
 *
 * 
 */

struct _PkManifest
{
	volatile gint ref_count;
};

static void
pk_manifest_destroy (PkManifest *manifest)
{
}

/**
 * pk_manifest_new:
 *
 * Creates a new instance of #PkManifest.
 *
 * Returns: The newly created instance of PkManifest.
 *
 * Side effects: None.
 */
PkManifest*
pk_manifest_new (void)
{
	PkManifest *manifest;

	manifest = g_slice_new0(PkManifest);
	manifest->ref_count = 1;

	return manifest;
}

/**
 * pk_manifest_ref:
 * manifest: A #PkManifest
 *
 * Atomically increments the reference count of @manifest by one.
 *
 * Returns: The @manifest pointer with its reference count incremented.
 *
 * Side effects: None.
 */
PkManifest*
pk_manifest_ref (PkManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, NULL);
	g_return_val_if_fail(manifest->ref_count > 0, NULL);

	g_atomic_int_inc(&manifest->ref_count);

	return manifest;
}

/**
 * pk_manifest_unref:
 * manifest: A #PkManifest
 *
 * Atomically decrements the reference count of @manifest by one.
 * When the reference count reaches zero, the structures resources as well as
 * the structure are freed.
 *
 * Returns: The @manifest pointer with its reference count incremented.
 *
 * Side effects: None.
 */
void
pk_manifest_unref (PkManifest *manifest)
{
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(manifest->ref_count > 0);

	if (g_atomic_int_dec_and_test(&manifest->ref_count)) {
		pk_manifest_destroy(manifest);
		g_slice_free(PkManifest, manifest);
	}
}

GType
pk_manifest_get_type (void)
{
	static GType type_id = 0;
	GType _type_id;

	if (g_once_init_enter((gsize *)&type_id)) {
		_type_id = g_boxed_type_register_static("PkManifest",
		                                        (GBoxedCopyFunc)pk_manifest_ref,
		                                        (GBoxedFreeFunc)pk_manifest_unref);
		g_once_init_leave((gsize *)&type_id, _type_id);
	}

	return type_id;
}
