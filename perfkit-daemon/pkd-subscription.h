/* pkd-subscription.h
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

#ifndef __PKD_SUBSCRIPTION_H__
#define __PKD_SUBSCRIPTION_H__

#include <glib.h>

#include "pkd-channel.h"
#include "pkd-encoder.h"
#include "pkd-encoder-info.h"
#include "pkd-manifest.h"
#include "pkd-sample.h"

G_BEGIN_DECLS

typedef struct _PkdSubscription PkdSubscription;

/**
 * PkdManifestFunc:
 * @buf: A buffer containing the encoded manifest
 * @buflen: The length of @buf in bytes
 * @user_data: user data provided to pkd_subscription_new().
 *
 * Callback to receive an encoded manifest when it is ready.
 */
typedef void (*PkdManifestFunc) (gchar *buf, gsize buflen, gpointer user_data);

/**
 * PkdSampleFunc:
 * @buf: A buffer containing the encoded sample
 * @buflen: The length of @buf in bytes
 * @user_data: user data provided to pkd_subscription_new().
 *
 * Callback to receive an encoded stream of samples when they are ready.
 */
typedef void (*PkdSampleFunc) (gchar *buf, gsize buflen, gpointer user_data);

PkdSubscription* pkd_subscription_new         (PkdChannel      *channel,
                                               PkdEncoderInfo  *encoder_info,
                                               gsize            buffer_max,
                                               glong            buffer_timeout,
                                               PkdManifestFunc  manifest_func,
                                               gpointer         manifest_data,
                                               PkdSampleFunc    sample_func,
                                               gpointer         sample_data);
PkdSubscription* pkd_subscription_ref         (PkdSubscription *subscription);
void             pkd_subscription_unref       (PkdSubscription *subscription);
void             pkd_subscription_pause       (PkdSubscription *subscription);
void             pkd_subscription_unpause     (PkdSubscription *subscription);
PkdEncoder*      pkd_subscription_get_encoder (PkdSubscription *subscription);

G_END_DECLS

#endif /* __PKD_SUBSCRIPTION_H__ */
