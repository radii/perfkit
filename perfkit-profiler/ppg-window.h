/* ppg-window.h
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

#ifndef __PPG_WINDOW_H__
#define __PPG_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_WINDOW            (ppg_window_get_type())
#define PPG_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_WINDOW, PpgWindow))
#define PPG_WINDOW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_WINDOW, PpgWindow const))
#define PPG_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_WINDOW, PpgWindowClass))
#define PPG_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_WINDOW))
#define PPG_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_WINDOW))
#define PPG_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_WINDOW, PpgWindowClass))

typedef struct _PpgWindow        PpgWindow;
typedef struct _PpgWindowClass   PpgWindowClass;
typedef struct _PpgWindowPrivate PpgWindowPrivate;

struct _PpgWindow
{
	GtkWindow parent;

	/*< private >*/
	PpgWindowPrivate *priv;
};

struct _PpgWindowClass
{
	GtkWindowClass parent_class;
};

GType ppg_window_get_type (void) G_GNUC_CONST;
guint ppg_window_count    (void);

G_END_DECLS

#endif /* __PPG_WINDOW_H__ */
