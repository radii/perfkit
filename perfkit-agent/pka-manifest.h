/* pka-manifest.h
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

#ifndef __PKA_MANIFEST_H__
#define __PKA_MANIFEST_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKA_TYPE_MANIFEST (pka_manifest_get_type())

typedef struct _PkaManifest PkaManifest;

/**
 * PkaResolution:
 * @PKA_RESOLUTION_PRECISE: Samples include relative timestamp in 100-ns ticks.
 * @PKA_RESOLUTION_USEC: Samples include relative timestamp in microseconds.
 * @PKA_RESOLUTION_MSEC: Samples include relative timestamps in millseconds.
 * @PKA_RESOLUTION_SECOND: Samples include relative timestamps in seconds.
 * @PKA_RESOLUTION_MINUTE: Samples include relative timestamps in minutes.
 * @PKA_RESOLUTION_HOUR: Samples include relative timestamps in hours.
 *
 * The #PkaResolution enumeration.  Samples include their difference in time
 * since the describing #PkManifest.  The resolution allows control over how
 * that relative timestamp is encoded.
 *
 * Normally, a sample 1 second difference would be 100000000 100-ns ticks.
 * However, if @PKA_RESOLUTION_SECOND is used, it would be 1.  This is helpful
 * in that "varint" encoding allows for very tight compression of small
 * integers, thus saving precious bytes in the wire format.
 */
typedef enum
{
	PKA_RESOLUTION_PRECISE,
	PKA_RESOLUTION_USEC,
	PKA_RESOLUTION_MSEC,
	PKA_RESOLUTION_SECOND,
	PKA_RESOLUTION_MINUTE,
	PKA_RESOLUTION_HOUR,
} PkaResolution;

GType            pka_manifest_get_type       (void) G_GNUC_CONST;
gint             pka_manifest_compare        (gconstpointer  a,
                                              gconstpointer  b);
PkaManifest*     pka_manifest_new            (void);
PkaManifest*     pka_manifest_sized_new      (gint           size);
PkaManifest*     pka_manifest_ref            (PkaManifest   *manifest);
void             pka_manifest_unref          (PkaManifest   *manifest);
gint             pka_manifest_get_source_id  (PkaManifest   *manifest);
PkaResolution    pka_manifest_get_resolution (PkaManifest   *manifest);
void             pka_manifest_set_resolution (PkaManifest   *manifest,
                                              PkaResolution  resolution);
void             pka_manifest_get_timeval    (PkaManifest   *manifest,
                                              GTimeVal      *tv);
void             pka_manifest_set_timeval    (PkaManifest   *manifest,
                                              GTimeVal      *tv);
guint            pka_manifest_append         (PkaManifest   *manifest,
                                              const gchar   *name,
                                              GType          type);
guint            pka_manifest_get_n_rows     (PkaManifest   *manifest);
GType            pka_manifest_get_row_type   (PkaManifest   *manifest,
                                              gint           row);
const gchar*     pka_manifest_get_row_name   (PkaManifest   *manifest,
                                              gint           row);

G_END_DECLS

#endif /* __PKA_MANIFEST_H__ */
