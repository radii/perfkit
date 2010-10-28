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
#include "pk-log.h"
#include "pk-util.h"

static gboolean decode (PkManifest *manifest, EggBuffer *buffer);

struct _PkManifest
{
	volatile gint   ref_count;
	struct timespec ts;
	PkResolution    resolution;
	gint            source_id;
	gint            n_rows;
	GArray         *rows;
};

typedef struct
{
	gint   id;
	GType  type;
	gchar *name;
} PkManifestRow;

static void
pk_manifest_destroy (PkManifest *manifest) /* IN */
{
	gint i;

	g_return_if_fail(manifest != NULL);

	ENTRY;

	/* free row names */
	for (i = 0; i < manifest->n_rows; i++) {
		g_free(g_array_index(manifest->rows, PkManifestRow, i).name);
	}

	/* free row array */
	g_array_unref(manifest->rows);

	/* mark fields as canaries */
	manifest->rows = NULL;
	manifest->source_id = -1;
	EXIT;
}

static PkManifest*
pk_manifest_new (void)
{
	PkManifest *manifest;

	ENTRY;
	manifest = g_slice_new0(PkManifest);
	manifest->ref_count = 1;
	manifest->rows = g_array_new(FALSE, FALSE, sizeof(PkManifestRow));
	RETURN(manifest);
}

gint
pk_manifest_get_source_id (PkManifest *manifest) /* IN */
{
	g_return_val_if_fail(manifest != NULL, -1);
	return manifest->source_id;
}

gint
pk_manifest_get_row_id (PkManifest  *manifest,
                        const gchar *name)
{
	PkManifestRow *row;
	gint i;

	g_return_val_if_fail(manifest != NULL, -1);
	g_return_val_if_fail(manifest->rows != NULL, -1);
	g_return_val_if_fail(name != NULL, -1);

	for (i = 0; i < manifest->rows->len; i++) {
		row = &g_array_index(manifest->rows, PkManifestRow, i);
		if (!g_strcmp0(name, row->name)) {
			return i + 1;
		}
	}

	return -1;
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
pk_manifest_new_from_data (const guint8 *data,   /* IN */
                           gsize         length) /* IN */
{
	PkManifest *manifest;
	EggBuffer *buffer;

	ENTRY;
	manifest = pk_manifest_new();
	buffer = egg_buffer_new_from_data(data, length);
	if (!decode(manifest, buffer)) {
		GOTO(error);
	}
	egg_buffer_unref(buffer);
	RETURN(manifest);

  error:
	egg_buffer_unref(buffer);
	pk_manifest_unref(manifest);
	RETURN(NULL);
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
pk_manifest_ref (PkManifest *manifest) /* IN */
{
	g_return_val_if_fail(manifest != NULL, NULL);
	g_return_val_if_fail(manifest->ref_count > 0, NULL);

	ENTRY;
	g_atomic_int_inc(&manifest->ref_count);
	RETURN(manifest);
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

	ENTRY;
	if (g_atomic_int_dec_and_test(&manifest->ref_count)) {
		pk_manifest_destroy(manifest);
		g_slice_free(PkManifest, manifest);
	}
	EXIT;
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
pk_manifest_get_resolution (PkManifest *manifest) /* IN */
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
pk_manifest_get_n_rows (PkManifest *manifest) /* IN */
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
pk_manifest_get_row_type (PkManifest *manifest, /* IN */
                          gint        row)      /* IN */
{
	g_return_val_if_fail(manifest != NULL, G_TYPE_INVALID);
	g_return_val_if_fail(manifest->rows != NULL, G_TYPE_INVALID);
	g_return_val_if_fail(row > 0, G_TYPE_INVALID);
	g_return_val_if_fail(row <= manifest->n_rows, G_TYPE_INVALID);

	ENTRY;
	RETURN(g_array_index(manifest->rows, PkManifestRow, --row).type);
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
pk_manifest_get_row_name (PkManifest *manifest, /* IN */
                          gint        row)      /* IN */
{
	g_return_val_if_fail(manifest != NULL, NULL);
	g_return_val_if_fail(row > 0, NULL);
	g_return_val_if_fail(row <= manifest->n_rows, NULL);

	ENTRY;
	RETURN(g_array_index(manifest->rows, PkManifestRow, row - 1).name);
}

/**
 * pk_manifest_get_timeval:
 * @manifest: A #PkManifest.
 * @ts: A struct timespec.
 *
 * Retrieves the struct timespec for with the manifest is authoritative.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pk_manifest_get_timespec (PkManifest      *manifest, /* IN */
                          struct timespec *ts)       /* OUT */
{
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(ts != NULL);

	*ts = manifest->ts;
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
		return FALSE;
	}
	if (!egg_buffer_read_uint64(buffer, &u64)) {
		return FALSE;
	}
	timespec_from_usec(&manifest->ts, u64);

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
		gchar *name = NULL;
		guint row_id = 0;
		GType row_type = 0;

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
		if (!egg_buffer_read_uint(buffer, (guint *)&row_type)) {
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
		PkManifestRow *row;

		row = &g_array_index(manifest->rows, PkManifestRow, i);
		if (row->id != (i + 1)) {
			return FALSE;
		}
	}

	return TRUE;
}
