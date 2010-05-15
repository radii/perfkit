/* pka-source.c
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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "pka-channel.h"
#include "pka-log.h"
#include "pka-source.h"

/**
 * SECTION:pka-source
 * @title: PkaSource
 * @short_description: 
 *
 * 
 */

G_DEFINE_ABSTRACT_TYPE (PkaSource, pka_source, G_TYPE_OBJECT)

/*
 * Internal callbacks for channel delivery.
 */
extern void pka_channel_deliver_sample   (PkaChannel  *channel,
                                          PkaSource   *source,
                                          PkaSample   *sample);
extern void pka_channel_deliver_manifest (PkaChannel  *channel,
                                          PkaSource   *source,
                                          PkaManifest *manifest);

struct _PkaSourcePrivate
{
	guint       source_id;
	PkaChannel *channel;
	PkaPlugin  *plugin;
};

static guint source_seq = 0;

/**
 * pka_source_set_channel:
 * @source: A #PkaSource
 * @channel: A #PkaChannel
 *
 * Internal method used by channels to attach themselves as the destination
 * for a sources samples and manifest.
 *
 * Side effects: None.
 */
void
pka_source_set_channel (PkaSource  *source,
                        PkaChannel *channel)
{
	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(PKA_IS_CHANNEL(channel));
	g_return_if_fail(!source->priv->channel);

	source->priv->channel = g_object_ref(channel);
}

/**
 * pka_source_deliver_sample:
 * @source: A #PkaSource
 * @sample: A #PkaSample
 *
 * Delivers a sample to the channel the source is attached to.
 *
 * This function
 * implies that the function becomes the new owner to @sample.  Therefore,
 * no need to call pka_sample_unref() is required by the caller.  However, if
 * the caller for some reason needs to continue using the sample afterwards,
 * it should call pka_sample_ref() before-hand.
 *
 * Side effects: None.
 */
void
pka_source_deliver_sample (PkaSource *source,
                           PkaSample *sample)
{
	g_return_if_fail(source != NULL);
	g_return_if_fail(sample != NULL);
	g_return_if_fail(source->priv->channel);

	pka_channel_deliver_sample(source->priv->channel, source, sample);
}

/**
 * pka_source_deliver_manifest:
 * @source: A #PkaSource
 * @manifest: A #PkaManifest
 *
 * Delivers a manifest to the channel the source is attached to.
 *
 * This function implies that the function becomes the owner of @manifest.
 * Therefore, no need to call pka_manifest_unref() is required by the caller.
 * However, if the caller for some reason needs to continue using the
 * manifest afterwards, it should call pka_manifest_ref() before-hand.
 *
 * Side effects: None.
 */
void
pka_source_deliver_manifest (PkaSource   *source,
                             PkaManifest *manifest)
{
	g_return_if_fail(source != NULL);
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(source->priv->channel);

	pka_channel_deliver_manifest(source->priv->channel, source, manifest);
}

/**
 * pka_source_get_id:
 * @source: A #PkaSource
 *
 * Retrieves the unique source identifier for the source instance.  This is not
 * the same as the source identifier within a #PkaChannel.  This is a monotonic
 * identifier assigned to each source upon instance creation.
 *
 * Returns: An unsigned integer containing the source id.
 *
 * Side effects: None.
 */
guint
pka_source_get_id (PkaSource *source)
{
	g_return_val_if_fail(PKA_IS_SOURCE(source), 0);
	return source->priv->source_id;
}

void
pka_source_notify_started (PkaSource    *source,
                           PkaSpawnInfo *spawn_info)
{
	if (PKA_SOURCE_GET_CLASS(source)->notify_started)
		PKA_SOURCE_GET_CLASS(source)->notify_started(source, spawn_info);
}

void
pka_source_notify_stopped (PkaSource *source)
{
	if (PKA_SOURCE_GET_CLASS(source)->notify_stopped)
		PKA_SOURCE_GET_CLASS(source)->notify_stopped(source);
}

void
pka_source_notify_muted (PkaSource *source)
{
	if (PKA_SOURCE_GET_CLASS(source)->notify_muted)
		PKA_SOURCE_GET_CLASS(source)->notify_muted(source);
}

void
pka_source_notify_unmuted (PkaSource *source)
{
	if (PKA_SOURCE_GET_CLASS(source)->notify_unmuted)
		PKA_SOURCE_GET_CLASS(source)->notify_unmuted(source);
}

PkaPlugin*
pka_source_get_plugin (PkaSource *source) /* IN */
{
	g_return_val_if_fail(PKA_IS_SOURCE(source), NULL);
	return source->priv->plugin;
}

void
pka_source_set_plugin (PkaSource *source,
                       PkaPlugin *plugin) /* IN */
{
	g_return_if_fail(PKA_IS_SOURCE(source));
	g_return_if_fail(source->priv->plugin == NULL);
	source->priv->plugin = g_object_ref(plugin);
}

static void
pka_source_finalize (GObject *object)
{
	PkaSourcePrivate *priv = PKA_SOURCE(object)->priv;

	if (priv->plugin) {
		g_object_unref(priv->plugin);
	}

	if (priv->channel) {
		g_object_unref(priv->channel);
	}

	G_OBJECT_CLASS(pka_source_parent_class)->finalize(object);
}

static void
pka_source_dispose (GObject *object)
{
	PkaSourcePrivate *priv = PKA_SOURCE(object)->priv;

	if (priv->channel) {
		g_object_unref(priv->channel);
	}

	G_OBJECT_CLASS(pka_source_parent_class)->dispose(object);
}

static void
pka_source_class_init (PkaSourceClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_source_finalize;
	object_class->dispose = pka_source_dispose;
	g_type_class_add_private (object_class, sizeof (PkaSourcePrivate));
}

static void
pka_source_init (PkaSource *source)
{
	gint source_id;

	source->priv = G_TYPE_INSTANCE_GET_PRIVATE(source,
	                                           PKA_TYPE_SOURCE,
	                                           PkaSourcePrivate);

	source_id = g_atomic_int_exchange_and_add((gint *)&source_seq, 1);
	source->priv->source_id = source_id;
}

gboolean
pka_source_modify_spawn_info (PkaSource     *source,     /* IN */
                              PkaSpawnInfo  *spawn_info, /* IN */
                              GError       **error)      /* OUT */
{
	ENTRY;
	RETURN(TRUE);
}
