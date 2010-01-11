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
#include "pk-subscription.h"

G_DEFINE_TYPE (PkSubscription, pk_subscription, G_TYPE_OBJECT)

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
};

static void
pk_subscription_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_subscription_parent_class)->finalize(object);
}

static void
pk_subscription_class_init (PkSubscriptionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_subscription_finalize;
	g_type_class_add_private(object_class, sizeof(PkSubscriptionPrivate));
}

static void
pk_subscription_init (PkSubscription *subscription)
{
	subscription->priv = G_TYPE_INSTANCE_GET_PRIVATE(subscription,
	                                                 PK_TYPE_SUBSCRIPTION,
	                                                 PkSubscriptionPrivate);
}
