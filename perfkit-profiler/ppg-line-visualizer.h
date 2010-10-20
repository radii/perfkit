/* ppg-line-visualizer.h
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

#ifndef __PPG_LINE_VISUALIZER_H__
#define __PPG_LINE_VISUALIZER_H__

#include <gdk/gdk.h>

#include "ppg-model.h"
#include "ppg-visualizer.h"

G_BEGIN_DECLS

#define PPG_TYPE_LINE_VISUALIZER            (ppg_line_visualizer_get_type())
#define PPG_LINE_VISUALIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_LINE_VISUALIZER, PpgLineVisualizer))
#define PPG_LINE_VISUALIZER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_LINE_VISUALIZER, PpgLineVisualizer const))
#define PPG_LINE_VISUALIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_LINE_VISUALIZER, PpgLineVisualizerClass))
#define PPG_IS_LINE_VISUALIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_LINE_VISUALIZER))
#define PPG_IS_LINE_VISUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_LINE_VISUALIZER))
#define PPG_LINE_VISUALIZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_LINE_VISUALIZER, PpgLineVisualizerClass))

typedef struct _PpgLineVisualizer        PpgLineVisualizer;
typedef struct _PpgLineVisualizerClass   PpgLineVisualizerClass;
typedef struct _PpgLineVisualizerPrivate PpgLineVisualizerPrivate;

struct _PpgLineVisualizer
{
	PpgVisualizer parent;

	/*< private >*/
	PpgLineVisualizerPrivate *priv;
};

struct _PpgLineVisualizerClass
{
	PpgVisualizerClass parent_class;
};

GType ppg_line_visualizer_get_type (void) G_GNUC_CONST;
void  ppg_line_visualizer_append (PpgLineVisualizer *visualizer,
                                  const gchar *name,
                                  GdkColor *color,
                                  gboolean fill,
                                  PpgModel *model,
                                  gint row);

G_END_DECLS

#endif /* __PPG_LINE_VISUALIZER_H__ */
