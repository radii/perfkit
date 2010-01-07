/* pkd-sample.h
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

#if !defined (__PERFKIT_DAEMON_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-daemon/perfkit-daemon.h> can be included directly."
#endif

#ifndef __PKD_SAMPLE_H__
#define __PKD_SAMPLE_H__

#include <glib.h>

#include "pkd-manifest.h"

G_BEGIN_DECLS

typedef struct _PkdSample PkdSample;

GType       pkd_sample_get_type       (void) G_GNUC_CONST;
PkdSample*  pkd_sample_new            (void);
PkdSample*  pkd_sample_ref            (PkdSample        *sample);
void        pkd_sample_unref          (PkdSample        *sample);
void        pkd_sample_get_data       (PkdSample        *sample,
                                       const guint8    **data,
                                       gsize            *dapkd_len);
gint        pkd_sample_get_source_id  (PkdSample        *sample);
void        pkd_sample_get_timeval    (PkdSample        *sample,
                                       GTimeVal         *tv);
void        pkd_sample_set_timeval    (PkdSample        *sample,
                                       GTimeVal         *tv);
void        pkd_sample_append_boolean (PkdSample        *sample,
                                       gint              field,
                                       gboolean          b);
void        pkd_sample_append_double  (PkdSample        *sample,
                                       gint              field,
                                       gdouble           d);
void        pkd_sample_append_float   (PkdSample        *sample,
                                       gint              field,
                                       gfloat            f);
void        pkd_sample_append_int     (PkdSample        *sample,
                                       gint              field,
                                       gint              i);
void        pkd_sample_append_int64   (PkdSample        *sample,
                                       gint              field,
                                       gint64            i);
void        pkd_sample_append_string  (PkdSample        *sample,
                                       gint              field,
                                       const gchar      *s);
void        pkd_sample_append_uint    (PkdSample        *sample,
                                       gint              field,
                                       guint             u);
void        pkd_sample_append_uint64  (PkdSample        *sample,
                                       gint              field,
                                       guint64           u);
void        pkd_sample_append_timeval (PkdSample        *sample,
                                       gint              field,
                                       GTimeVal         *tv);

G_END_DECLS

#endif /* __PKD_SAMPLE_H__ */
