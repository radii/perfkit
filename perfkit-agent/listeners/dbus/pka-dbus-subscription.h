/* pka-dbus-subscription.h
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

#ifndef __PKA_DBUS_SUBSCRIPTION_H__
#define __PKA_DBUS_SUBSCRIPTION_H__

#include <glib-object.h>

#include <perfkit-agent/perfkit-agent.h>

G_BEGIN_DECLS

#define PKA_DBUS_TYPE_SUBSCRIPTION            (pka_dbus_subscription_get_type())
#define PKA_DBUS_SUBSCRIPTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_DBUS_TYPE_SUBSCRIPTION, PkaDbusSubscription))
#define PKA_DBUS_SUBSCRIPTION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_DBUS_TYPE_SUBSCRIPTION, PkaDbusSubscription const))
#define PKA_DBUS_SUBSCRIPTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_DBUS_TYPE_SUBSCRIPTION, PkaDbusSubscriptionClass))
#define PKA_DBUS_IS_SUBSCRIPTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_DBUS_TYPE_SUBSCRIPTION))
#define PKA_DBUS_IS_SUBSCRIPTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_DBUS_TYPE_SUBSCRIPTION))
#define PKA_DBUS_SUBSCRIPTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_DBUS_TYPE_SUBSCRIPTION, PkaDbusSubscriptionClass))

typedef struct _PkaDbusSubscription        PkaDbusSubscription;
typedef struct _PkaDbusSubscriptionClass   PkaDbusSubscriptionClass;
typedef struct _PkaDbusSubscriptionPrivate PkaDbusSubscriptionPrivate;

struct _PkaDbusSubscription
{
	GObject parent;

	/*< private >*/
	PkaDbusSubscriptionPrivate *priv;
};

struct _PkaDbusSubscriptionClass
{
	GObjectClass parent_class;
};

GType                pka_dbus_subscription_get_type (void) G_GNUC_CONST;
PkaDbusSubscription* pka_dbus_subscription_new      (PkaSubscription *sub);
gboolean             pka_dbus_subscription_enable   (PkaDbusSubscription *subscription, GError **error);
gboolean             pka_dbus_subscription_disable  (PkaDbusSubscription *subscription, gboolean drain, GError **error);

G_END_DECLS

#endif /* __PKA_DBUS_SUBSCRIPTION_H__ */
