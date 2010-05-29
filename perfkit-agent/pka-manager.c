/* pka-manager.c
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

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Manager"

#include "pka-context.h"
#include "pka-listener.h"
#include "pka-log.h"
#include "pka-manager.h"
#include "pka-plugin.h"
#include "pka-source.h"
#include "pka-source-simple.h"

#define AUTHORIZE_IOCTL(_c, _i)                                     \
    G_STMT_START {                                                  \
        if (!pka_context_is_authorized(_c, PKA_IOCTL_##_i)) {       \
            g_set_error(error, PKA_CONTEXT_ERROR,                   \
                        PKA_CONTEXT_ERROR_NOT_AUTHORIZED,           \
                        "The context is not authorized for the "    \
                        "requested command.");                      \
            RETURN(FALSE);                                          \
        }                                                           \
    } G_STMT_END

#define NOTIFY_LISTENERS(_c, _o)                                    \
    G_STMT_START {                                                  \
    	gint _i = 0;                                                \
        G_LOCK(listeners);                                          \
        for (_i = 0; _i < manager.listeners->len; _i++) {           \
            pka_listener_##_c(                                      \
                g_ptr_array_index(manager.listeners, _i),           \
                (_o));                                              \
        }                                                           \
        G_UNLOCK(listeners);                                        \
    } G_STMT_END

typedef struct
{
	GPtrArray *channels;
	GPtrArray *encoders;
	GPtrArray *listeners;
	GPtrArray *plugins;
	GPtrArray *sources;
	GPtrArray *subscriptions;
	GMainLoop *mainloop;
} PkaManager;

static PkaManager manager = { 0 };

G_LOCK_DEFINE(channels);
G_LOCK_DEFINE(encoders);
G_LOCK_DEFINE(plugins);
G_LOCK_DEFINE(sources);
G_LOCK_DEFINE(subscriptions);
G_LOCK_DEFINE(listeners);

/**
 * pka_manager_load_plugins:
 * @plugins_dir: The directory containing plugins.
 *
 * Loads plugins discovered in @plugins_dir.
 *
 * Returns: None.
 * Side effects: Plugins are loaded into process.
 */
static void
pka_manager_load_plugins (const gchar *plugins_dir) /* IN */
{
	GError *error = NULL;
	PkaPlugin *plugin;
	const gchar *name;
	gchar *path;
	GDir *dir;

	ENTRY;
	INFO(Plugin, "Loading plugins from directory: %s", plugins_dir);
	if (!(dir = g_dir_open(plugins_dir, 0, &error))) {
		WARNING(Manager, "%s", error->message);
		g_error_free(error);
		GOTO(cleanup);
	}
	while ((name = g_dir_read_name(dir))) {
		if (!g_str_has_suffix(name, G_MODULE_SUFFIX)) {
			DEBUG(Plugin, "Skipping non-plugin: %s", name);
			continue;
		}
		INFO(Manager, "Discovered plugin: %s", name);
		path = g_build_filename(plugins_dir, name, NULL);
		plugin = pka_plugin_new();
		if (!pka_plugin_load_from_file(plugin, path, &error)) {
			WARNING(Plugin, "%s", error->message);
			g_object_unref(plugin);
			g_error_free(error);
			error = NULL;
			GOTO(next);
		}
		G_LOCK(plugins);
		g_ptr_array_add(manager.plugins, plugin);
		G_UNLOCK(plugins);
	  next:
		g_free(path);
	}
  cleanup:
  	if (dir) {
  		g_dir_close(dir);
	}
	EXIT;
}

/**
 * pka_manager_load_all_plugins:
 *
 * Discovers plugin directories and loads plugins.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_manager_load_all_plugins (void)
{
	gchar **paths;
	gint i;

	ENTRY;
	if (g_getenv("PERFKIT_PLUGINS_PATH")) {
		paths = g_strsplit(g_getenv("PERFKIT_PLUGINS_PATH"), ":", 0);
	} else {
		paths = g_new0(gchar*, 2);
		paths[0] = g_build_filename(PACKAGE_LIB_DIR,
		                            "perfkit", "plugins",
		                            NULL);
	}
	for (i = 0; paths[i]; i++) {
		pka_manager_load_plugins(paths[i]);
	}
	g_strfreev(paths);
	EXIT;
}

/**
 * pka_manager_init_listeners:
 *
 * Initializes configured listeners.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_manager_init_listeners (void)
{
	PkaListener *listener;
	PkaPlugin *plugin;
	GError *error = NULL;
	gint i;
	gint j;

	ENTRY;
	G_LOCK(plugins);
	for (i = 0; i < manager.plugins->len; i++) {
		plugin = g_ptr_array_index(manager.plugins, i);
		if (pka_plugin_get_plugin_type(plugin) == PKA_PLUGIN_LISTENER) {
			if (pka_plugin_is_disabled(plugin)) {
				INFO(Listener, "Listener plugin is disabled: %s",
				     pka_plugin_get_id(plugin));
				continue;
			}
			if (!(listener = PKA_LISTENER(pka_plugin_create(plugin, &error)))) {
				WARNING(Listener, "Error creating listener: %s: %s",
				        pka_plugin_get_id(plugin),
				        error ? error->message : "unknown error");
				g_clear_error(&error);
				continue;
			}
			G_LOCK(listeners);
			g_ptr_array_add(manager.listeners, listener);
			G_UNLOCK(listeners);
		}
	}
	G_LOCK(listeners);
	for (i = 0; i < manager.listeners->len; i++) {
		listener = g_ptr_array_index(manager.listeners, i);
		if (!pka_listener_listen(listener, &error)) {
			WARNING(Listener, "Error starting listener: %s",
			        error ? error->message : "unknown error");
			g_clear_error(&error);
			continue;
		}
		for (j = 0; j < manager.plugins->len; j++) {
			plugin = g_ptr_array_index(manager.plugins, j);
			if (pka_plugin_get_plugin_type(plugin) != PKA_PLUGIN_LISTENER) {
				pka_listener_plugin_added(listener, pka_plugin_get_id(plugin));
			}
		}
	}
	G_UNLOCK(listeners);
	G_UNLOCK(plugins);
	EXIT;
}

/**
 * pka_manager_init:
 *
 * Initializes the manager subsystem.
 *
 * Returns: None.
 * Side effects: Everything.
 */
void
pka_manager_init (void)
{
	ENTRY;
	manager.channels = g_ptr_array_new();
	manager.encoders = g_ptr_array_new();
	manager.listeners = g_ptr_array_new();
	manager.plugins = g_ptr_array_new();
	manager.sources = g_ptr_array_new();
	manager.subscriptions = g_ptr_array_new();
	manager.mainloop = g_main_loop_new(NULL, FALSE);
	pka_manager_load_all_plugins();
	pka_manager_init_listeners();
	/*
	 * Work around linker issues by using the source types.
	 */
	DEBUG(Source, "Registering %s source.",
	      g_type_name(PKA_TYPE_SOURCE_SIMPLE));
	EXIT;
}

/**
 * pka_manager_run:
 *
 * Beings the application main loop.
 *
 * Returns: None.
 * Side effects: Everything.
 */
void
pka_manager_run (void)
{
	g_assert(manager.mainloop);
	g_main_loop_run(manager.mainloop);
}

/**
 * pka_manager_quit:
 *
 * Quits the application main loop.
 *
 * Returns: None.
 * Side effects: Mainloop is stopped.
 */
void
pka_manager_quit (void)
{
	g_assert(manager.mainloop);
	g_main_loop_quit(manager.mainloop);
}

/**
 * pka_manager_shutdown:
 *
 * Shuts down the manager subsystem and the channels, sources, and listeners
 * registered with it.
 *
 * Returns: None.
 * Side effects: Process armageddon.
 */
void
pka_manager_shutdown (void)
{
	PkaSubscription *subscription;
	PkaChannel *channel;
	gint i;

	G_LOCK(subscriptions);
	for (i = manager.subscriptions->len - 1; i >= 0; --i) {
		subscription = g_ptr_array_index(manager.subscriptions, i);
		pka_subscription_disable(subscription, FALSE);
		pka_subscription_unref(subscription);
		g_ptr_array_remove_index(manager.subscriptions, i);
	}
	G_UNLOCK(subscriptions);

	G_LOCK(channels);
	for (i = manager.channels->len - 1; i >= 0; --i) {
		channel = g_ptr_array_index(manager.channels, i);
		pka_channel_stop(channel, FALSE, NULL);
		g_object_unref(channel);
		g_ptr_array_remove_index(manager.channels, i);
	}
	G_UNLOCK(channels);
}

/**
 * pka_manager_add_channel:
 * @context: A #PkaContext.
 * @channel: A location for a #PkaChannel.
 * @error: A location for #GError or %NULL.
 *
 * Adds a channel to the Perfkit Agent.  If the context does not have
 * sufficient permissions this operation will fail.
 *
 * If successful, the newly created #PkaChannel is stored in @channel.
 *
 * The channel has a reference owned by the caller and should be
 * freed using g_object_unref().
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_manager_add_channel (PkaContext  *context, /* IN */
                         PkaChannel **channel, /* OUT */
                         GError     **error)   /* OUT */
{
	gint channel_id;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(channel != NULL, FALSE);

	ENTRY;
	AUTHORIZE_IOCTL(context, ADD_CHANNEL);
	*channel = g_object_new(PKA_TYPE_CHANNEL, NULL);
	channel_id = pka_channel_get_id(*channel);
	INFO(Channel, "Added channel %d on behalf of context %d.",
	     channel_id, pka_context_get_id(context));
	G_LOCK(channels);
	g_ptr_array_add(manager.channels, g_object_ref(*channel));
	G_UNLOCK(channels);
	NOTIFY_LISTENERS(channel_added, channel_id);
	RETURN(TRUE);
}

/**
 * pka_manager_add_encoder:
 * @context: A #PkaContext.
 * @plugin: A #PkaPlugin.
 * @encoder: A location for a #PkaEncoder.
 * @error: A location for a #GError or %NULL.
 *
 * Creates a new encoder in the Perfkit Agent.  If the context does
 * not have permissions to create an encoder, this operation will fail.
 *
 * If successful, the newly created instance is stored in @encoder.
 *
 * The caller owns a reference to the encoder and should free it with
 * g_object_unref().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_add_encoder (PkaContext  *context, /* IN */
                         PkaPlugin   *plugin,  /* IN */
                         PkaEncoder **encoder, /* OUT */
                         GError     **error)   /* OUT */
{
	gint encoder_id;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(encoder != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	AUTHORIZE_IOCTL(context, ADD_ENCODER);
	if (pka_plugin_get_plugin_type(plugin) != PKA_PLUGIN_ENCODER) {
		g_set_error(error, PKA_PLUGIN_ERROR, PKA_PLUGIN_ERROR_INVALID_TYPE,
		            "The specified plugin is not an encoder plugin.");
		RETURN(FALSE);
	}
	if (!(*encoder = PKA_ENCODER(pka_plugin_create(plugin, error)))) {
		RETURN(FALSE);
	}
	encoder_id = pka_encoder_get_id(*encoder);
	INFO(Encoder, "Added encoder %d on behalf of context %d.",
	     encoder_id, pka_context_get_id(context));
	G_LOCK(encoders);
	g_ptr_array_add(manager.encoders, g_object_ref(*encoder));
	G_UNLOCK(encoders);
	NOTIFY_LISTENERS(encoder_added, encoder_id);
	RETURN(TRUE);
}

/**
 * pka_manager_add_source:
 * @context: A #PkaContext.
 * @plugin: A #PkaPlugin.
 * @source: A location for a #PkaSource.
 * @error: A location for a #GError or %NULL.
 *
 * Creates a new source in the Perfkit Agent.  If the context does
 * not have permissions to create the source, this operation will fail.
 *
 * If successful, the newly created instance is stored in @source.
 *
 * The caller owns a reference to the source and should free it with
 * g_object_unref().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_add_source (PkaContext  *context, /* IN */
                        PkaPlugin   *plugin,  /* IN */
                        PkaSource  **source,  /* OUT */
                        GError     **error)   /* OUT */
{
	gint source_id;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(source != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_PLUGIN(plugin), FALSE);

	ENTRY;
	AUTHORIZE_IOCTL(context, ADD_SOURCE);
	if (pka_plugin_get_plugin_type(plugin) != PKA_PLUGIN_SOURCE) {
		g_set_error(error, PKA_PLUGIN_ERROR, PKA_PLUGIN_ERROR_INVALID_TYPE,
		            "The specified plugin is not a source plugin.");
		RETURN(FALSE);
	}
	if (!(*source = PKA_SOURCE(pka_plugin_create(plugin, error)))) {
		RETURN(FALSE);
	}
	source_id = pka_source_get_id(*source);
	INFO(Source, "Added source %d on behalf of context %d.",
	     source_id, pka_context_get_id(context));
	G_LOCK(sources);
	g_ptr_array_add(manager.sources, g_object_ref(*source));
	G_UNLOCK(sources);
	NOTIFY_LISTENERS(source_added, source_id);
	RETURN(TRUE);
}

/**
 * pka_manager_add_subscription:
 * @context: A #PkaContext.
 * @subscription: A location for a #PkaSubscription.
 * @error: A location for a #GError, or %NULL.
 *
 * Adds a new subscription to the Perfkit Agent.  If the context does
 * not have permissions to create a subscription, this operation will fail.
 *
 * If successful, the newly created subscription instance is stored in
 * @subscription.  The caller owns a reference to the subscription and
 * should free it with pka_subscription_unref().
 *
 * Returns: %TRUE is successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_add_subscription (PkaContext       *context,      /* IN */
                              PkaSubscription **subscription, /* OUT */
                              GError          **error)        /* OUT */
{
	gint subscription_id;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(subscription != NULL, FALSE);

	ENTRY;
	AUTHORIZE_IOCTL(context, ADD_SUBSCRIPTION);
	*subscription = NULL;
	subscription_id = 0;
	g_warn_if_reached();
	INFO(Subscription, "Added subscription %d on behalf of context %d.",
	     subscription_id, pka_context_get_id(context));
	G_LOCK(subscriptions);
	g_ptr_array_add(manager.subscriptions,
	                pka_subscription_ref(*subscription));
	G_UNLOCK(subscriptions);
	NOTIFY_LISTENERS(subscription_added, subscription_id);
	RETURN(TRUE);
}

/**
 * pka_manager_find_channel:
 * @context: A #PkaContext.
 * @channel_id: The channel identifier.
 * @channel: A location for a #PkaChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Finds the channel matching the requested @channel_id.  If the context
 * does not have permissions, this operation will fail.
 *
 * If successful, the channel instance is stored in @channel.  The caller
 * owns a reference to this channel and should be freed with g_object_unref().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_find_channel (PkaContext  *context,    /* IN */
                          gint         channel_id, /* IN */
                          PkaChannel **channel,    /* OUT */
                          GError     **error)      /* OUT */
{
	PkaChannel *iter;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(channel != NULL, FALSE);

	ENTRY;
	*channel = NULL;
	G_LOCK(channels);
	for (i = 0; i < manager.channels->len; i++) {
		iter = g_ptr_array_index(manager.channels, i);
		if (pka_channel_get_id(iter) == channel_id) {
			/*
			 * TODO: Verify permissions.
			 */
			*channel = g_object_ref(iter);
			BREAK;
		}
	}
	G_UNLOCK(channels);
	if (!*channel) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Not authorized or invalid channel.");
	}
	RETURN(*channel != NULL);
}

/**
 * pka_manager_find_encoder:
 * @context: A #PkaContext.
 * @encoder_id: The encoder identifier.
 * @encoder: A location for a #PkaEncoder.
 * @error: A location for a #GError, or %NULL.
 *
 * Finds the encoder matching the requested @encoder_id.  If the context
 * does not have permissions, this operation will fail.
 *
 * If successful, the encoder instance is stored in @encoder.  The caller
 * owns a reference to this encoder and should free it with g_object_unref().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_find_encoder (PkaContext  *context,    /* IN */
                          gint         encoder_id, /* IN */
                          PkaEncoder **encoder,    /* OUT */
                          GError     **error)      /* OUT */
{
	PkaEncoder *iter;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(encoder != NULL, FALSE);

	ENTRY;
	*encoder = NULL;
	G_LOCK(encoders);
	for (i = 0; i < manager.encoders->len; i++) {
		iter = g_ptr_array_index(manager.encoders, i);
		if (pka_encoder_get_id(iter) == encoder_id) {
			/*
			 * TODO: Verify permissions.
			 */
			*encoder = iter;
			BREAK;
		}
	}
	G_UNLOCK(encoders);
	if (!*encoder) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Not authorized or invalid encoder.");
	}
	RETURN(*encoder != NULL);
}

/**
 * pka_manager_find_plugin:
 * @context: A #PkaContext.
 * @plugin_id: The plugin identifier.
 * @plugin: A location for a #PkaPlugin.
 * @error: A location for a #GError, or %NULL.
 *
 * Finds the plugin matching the requested @plugin_id.  If the context
 * does not have permissions, this operation will fail.
 *
 * If successful, the plugin instance is stored in @plugin.  The caller
 * owns a reference to this plugin and should free it with g_object_unref().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_find_plugin (PkaContext       *context,   /* IN */
                         const gchar      *plugin_id, /* IN */
                         PkaPlugin       **plugin,    /* OUT */
                         GError          **error)     /* OUT */
{
	PkaPlugin *iter;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(plugin != NULL, FALSE);

	ENTRY;
	*plugin = NULL;
	if (!plugin_id) {
		GOTO(failed);
	}
	G_LOCK(plugins);
	for (i = 0; i < manager.plugins->len; i++) {
		iter = g_ptr_array_index(manager.plugins, i);
		if (g_str_equal(pka_plugin_get_id(iter), plugin_id)) {
			/*
			 * TODO: Verify permissions.
			 */
			*plugin = g_object_ref(iter);
			BREAK;
		}
	}
	G_UNLOCK(plugins);
  failed:
  	if (!*plugin) {
  		g_set_error(error, PKA_PLUGIN_ERROR, PKA_PLUGIN_ERROR_INVALID_TYPE,
  		            "The specified plugin could not be found.");
	}
	RETURN(*plugin != NULL);
}

/**
 * pka_manager_find_source:
 * @context: A #PkaContext.
 * @source_id: The source identifier.
 * @source: A location for a #PkaSource.
 * @error: A location for a #GError, or %NULL.
 *
 * Finds the source matching the requested @source_id.  If the context
 * does not have permissions, this operation will fail.
 *
 * If successful, the source instance is stored in @source.  The caller
 * owns a reference to this source and should free it with g_object_unref().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_find_source (PkaContext  *context,   /* IN */
                         gint         source_id, /* IN */
                         PkaSource  **source,    /* OUT */
                         GError     **error)     /* OUT */
{
	PkaSource *iter;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(source != NULL, FALSE);

	ENTRY;
	*source = NULL;
	G_LOCK(sources);
	for (i = 0; i < manager.sources->len; i++) {
		iter = g_ptr_array_index(manager.sources, i);
		if (pka_source_get_id(iter) == source_id) {
			/*
			 * TODO: Verify permissions.
			 */
			*source = g_object_ref(iter);
			BREAK;
		}
	}
	G_UNLOCK(sources);
	if (!*source) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Not authorized or invalid source.");
	}
	RETURN(*source != NULL);
}

/**
 * pka_manager_find_subscription:
 * @context: A #PkaContext.
 * @subscription_id: The subscription identifier.
 * @subscription: A location for a #PkaSubscription.
 * @error: A location for a #GError, or %NULL.
 *
 * Finds the subscription matching the requested @subscription_id.  If the
 * context does not have permissions, this operation will fail.
 *
 * If successful, the subscription instance is stored in @subscription.  The
 * caller owns a reference to the subscription and should free it with
 * pka_subscription_unref().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_find_subscription (PkaContext       *context,         /* IN */
                               gint              subscription_id, /* IN */
                               PkaSubscription **subscription,    /* OUT */
                               GError          **error)           /* OUT */
{
	PkaSubscription *iter;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(subscription != NULL, FALSE);

	ENTRY;
	*subscription = NULL;
	G_LOCK(subscriptions);
	for (i = 0; i < manager.subscriptions->len; i++) {
		iter = g_ptr_array_index(manager.subscriptions, i);
		if (pka_subscription_get_id(iter) == subscription_id) {
			/*
			 * TODO: Verify permissions.
			 */
			*subscription = g_object_ref(iter);
			BREAK;
		}
	}
	G_UNLOCK(subscriptions);
	if (!*subscription) {
		g_set_error(error, PKA_CONTEXT_ERROR,
		            PKA_CONTEXT_ERROR_NOT_AUTHORIZED,
		            "Not authorized or invalid subscription.");
	}
	RETURN(*subscription != NULL);
}

/**
 * pka_manager_get_channels:
 * @context: A #PkaContext.
 * @channels: A location for the #GList containing #PkaChannel<!-- -->'s.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the channels available to the context.  If the context
 * does not have permissions, this operation will fail.
 *
 * If successful, the channels will be stored in @channels.  The caller
 * owns the reference to the resulting #GList and instances within the list.
 * The instances within the list should all be freed with g_object_unref()
 * before calling g_list_free() to release the list.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_get_channels (PkaContext  *context,  /* IN */
                          GList      **channels, /* OUT */
                          GError     **error)    /* OUT */
{
	PkaChannel *channel;
	GList *iter = NULL;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(channels != NULL, FALSE);

	ENTRY;
	G_LOCK(channels);
	for (i = 0; i < manager.channels->len; i++) {
		channel = g_ptr_array_index(manager.channels, i);
		/*
		 * TODO: Verify permissions.
		 */
		iter = g_list_prepend(iter, g_object_ref(channel));
	}
	G_UNLOCK(channels);
	*channels = iter;
	RETURN(TRUE);
}

/**
 * pka_manager_get_encoders:
 * @context: A #PkaContext.
 * @encoders: A location for a #GList containing #PkaEncoder.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the list of encoders in the Perfkit Agent.  If the contxt does
 * not have permissions, this operation will fail.
 *
 * The caller owns a reference to the list and the instances of #PkaEncoder
 * within it.  The caller should free the instances with g_object_unref()
 * before freeing the list with g_list_free().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_get_encoders (PkaContext  *context,  /* IN */
                          GList      **encoders, /* OUT */
                          GError     **error)    /* OUT */
{
	PkaEncoder *encoder;
	GList *iter = NULL;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(encoders != NULL, FALSE);

	ENTRY;
	G_LOCK(encoders);
	for (i = 0; i < manager.encoders->len; i++) {
		encoder = g_ptr_array_index(manager.encoders, i);
		/*
		 * TODO: Verify permissions.
		 */
		iter = g_list_prepend(iter, g_object_ref(encoder));
	}
	G_UNLOCK(encoders);
	*encoders = iter;
	RETURN(TRUE);
}

/**
 * pka_manager_get_plugins:
 * @context: A #PkaContext.
 * @plugins: A location for a #GList containing #PkaPlugin.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the list of plugins in the Perfkit Agent.  If the context does
 * not have permissions, this operation will fail.
 *
 * The caller owns a reference to the list and the instances within it.  The
 * caller should free the instances with g_object_unref() before freeing the
 * list with g_list_free().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_get_plugins (PkaContext  *context, /* IN */
                         GList      **plugins, /* OUT */
                         GError     **error)   /* OUT */
{
	PkaPlugin *plugin;
	GList *iter = NULL;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(plugins != NULL, FALSE);

	ENTRY;
	G_LOCK(plugins);
	for (i = 0; i < manager.plugins->len; i++) {
		plugin = g_ptr_array_index(manager.plugins, i);
		/*
		 * TODO: Verify permissions.
		 */
		iter = g_list_prepend(iter, g_object_ref(plugin));
	}
	G_UNLOCK(plugins);
	*plugins = iter;
	RETURN(TRUE);
}

/**
 * pka_manager_get_sources:
 * @context: A #PkaContext.
 * @sources: A location for a #GList containing #PkaSource.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the list of sources in the Perfkit Agent.  If the context does
 * not have permissions, this operation will fail.
 *
 * The caller owns a reference to the list and the #PkaSource instances within
 * it.  The caller should free the instances with g_object_unref() before
 * freeing the list with g_list_free().
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_get_sources (PkaContext  *context, /* IN */
                         GList      **sources, /* OUT */
                         GError     **error)   /* OUT */
{
	PkaSource *source;
	GList *iter = NULL;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(sources != NULL, FALSE);

	ENTRY;
	G_LOCK(sources);
	for (i = 0; i < manager.sources->len; i++) {
		source = g_ptr_array_index(manager.sources, i);
		/*
		 * TODO: Verify permissions.
		 */
		iter = g_list_prepend(iter, g_object_ref(source));
	}
	G_UNLOCK(sources);
	*sources = iter;
	RETURN(TRUE);
}

/**
 * pka_manager_get_subscriptions:
 * @context: A #PkaContext.
 * @subscriptions: A location for a #GList containing #PkaSubscription.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the available subscriptions in the Perfkit Agent.  If the
 * context does not have permissions, this operation will %FALSE.
 *
 * The caller owns a reference to the #GList and the instances of
 * #PkaSubscription within the list.  The caller should free the subscriptions
 * with pka_subscription_unref() before calling g_list_free() to free the
 * list.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_get_subscriptions (PkaContext  *context,       /* IN */
                               GList      **subscriptions, /* OUT */
                               GError     **error)         /* OUT */
{
	PkaSubscription *subscription;
	GList *iter = NULL;
	gint i;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(subscriptions != NULL, FALSE);

	ENTRY;
	G_LOCK(subscriptions);
	for (i = 0; i < manager.subscriptions->len; i++) {
		subscription = g_ptr_array_index(manager.subscriptions, i);
		/*
		 * TODO: Verify permissions.
		 */
		iter = g_list_prepend(iter, pka_subscription_ref(subscription));
	}
	G_UNLOCK(subscriptions);
	*subscriptions = iter;
	RETURN(TRUE);
}

/**
 * pka_manager_remove_channel:
 * @context: A #PkaContext.
 * @channel: A #PkaChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Removes @channel from the Perfkit Agent.  If the context does not have
 * permissions, the operation will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_remove_channel (PkaContext  *context, /* IN */
                            PkaChannel  *channel, /* IN */
                            GError     **error)   /* OUT */
{
	gint channel_id;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	ENTRY;
	AUTHORIZE_IOCTL(context, REMOVE_CHANNEL);
	/*
	 * TODO:
	 *
	 *   1) Ensure the channel is completely stopped.
	 *   2) Ensure the channels sources are stopped.
	 *   3) Notify the subscriptions the channel is gone.
	 *   4) Notify the listeners the subscription is gone.
	 *
	 */
	if (FALSE) {
		channel_id = pka_channel_get_id(channel);
		INFO(Channel, "Removing channel %d on behalf of context %d.",
		     channel_id, pka_context_get_id(context));
		G_LOCK(channels);
		g_ptr_array_remove(manager.channels, channel);
		G_UNLOCK(channels);
		NOTIFY_LISTENERS(channel_removed, channel_id);
		g_object_unref(channel);
	}
	RETURN(TRUE);
}

/**
 * pka_manager_remove_encoder:
 * @context: A #PkaContext.
 * @encoder: A #PkaEncoder.
 * @error: A location for a #GError, or %NULL.
 *
 * Removes @encoder from the Perfkit Agent.  If the context does not have
 * permissions, this operation will fail.
 *
 * Returns: %TRUE if succesful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_remove_encoder (PkaContext  *context, /* IN */
                            PkaEncoder  *encoder, /* IN */
                            GError     **error)   /* OUT */
{
	gint encoder_id;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_ENCODER(encoder), FALSE);

	ENTRY;
	AUTHORIZE_IOCTL(context, REMOVE_ENCODER);
	/*
	 * TODO:
	 *
	 *   1) Enure the encoder is not attached to a running channel.
	 *
	 */
	if (FALSE) {
		encoder_id = pka_encoder_get_id(encoder);
		INFO(Encoder, "Removing encoder %d on behalf of context %d.",
		     encoder_id, pka_context_get_id(context));
		G_LOCK(encoders);
		g_ptr_array_remove(manager.encoders, encoder);
		G_UNLOCK(encoders);
		NOTIFY_LISTENERS(encoder_removed, encoder_id);
		g_object_unref(encoder);
	}
	RETURN(TRUE);
}

/**
 * pka_manager_remove_source:
 * @context: A #PkaContext.
 * @source: A #PkaSource.
 * @error: A location for a #GError, or %NULL.
 *
 * Removes @source from the Perfkit Agent.  If the context does not have
 * permissions, this operation will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_remove_source (PkaContext  *context, /* IN */
                           PkaSource   *source,  /* IN */
                           GError     **error)   /* OUT */
{
	gint source_id;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);

	ENTRY;
	AUTHORIZE_IOCTL(context, REMOVE_SOURCE);
	/*
	 * TODO:
	 *
	 *   1) Ensure the source is stopped.
	 *   2) Remove the source from a channel if attached.
	 *
	 */
	if (FALSE) {
		source_id = pka_source_get_id(source);
		INFO(Source, "Removing source %d on behalf of context %d.",
		     source_id, pka_context_get_id(context));
		G_LOCK(sources);
		g_ptr_array_remove(manager.sources, source);
		G_UNLOCK(sources);
		NOTIFY_LISTENERS(source_removed, source_id);
		g_object_unref(source);
	}
	RETURN(TRUE);
}

/**
 * pka_manager_remove_subscription:
 * @context: A #PkaContext.
 * @subscription: A #PkaSubscription.
 * @error: A location for a #GError, or %NULL.
 *
 * Removes a subscription from the Perfkit Agent.  If the context does not
 * have permissions, this operation will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_manager_remove_subscription (PkaContext       *context,      /* IN */
                                 PkaSubscription  *subscription, /* IN */
                                 GError          **error)        /* OUT */
{
	gint subscription_id;

	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(subscription != NULL, FALSE);

	ENTRY;
	AUTHORIZE_IOCTL(context, REMOVE_SUBSCRIPTION);
	/*
	 * TODO:
	 *
	 *   1) Disable the subscription if necessary.
	 *
	 */
	if (FALSE) {
		subscription_id = pka_subscription_get_id(subscription);
		INFO(Subscription, "Removing subscription %d on behalf of context %d",
		     subscription_id, pka_context_get_id(context));
		G_LOCK(subscriptions);
		g_ptr_array_remove(manager.subscriptions, subscription);
		G_UNLOCK(subscriptions);
		NOTIFY_LISTENERS(subscription_removed, subscription_id);
		pka_subscription_unref(subscription);
	}
	RETURN(TRUE);
}
