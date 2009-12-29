/* pkd-manifest.h
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

#ifndef __PKD_MANIFEST_H__
#define __PKD_MANIFEST_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_MANIFEST (pkd_manifest_get_type())

typedef struct _PkdManifest PkdManifest;

GType            pkd_manifest_get_type      (void) G_GNUC_CONST;
PkdManifest*     pkd_manifest_new           (void);
PkdManifest*     pkd_manifest_sized_new     (gint          size);
PkdManifest*     pkd_manifest_ref           (PkdManifest  *manifest);
void             pkd_manifest_unref         (PkdManifest  *manifest);
guint            pkd_manifest_append        (PkdManifest  *manifest,
                                             const gchar  *name,
                                             GType         type);
guint            pkd_manifest_get_n_rows    (PkdManifest  *manifest);
GType            pkd_manifest_get_row_type  (PkdManifest  *manifest,
                                             gint          row);
const gchar*     pkd_manifest_get_row_name  (PkdManifest  *manifest,
                                             gint          row);

G_END_DECLS

#endif /* __PKD_MANIFEST_H__ */
