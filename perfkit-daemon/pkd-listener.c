/* pkd-listener.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pkd-listener.h"

G_DEFINE_TYPE (PkdListener, pkd_listener, G_TYPE_OBJECT)

/**
 * SECTION:pkd-listener
 * @title: PkdListener
 * @short_description: 
 *
 * 
 */

/**
 * pkd_listener_start:
 * @listener: A #PkdListener
 * @error: A location for a #GError or %NULL
 *
 * Starts a #PkdListener.
 *
 * Returns: %TRUE on success; otherwise %FALSE and @error is set.
 *
 * Side effects: Listener implementation dependent.
 */
gboolean
pkd_listener_start (PkdListener  *listener,
				    GError      **error)
{
	g_return_val_if_fail(PKD_IS_LISTENER(listener), FALSE);
	return PKD_LISTENER_GET_CLASS(listener)->start(listener, error);
}

/**
 * pkd_listener_stop:
 * @listener: A #PkdListener
 *
 * Stops the #PkdListener.
 *
 * Side effects: Listener implementation dependent.
 */
void
pkd_listener_stop (PkdListener *listener)
{
	g_return_if_fail(PKD_IS_LISTENER(listener));
	return PKD_LISTENER_GET_CLASS(listener)->stop(listener);
}

/**
 * pkd_listener_source_info_added:
 * @listener: A #PkdListener
 * @source_info: A #PkdSourceInfo
 *
 * Executes the "source-info-added" pipeline callback.
 */
void
pkd_listener_source_info_added (PkdListener   *listener,
							    PkdSourceInfo *source_info)
{
	if (PKD_LISTENER_GET_CLASS(listener)->source_info_added)
		PKD_LISTENER_GET_CLASS(listener)->
			source_info_added(listener, source_info);
}

/**
 * pkd_listener_source_added:
 * @listener: A #PkdListener
 * @source: A #PkdSource
 *
 * Executes the "source-added" pipeline callback.
 */
void
pkd_listener_source_added (PkdListener *listener,
						   PkdSource   *source)
{
	if (PKD_LISTENER_GET_CLASS(listener)->source_added)
		PKD_LISTENER_GET_CLASS(listener)->
			source_added(listener, source);
}

/**
 * pkd_listener_channel_added:
 * @listener: A #PkdListener
 * @channel: A #PkdChannel
 *
 * Executes the "channel-added" pipeline callback.
 */
void
pkd_listener_channel_added (PkdListener *listener,
						    PkdChannel  *channel)
{
	if (PKD_LISTENER_GET_CLASS(listener)->channel_added)
		PKD_LISTENER_GET_CLASS(listener)->
			channel_added(listener, channel);
}

/**
 * pkd_listener_subscription_added:
 * @listener: A #PkdListener
 * @channel: A #PkdChannel
 *
 * Executes the "subscription-added" pipeline callbac.
 */
void
pkd_listener_subscription_added (PkdListener     *listener,
								 PkdSubscription *subscription)
{
	if (PKD_LISTENER_GET_CLASS(listener)->subscription_added)
		PKD_LISTENER_GET_CLASS(listener)->
			subscription_added(listener, subscription);
}

/**
 * pkd_listener_encoder_info_added:
 * @listener: A #PkdListener
 * @encoder_info: A #PkdEncoderInfo
 *
 * Executes the "encoder-info-added" pipeline callback.
 */
void
pkd_listener_encoder_info_added (PkdListener    *listener,
                                 PkdEncoderInfo *encoder_info)
{
	if (PKD_LISTENER_GET_CLASS(listener)->encoder_info_added)
		PKD_LISTENER_GET_CLASS(listener)->
			encoder_info_added(listener, encoder_info);
}

static void
pkd_listener_finalize (GObject *object)
{
	G_OBJECT_CLASS(pkd_listener_parent_class)->finalize(object);
}

static void
pkd_listener_class_init (PkdListenerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkd_listener_finalize;
}

static void
pkd_listener_init (PkdListener *listener)
{
}
