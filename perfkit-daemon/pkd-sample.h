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
typedef struct _PkdSampleWriter PkdSampleWriter;

struct _PkdSampleWriter
{
	PkdManifest *manifest;
	PkdSample   *sample;
	gint         row_count;
	gint         pos;
	gint         extra;
	gchar       *data;
	gchar        inline_data[64];
};

GType       pkd_sample_get_type       (void) G_GNUC_CONST;
PkdSample*  pkd_sample_new            (void);
PkdSample*  pkd_sample_ref            (PkdSample        *sample);
void        pkd_sample_unref          (PkdSample        *sample);
void        pkd_sample_get_data       (PkdSample        *sample,
                                       gchar           **data,
                                       gsize            *dapkd_len);
void        pkd_sample_writer_init    (PkdSampleWriter  *writer,
                                       PkdManifest      *manifest,
                                       PkdSample        *sample);
void        pkd_sample_writer_string  (PkdSampleWriter  *writer,
                                       gint              index,
                                       const gchar      *s);
void        pkd_sample_writer_boolean (PkdSampleWriter  *writer,
                                       gint              index,
                                       gboolean          b);
void        pkd_sample_writer_integer (PkdSampleWriter  *writer,
                                       gint              index,
                                       gint              i);
void        pkd_sample_writer_finish  (PkdSampleWriter  *writer);

G_END_DECLS

#endif /* __PKD_SAMPLE_H__ */
