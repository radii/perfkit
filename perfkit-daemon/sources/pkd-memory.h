/* pkd-memory.h
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

#ifndef __PKD_MEMORY_H__
#define __PKD_MEMORY_H__

#include <glib-object.h>

#include <perfkit-daemon/perfkit-daemon.h>

G_BEGIN_DECLS

#define PKD_TYPE_MEMORY            (pkd_memory_get_type())
#define PKD_MEMORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_MEMORY, PkdMemory))
#define PKD_MEMORY_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_MEMORY, PkdMemory const))
#define PKD_MEMORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_MEMORY, PkdMemoryClass))
#define PKD_IS_MEMORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_MEMORY))
#define PKD_IS_MEMORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_MEMORY))
#define PKD_MEMORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_MEMORY, PkdMemoryClass))

typedef struct _PkdMemory        PkdMemory;
typedef struct _PkdMemoryClass   PkdMemoryClass;
typedef struct _PkdMemoryPrivate PkdMemoryPrivate;

struct _PkdMemory
{
	PkdSource parent;

	/*< private >*/
	PkdMemoryPrivate *priv;
};

struct _PkdMemoryClass
{
	PkdSourceClass parent_class;
};

GType      pkd_memory_get_type (void) G_GNUC_CONST;
PkdSource* pkd_memory_new      (void);

G_END_DECLS

#endif /* __PKD_MEMORY_H__ */
