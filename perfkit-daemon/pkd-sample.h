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

#ifndef __PKD_SAMPLE_H__
#define __PKD_SAMPLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_SAMPLE (pkd_sample_get_type())

typedef struct _PkdSample PkdSample;

GType        pkd_sample_get_type   (void) G_GNUC_CONST;
PkdSample*   pkd_sample_new        (void);
PkdSample*   pkd_sample_sized_new  (gsize      n_bytes);
PkdSample*   pkd_sample_ref        (PkdSample *sample);
void         pkd_sample_unref      (PkdSample *sample);
void         pkd_sample_write_int  (PkdSample *sample,
                                    gint       v_int);
void         pkd_sample_write_char (PkdSample *sample,
                                    gchar      v_char);
GArray*      pkd_sample_get_array  (PkdSample *sample);

G_END_DECLS

#endif /* __PKD_SAMPLE_H__ */
