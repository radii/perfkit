/* pka-dbus-subscription.c
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

#include "pka-dbus-subscription.h"
#include "pka-dbus-subscription-dbus.h"

G_DEFINE_TYPE (PkaDbusSubscription, pka_dbus_subscription, G_TYPE_OBJECT)

struct _PkaDbusSubscriptionPrivate
{
	PkaSubscription *sub;
};

PkaDbusSubscription*
pka_dbus_subscription_new (PkaSubscription *sub)
{
	PkaDbusSubscription *d_sub;

	d_sub = g_object_new (PKA_DBUS_TYPE_SUBSCRIPTION, NULL);
	d_sub->priv->sub = pka_subscription_ref(sub);

	return d_sub;
}

gboolean
pka_dbus_subscription_enable (PkaDbusSubscription  *sub,
                              GError              **error)
{
	g_return_val_if_fail(PKA_DBUS_IS_SUBSCRIPTION(sub), FALSE);

	pka_subscription_enable(sub->priv->sub);
	return TRUE;
}

gboolean
pka_dbus_subscription_disable (PkaDbusSubscription  *sub,
                               gboolean              drain,
                               GError              **error)
{
	pka_subscription_disable(sub->priv->sub, drain);
	return TRUE;
}

static void
pka_dbus_subscription_finalize (GObject *object)
{
	PkaDbusSubscriptionPrivate *priv;

	g_return_if_fail (PKA_DBUS_IS_SUBSCRIPTION (object));

	priv = PKA_DBUS_SUBSCRIPTION (object)->priv;

	G_OBJECT_CLASS (pka_dbus_subscription_parent_class)->finalize (object);
}

static void
pka_dbus_subscription_class_init (PkaDbusSubscriptionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_dbus_subscription_finalize;
	g_type_class_add_private (object_class, sizeof (PkaDbusSubscriptionPrivate));

	dbus_g_object_type_install_info(PKA_DBUS_TYPE_SUBSCRIPTION,
	                                &dbus_glib_pka_dbus_subscription_object_info);
}

static void
pka_dbus_subscription_init (PkaDbusSubscription *subscription)
{
	subscription->priv = G_TYPE_INSTANCE_GET_PRIVATE (subscription,
	                                                  PKA_DBUS_TYPE_SUBSCRIPTION,
	                                                  PkaDbusSubscriptionPrivate);
}
