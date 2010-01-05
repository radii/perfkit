/* egg-buffer.h
 *
 * Copyright (c) 2010 Christian Hergert
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __EGG_BUFFER_H__
#define __EGG_BUFFER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define EGG_TYPE_BUFFER (egg_buffer_get_type())

typedef struct _EggBuffer EggBuffer;

typedef enum
{
	EGG_BUFFER_INT      = 0,
	EGG_BUFFER_UINT     = 0,
	EGG_BUFFER_INT64    = 0,
	EGG_BUFFER_UINT64   = 0,
	EGG_BUFFER_ENUM     = 0,
	EGG_BUFFER_BOOLEAN  = 0,
	EGG_BUFFER_DOUBLE   = 1,
	EGG_BUFFER_STRING   = 2,
	EGG_BUFFER_DATA     = 2,
	EGG_BUFFER_REPEATED = 2,
	EGG_BUFFER_MESSAGE  = 2,
	EGG_BUFFER_FLOAT    = 5,
} EggBufferTag;

GType          egg_buffer_get_type      (void) G_GNUC_CONST;
EggBuffer*     egg_buffer_new           (void);
EggBuffer*     egg_buffer_new_from_data (const gchar  *data,
                                         gsize         len);
EggBuffer*     egg_buffer_ref           (EggBuffer    *buffer);
void           egg_buffer_unref         (EggBuffer    *buffer);
void           egg_buffer_get_buffer    (EggBuffer    *buffer,
                                         gchar       **data,
                                         gsize        *len);
gboolean       egg_buffer_read_boolean  (EggBuffer    *buffer,
                                         gboolean     *b);
gboolean       egg_buffer_read_data     (EggBuffer    *buffer,
                                         gchar       **data,
                                         gsize        *len);
gboolean       egg_buffer_read_double   (EggBuffer    *buffer,
                                         gdouble      *d);
gboolean       egg_buffer_read_float    (EggBuffer    *buffer,
                                         gfloat       *f);
gboolean       egg_buffer_read_int      (EggBuffer    *buffer,
                                         gint         *i);
gboolean       egg_buffer_read_int64    (EggBuffer    *buffer,
                                         gint64       *i);
gboolean       egg_buffer_read_string   (EggBuffer    *buffer,
                                         gchar       **s);
gboolean       egg_buffer_read_tag      (EggBuffer    *buffer,
                                         guint        *field,
                                         EggBufferTag *tag);
gboolean       egg_buffer_read_uint     (EggBuffer    *buffer,
                                         guint        *i);
gboolean       egg_buffer_read_uint64   (EggBuffer    *buffer,
                                         guint64      *i);
void           egg_buffer_write_boolean (EggBuffer    *buffer,
                                         gboolean      b);
void           egg_buffer_write_data    (EggBuffer    *buffer,
                                         const gchar  *data,
                                         gsize         len);
void           egg_buffer_write_double  (EggBuffer    *buffer,
                                         gdouble       d);
void           egg_buffer_write_float   (EggBuffer    *buffer,
                                         gfloat        f);
void           egg_buffer_write_int     (EggBuffer    *buffer,
                                         gint          i);
void           egg_buffer_write_int64   (EggBuffer    *buffer,
                                         gint64        i);
void           egg_buffer_write_string  (EggBuffer    *buffer,
                                         const gchar  *s);
void           egg_buffer_write_tag     (EggBuffer    *buffer,
                                         guint         field,
                                         EggBufferTag  tag);
void           egg_buffer_write_uint    (EggBuffer    *buffer,
                                         guint         i);
void           egg_buffer_write_uint64  (EggBuffer    *buffer,
                                         guint64       i);

inline gint
egg_buffer_bytes_for_int (gint i)
{
	gint b = 0;

	do {
		b++;
		i >>= 7;
	} while (i > 0);

	return b;
}

G_END_DECLS

#endif /* __EGG_BUFFER_H__ */
