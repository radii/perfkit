/* pka-manager.h
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

#ifndef __PKA_MANAGER_H__
#define __PKA_MANAGER_H__

#include <glib-object.h>

#include "pka-channel.h"
#include "pka-context.h"
#include "pka-encoder.h"
#include "pka-plugin.h"
#include "pka-source.h"
#include "pka-subscription.h"

G_BEGIN_DECLS

gboolean pka_manager_add_channel         (PkaContext       *context,
                                          PkaChannel      **channel,
                                          GError          **error);
gboolean pka_manager_add_encoder         (PkaContext       *context,
                                          PkaPlugin        *plugin,
                                          PkaEncoder      **encoder,
                                          GError          **error);
gboolean pka_manager_add_source          (PkaContext       *context,
                                          PkaPlugin        *plugin,
                                          PkaSource       **source,
                                          GError          **error);
gboolean pka_manager_add_subscription    (PkaContext       *context,
                                          PkaSubscription **subscription,
                                          GError          **error);
gboolean pka_manager_find_channel        (PkaContext       *context,
                                          gint              channel_id,
                                          PkaChannel      **channel,
                                          GError          **error);
gboolean pka_manager_find_encoder        (PkaContext       *context,
                                          gint              encoder_id,
                                          PkaEncoder      **encoder,
                                          GError          **error);
gboolean pka_manager_find_plugin         (PkaContext       *context,
                                          const gchar      *plugin_id,
                                          PkaPlugin       **plugin,
                                          GError          **error);
gboolean pka_manager_find_source         (PkaContext       *context,
                                          gint              source_id,
                                          PkaSource       **source,
                                          GError          **error);
gboolean pka_manager_find_subscription   (PkaContext       *context,
                                          gint              subscription_id,
                                          PkaSubscription **subscription,
                                          GError          **error);
gboolean pka_manager_get_channels        (PkaContext       *context,
                                          GList           **channels,
                                          GError          **error);
gboolean pka_manager_get_encoders        (PkaContext       *context,
                                          GList           **encoders,
                                          GError          **error);
gboolean pka_manager_get_plugins         (PkaContext       *context,
                                          GList           **plugins,
                                          GError          **error);
gboolean pka_manager_get_sources         (PkaContext       *context,
                                          GList           **sources,
                                          GError          **error);
gboolean pka_manager_get_subscriptions   (PkaContext       *context,
                                          GList           **subscriptions,
                                          GError          **error);
gboolean pka_manager_remove_channel      (PkaContext       *context,
                                          PkaChannel       *channel,
                                          GError          **error);
gboolean pka_manager_remove_encoder      (PkaContext       *context,
                                          PkaEncoder       *encoder,
                                          GError          **error);
gboolean pka_manager_remove_source       (PkaContext       *context,
                                          PkaSource        *source,
                                          GError          **error);
gboolean pka_manager_remove_subscription (PkaContext       *context,
                                          PkaSubscription  *subscription,
                                          GError          **error);

G_END_DECLS

#endif /* __PKA_MANAGER_H__ */
