/* pka-subscription.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#if !defined (__PERFKIT_AGENT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-agent/perfkit-agent.h> can be included directly."
#endif

#ifndef __PKA_SUBSCRIPTION_H__
#define __PKA_SUBSCRIPTION_H__

#include "pka-channel.h"
#include "pka-context.h"
#include "pka-manifest.h"
#include "pka-sample.h"
#include "pka-source.h"

G_BEGIN_DECLS

#define PKA_TYPE_SUBSCRIPTION (pka_subscription_get_type())

typedef struct _PkaSubscription PkaSubscription;

/**
 * PkaManifestFunc:
 * @buf: A buffer containing the encoded manifest
 * @buflen: The length of @buf in bytes
 * @user_data: user data provided to pka_subscription_new().
 *
 * Callback to receive an encoded manifest when it is ready.
 */
typedef void (*PkaManifestFunc) (PkaSubscription *subscription,
                                 const guint8    *buf,
                                 gsize            buflen,
                                 gpointer         user_data);

/**
 * PkaSampleFunc:
 * @buf: A buffer containing the encoded sample
 * @buflen: The length of @buf in bytes
 * @user_data: user data provided to pka_subscription_new().
 *
 * Callback to receive an encoded stream of samples when they are ready.
 */
typedef void (*PkaSampleFunc) (PkaSubscription *subscription,
                               const guint8    *buf,
                               gsize            buflen,
                               gpointer         user_data);

typedef enum
{
	PKA_SUBSCRIPTION_UNMUTED,
	PKA_SUBSCRIPTION_MUTED,
} PkaSubscriptionState;

gint             pka_subscription_get_id           (PkaSubscription *subscription);
GType            pka_subscription_get_type         (void) G_GNUC_CONST;
PkaSubscription* pka_subscription_new              (void);
PkaSubscription* pka_subscription_ref              (PkaSubscription *subscription);
void             pka_subscription_unref            (PkaSubscription *subscription);
gboolean         pka_subscription_add_channel      (PkaSubscription  *subscription,
                                                    PkaContext       *context,
                                                    PkaChannel       *channel,
                                                    GError          **error);
gboolean         pka_subscription_remove_source    (PkaSubscription  *subscription,
                                                    PkaContext       *context,
                                                    PkaSource        *source,
                                                    GError          **error);
gboolean         pka_subscription_add_source       (PkaSubscription  *subscription,
                                                    PkaContext       *context,
                                                    PkaSource        *source,
                                                    GError          **error);
void             pka_subscription_set_handlers     (PkaSubscription  *subscription,
                                                    PkaContext       *context,
                                                    PkaManifestFunc   manifest_func,
                                                    gpointer          manifest_data,
                                                    GDestroyNotify    manifest_destroy,
                                                    PkaSampleFunc     sample_func,
                                                    gpointer          sample_data,
                                                    GDestroyNotify    sample_destroy,
                                                    GError          **error);
gboolean         pka_subscription_mute             (PkaSubscription  *subscription,
                                                    PkaContext       *context,
                                                    gboolean          drain,
                                                    GError          **error);
gboolean         pka_subscription_unmute           (PkaSubscription  *subscription,
                                                    PkaContext       *context,
                                                    GError          **error);
void             pka_subscription_deliver_sample   (PkaSubscription  *subscription,
                                                    PkaSource        *source,
                                                    PkaManifest      *manifest,
                                                    PkaSample        *sample);
void             pka_subscription_deliver_manifest (PkaSubscription  *subscription,
                                                    PkaSource        *source,
                                                    PkaManifest      *manifest);
void             pka_subscription_get_created_at   (PkaSubscription  *subscription,
                                                    GTimeVal         *tv);
GList*           pka_subscription_get_sources      (PkaSubscription  *subscription);
void             pka_subscription_get_buffer       (PkaSubscription  *subscription,
                                                    gint             *buffer_timeout,
                                                    gint             *buffer_size);

G_END_DECLS

#endif /* __PKA_SUBSCRIPTION_H__ */

