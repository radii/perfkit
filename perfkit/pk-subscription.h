/* pk-subscription.h
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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

#include "pk-connection.h"
#include "pk-source.h"
#include "pk-channel.h"

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
	GObject parent;

	/*< private >*/
	PkSubscriptionPrivate *priv;
};

struct _PkSubscriptionClass
{
	GObjectClass parent_class;
};

GType         pk_subscription_get_type           (void) G_GNUC_CONST;
gint          pk_subscription_get_id             (PkSubscription        *subscription) G_GNUC_PURE;
PkConnection* pk_subscription_get_connection     (PkSubscription        *subscription) G_GNUC_PURE;
gboolean      pk_subscription_add_channel        (PkSubscription        *subscription,
                                                  PkChannel             *channel,
                                                  gboolean               monitor,
                                                  GError               **error);
void          pk_subscription_add_channel_async  (PkSubscription        *subscription,
                                                  PkChannel             *channel,
                                                  gboolean               monitor,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_subscription_add_channel_finish (PkSubscription        *subscription,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_subscription_add_source         (PkSubscription        *subscription,
                                                  PkSource              *source,
                                                  GError               **error);
void          pk_subscription_add_source_async   (PkSubscription        *subscription,
                                                  PkSource              *source,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_subscription_add_source_finish  (PkSubscription        *subscription,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_subscription_mute               (PkSubscription        *subscription,
                                                  gboolean               drain,
                                                  GError               **error);
void          pk_subscription_mute_async         (PkSubscription        *subscription,
                                                  gboolean               drain,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_subscription_mute_finish        (PkSubscription        *subscription,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_subscription_remove_channel     (PkSubscription        *subscription,
                                                  PkChannel             *channel,
                                                  GError               **error);
void          pk_subscription_remove_channel_async (PkSubscription        *subscription,
                                                    PkChannel             *channel,
                                                    GCancellable          *cancellable,
                                                    GAsyncReadyCallback    callback,
                                                    gpointer               user_data);
gboolean      pk_subscription_remove_channel_finish (PkSubscription        *subscription,
                                                     GAsyncResult          *result,
                                                     GError               **error);
gboolean      pk_subscription_remove_source      (PkSubscription        *subscription,
                                                  PkSource              *source,
                                                  GError               **error);
void          pk_subscription_remove_source_async (PkSubscription        *subscription,
                                                   PkSource              *source,
                                                   GCancellable          *cancellable,
                                                   GAsyncReadyCallback    callback,
                                                   gpointer               user_data);
gboolean      pk_subscription_remove_source_finish (PkSubscription        *subscription,
                                                    GAsyncResult          *result,
                                                    GError               **error);
gboolean      pk_subscription_set_buffer         (PkSubscription        *subscription,
                                                  gint                   timeout,
                                                  gint                   size,
                                                  GError               **error);
void          pk_subscription_set_buffer_async   (PkSubscription        *subscription,
                                                  gint                   timeout,
                                                  gint                   size,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_subscription_set_buffer_finish  (PkSubscription        *subscription,
                                                  GAsyncResult          *result,
                                                  GError               **error);
gboolean      pk_subscription_unmute             (PkSubscription        *subscription,
                                                  GError               **error);
void          pk_subscription_unmute_async       (PkSubscription        *subscription,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);
gboolean      pk_subscription_unmute_finish      (PkSubscription        *subscription,
                                                  GAsyncResult          *result,
                                                  GError               **error);

G_END_DECLS

#endif /* __PK_SUBSCRIPTION_H__ */
