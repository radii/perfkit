/* pkd-manifest.c
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

#include "pkd-manifest.h"

/**
 * SECTION:pkd-manifest
 * @title: PkdManifest
 * @short_description: 
 *
 * 
 */

typedef struct {
	gint   id;
	GType  type;
	gchar *name;
} PkdManifestRow;

struct _PkdManifest
{
	GArray        *rows;
	gint           source_id;
	volatile gint  ref_count;
};

static void
pkd_manifest_destroy (PkdManifest *manifest)
{
	PkdManifestRow row;
	gint i;

	/*
	 * Free strings in rows.  We copy the entire structure, which is okay.
	 */
	for (i = 0; i < manifest->rows->len; i++) {
		row = g_array_index(manifest->rows, PkdManifestRow, i);
		g_free(row.name);
	}

	/*
	 * Free row array.
	 */
	g_array_unref(manifest->rows);
}

/**
 * pkd_manifest_new:
 *
 * Creates a new instance of #PkdManifest.
 *
 * Returns: The newly created instance of PkdManifest.
 *
 * Side effects: None.
 */
PkdManifest*
pkd_manifest_new (void)
{
	return pkd_manifest_sized_new(1);
}

/**
 * pkd_manifest_sized_new:
 * @size: The number of pre-allocated rows in the manifest.
 *
 * Creates a new #PkdManifest with space for @size rows pre-allocated.
 *
 * Returns: The newly created #PkdManifest with a reference count of 1.
 *
 * Side effects: None.
 */
PkdManifest*
pkd_manifest_sized_new (gint size)
{
	PkdManifest *manifest;

	manifest = g_slice_new0(PkdManifest);
	manifest->ref_count = 1;
	manifest->rows = g_array_sized_new(FALSE,
	                                   FALSE,
	                                   sizeof(PkdManifestRow),
	                                   size);

	return manifest;
}

/**
 * pkd_manifest_ref:
 * manifest: A #PkdManifest
 *
 * Atomically increments the reference count of @manifest by one.
 *
 * Returns: The @manifest pointer with its reference count incremented.
 *
 * Side effects: None.
 */
PkdManifest*
pkd_manifest_ref (PkdManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, NULL);
	g_return_val_if_fail(manifest->ref_count > 0, NULL);

	g_atomic_int_inc(&manifest->ref_count);

	return manifest;
}

/**
 * pkd_manifest_unref:
 * manifest: A #PkdManifest
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
pkd_manifest_unref (PkdManifest *manifest)
{
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(manifest->ref_count > 0);

	if (g_atomic_int_dec_and_test(&manifest->ref_count)) {
		pkd_manifest_destroy(manifest);
		g_slice_free(PkdManifest, manifest);
	}
}

/**
 * pkd_manifest_append:
 * @manifest: A #PkdManifest
 * @name: The name of the field
 * @type: The type of the field
 *
 * Appends a row to the table for the (@name, @type) tuple.  The resulting
 * row identifier that should be used for encoding samples is returned.
 *
 * Returns: The row identifier.
 *
 * Side effects: None.
 */
guint
pkd_manifest_append (PkdManifest  *manifest,
                    const gchar *name,
                    GType        type)
{
	PkdManifestRow row;

	g_return_val_if_fail(manifest != NULL, 0);
	g_return_val_if_fail(name != NULL, 0);
	g_return_val_if_fail(type != 0, 0);

	row.id = manifest->rows->len + 1;
	row.name = g_strdup(name);
	row.type = type;

	g_array_append_val(manifest->rows, row);

	return row.id;
}

/**
 * pkd_manifest_get_n_rows:
 * @manifest: A #PkdManifest
 *
 * Retrieves the number of rows in the manifest.
 *
 * Returns: the row count.
 *
 * Side effects: None.
 */
guint
pkd_manifest_get_n_rows (PkdManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, 0);
	g_return_val_if_fail(manifest->rows != NULL, 0);

	return manifest->rows->len;
}

/**
 * pkd_manifest_get_row_type:
 * @manifest: A #PkdManifest
 * @row: The row index (one-based)
 *
 * Retrieves the type for a given row.  Rows are 1-indexed.
 *
 * Returns: The GType of the row.
 *
 * Side effects: None.
 */
GType
pkd_manifest_get_row_type (PkdManifest *manifest,
                          gint        row)
{
	g_return_val_if_fail(manifest != NULL, G_TYPE_INVALID);
	g_return_val_if_fail(manifest->rows != NULL, G_TYPE_INVALID);
	g_return_val_if_fail(row > 0 && row <= manifest->rows->len, G_TYPE_INVALID);

	return g_array_index(manifest->rows, PkdManifestRow, row - 1).type;
}

/**
 * pkd_manifest_get_row_name:
 * @manifest: A #PkdManifest
 * @row: The row index (one-based)
 *
 * Retrieves the row name.  The row index starts at 1.
 *
 * Returns: A string containing the row name.
 *
 * Side effects: None.
 */
const gchar*
pkd_manifest_get_row_name (PkdManifest *manifest,
                          gint        row)
{
	g_return_val_if_fail(manifest != NULL, NULL);
	g_return_val_if_fail(manifest->rows != NULL, NULL);
	g_return_val_if_fail(row > 0 && row <= manifest->rows->len, NULL);

	return g_array_index(manifest->rows, PkdManifestRow, row - 1).name;
}

/**
 * pkd_manifest_get_source_id:
 * @manifest: A #PkdManifest
 *
 * Retrieves the source identifier for the #PkdManifest within its configured
 * #PkdChannel.  This id can be used by encoders to ensure the client knows
 * which source the manifest belongs to.
 *
 * Returns: An integer containing the source id.
 *
 * Side effects: None.
 */
gint
pkd_manifest_get_source_id (PkdManifest *manifest)
{
	return manifest->source_id;
}

/**
 * pkd_manifest_set_source_id:
 * @manifest: A #PkdManifest
 * @source_id: A source id
 *
 * Internal method used to set the source id by a channel receiving the
 * manifest.
 *
 * Side effects: None.
 */
void
pkd_manifest_set_source_id (PkdManifest *manifest,
                            gint         source_id)
{
	manifest->source_id = source_id;
}

GType
pkd_manifest_get_type (void)
{
	static GType type_id = 0;
	GType _type_id;

	if (g_once_init_enter((gsize *)&type_id)) {
		_type_id = g_boxed_type_register_static("PkdManifest",
		                                        (GBoxedCopyFunc)pkd_manifest_ref,
		                                        (GBoxedFreeFunc)pkd_manifest_unref);
		g_once_init_leave((gsize *)&type_id, _type_id);
	}

	return type_id;
}
