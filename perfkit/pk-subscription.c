/* pk-subscription.c
 *
 * Copyright (C) 2010 Christian Hergert
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pk-connection.h"
#include "pk-sample.h"
#include "pk-subscription.h"

G_DEFINE_TYPE(PkSubscription, pk_subscription, G_TYPE_OBJECT)

/**
 * SECTION:pk-subscription
 * @title: PkSubscription
 * @short_description: 
 *
 * 
 */

struct _PkSubscriptionPrivate
{
	PkConnection *conn;
	PkManifest *manifest;
};

enum
{
	PROP_0,
	PROP_MANIFEST,
};

enum
{
	MANIFEST_EVENT,
	SAMPLE_EVENT,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

/**
 * pk_subscription_get_manifest:
 * @subscription: A #PkSubscription
 *
 * 
 *
 * Returns: The current #PkManifest if one has been received; otherwise %NULL.
 */
PkManifest*
pk_subscription_get_manifest (PkSubscription *subscription)
{
	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), NULL);
	return subscription->priv->manifest;
}

/**
 * pk_subscription_enable:
 * @subscription: A #PkSubscription.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to enable the subscription on the remote agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 */
gboolean
pk_subscription_enable (PkSubscription  *subscription,
                        GError         **error)
{
	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);
	return TRUE;
}

/**
 * pk_subscription_disable:
 * @subscription: A #PkSubscription
 * @error: A location for a #GError or %NULL.
 *
 * Disables a subscription found on a remote agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 */
gboolean
pk_subscription_disable (PkSubscription  *subscription,
                         GError         **error)
{
	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);
	return TRUE;
}

/**
 * pk_subscription_get_encoder:
 * @subscription: A #PkSubscription.
 *
 * Retrieves a #PkEncoder representing the remote encoder configured for the
 * subscription.
 *
 * Returns: A #PkEncoder or %NULL if no encoder is present.
 */
PkEncoder*
pk_subscription_get_encoder (PkSubscription *subscription)
{
	g_return_val_if_fail(PK_IS_SUBSCRIPTION(subscription), FALSE);
	return NULL;
}

static void
pk_subscription_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_MANIFEST:
		g_value_set_object(value,
		                   pk_subscription_get_manifest((gpointer)object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pk_subscription_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_subscription_parent_class)->finalize(object);
}

static void
pk_subscription_dispose (GObject *object)
{
	G_OBJECT_CLASS(pk_subscription_parent_class)->dispose(object);
}

static void
pk_subscription_class_init (PkSubscriptionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_subscription_finalize;
	object_class->dispose = pk_subscription_dispose;
	object_class->get_property = pk_subscription_get_property;
	g_type_class_add_private(object_class, sizeof(PkSubscriptionPrivate));

	/**
	 * PkSubscription:manifest:
	 *
	 * The "manifest" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_MANIFEST,
	                                g_param_spec_object("manifest",
	                                                    "manifest",
	                                                    "Current manifest",
	                                                    PK_TYPE_MANIFEST,
	                                                    G_PARAM_READABLE));

	/**
	 * PkSubscription::manifest-event:
	 * @manifest: A #PkManifest.
	 *
	 * The "manifest-event" signal.
	 */
	signals[MANIFEST_EVENT] = g_signal_new("manifest-event",
	                                       PK_TYPE_SUBSCRIPTION,
	                                       G_SIGNAL_RUN_FIRST,
	                                       0,
	                                       NULL,
	                                       NULL,
	                                       g_cclosure_marshal_VOID__BOXED,
	                                       G_TYPE_NONE,
	                                       1,
	                                       PK_TYPE_MANIFEST);

	/**
	 * PkSubscription::sample-event:
	 * @sample: A #PkSample.
	 *
	 * The "sample-event" signal.
	 */
	signals[SAMPLE_EVENT] = g_signal_new("sample-event",
	                                     PK_TYPE_SUBSCRIPTION,
	                                     G_SIGNAL_RUN_FIRST,
	                                     0,
	                                     NULL,
	                                     NULL,
	                                     g_cclosure_marshal_VOID__BOXED,
	                                     G_TYPE_NONE,
	                                     1,
	                                     PK_TYPE_SAMPLE);
}

static void
pk_subscription_init (PkSubscription *subscription)
{
	subscription->priv = G_TYPE_INSTANCE_GET_PRIVATE(subscription,
	                                                 PK_TYPE_SUBSCRIPTION,
	                                                 PkSubscriptionPrivate);
}
