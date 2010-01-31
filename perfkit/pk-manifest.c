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

#include <egg-buffer.h>
#include <egg-time.h>

#include "pk-manifest.h"

/**
 * SECTION:pk-manifest
 * @title: PkManifest
 * @short_description: 
 *
 * 
 */

static gboolean decode (PkManifest *manifest, EggBuffer *buffer);

struct _PkManifest
{
	volatile gint ref_count;

	GTimeVal      tv;
	PkResolution  resolution;
	gint          source_id;
	gint          n_rows;
	GArray       *rows;
};

typedef struct
{
	gint   id;
	GType  type;
	gchar *name;
} PkManifestRow;

static void
pk_manifest_destroy (PkManifest *manifest)
{
}

static PkManifest*
pk_manifest_new (void)
{
	PkManifest *manifest;

	manifest = g_slice_new0(PkManifest);
	manifest->ref_count = 1;
	manifest->rows = g_array_new(FALSE, FALSE, sizeof(PkManifestRow));

	return manifest;
}

/**
 * pk_manifest_new_from_data:
 * @data: The manifest data.
 * @length: The length of data.
 *
 * Creates a new instance of #PkManifest from a buffer of data.
 *
 * Returns: the newly created #PkManifest instance.
 *
 * Side effects: None.
 */
PkManifest*
pk_manifest_new_from_data (const guint8 *data,
                           gsize         length)
{
	PkManifest *manifest;
	EggBuffer *buffer;

	manifest = pk_manifest_new();
	buffer = egg_buffer_new_from_data(data, length);

	if (!decode(manifest, buffer)) {
		goto error;
	}

	egg_buffer_unref(buffer);

	return manifest;

error:
	egg_buffer_unref(buffer);
	pk_manifest_unref(manifest);
	return NULL;
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

/**
 * pk_manifest_get_resolution:
 * @manifest: A #PkManifest.
 *
 * Retrieves the time resolution of the manifest here-forth.  This can be
 * used for bucketing of samples.
 *
 * Returns: The #PkResolution of the manifest.
 *
 * Side effects: None.
 */
PkResolution
pk_manifest_get_resolution (PkManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, 0);

	return manifest->resolution;
}

/**
 * pk_manifest_get_n_rows:
 * @manifest: A #PkManifest.
 *
 * Retrieves the number of rows in the manifest.
 *
 * Returns: The number of rows.
 *
 * Side effects: None.
 */
gint
pk_manifest_get_n_rows (PkManifest *manifest)
{
	g_return_val_if_fail(manifest != NULL, 0);

	return manifest->n_rows;
}

/**
 * pk_manifest_get_row_type:
 * @manifest: A #PkManifest.
 * @row: row number which starts from 1.
 *
 * Retrieves the #GType for the row content.
 *
 * Returns: A #GType.
 *
 * Side effects: None.
 */
GType
pk_manifest_get_row_type (PkManifest *manifest,
                          gint        row)
{
	g_return_val_if_fail(manifest != NULL, G_TYPE_INVALID);
	g_return_val_if_fail(row > 0 && row <= manifest->n_rows, G_TYPE_INVALID);

	return g_array_index(manifest->rows, PkManifestRow, row - 1).type;
}

/**
 * pk_manifest_get_row_name:
 * @manifest: A #PkManifest.
 * @row: row number which starts from 1.
 *
 * Retrieves the name of a row.
 *
 * Returns: 
 *
 * Side effects: None.
 */
const gchar*
pk_manifest_get_row_name (PkManifest *manifest,
                          gint        row)
{
	g_return_val_if_fail(manifest != NULL, NULL);
	g_return_val_if_fail(row > 0 && row <= manifest->n_rows, NULL);

	return g_array_index(manifest->rows, PkManifestRow, row - 1).name;
}

/**
 * pk_manifest_get_timeval:
 * @manifest: A #PkManifest.
 * @row: row number which starts from 1.
 *
 * Retrieves the #GTimeVal for with the manifest is authoritative.
 *
 * Side effects: None.
 */
void
pk_manifest_get_timeval (PkManifest *manifest,
                         GTimeVal   *tv)
{
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(tv != NULL);

	*tv = manifest->tv;
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

static gint
sort_func (gconstpointer a,
           gconstpointer b)
{
	const PkManifestRow *row_a = a,
	                    *row_b = b;

	return (row_a->id == row_b->id) ? 0 : row_a->id - row_b->id;
}

static gboolean
decode (PkManifest *manifest,
        EggBuffer  *buffer)
{
	guint field, tag, u32, len;
	guint64 u64;
	gsize end;
	gint i;

	g_return_val_if_fail(manifest != NULL, FALSE);
	g_return_val_if_fail(buffer != NULL, FALSE);

	/* timestamp */
	if (!egg_buffer_read_tag(buffer, &field, &tag)) {
		return FALSE;
	}
	if (tag != EGG_BUFFER_UINT64 || field != 1) {
		g_debug("%d", __LINE__);
		return FALSE;
	}
	if (!egg_buffer_read_uint64(buffer, &u64)) {
		return FALSE;
	}
	g_time_val_from_ticks(&manifest->tv, (GTimeSpan *)&u64);

	/* resolution */
	if (!egg_buffer_read_tag(buffer, &field, &tag)) {
		return FALSE;
	}
	if (tag != EGG_BUFFER_UINT || field != 2) {
		return FALSE;
	}
	if (!egg_buffer_read_uint(buffer, &u32)) {
		return FALSE;
	}
	if (u32 > PK_RESOLUTION_HOUR) {
		return FALSE;
	}
	manifest->resolution = u32;

	/* source */
	if (!egg_buffer_read_tag(buffer, &field, &tag)) {
		return FALSE;
	}
	if (tag != EGG_BUFFER_UINT || field != 3) {
		return FALSE;
	}
	if (!egg_buffer_read_uint(buffer, &u32)) {
		return FALSE;
	}
	manifest->source_id = u32;

	/* columns */
	if (!egg_buffer_read_tag(buffer, &field, &tag)) {
		return FALSE;
	}
	if (tag != EGG_BUFFER_REPEATED || field != 4) {
		return FALSE;
	}

	/* len of data */
	if (!egg_buffer_read_uint(buffer, &len)) {
		return FALSE;
	}

	/* determine end of buffer */
	end = egg_buffer_get_pos(buffer) + len;

	/* get manifest rows */
	while (egg_buffer_get_pos(buffer) < end) {
		PkManifestRow row;
		gchar *name;
		guint row_id;
		GType row_type;

		/* row data length */
		if (!egg_buffer_read_uint(buffer, &u32)) {
			return FALSE;
		}

		/* row id */
		if (!egg_buffer_read_tag(buffer, &field, &tag)) {
			return FALSE;
		}
		if (field != 1 || tag != EGG_BUFFER_UINT) {
			return FALSE;
		}
		if (!egg_buffer_read_uint(buffer, &row_id)) {
			return FALSE;
		}

		/* row type */
		if (!egg_buffer_read_tag(buffer, &field, &tag)) {
			return FALSE;
		}
		if (field != 2 || tag != EGG_BUFFER_ENUM) {
			return FALSE;
		}
		if (!egg_buffer_read_uint(buffer, &row_type)) {
			return FALSE;
		}

		/* row name */
		if (!egg_buffer_read_tag(buffer, &field, &tag)) {
			return FALSE;
		}
		if (field != 3 || tag != EGG_BUFFER_STRING) {
			return FALSE;
		}
		if (!egg_buffer_read_string(buffer, &name)) {
			return FALSE;
		}

		row.id = row_id;
		row.type = row_type;
		row.name = name;

		g_array_append_val(manifest->rows, row);
		g_array_sort(manifest->rows, sort_func);
		manifest->n_rows++;
	}

	/* make sure all rows were sent */
	for (i = 0; i < manifest->n_rows; i++) {
		PkManifestRow *row = &g_array_index(manifest->rows, PkManifestRow, i);
		if (row->id != (i + 1)) {
			return FALSE;
		}
	}

	return TRUE;
}
