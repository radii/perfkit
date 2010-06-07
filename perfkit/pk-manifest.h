/* pk-manifest.h
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit/perfkit.h> can be included directly."
#endif

#ifndef __PK_MANIFEST_H__
#define __PK_MANIFEST_H__

#include <glib-object.h>
#include <time.h>

G_BEGIN_DECLS

#define PK_TYPE_MANIFEST (pk_manifest_get_type())

typedef struct _PkManifest PkManifest;

typedef void (*PkManifestFunc) (PkManifest *manifest, gpointer user_data);

typedef enum
{
	PK_RESOLUTION_USEC,
	PK_RESOLUTION_MSEC,
	PK_RESOLUTION_SECOND,
	PK_RESOLUTION_MINUTE,
	PK_RESOLUTION_HOUR,
} PkResolution;

GType           pk_manifest_get_type       (void) G_GNUC_CONST;
PkManifest*     pk_manifest_new_from_data  (const guint8 *data,
                                            gsize         length);
PkManifest*     pk_manifest_ref            (PkManifest *manifest);
void            pk_manifest_unref          (PkManifest *manifest);
PkResolution    pk_manifest_get_resolution (PkManifest *manifest);
gint            pk_manifest_get_n_rows     (PkManifest *manifest);
GType           pk_manifest_get_row_type   (PkManifest *manifest,
                                            gint        row);
const gchar*    pk_manifest_get_row_name   (PkManifest *manifest,
                                            gint        row);
void            pk_manifest_get_timespec   (PkManifest      *manifest,
                                            struct timespec *ts);
gint            pk_manifest_get_source_id  (PkManifest *manifest);

G_END_DECLS

#endif /* __PK_MANIFEST_H__ */
