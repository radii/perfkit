/* ppg-instrument.c
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

#include <string.h>

#include "ppg-instrument.h"

G_DEFINE_ABSTRACT_TYPE(PpgInstrument, ppg_instrument, G_TYPE_INITIALLY_UNOWNED)

typedef struct
{
	PpgVisualizerEntry *entry;
	gpointer            user_data;
} PpgVisualizerFactory;

struct _PpgInstrumentPrivate
{
	PpgSession *session;
	gboolean failed;
	gchar *name;
	GArray *factories;
	GList  *visualizers;
};

enum
{
	PROP_0,
	PROP_FAILED,
	PROP_NAME,
	PROP_SESSION,
};

enum
{
	VISUALIZER_ADDED,
	VISUALIZER_REMOVED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GList*
ppg_instrument_get_visualizers (PpgInstrument *instrument)
{
	g_return_val_if_fail(PPG_IS_INSTRUMENT(instrument), NULL);
	return instrument->priv->visualizers;
}

static void
ppg_instrument_set_session (PpgInstrument *instrument,
                            PpgSession    *session)
{
	PpgInstrumentClass *klass;
	PpgInstrumentPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = instrument->priv;
	klass = PPG_INSTRUMENT_GET_CLASS(instrument);

	priv->session = session;
	g_object_add_weak_pointer(G_OBJECT(priv->session), (gpointer *)&priv->session);

	if (klass->load) {
		if (!klass->load(instrument, session, &error)) {
			priv->failed = TRUE;
			g_critical("Failed to load instrument %s: %s",
			           g_type_name(G_TYPE_FROM_INSTANCE(instrument)),
			           error->message);
			/*
			 * XXX: Should we store the error?
			 */
			g_error_free(error);
			return;
		}
	}
}

static void
ppg_instrument_visualizer_added (PpgInstrument *instrument,
                                 PpgVisualizer *visualizer)
{
	PpgInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	priv = instrument->priv;
	priv->visualizers = g_list_prepend(priv->visualizers, visualizer);
}

static void
ppg_instrument_visualizer_removed (PpgInstrument *instrument,
                                   PpgVisualizer *visualizer)
{
	PpgInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	priv = instrument->priv;
	priv->visualizers = g_list_remove(priv->visualizers, visualizer);
}

void
ppg_instrument_register_visualizer (PpgInstrument      *instrument,
                                    PpgVisualizerEntry *entry,
                                    gpointer            user_data)
{
	PpgInstrumentPrivate *priv;
	PpgVisualizerFactory factory = { 0 };

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(entry != NULL);

	priv = instrument->priv;

	if (!entry->callback) {
		g_critical("PpgVisualizerEntry \"%s\" missing callback!",
		           entry->name);
		return;
	}

	factory.entry = g_new0(PpgVisualizerEntry, 1);
	memcpy(factory.entry, entry, sizeof(*entry));
	factory.user_data = user_data;

	g_array_append_val(priv->factories, factory);
}

void
ppg_instrument_register_visualizers (PpgInstrument      *instrument,
                                     PpgVisualizerEntry *entries,
                                     guint               n_entries,
                                     gpointer            user_data)
{
	PpgInstrumentPrivate *priv;
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(entries != NULL);

	priv = instrument->priv;

	for (i = 0; i < n_entries; i++) {
		ppg_instrument_register_visualizer(instrument,
		                                   &entries[i],
		                                   user_data);
	}
}

void
ppg_instrument_add_visualizer (PpgInstrument *instrument,
                               const gchar   *name)
{
	PpgInstrumentPrivate *priv;
	PpgVisualizer *visualizer = NULL;
	PpgVisualizerFactory *factory;
	PpgVisualizer* (*factory_func) (gpointer user_data);
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(name != NULL);

	g_debug("%s():%d", G_STRFUNC, __LINE__);

	priv = instrument->priv;

	for (i = 0; i < priv->factories->len; i++) {
		factory = &g_array_index(priv->factories, PpgVisualizerFactory, i);
		if (g_str_equal(factory->entry->name, name)) {
			if ((factory_func = (gpointer)factory->entry->callback)) {
				visualizer = factory_func(factory->user_data);
			}
		}
	}

	if (visualizer) {
		g_signal_emit(instrument, signals[VISUALIZER_ADDED], 0, visualizer);
	}
}

void
ppg_instrument_remove_visualizer (PpgInstrument *instrument,
                                  PpgVisualizer *visualizer)
{
	PpgInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	g_debug("%s():%d", G_STRFUNC, __LINE__);

	priv = instrument->priv;

	if (!g_list_find(priv->visualizers, visualizer)) {
		g_critical("Instrument does not contain visualizer instance!");
		return;
	}

	priv->visualizers = g_list_remove(priv->visualizers, visualizer);
	g_signal_emit(instrument, signals[VISUALIZER_REMOVED], 0, visualizer);
}

void
ppg_instrument_remove_visualizer_named (PpgInstrument *instrument,
                                        const gchar   *name)
{
	PpgInstrumentPrivate *priv;
	GList *iter;
	gchar *iter_name;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(name != NULL);

	g_debug("%s():%d", G_STRFUNC, __LINE__);

	priv = instrument->priv;

	for (iter = priv->visualizers; iter; iter = iter->next) {
		g_object_get(iter->data, "name", &iter_name, NULL);
		if (!g_strcmp0(name, iter_name)) {
			ppg_instrument_remove_visualizer(instrument, iter->data);
			g_free(iter_name);
			return;
		}
		g_free(iter_name);
	}
}

GList*
ppg_instrument_get_visualizer_entries (PpgInstrument *instrument)
{
	PpgInstrumentPrivate *priv;
	PpgVisualizerFactory *factory;
	GList *list = NULL;
	gint i;

	g_return_val_if_fail(PPG_IS_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	for (i = 0; i < priv->factories->len; i++) {
		factory = &g_array_index(priv->factories, PpgVisualizerFactory, i);
		list = g_list_prepend(list, factory->entry);
	}

	return list;
}

static void
ppg_instrument_set_name (PpgInstrument *instrument,
                         const gchar *name)
{
	PpgInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = instrument->priv;

	g_free(priv->name);
	priv->name = g_strdup(name);
}

/**
 * ppg_instrument_finalize:
 * @object: (in): A #PpgInstrument.
 *
 * Finalizer for a #PpgInstrument instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_finalize (GObject *object)
{
	PpgInstrumentPrivate *priv = PPG_INSTRUMENT(object)->priv;

	g_object_remove_weak_pointer(G_OBJECT(priv->session),
	                             (gpointer *)&priv->session);
	g_array_unref(priv->factories);
	g_free(priv->name);

	G_OBJECT_CLASS(ppg_instrument_parent_class)->finalize(object);
}

/**
 * ppg_instrument_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_instrument_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
	PpgInstrument *instrument = PPG_INSTRUMENT(object);

	switch (prop_id) {
	case PROP_FAILED:
		g_value_set_boolean(value, instrument->priv->failed);
		break;
	case PROP_NAME:
		g_value_set_string(value, instrument->priv->name);
		break;
	case PROP_SESSION:
		g_value_set_object(value, instrument->priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_instrument_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_instrument_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
	PpgInstrument *instrument = PPG_INSTRUMENT(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_instrument_set_session(instrument, g_value_get_object(value));
		break;
	case PROP_NAME:
		ppg_instrument_set_name(instrument, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_instrument_class_init:
 * @klass: (in): A #PpgInstrumentClass.
 *
 * Initializes the #PpgInstrumentClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_class_init (PpgInstrumentClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_instrument_finalize;
	object_class->get_property = ppg_instrument_get_property;
	object_class->set_property = ppg_instrument_set_property;
	g_type_class_add_private(object_class, sizeof(PpgInstrumentPrivate));

	klass->visualizer_added = ppg_instrument_visualizer_added;
	klass->visualizer_removed = ppg_instrument_visualizer_removed;

	g_object_class_install_property(object_class,
	                                PROP_NAME,
	                                g_param_spec_string("name",
	                                                    "name",
	                                                    "name",
	                                                    NULL,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_FAILED,
	                                g_param_spec_boolean("failed",
	                                                     "failed",
	                                                     "failed",
	                                                     FALSE,
	                                                     G_PARAM_READABLE));

	signals[VISUALIZER_ADDED] = g_signal_new("visualizer-added",
	                                         PPG_TYPE_INSTRUMENT,
	                                         G_SIGNAL_RUN_LAST,
	                                         G_STRUCT_OFFSET(PpgInstrumentClass, visualizer_added),
	                                         NULL,
	                                         NULL,
	                                         g_cclosure_marshal_VOID__OBJECT,
	                                         G_TYPE_NONE,
	                                         1,
	                                         PPG_TYPE_VISUALIZER);

	signals[VISUALIZER_REMOVED] = g_signal_new("visualizer-removed",
	                                           PPG_TYPE_INSTRUMENT,
	                                           G_SIGNAL_RUN_LAST,
	                                           G_STRUCT_OFFSET(PpgInstrumentClass, visualizer_removed),
	                                           NULL,
	                                           NULL,
	                                           g_cclosure_marshal_VOID__OBJECT,
	                                           G_TYPE_NONE,
	                                           1,
	                                           PPG_TYPE_VISUALIZER);
}

/**
 * ppg_instrument_init:
 * @instrument: (in): A #PpgInstrument.
 *
 * Initializes the newly created #PpgInstrument instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_init (PpgInstrument *instrument)
{
	PpgInstrumentPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(instrument, PPG_TYPE_INSTRUMENT,
	                                   PpgInstrumentPrivate);
	instrument->priv = priv;

	priv->factories = g_array_new(FALSE, FALSE, sizeof(PpgVisualizerFactory));
}
