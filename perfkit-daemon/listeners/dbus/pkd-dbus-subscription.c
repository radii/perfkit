/* pkd-dbus-subscription.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pkd-dbus-subscription.h"
#include "pkd-dbus-subscription-dbus.h"

G_DEFINE_TYPE (PkdDbusSubscription, pkd_dbus_subscription, G_TYPE_OBJECT)

struct _PkdDbusSubscriptionPrivate
{
	PkdSubscription *sub;
};

PkdDbusSubscription*
pkd_dbus_subscription_new (PkdSubscription *sub)
{
	PkdDbusSubscription *d_sub;
	
	d_sub = g_object_new (PKD_DBUS_TYPE_SUBSCRIPTION, NULL);
	d_sub->priv->sub = pkd_subscription_ref(sub);

	return d_sub;
}

gboolean
pkd_dbus_subscription_enable (PkdDbusSubscription  *sub,
                              GError              **error)
{
	pkd_subscription_enable(sub->priv->sub);
	return TRUE;
}

gboolean
pkd_dbus_subscription_disable (PkdDbusSubscription  *sub,
                               gboolean              drain,
                               GError              **error)
{
	pkd_subscription_disable(sub->priv->sub, drain);
	return TRUE;
}

static void
pkd_dbus_subscription_finalize (GObject *object)
{
	PkdDbusSubscriptionPrivate *priv;

	g_return_if_fail (PKD_DBUS_IS_SUBSCRIPTION (object));

	priv = PKD_DBUS_SUBSCRIPTION (object)->priv;

	G_OBJECT_CLASS (pkd_dbus_subscription_parent_class)->finalize (object);
}

static void
pkd_dbus_subscription_class_init (PkdDbusSubscriptionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_dbus_subscription_finalize;
	g_type_class_add_private (object_class, sizeof (PkdDbusSubscriptionPrivate));

	dbus_g_object_type_install_info(PKD_DBUS_TYPE_SUBSCRIPTION,
	                                &dbus_glib_pkd_dbus_subscription_object_info);
}

static void
pkd_dbus_subscription_init (PkdDbusSubscription *subscription)
{
	subscription->priv = G_TYPE_INSTANCE_GET_PRIVATE (subscription,
	                                                  PKD_DBUS_TYPE_SUBSCRIPTION,
	                                                  PkdDbusSubscriptionPrivate);
}
