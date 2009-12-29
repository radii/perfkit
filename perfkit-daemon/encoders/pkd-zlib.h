/* pkd-zlib.h
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKD_ZLIB_H__
#define __PKD_ZLIB_H__

#include <glib-object.h>

#include <perfkit-daemon/perfkit-daemon.h>

G_BEGIN_DECLS

#define PKD_TYPE_ZLIB            (pkd_zlib_get_type())
#define PKD_ZLIB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_ZLIB, PkdZlib))
#define PKD_ZLIB_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_ZLIB, PkdZlib const))
#define PKD_ZLIB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_ZLIB, PkdZlibClass))
#define PKD_IS_ZLIB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_ZLIB))
#define PKD_IS_ZLIB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_ZLIB))
#define PKD_ZLIB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_ZLIB, PkdZlibClass))

typedef struct _PkdZlib        PkdZlib;
typedef struct _PkdZlibClass   PkdZlibClass;
typedef struct _PkdZlibPrivate PkdZlibPrivate;

struct _PkdZlib
{
	GObject parent;

	/*< private >*/
	PkdZlibPrivate *priv;
};

struct _PkdZlibClass
{
	GObjectClass parent_class;
};

GType      pkd_zlib_get_type (void) G_GNUC_CONST;
PkdEncoder* pkd_zlib_new      (void);

G_END_DECLS

#endif /* __PKD_ZLIB_H__ */
