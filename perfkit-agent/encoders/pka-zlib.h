/* pka-zlib.h
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

#ifndef __PKA_ZLIB_H__
#define __PKA_ZLIB_H__

#include <glib-object.h>

#include <perfkit-agent/perfkit-agent.h>

G_BEGIN_DECLS

#define PKA_TYPE_ZLIB            (pka_zlib_get_type())
#define PKA_ZLIB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_ZLIB, PkaZlib))
#define PKA_ZLIB_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_ZLIB, PkaZlib const))
#define PKA_ZLIB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_ZLIB, PkaZlibClass))
#define PKA_IS_ZLIB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_ZLIB))
#define PKA_IS_ZLIB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_ZLIB))
#define PKA_ZLIB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_ZLIB, PkaZlibClass))

typedef struct _PkaZlib        PkaZlib;
typedef struct _PkaZlibClass   PkaZlibClass;
typedef struct _PkaZlibPrivate PkaZlibPrivate;

struct _PkaZlib
{
	GObject parent;

	/*< private >*/
	PkaZlibPrivate *priv;
};

struct _PkaZlibClass
{
	GObjectClass parent_class;
};

GType       pka_zlib_get_type (void) G_GNUC_CONST;
PkaEncoder* pka_zlib_new      (void);

G_END_DECLS

#endif /* __PKA_ZLIB_H__ */
