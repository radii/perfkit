/* pkd-dbus-subscription.h
 *
 * Copyright (C) 2010 Christian Hergert
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

#ifndef __PKD_DBUS_SUBSCRIPTION_H__
#define __PKD_DBUS_SUBSCRIPTION_H__

#include <glib-object.h>

#include <perfkit-daemon/perfkit-daemon.h>

G_BEGIN_DECLS

#define PKD_DBUS_TYPE_SUBSCRIPTION            (pkd_dbus_subscription_get_type())
#define PKD_DBUS_SUBSCRIPTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_DBUS_TYPE_SUBSCRIPTION, PkdDbusSubscription))
#define PKD_DBUS_SUBSCRIPTION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_DBUS_TYPE_SUBSCRIPTION, PkdDbusSubscription const))
#define PKD_DBUS_SUBSCRIPTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_DBUS_TYPE_SUBSCRIPTION, PkdDbusSubscriptionClass))
#define PKD_DBUS_IS_SUBSCRIPTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_DBUS_TYPE_SUBSCRIPTION))
#define PKD_DBUS_IS_SUBSCRIPTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_DBUS_TYPE_SUBSCRIPTION))
#define PKD_DBUS_SUBSCRIPTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_DBUS_TYPE_SUBSCRIPTION, PkdDbusSubscriptionClass))

typedef struct _PkdDbusSubscription        PkdDbusSubscription;
typedef struct _PkdDbusSubscriptionClass   PkdDbusSubscriptionClass;
typedef struct _PkdDbusSubscriptionPrivate PkdDbusSubscriptionPrivate;

struct _PkdDbusSubscription
{
	GObject parent;

	/*< private >*/
	PkdDbusSubscriptionPrivate *priv;
};

struct _PkdDbusSubscriptionClass
{
	GObjectClass parent_class;
};

GType                pkd_dbus_subscription_get_type (void) G_GNUC_CONST;
PkdDbusSubscription* pkd_dbus_subscription_new      (PkdSubscription *sub);
gboolean             pkd_dbus_subscription_enable   (PkdDbusSubscription *subscription, GError **error);
gboolean             pkd_dbus_subscription_disable  (PkdDbusSubscription *subscription, gboolean drain, GError **error);

G_END_DECLS

#endif /* __PKD_DBUS_SUBSCRIPTION_H__ */
