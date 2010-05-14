/* pka-listener-dbus.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKA_LISTENER_DBUS_H__
#define __PKA_LISTENER_DBUS_H__

#include <perfkit-agent/perfkit-agent.h>

G_BEGIN_DECLS

#define PKA_TYPE_LISTENER_DBUS            (pka_listener_dbus_get_type())
#define PKA_LISTENER_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_LISTENER_DBUS, PkaListenerDBus))
#define PKA_LISTENER_DBUS_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_LISTENER_DBUS, PkaListenerDBus const))
#define PKA_LISTENER_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_LISTENER_DBUS, PkaListenerDBusClass))
#define PKA_IS_LISTENER_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_LISTENER_DBUS))
#define PKA_IS_LISTENER_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_LISTENER_DBUS))
#define PKA_LISTENER_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_LISTENER_DBUS, PkaListenerDBusClass))
#define PKA_LISTENER_DBUS_ERROR           (pka_listener_dbus_error_quark())

/**
 * PkaListenerError:
 * @PKA_LISTENER_DBUS_ERROR_NOT_AVAILABLE: 
 * @PKA_LISTENER_DBUS_ERROR_STATE: 
 *
 * The #PkaConnectionDBus error enumeration.
 */
typedef enum
{
	PKA_LISTENER_DBUS_ERROR_NOT_AVAILABLE,
	PKA_LISTENER_DBUS_ERROR_STATE,
} PkaListenerError;

typedef struct _PkaListenerDBus        PkaListenerDBus;
typedef struct _PkaListenerDBusClass   PkaListenerDBusClass;
typedef struct _PkaListenerDBusPrivate PkaListenerDBusPrivate;

struct _PkaListenerDBus
{
	PkaListener parent;

	/*< private >*/
	PkaListenerDBusPrivate *priv;
};

struct _PkaListenerDBusClass
{
	PkaListenerClass parent_class;
};

GType  pka_listener_dbus_get_type    (void) G_GNUC_CONST;
GQuark pka_listener_dbus_error_quark (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PKA_LISTENER_DBUS_H__ */
