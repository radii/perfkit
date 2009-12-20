/* pkd-listener.h
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

#ifndef __PKD_LISTENER_H__
#define __PKD_LISTENER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_LISTENER             (pkd_listener_get_type())
#define PKD_LISTENER(o)               (G_TYPE_CHECK_INSTANCE_CAST((o),    PKD_TYPE_LISTENER, PkdListener))
#define PKD_IS_LISTENER(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o),    PKD_TYPE_LISTENER))
#define PKD_LISTENER_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), PKD_TYPE_LISTENER, PkdListenerIface))

typedef struct _PkdListener      PkdListener;
typedef struct _PkdListenerIface PkdListenerIface;

struct _PkdListenerIface
{
	GTypeInterface parent;

	gboolean (*initialize) (PkdListener *listener, GError **error);
	gboolean (*listen)     (PkdListener *listener, GError **error);
	void     (*shutdown)   (PkdListener *listener);
};

GType    pkd_listener_get_type   (void) G_GNUC_CONST;
gboolean pkd_listener_initialize (PkdListener *listener, GError **error);
gboolean pkd_listener_listen     (PkdListener *listener, GError **error);
void     pkd_listener_shutdown   (PkdListener *listener);

G_END_DECLS

#endif /* __PKD_LISTENER_H__ */
