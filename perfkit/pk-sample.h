/* pk-sample.h
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit/perfkit.h> can be included directly."
#endif

#ifndef __PK_SAMPLE_H__
#define __PK_SAMPLE_H__

#include <glib-object.h>
#include <time.h>

G_BEGIN_DECLS

#define PK_TYPE_SAMPLE (pk_sample_get_type())

typedef struct _PkSample PkSample;

/**
 * PkSampleFunc:
 * @manifest: A #PkManifest.
 * @sample: A #PkSample.
 *
 * 
 *
 * Returns: None.
 */
typedef void (*PkSampleFunc) (PkManifest  *manifest,
                              PkSample    *sample,
                              gpointer     user_data);

/**
 * PkManifestResolver:
 * @source_id: The source identifier for which to retrieve the manifest.
 * @manifest: A location for a #PkManifest.
 * @user_data: user data supplied to pk_sample_new_from_data().
 *
 * 
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
typedef gboolean (*PkManifestResolver) (gint         source_id,
                                        PkManifest **manifest,
                                        gpointer     user_data);

GType         pk_sample_get_type      (void) G_GNUC_CONST;
PkSample*     pk_sample_new_from_data (PkManifestResolver  resolver,
                                       gpointer            user_data,
                                       const guint8       *data,
                                       gsize               length,
                                       gsize              *n_read);
PkSample*     pk_sample_ref           (PkSample           *sample);
void          pk_sample_unref         (PkSample           *sample);
gboolean      pk_sample_get_value     (PkSample           *sample,
                                       guint               row_id,
                                       GValue             *value);
gint          pk_sample_get_source_id (PkSample           *sample);
void          pk_sample_get_timespec  (PkSample           *sample,
                                       struct timespec    *ts);
void          pk_sample_get_timeval   (PkSample           *sample,
                                       GTimeVal           *tv);

G_END_DECLS

#endif /* __PK_SAMPLE_H__ */
