/* pka-dbus.h
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

#ifndef __PKA_DBUS_H__
#define __PKA_DBUS_H__

#include <glib-object.h>

#include <perfkit-agent/perfkit-agent.h>

G_BEGIN_DECLS

#define PKA_TYPE_DBUS            (pka_dbus_get_type())
#define PKA_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_DBUS, PkaDBus))
#define PKA_DBUS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_DBUS, PkaDBus const))
#define PKA_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_DBUS, PkaDBusClass))
#define PKA_IS_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_DBUS))
#define PKA_IS_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_DBUS))
#define PKA_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_DBUS, PkaDBusClass))

typedef struct _PkaDBus        PkaDBus;
typedef struct _PkaDBusClass   PkaDBusClass;
typedef struct _PkaDBusPrivate PkaDBusPrivate;

struct _PkaDBus
{
	PkaListener parent;

	/*< private >*/
	PkaDBusPrivate *priv;
};

struct _PkaDBusClass
{
	PkaListenerClass parent_class;
};

GType            pka_dbus_get_type       (void) G_GNUC_CONST;
DBusGConnection* pka_dbus_get_connection (void);

G_END_DECLS

#endif /* __PKA_DBUS_H__ */
