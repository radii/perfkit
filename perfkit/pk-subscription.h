/* pk-subscription.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PK_SUBSCRIPTION_H__
#define __PK_SUBSCRIPTION_H__

#include <gio/gio.h>

#include "pk-connection.h"

G_BEGIN_DECLS

#define PK_TYPE_SUBSCRIPTION            (pk_subscription_get_type())
#define PK_SUBSCRIPTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_SUBSCRIPTION, PkSubscription))
#define PK_SUBSCRIPTION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_SUBSCRIPTION, PkSubscription const))
#define PK_SUBSCRIPTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_SUBSCRIPTION, PkSubscriptionClass))
#define PK_IS_SUBSCRIPTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_SUBSCRIPTION))
#define PK_IS_SUBSCRIPTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_SUBSCRIPTION))
#define PK_SUBSCRIPTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_SUBSCRIPTION, PkSubscriptionClass))

typedef struct _PkSubscription        PkSubscription;
typedef struct _PkSubscriptionClass   PkSubscriptionClass;
typedef struct _PkSubscriptionPrivate PkSubscriptionPrivate;

struct _PkSubscription
{
	GInitiallyUnowned parent;

	/*< private >*/
	PkSubscriptionPrivate *priv;
};

struct _PkSubscriptionClass
{
	GInitiallyUnownedClass parent_class;
};

GType           pk_subscription_get_type            (void) G_GNUC_CONST;
PkSubscription* pk_subscription_new_for_connection  (PkConnection        *connection);
void            pk_subscription_enable_async        (PkSubscription      *subscription,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);
gboolean        pk_subscription_enable_finish       (PkSubscription       *subscription,
                                                     GAsyncResult         *result,
                                                     GError              **error);
void            pk_subscription_disable_async       (PkSubscription      *subscription,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);
gboolean        pk_subscription_disable_finish      (PkSubscription       *subscription,
                                                     GAsyncResult         *result,
                                                     GError              **error);
void            pk_subscription_set_handlers_async  (PkSubscription      *subscription,
                                                     GFunc                manifest,
                                                     GFunc                sample,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);
gboolean        pk_subscription_set_handlers_finish (PkSubscription       *subscription,
                                                     GAsyncResult         *result,
                                                     GError              **error);
void            pk_subscription_get_encoder_async   (PkSubscription      *subscription,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);
gboolean        pk_subscription_get_encoder_finish  (PkSubscription       *subscription,
                                                     GAsyncResult         *result,
                                                     gint                 *encoder,
                                                     GError              **error);

G_END_DECLS

#endif /* __PK_SUBSCRIPTION_H__ */
