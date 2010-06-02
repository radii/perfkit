/* pka-subscription.h
 *
 * Copyright (C) 2009 Christian Hergert
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

#include <glib.h>

#include "pka-channel.h"
#include "pka-encoder.h"
#include "pka-manifest.h"
#include "pka-sample.h"

G_BEGIN_DECLS

#define PKA_TYPE_SUBSCRIPTION (pka_subscription_get_type())

/**
 * PkaSubscription:
 *
 * #PkaSubscription is an opaque type representing a particular set of data
 * that a client wishes to receive updates about.  This structure is typically
 * created by a #PkaListener implementation to manage the work of aggregating
 * the samples and encoding them into a give format.  This allows the listener
 * to simply ship the bytes off to the client.
 */
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

#if 0
PkaSubscription* pka_subscription_new         (PkaChannel      *channel,
                                               PkaEncoderInfo  *encoder_info,
                                               gsize            buffer_max,
                                               gulong           buffer_timeout,
                                               PkaManifestFunc  manifest_func,
                                               gpointer         manifest_data,
                                               PkaSampleFunc    sample_func,
                                               gpointer         sample_data);
#endif
GType            pka_subscription_get_type     (void) G_GNUC_CONST;
PkaSubscription* pka_subscription_new          (void);
PkaSubscription* pka_subscription_ref          (PkaSubscription *subscription);
void             pka_subscription_unref        (PkaSubscription *subscription);
guint            pka_subscription_get_id       (PkaSubscription *subscription);
void             pka_subscription_mute         (PkaSubscription *subscription,
                                                gboolean         drain);
void             pka_subscription_unmute       (PkaSubscription *subscription);
PkaEncoder*      pka_subscription_get_encoder  (PkaSubscription *subscription);
void             pka_subscription_set_handlers (PkaSubscription *subscription,
                                                PkaManifestFunc  manifest_func,
                                                gpointer         manifest_data,
                                                GDestroyNotify   manifest_destroy,
                                                PkaSampleFunc    sample_func,
                                                gpointer         sample_data,
                                                GDestroyNotify   sample_destroy);

G_END_DECLS

#endif /* __PKA_SUBSCRIPTION_H__ */
