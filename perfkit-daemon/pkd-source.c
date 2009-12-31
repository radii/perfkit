/* pkd-source.c
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

#include "pkd-channel.h"
#include "pkd-source.h"

G_DEFINE_ABSTRACT_TYPE (PkdSource, pkd_source, G_TYPE_OBJECT)

/**
 * SECTION:pkd-source
 * @title: PkdSource
 * @short_description: 
 *
 * 
 */

extern void pkd_channel_deliver_sample   (PkdChannel *channel, PkdSource *source, PkdSample *sample);
extern void pkd_channel_deliver_manifest (PkdChannel *channel, PkdSource *source, PkdManifest *manifest);

struct _PkdSourcePrivate
{
	guint       source_id;
	PkdChannel *channel;
};

static guint source_seq = 0;

/**
 * pkd_source_set_channel:
 * @source: A #PkdSource
 * @channel: A #PkdChannel
 *
 * Internal method used by channels to attach themselves as the destination
 * for a sources samples and manifest.
 *
 * Side effects: None.
 */
void
pkd_source_set_channel (PkdSource  *source,
                       PkdChannel *channel)
{
	g_return_if_fail(PKD_IS_SOURCE(source));
	g_return_if_fail(PKD_IS_CHANNEL(channel));
	g_return_if_fail(!source->priv->channel);

	source->priv->channel = g_object_ref(channel);
}

/**
 * pkd_source_deliver_sample:
 * @source: A #PkdSource
 * @sample: A #PkdSample
 *
 * Delivers a sample to the channel the source is attached to.
 *
 * This function
 * implies that the function becomes the new owner to @sample.  Therefore,
 * no need to call pkd_sample_unref() is required by the caller.  However, if
 * the caller for some reason needs to continue using the sample afterwards,
 * it should call pkd_sample_ref() before-hand.
 *
 * Side effects: None.
 */
void
pkd_source_deliver_sample (PkdSource *source,
                          PkdSample *sample)
{
	g_return_if_fail(source != NULL);
	g_return_if_fail(sample != NULL);
	g_return_if_fail(source->priv->channel);

	pkd_channel_deliver_sample(source->priv->channel, source, sample);
}

/**
 * pkd_source_deliver_manifest:
 * @source: A #PkdSource
 * @manifest: A #PkdManifest
 *
 * Delivers a manifest to the channel the source is attached to.
 *
 * This function implies that the function becomes the owner of @manifest.
 * Therefore, no need to call pkd_manifest_unref() is required by the caller.
 * However, if the caller for some reason needs to continue using the
 * manifest afterwards, it should call pkd_manifest_ref() before-hand.
 *
 * Side effects: None.
 */
void
pkd_source_deliver_manifest (PkdSource   *source,
                            PkdManifest *manifest)
{
	g_return_if_fail(source != NULL);
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(source->priv->channel);

	pkd_channel_deliver_manifest(source->priv->channel, source, manifest);
}

static void
pkd_source_finalize (GObject *object)
{
#if 0
	PkdSourcePrivate *priv = PKD_SOURCE(object)->priv;
#endif

	G_OBJECT_CLASS(pkd_source_parent_class)->finalize(object);
}

static void
pkd_source_dispose (GObject *object)
{
	PkdSourcePrivate *priv = PKD_SOURCE(object)->priv;

	if (priv->channel) {
		g_object_unref(priv->channel);
	}

	G_OBJECT_CLASS(pkd_source_parent_class)->dispose(object);
}

static void
pkd_source_class_init (PkdSourceClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkd_source_finalize;
	object_class->dispose = pkd_source_dispose;
	g_type_class_add_private (object_class, sizeof (PkdSourcePrivate));
}

static void
pkd_source_init (PkdSource *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE (source,
	                                            PKD_TYPE_SOURCE,
	                                            PkdSourcePrivate);
	source->priv->source_id = g_atomic_int_exchange_and_add((gint *)&source_seq, 1);
}
