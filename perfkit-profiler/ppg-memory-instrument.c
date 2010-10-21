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

#include "ppg-line-visualizer.h"
#include "ppg-memory-instrument.h"

G_DEFINE_TYPE(PpgMemoryInstrument, ppg_memory_instrument, PPG_TYPE_INSTRUMENT)

struct _PpgMemoryInstrumentPrivate
{
	PpgModel *model;
};

static PpgVisualizer*
create_used_visualizer (PpgMemoryInstrument *instrument)
{
	PpgMemoryInstrumentPrivate *priv;
	PpgLineVisualizer *viz;
	GdkColor color;

	priv = instrument->priv;

	viz = g_object_new(PPG_TYPE_LINE_VISUALIZER,
	                   "name", "used",
	                   "title", _("Used"),
	                   NULL);
	gdk_color_parse("#2e3436", &color);
	ppg_line_visualizer_append(viz, "Used", &color, TRUE, priv->model, 0);

	return PPG_VISUALIZER(viz);
}

static PpgVisualizer*
create_free_visualizer (PpgMemoryInstrument *instrument)
{
	PpgMemoryInstrumentPrivate *priv;
	PpgLineVisualizer *viz;
	GdkColor color;

	priv = instrument->priv;

	viz = g_object_new(PPG_TYPE_LINE_VISUALIZER,
	                   "name", "free",
	                   "title", _("Free"),
	                   NULL);
	gdk_color_parse("#2e3436", &color);
	ppg_line_visualizer_append(viz, "Free", &color, TRUE, priv->model, 0);

	return PPG_VISUALIZER(viz);
}

static PpgVisualizer*
create_cache_visualizer (PpgMemoryInstrument *instrument)
{
	PpgMemoryInstrumentPrivate *priv;
	PpgLineVisualizer *viz;
	GdkColor color;

	priv = instrument->priv;

	viz = g_object_new(PPG_TYPE_LINE_VISUALIZER,
	                   "name", "cached",
	                   "title", _("Cache Size"),
	                   NULL);
	gdk_color_parse("#2e3436", &color);
	ppg_line_visualizer_append(viz, "Cache", &color, TRUE, priv->model, 0);

	return PPG_VISUALIZER(viz);
}

static PpgVisualizer*
create_combined_visualizer (PpgMemoryInstrument *instrument)
{
	PpgMemoryInstrumentPrivate *priv;
	PpgLineVisualizer *viz;
	GdkColor color;

	priv = instrument->priv;

	viz = g_object_new(PPG_TYPE_LINE_VISUALIZER,
	                   "name", "combined",
	                   "title", _("Combined"),
	                   NULL);
	gdk_color_parse("#2e3436", &color);
	ppg_line_visualizer_append(viz, "Free", &color, FALSE, priv->model, 0);
	gdk_color_parse("#555753", &color);
	ppg_line_visualizer_append(viz, "Used", &color, FALSE, priv->model, 0);
	gdk_color_parse("#888a85", &color);
	ppg_line_visualizer_append(viz, "Cache", &color, FALSE, priv->model, 0);

	return PPG_VISUALIZER(viz);
}

static PpgVisualizerEntry visualizer_entries[] = {
	{ "combined", N_("Combined Memory"), NULL, G_CALLBACK(create_combined_visualizer) },
	{ "used", N_("Used Memory"), NULL, G_CALLBACK(create_used_visualizer) },
	{ "free", N_("Free Memory"), NULL, G_CALLBACK(create_free_visualizer) },
	{ "cached", N_("Cache Size"), NULL, G_CALLBACK(create_cache_visualizer) },
};

static gboolean
ppg_memory_instrument_load (PpgInstrument  *instrument,
                            PpgSession     *session,
                            GError        **error)
{
	ppg_instrument_add_visualizer(instrument, "combined");
	ppg_instrument_add_visualizer(instrument, "cached");
	ppg_instrument_add_visualizer(instrument, "free");

	return TRUE;
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
