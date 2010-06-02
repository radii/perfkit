/* pka-manifest.c
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

#include "pka-manifest.h"

/**
 * SECTION:pka-manifest
 * @title: PkaManifest
 * @short_description: Data stream information manifest
 *
 * #PkaManifest represents the potential types of information that may be
 * delivered over a data stream.  It maps index ids to a string name
 * and data type.  It should also contain the byte ordering of the host
 * system creating the data.  The system can work in various ways depending
 * on if you want extra overhead on the producer or consumer as to byte
 * reordering.
 *
 * If there are less than 255 data fields in a manifest, extra compression
 * can be taken in the data stream.  Instead of a full 4 bytes for the
 * manifest index, a single byte may be used.  This means that for a single
 * integer, only 5 bytes are required.
 *
 * However, this is mostly dominated by the choice of encoder to change
 * the stream as it is passed to the client.
 */

typedef struct
{
	gint   id;                  /* Row identifier */
	GType  type;                /* Row Type (String, Int, ..) */
	gchar *name;                /* Row Name ("Ingress", ..) */
} PkaManifestRow;

struct _PkaManifest
{
	volatile gint  ref_count;

	GArray        *rows;        /* Array of PkaManifestRow */
	gint           source_id;   /* Channel assigned Source Id */
	GTimeVal       tv;          /* Timing of manifest */
	PkaResolution  resolution;  /* Relative timestamp resolution */
};

static void
pka_manifest_destroy (PkaManifest *manifest)
{
	PkaManifestRow *row;
	gint i;

	/*
	 * Free strings in rows.  We copy the entire structure, which is okay.
	 */
	for (i = 0; i < manifest->rows->len; i++) {
		row = &g_array_index(manifest->rows, PkaManifestRow, i);
		g_free(row->name);
	}

	/*
	 * Free row array.
	 */
	g_array_unref(manifest->rows);
}

/**
 * pka_manifest_new:
 *
 * Creates a new instance of #PkaManifest.
 *
 * Returns: The newly created instance of PkaManifest.
 *
 * Side effects: None.
 */
PkaManifest*
pka_manifest_new (void)
{
	return pka_manifest_sized_new(4);
}

/**
 * pka_manifest_sized_new:
 * @size: The number of pre-allocated rows in the manifest.
 *
 * Creates a new #PkaManifest with space for @size rows pre-allocated.
 *
 * Returns: The newly created #PkaManifest with a reference count of 1.
 *
 * Side effects: None.
 */
PkaManifest*
pka_manifest_sized_new (gint size)
{
	PkaManifest *manifest;

	manifest = g_slice_new0(PkaManifest);
	manifest->ref_count = 1;
	manifest->rows = g_array_sized_new(FALSE,
	                                   FALSE,
	                                   sizeof(PkaManifestRow),
	                                   size);
	g_get_current_time(&manifest->tv);

	return manifest;
}

/**
 * pka_manifest_ref:
 * manifest: A #PkaManifest
 *
 * Atomically increments the reference count of @manifest by one.
 *
 * Returns: The @manifest pointer with its reference count incremented.
 *
 * Side effects: None.
 */
PkaManifest*
pka_manifest_ref (PkaManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, NULL);
	g_return_val_if_fail(manifest->ref_count > 0, NULL);

	g_atomic_int_inc(&manifest->ref_count);

	return manifest;
}

/**
 * pka_manifest_unref:
 * manifest: A #PkaManifest
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
pka_manifest_unref (PkaManifest *manifest)
{
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(manifest->ref_count > 0);

	if (g_atomic_int_dec_and_test(&manifest->ref_count)) {
		pka_manifest_destroy(manifest);
		g_slice_free(PkaManifest, manifest);
	}
}

/**
 * pka_manifest_get_timeval:
 * @manifest: A #PkaManifest
 * @tv: A #GTimeVal
 *
 * Stores the time of the manifest to @tv.
 *
 * Side effects: None.
 */
void
pka_manifest_get_timeval (PkaManifest *manifest,
                          GTimeVal    *tv)
{
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(tv != NULL);

	*tv = manifest->tv;
}

/**
 * pka_manifest_set_timeval:
 * @manifest: A #PkaManifest
 * @tv: A #GTimeVal
 *
 * Sets the time for when @manifest is the authority for the stream.
 */
void
pka_manifest_set_timeval (PkaManifest *manifest,
                          GTimeVal    *tv)
{
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(tv != NULL);

	manifest->tv = *tv;
}

/**
 * pka_manifest_set_resolution:
 * @manifest: A #PkaManifest.
 * @resolution: A #PkaResolution.
 *
 * Sets the relative timestamp resolution for the manifest.  Setting this
 * allows samples to track only there drift in time since the last manifest
 * was sent.  If you only update on second intervals, then set this to
 * PKA_RESOLUTION_SECOND and we can calculate our drift in seconds.  Since
 * integers are encoded using variable width we can save many bytes in the
 * samples timestamp (in many cases 9 bytes!).
 *
 * Side effects: None.
 */
void
pka_manifest_set_resolution (PkaManifest   *manifest,
                             PkaResolution  resolution)
{
	g_return_if_fail(manifest != NULL);
	manifest->resolution = resolution;
}

/**
 * pka_manifest_get_resolution:
 * @manifest: A #PkaManifest.
 *
 * Retrieves the relative timestamp resolution.
 *
 * Returns: The resolution as a #PkaResolution.
 *
 * Side effects: None.
 */
PkaResolution
pka_manifest_get_resolution (PkaManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, 0);
	return manifest->resolution;
}

/**
 * pka_manifest_append:
 * @manifest: A #PkaManifest
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
pka_manifest_append (PkaManifest *manifest,
                     const gchar *name,
                     GType        type)
{
	PkaManifestRow row;

	g_return_val_if_fail(manifest != NULL, 0);
	g_return_val_if_fail(name != NULL, 0);
	g_return_val_if_fail(type != 0, 0);

	/*
	 * Check for supported types.
	 */
	switch (type) {
	case G_TYPE_INT:
	case G_TYPE_UINT:
	case G_TYPE_LONG:
	case G_TYPE_ULONG:
	case G_TYPE_STRING:
	case G_TYPE_CHAR:
	case G_TYPE_BOOLEAN:
	case G_TYPE_DOUBLE:
		break;
	default:
		g_warn_if_reached();
		return 0;
	}

	row.id = manifest->rows->len + 1;
	row.name = g_strdup(name);
	row.type = type;

	g_array_append_val(manifest->rows, row);

	return row.id;
}

/**
 * pka_manifest_get_n_rows:
 * @manifest: A #PkaManifest
 *
 * Retrieves the number of rows in the manifest.
 *
 * Returns: the row count.
 *
 * Side effects: None.
 */
guint
pka_manifest_get_n_rows (PkaManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, 0);
	g_return_val_if_fail(manifest->rows != NULL, 0);

	return manifest->rows->len;
}

/**
 * pka_manifest_get_row_type:
 * @manifest: A #PkaManifest
 * @row: The row index (one-based)
 *
 * Retrieves the type for a given row.  Rows are 1-indexed.
 *
 * Returns: The GType of the row.
 *
 * Side effects: None.
 */
GType
pka_manifest_get_row_type (PkaManifest *manifest,
                           gint         row)
{
	g_return_val_if_fail(manifest != NULL, G_TYPE_INVALID);
	g_return_val_if_fail(manifest->rows != NULL, G_TYPE_INVALID);
	g_return_val_if_fail(row && row <= manifest->rows->len, G_TYPE_INVALID);

	return g_array_index(manifest->rows, PkaManifestRow, row - 1).type;
}

/**
 * pka_manifest_get_row_name:
 * @manifest: A #PkaManifest
 * @row: The row index (one-based)
 *
 * Retrieves the row name.  The row index starts at 1.
 *
 * Returns: A string containing the row name.
 *
 * Side effects: None.
 */
const gchar*
pka_manifest_get_row_name (PkaManifest *manifest,
                          gint        row)
{
	g_return_val_if_fail(manifest != NULL, NULL);
	g_return_val_if_fail(manifest->rows != NULL, NULL);
	g_return_val_if_fail(row > 0 && row <= manifest->rows->len, NULL);

	return g_array_index(manifest->rows, PkaManifestRow, row - 1).name;
}

/**
 * pka_manifest_get_source_id:
 * @manifest: A #PkaManifest
 *
 * Retrieves the source identifier for the #PkaManifest within its configured
 * #PkaChannel.  This id can be used by encoders to ensure the client knows
 * which source the manifest belongs to.
 *
 * Returns: An integer containing the source id.
 *
 * Side effects: None.
 */
gint
pka_manifest_get_source_id (PkaManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, -1);
	return manifest->source_id;
}

/**
 * pka_manifest_set_source_id:
 * @manifest: A #PkaManifest
 * @source_id: A source id
 *
 * Internal method used to set the source id by a channel receiving the
 * manifest.
 *
 * Side effects: None.
 */
void
pka_manifest_set_source_id (PkaManifest *manifest,
                            gint         source_id)
{
	g_return_if_fail(manifest != NULL);
	manifest->source_id = source_id;
}

/**
 * pka_manifest_compare:
 * @a: A #PkaManifest.
 * @b: A #PkaManifest.
 *
 * Standard qsort() style compare function.
 *
 * Returns: Less than zero if @a precedes @b. Greater than zero if @b
 *   precedes @a. Otherwise, zero.
 * Side effects: None.
 */
gint
pka_manifest_compare (gconstpointer a, /* IN */
                      gconstpointer b) /* IN */
{
	gint aid;
	gint bid;

	aid = pka_manifest_get_source_id((PkaManifest *)a);
	bid = pka_manifest_get_source_id((PkaManifest *)b);

	return aid - bid;
}

/**
 * pka_manifest_get_type:
 *
 * Returns: the #PkaManifest #GType.
 */
GType
pka_manifest_get_type (void)
{
	static GType type_id = 0;
	GType _type_id;

	if (g_once_init_enter((gsize *)&type_id)) {
		_type_id = g_boxed_type_register_static("PkaManifest",
		                                        (GBoxedCopyFunc)pka_manifest_ref,
		                                        (GBoxedFreeFunc)pka_manifest_unref);
		g_once_init_leave((gsize *)&type_id, _type_id);
	}

	return type_id;
}
