/* ppg-memory-instrument.c
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

#include <glib/gi18n.h>

#include "ppg-color.h"
#include "ppg-line-visualizer.h"
#include "ppg-memory-instrument.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Memory"

#define RPC_OR_GOTO(_rpc, _args, _label)       \
    G_STMT_START {                             \
        ret = pk_connection_##_rpc _args;      \
        if (!ret) {                            \
            goto _label;                       \
        }                                      \
    } G_STMT_END

#define RPC_OR_FAILURE(_rpc, _args) RPC_OR_GOTO(_rpc, _args, failure)

G_DEFINE_TYPE(PpgMemoryInstrument, ppg_memory_instrument, PPG_TYPE_INSTRUMENT)

struct _PpgMemoryInstrumentPrivate
{
	PpgModel *model;   /* Data model for storing statistics. */
	gint      source;  /* Perfkit memory source id. */
	gint      sub;     /* Perfkit subscription id. */
};

enum
{
	COLUMN_SIZE,
	COLUMN_RESIDENT,
	COLUMN_SHARE,
	COLUMN_TEXT,
	COLUMN_DATA,
	COLUMN_LAST
};

/**
 * ppg_memory_instrument_process_cb:
 * @instrument: A #PpgMemoryInstrument.
 *
 * Creates the process memory usage visualizer to be embedded in the
 * instrument view.
 *
 * Returns: A PpgVisualizer.
 */
static PpgVisualizer*
ppg_memory_instrument_process_cb (PpgMemoryInstrument *instrument)
{
	PpgMemoryInstrumentPrivate *priv;
	PpgLineVisualizer *visualizer;
	PpgColorIter iter;

	g_return_val_if_fail(PPG_IS_MEMORY_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	visualizer = g_object_new(PPG_TYPE_LINE_VISUALIZER,
	                          "name", "process",
	                          "title", _("Process Memory Usage"),
	                          NULL);

	ppg_color_iter_init(&iter);
	ppg_line_visualizer_append(visualizer, _("Size"), &iter.color,
	                           FALSE, priv->model, COLUMN_SIZE);

	ppg_color_iter_next(&iter);
	ppg_line_visualizer_append(visualizer, _("Resident"), &iter.color,
	                           FALSE, priv->model, COLUMN_RESIDENT);

	ppg_color_iter_next(&iter);
	ppg_line_visualizer_append(visualizer, _("Share"), &iter.color,
	                           FALSE, priv->model, COLUMN_SHARE);

	ppg_color_iter_next(&iter);
	ppg_line_visualizer_append(visualizer, _("Text"), &iter.color,
	                           FALSE, priv->model, COLUMN_TEXT);

	ppg_color_iter_next(&iter);
	ppg_line_visualizer_append(visualizer, _("Data"), &iter.color,
	                           FALSE, priv->model, COLUMN_DATA);

	return PPG_VISUALIZER(visualizer);
}

static PpgVisualizerEntry visualizer_entries[] = {
	{ "process",
	  N_("Process Memory Usage"),
	  NULL,
	  G_CALLBACK(ppg_memory_instrument_process_cb) },
};

/**
 * ppg_memory_instrument_manifest_cb:
 * @manifest: (in): An incoming #PkManifest.
 * @user_data: (in): User data for the closure.
 *
 * Handles an incoming manifest from the Perfkit agent. The contents
 * are stored to the instruments data model.
 *
 * Returns: None.
 * Side effects: Data is stored.
 */
static void
ppg_memory_instrument_manifest_cb (PkManifest *manifest,
                                   gpointer    user_data)
{
	PpgMemoryInstrument *instrument = (PpgMemoryInstrument *)user_data;
	PpgMemoryInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_MEMORY_INSTRUMENT(instrument));

	priv = instrument->priv;

	g_debug("Received new manifest");
	ppg_model_insert_manifest(priv->model, manifest);
}

/**
 * ppg_memory_instrument_sample_cb:
 * @manifest: (in): The current #PkManifest.
 * @sample: (in): An incoming #PkSample.
 *
 * Handles an incoming sample from the Perfkit agent. The contents
 * are stored to the instruments data model.
 *
 * Returns: None.
 * Side effects: Data is stored.
 */
static void
ppg_memory_instrument_sample_cb (PkManifest *manifest,
                                 PkSample   *sample,
                                 gpointer    user_data)
{
	PpgMemoryInstrument *instrument = (PpgMemoryInstrument *)user_data;
	PpgMemoryInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_MEMORY_INSTRUMENT(instrument));

	priv = instrument->priv;

	g_debug("Received new sample");
	ppg_model_insert_sample(priv->model, manifest, sample);
}

/**
 * ppg_memory_instrument_set_handlers_cb:
 * @object: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @user_data: (in): User data for closure.
 *
 * Handles an asynchronous request to set the delivery closures for incoming
 * manifests and samples.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_memory_instrument_set_handlers_cb (GObject *object,
                                       GAsyncResult *result,
                                       gpointer user_data)
{
	PpgMemoryInstrument *instrument = (PpgMemoryInstrument *)user_data;
	PpgMemoryInstrumentPrivate *priv;
	PkConnection *conn = (PkConnection *)object;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_MEMORY_INSTRUMENT(instrument));
	g_return_if_fail(PK_IS_CONNECTION(object));

	priv = instrument->priv;

	if (!pk_connection_subscription_set_handlers_finish(conn, result, &error)) {
		g_critical("Failed to subscribe to subscription: %d", priv->sub);
	}
}

/**
 * ppg_memory_instrument_load:
 * @instrument: (in): A #PpgMemoryInstrument.
 * @session: (in): A #PpgSession.
 * @error: (error): A location for a #GError or %NULL.
 *
 * Loads the required resources for the PpgMemoryInstrument to run within the
 * Perfkit agent. Default visualizers are also created.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: Memory source is added to agent.
 */
static gboolean
ppg_memory_instrument_load (PpgInstrument  *instrument,
                            PpgSession     *session,
                            GError        **error)
{
	PpgMemoryInstrument *memory = (PpgMemoryInstrument *)instrument;
	PpgMemoryInstrumentPrivate *priv;
	PkConnection *conn;
	gboolean ret = FALSE;
	gint channel;

	g_return_val_if_fail(PPG_IS_MEMORY_INSTRUMENT(instrument), FALSE);

	priv = memory->priv;

	g_object_get(session,
	             "channel", &channel,
	             "connection", &conn,
	             NULL);

	RPC_OR_FAILURE(manager_add_source,
	               (conn, "Memory", &priv->source, error));

	RPC_OR_FAILURE(channel_add_source,
	               (conn, channel, priv->source, error));

	RPC_OR_FAILURE(manager_add_subscription,
	               (conn, 0, 0, &priv->sub, error));

	RPC_OR_FAILURE(subscription_add_source,
	               (conn, priv->sub, priv->source, error));

	pk_connection_subscription_set_handlers_async(
			conn, priv->sub,
			ppg_memory_instrument_manifest_cb, memory, NULL,
			ppg_memory_instrument_sample_cb, memory, NULL,
			NULL,
			ppg_memory_instrument_set_handlers_cb, instrument);

	ppg_instrument_add_visualizer(instrument, "process");

  failure:
	g_object_unref(conn);
	return ret;
}

/**
 * ppg_memory_instrument_unload:
 * @instrument: (in): A #PpgMemoryInstrument.
 * @session: (in): A #PpgSession.
 * @error: (out): A location for a #GError or %NULL.
 *
 * Unloads the instrument and destroys remote resources.
 *
 * Returns: TRUE if successful; otherwise FALSE.
 * Side effects: Agent resources are destroyed.
 */
static gboolean
ppg_memory_instrument_unload (PpgInstrument  *instrument,
                              PpgSession     *session,
                              GError        **error)
{
	PpgMemoryInstrumentPrivate *priv;
	PkConnection *conn;
	gboolean ret = FALSE;
	gboolean removed;

	g_return_val_if_fail(PPG_IS_MEMORY_INSTRUMENT(instrument), FALSE);
	g_return_val_if_fail(PPG_IS_SESSION(session), FALSE);

	priv = PPG_MEMORY_INSTRUMENT(instrument)->priv;

	g_object_get(session,
	             "connection", &conn,
	             NULL);

	RPC_OR_FAILURE(manager_remove_subscription,
	               (conn, priv->sub, &removed, error));
	RPC_OR_FAILURE(manager_remove_source,
	               (conn, priv->source, error));

  failure:
  	priv->sub = 0;
  	priv->source = 0;
  	g_object_unref(conn);
  	return ret;
}

/**
 * ppg_memory_instrument_finalize:
 * @object: (in): A #PpgMemoryInstrument.
 *
 * Finalizer for a #PpgMemoryInstrument instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_memory_instrument_finalize (GObject *object)
{
	PpgMemoryInstrumentPrivate *priv = PPG_MEMORY_INSTRUMENT(object)->priv;

	g_object_unref(priv->model);

	G_OBJECT_CLASS(ppg_memory_instrument_parent_class)->finalize(object);
}

/**
 * ppg_memory_instrument_class_init:
 * @klass: (in): A #PpgMemoryInstrumentClass.
 *
 * Initializes the #PpgMemoryInstrumentClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_memory_instrument_class_init (PpgMemoryInstrumentClass *klass)
{
	GObjectClass *object_class;
	PpgInstrumentClass *instrument_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_memory_instrument_finalize;
	g_type_class_add_private(object_class, sizeof(PpgMemoryInstrumentPrivate));

	instrument_class = PPG_INSTRUMENT_CLASS(klass);
	instrument_class->load = ppg_memory_instrument_load;
	instrument_class->unload = ppg_memory_instrument_unload;
}

/**
 * ppg_memory_instrument_init:
 * @instrument: (in): A #PpgMemoryInstrument.
 *
 * Initializes the newly created #PpgMemoryInstrument instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_memory_instrument_init (PpgMemoryInstrument *instrument)
{
	PpgMemoryInstrumentPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(instrument, PPG_TYPE_MEMORY_INSTRUMENT,
	                                   PpgMemoryInstrumentPrivate);
	instrument->priv = priv;

	ppg_instrument_register_visualizers(PPG_INSTRUMENT(instrument),
	                                    visualizer_entries,
	                                    G_N_ELEMENTS(visualizer_entries),
	                                    instrument);

	priv->model = g_object_new(PPG_TYPE_MODEL,
	                           NULL);
}
