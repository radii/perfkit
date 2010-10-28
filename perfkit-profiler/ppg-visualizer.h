/* ppg-visualizer.h
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

#ifndef __PPG_VISUALIZER_H__
#define __PPG_VISUALIZER_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define PPG_TYPE_VISUALIZER            (ppg_visualizer_get_type())
#define PPG_VISUALIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_VISUALIZER, PpgVisualizer))
#define PPG_VISUALIZER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_VISUALIZER, PpgVisualizer const))
#define PPG_VISUALIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_VISUALIZER, PpgVisualizerClass))
#define PPG_IS_VISUALIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_VISUALIZER))
#define PPG_IS_VISUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_VISUALIZER))
#define PPG_VISUALIZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_VISUALIZER, PpgVisualizerClass))

typedef struct _PpgVisualizer        PpgVisualizer;
typedef struct _PpgVisualizerClass   PpgVisualizerClass;
typedef struct _PpgVisualizerPrivate PpgVisualizerPrivate;
typedef struct _PpgVisualizerEntry   PpgVisualizerEntry;

struct _PpgVisualizer
{
	GInitiallyUnowned parent;

	/*< private >*/
	PpgVisualizerPrivate *priv;
};

struct _PpgVisualizerClass
{
	GInitiallyUnownedClass parent_class;

	ClutterActor* (*get_actor) (PpgVisualizer *visualizer);
	void          (*draw)      (PpgVisualizer *visualizer);
};

struct _PpgVisualizerEntry
{
	const gchar *name;
	const gchar *title;
	const gchar *icon_name;
	GCallback    callback;
};

GType         ppg_visualizer_get_type   (void) G_GNUC_CONST;
ClutterActor* ppg_visualizer_get_actor  (PpgVisualizer *visualizer);
void          ppg_visualizer_queue_draw (PpgVisualizer *visualizer);

G_END_DECLS

#endif /* __PPG_VISUALIZER_H__ */
