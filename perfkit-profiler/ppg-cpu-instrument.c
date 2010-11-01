/* ppg-cpu-instrument.c
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
#include "ppg-cpu-instrument.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Cpu"

#define RPC_OR_GOTO(_rpc, _args, _label)  \
    G_STMT_START {                        \
        ret = pk_connection_##_rpc _args; \
        if (!ret) {                       \
            goto _label;                  \
        }                                 \
    } G_STMT_END

#define RPC_OR_FAILURE(_rpc, _args) RPC_OR_GOTO(_rpc, _args, failure)

G_DEFINE_TYPE(PpgCpuInstrument, ppg_cpu_instrument, PPG_TYPE_INSTRUMENT)

struct _PpgCpuInstrumentPrivate
{
	PpgModel *model;   /* Data model for storing statistics. */
	gint      source;  /* Perfkit cpu source id. */
	gint      sub;     /* Perfkit subscription id. */
};

enum
{
	COLUMN_USER,
	COLUMN_NICE,
	COLUMN_SYSTEM,
	COLUMN_IDLE,
	COLUMN_COOKED,
	COLUMN_LAST
};

static PpgVisualizer*
ppg_cpu_instrument_combined_cb (PpgCpuInstrument *instrument)
{
	PpgCpuInstrumentPrivate *priv;
	PpgLineVisualizer *visualizer;
	PpgColorIter iter;

	g_return_val_if_fail(PPG_IS_CPU_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	visualizer = g_object_new(PPG_TYPE_LINE_VISUALIZER,
	                          "name", "combined",
	                          "title", _("Combined Cpu Usage"),
	                          NULL);

	ppg_color_iter_init(&iter);
	ppg_line_visualizer_append(visualizer, _("User"), &iter.color,
	                           FALSE, priv->model, COLUMN_COOKED);

	return PPG_VISUALIZER(visualizer);
}

static PpgVisualizerEntry visualizer_entries[] = {
	{ "combined",
	  N_("Combined CPU Usage"),
	  NULL,
	  G_CALLBACK(ppg_cpu_instrument_combined_cb) },
};

/**
 * ppg_cpu_instrument_manifest_cb:
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
ppg_cpu_instrument_manifest_cb (PkManifest *manifest,
                                   gpointer    user_data)
{
	PpgCpuInstrument *instrument = (PpgCpuInstrument *)user_data;
	PpgCpuInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_CPU_INSTRUMENT(instrument));

	priv = instrument->priv;

	g_debug("Received new manifest");
	ppg_model_insert_manifest(priv->model, manifest);
}

/**
 * ppg_cpu_instrument_sample_cb:
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
ppg_cpu_instrument_sample_cb (PkManifest *manifest,
                              PkSample   *sample,
                              gpointer    user_data)
{
	PpgCpuInstrument *instrument = (PpgCpuInstrument *)user_data;
	PpgCpuInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_CPU_INSTRUMENT(instrument));

	priv = instrument->priv;

#if PERFKIT_DEBUG
	g_assert_cmpint(pk_sample_get_source_id(sample), ==, priv->source);
	g_assert_cmpint(pk_manifest_get_source_id(manifest), ==, priv->source);
#endif

	g_debug("Received new sample");
	ppg_model_insert_sample(priv->model, manifest, sample);
}

/**
 * ppg_cpu_instrument_set_handlers_cb:
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
ppg_cpu_instrument_set_handlers_cb (GObject *object,
                                       GAsyncResult *result,
                                       gpointer user_data)
{
	PpgCpuInstrument *instrument = (PpgCpuInstrument *)user_data;
	PpgCpuInstrumentPrivate *priv;
	PkConnection *conn = (PkConnection *)object;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_CPU_INSTRUMENT(instrument));
	g_return_if_fail(PK_IS_CONNECTION(object));

	priv = instrument->priv;

	if (!pk_connection_subscription_set_handlers_finish(conn, result, &error)) {
		g_critical("Failed to subscribe to subscription: %d", priv->sub);
	}
}

/**
 * ppg_cpu_instrument_load:
 * @instrument: (in): A #PpgCpuInstrument.
 * @session: (in): A #PpgSession.
 * @error: (error): A location for a #GError or %NULL.
 *
 * Loads the required resources for the PpgCpuInstrument to run within the
 * Perfkit agent. Default visualizers are also created.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: Cpu source is added to agent.
 */
static gboolean
ppg_cpu_instrument_load (PpgInstrument  *instrument,
                            PpgSession     *session,
                            GError        **error)
{
	PpgCpuInstrument *cpu = (PpgCpuInstrument *)instrument;
	PpgCpuInstrumentPrivate *priv;
	PkConnection *conn;
	gboolean ret = FALSE;
	gint channel;

	g_return_val_if_fail(PPG_IS_CPU_INSTRUMENT(instrument), FALSE);

	priv = cpu->priv;

	g_object_get(session,
	             "channel", &channel,
	             "connection", &conn,
	             NULL);

	RPC_OR_FAILURE(manager_add_source,
	               (conn, "Cpu", &priv->source, error));

	RPC_OR_FAILURE(channel_add_source,
	               (conn, channel, priv->source, error));

	RPC_OR_FAILURE(manager_add_subscription,
	               (conn, 0, 0, &priv->sub, error));

	RPC_OR_FAILURE(subscription_add_source,
	               (conn, priv->sub, priv->source, error));

	pk_connection_subscription_set_handlers_async(
			conn, priv->sub,
			ppg_cpu_instrument_manifest_cb, cpu, NULL,
			ppg_cpu_instrument_sample_cb, cpu, NULL,
			NULL,
			ppg_cpu_instrument_set_handlers_cb, instrument);

	ppg_instrument_add_visualizer(instrument, "combined");

  failure:
	g_object_unref(conn);
	return ret;
}

/**
 * ppg_cpu_instrument_unload:
 * @instrument: (in): A #PpgCpuInstrument.
 * @session: (in): A #PpgSession.
 * @error: (out): A location for a #GError or %NULL.
 *
 * Unloads the instrument and destroys remote resources.
 *
 * Returns: TRUE if successful; otherwise FALSE.
 * Side effects: Agent resources are destroyed.
 */
static gboolean
ppg_cpu_instrument_unload (PpgInstrument  *instrument,
                              PpgSession     *session,
                              GError        **error)
{
	PpgCpuInstrumentPrivate *priv;
	PkConnection *conn;
	gboolean ret = FALSE;
	gboolean removed;

	g_return_val_if_fail(PPG_IS_CPU_INSTRUMENT(instrument), FALSE);
	g_return_val_if_fail(PPG_IS_SESSION(session), FALSE);

	priv = PPG_CPU_INSTRUMENT(instrument)->priv;

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

static void
ppg_cpu_instrument_calc_cpu (PpgModel *model,
                             PpgModelIter *iter,
                             gint key,
                             GValue *value,
                             gpointer user_data)
{
	gint user;
	gint nice_;
	gint system;
	gint idle;
	gdouble percent;

	ppg_model_get(model, iter,
	              COLUMN_USER, &user,
	              COLUMN_NICE, &nice_,
	              COLUMN_SYSTEM, &system,
	              COLUMN_IDLE, &idle,
	              -1);

	g_value_init(value, G_TYPE_DOUBLE);
	percent = (gdouble)(user + nice_ + system) / (gdouble)(user + nice_ + system + idle) * 100.0;
	g_debug("Percent: %f", percent);
	g_value_set_double(value, percent);
}

static void
ppg_cpu_instrument_finalize (GObject *object)
{
	PpgCpuInstrumentPrivate *priv = PPG_CPU_INSTRUMENT(object)->priv;

	g_object_unref(priv->model);

	G_OBJECT_CLASS(ppg_cpu_instrument_parent_class)->finalize(object);
}

static void
ppg_cpu_instrument_class_init (PpgCpuInstrumentClass *klass)
{
	GObjectClass *object_class;
	PpgInstrumentClass *instrument_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_cpu_instrument_finalize;
	g_type_class_add_private(object_class, sizeof(PpgCpuInstrumentPrivate));

	instrument_class = PPG_INSTRUMENT_CLASS(klass);
	instrument_class->load = ppg_cpu_instrument_load;
	instrument_class->unload = ppg_cpu_instrument_unload;
}

static void
ppg_cpu_instrument_init (PpgCpuInstrument *instrument)
{
	PpgCpuInstrumentPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(instrument, PPG_TYPE_CPU_INSTRUMENT,
	                                   PpgCpuInstrumentPrivate);
	instrument->priv = priv;

	ppg_instrument_register_visualizers(PPG_INSTRUMENT(instrument),
	                                    visualizer_entries,
	                                    G_N_ELEMENTS(visualizer_entries),
	                                    instrument);

	priv->model = g_object_new(PPG_TYPE_MODEL, NULL);
	ppg_model_add_mapping(priv->model, COLUMN_USER, "User", G_TYPE_INT);
	ppg_model_add_mapping(priv->model, COLUMN_NICE, "Nice", G_TYPE_INT);
	ppg_model_add_mapping(priv->model, COLUMN_SYSTEM, "System", G_TYPE_INT);
	ppg_model_add_mapping(priv->model, COLUMN_IDLE, "Idle", G_TYPE_INT);
	ppg_model_add_mapping_func(priv->model, COLUMN_COOKED,
	                           ppg_cpu_instrument_calc_cpu,
	                           instrument);
}
