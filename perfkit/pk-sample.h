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

#ifndef __PK_SAMPLE_H__
#define __PK_SAMPLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PK_TYPE_SAMPLE (pk_sample_get_type())

typedef struct _PkSample PkSample;

GType        pk_sample_get_type   (void) G_GNUC_CONST;
PkSample*    pk_sample_ref        (PkSample *sample);
void         pk_sample_unref      (PkSample *sample);
GArray*      pk_sample_get_array  (PkSample *sample);

G_END_DECLS

#endif /* __PK_SAMPLE_H__ */
