/* pka-sample.h
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

#if !defined (__PERFKIT_AGENT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-agent/perfkit-agent.h> can be included directly."
#endif

#ifndef __PKA_SAMPLE_H__
#define __PKA_SAMPLE_H__

#include <glib.h>

#include "pka-manifest.h"

G_BEGIN_DECLS

#define PKA_TYPE_SAMPLE (pka_sample_get_type())

typedef struct _PkaSample PkaSample;

GType       pka_sample_get_type       (void) G_GNUC_CONST;
PkaSample*  pka_sample_new            (void);
PkaSample*  pka_sample_ref            (PkaSample        *sample);
void        pka_sample_unref          (PkaSample        *sample);
void        pka_sample_get_data       (PkaSample        *sample,
                                       const guint8    **data,
                                       gsize            *dapka_len);
gint        pka_sample_get_source_id  (PkaSample        *sample) G_GNUC_PURE;
void        pka_sample_get_timeval    (PkaSample        *sample,
                                       GTimeVal         *tv);
void        pka_sample_set_timeval    (PkaSample        *sample,
                                       GTimeVal         *tv);
void        pka_sample_append_boolean (PkaSample        *sample,
                                       gint              field,
                                       gboolean          b);
void        pka_sample_append_double  (PkaSample        *sample,
                                       gint              field,
                                       gdouble           d);
void        pka_sample_append_float   (PkaSample        *sample,
                                       gint              field,
                                       gfloat            f);
void        pka_sample_append_int     (PkaSample        *sample,
                                       gint              field,
                                       gint              i);
void        pka_sample_append_int64   (PkaSample        *sample,
                                       gint              field,
                                       gint64            i);
void        pka_sample_append_string  (PkaSample        *sample,
                                       gint              field,
                                       const gchar      *s);
void        pka_sample_append_uint    (PkaSample        *sample,
                                       gint              field,
                                       guint             u);
void        pka_sample_append_uint64  (PkaSample        *sample,
                                       gint              field,
                                       guint64           u);
void        pka_sample_append_timeval (PkaSample        *sample,
                                       gint              field,
                                       GTimeVal         *tv);

G_END_DECLS

#endif /* __PKA_SAMPLE_H__ */
