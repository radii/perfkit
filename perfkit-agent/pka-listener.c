/* pka-listener.c
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

#include "pka-listener.h"

/**
 * SECTION:pka-listener
 * @title: PkaListener
 * @short_description: RPCs for client access.
 *
 * The #PkaListener class provides a base object for RPC systems to inherit
 * from.  It receives callbacks from the pipeline when import events occur.
 *
 * Listeners are implemented in their own shared module found in
 * ${libdir}/perfkit-agent/listeners.  Listener plugins should export the
 * pka_listener_register symbol which is called after the module is loaded.
 * They are responsible for creating and adding their #PkaListener instance
 * to the pipeline with pka_pipeline_add_listener().  It's good practice
 * to allow the listener to be disabled via the config subsystem.
 */

G_DEFINE_TYPE (PkaListener, pka_listener, G_TYPE_OBJECT)

/**
 * pka_listener_start:
 * @listener: A #PkaListener
 * @error: A location for a #GError or %NULL
 *
 * Starts a #PkaListener.
 *
 * Returns: %TRUE on success; otherwise %FALSE and @error is set.
 *
 * Side effects:
 *   Listener implementation dependent.
 */
gboolean
pka_listener_start (PkaListener  *listener,
				    GError      **error)
{
	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	return PKA_LISTENER_GET_CLASS(listener)->start(listener, error);
}

/**
 * pka_listener_stop:
 * @listener: A #PkaListener
 *
 * Stops the #PkaListener.
 *
 * Side effects:
 *   Listener implementation dependent.
 */
void
pka_listener_stop (PkaListener *listener)
{
	g_return_if_fail(PKA_IS_LISTENER(listener));
	return PKA_LISTENER_GET_CLASS(listener)->stop(listener);
}

/**
 * pka_listener_source_info_added:
 * @listener: A #PkaListener
 * @source_info: A #PkaSourceInfo
 *
 * Executes the "source-info-added" pipeline callback.
 */
void
pka_listener_source_info_added (PkaListener   *listener,
							    PkaSourceInfo *source_info)
{
	if (PKA_LISTENER_GET_CLASS(listener)->source_info_added)
		PKA_LISTENER_GET_CLASS(listener)->
			source_info_added(listener, source_info);
}

/**
 * pka_listener_source_added:
 * @listener: A #PkaListener
 * @source: A #PkaSource
 *
 * Executes the "source-added" pipeline callback.
 */
void
pka_listener_source_added (PkaListener *listener,
						   PkaSource   *source)
{
	if (PKA_LISTENER_GET_CLASS(listener)->source_added)
		PKA_LISTENER_GET_CLASS(listener)->source_added(listener, source);
}

/**
 * pka_listener_channel_added:
 * @listener: A #PkaListener
 * @channel: A #PkaChannel
 *
 * Executes the "channel-added" pipeline callback.
 */
void
pka_listener_channel_added (PkaListener *listener,
						    PkaChannel  *channel)
{
	if (PKA_LISTENER_GET_CLASS(listener)->channel_added)
		PKA_LISTENER_GET_CLASS(listener)->channel_added(listener, channel);
}

/**
 * pka_listener_encoder_added:
 * @listener: A #PkaListener
 * @encoder: A #PkaEncoder
 *
 * Executes the "encoder-added" pipeline callback.
 */
void
pka_listener_encoder_added (PkaListener *listener,
                            PkaEncoder  *encoder)
{
	if (PKA_LISTENER_GET_CLASS(listener)->encoder_added)
		PKA_LISTENER_GET_CLASS(listener)->encoder_added(listener, encoder);
}

/**
 * pka_listener_subscription_added:
 * @listener: A #PkaListener
 * @channel: A #PkaChannel
 *
 * Executes the "subscription-added" pipeline callbac.
 */
void
pka_listener_subscription_added (PkaListener     *listener,
								 PkaSubscription *subscription)
{
	if (PKA_LISTENER_GET_CLASS(listener)->subscription_added)
		PKA_LISTENER_GET_CLASS(listener)->
			subscription_added(listener, subscription);
}

/**
 * pka_listener_encoder_info_added:
 * @listener: A #PkaListener
 * @encoder_info: A #PkaEncoderInfo
 *
 * Executes the "encoder-info-added" pipeline callback.
 */
void
pka_listener_encoder_info_added (PkaListener    *listener,
                                 PkaEncoderInfo *encoder_info)
{
	if (PKA_LISTENER_GET_CLASS(listener)->encoder_info_added)
		PKA_LISTENER_GET_CLASS(listener)->
			encoder_info_added(listener, encoder_info);
}

static void
pka_listener_finalize (GObject *object)
{
	G_OBJECT_CLASS(pka_listener_parent_class)->finalize(object);
}

static void
pka_listener_class_init (PkaListenerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pka_listener_finalize;
}

static void
pka_listener_init (PkaListener *listener)
{
}
