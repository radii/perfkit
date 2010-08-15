/* ppg-log-window.h
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-gui/perfkit-gui.h> can be included directly."
#endif

#ifndef __PPG_LOG_WINDOW_H__
#define __PPG_LOG_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_LOG_WINDOW            (ppg_log_window_get_type())
#define PPG_LOG_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_LOG_WINDOW, PpgLogWindow))
#define PPG_LOG_WINDOW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_LOG_WINDOW, PpgLogWindow const))
#define PPG_LOG_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_LOG_WINDOW, PpgLogWindowClass))
#define PPG_IS_LOG_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_LOG_WINDOW))
#define PPG_IS_LOG_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_LOG_WINDOW))
#define PPG_LOG_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_LOG_WINDOW, PpgLogWindowClass))

typedef struct _PpgLogWindow        PpgLogWindow;
typedef struct _PpgLogWindowClass   PpgLogWindowClass;
typedef struct _PpgLogWindowPrivate PpgLogWindowPrivate;

struct _PpgLogWindow
{
	GtkWindow parent;

	/*< private >*/
	PpgLogWindowPrivate *priv;
};

struct _PpgLogWindowClass
{
	GtkWindowClass parent_class;
};

GType      ppg_log_window_get_type (void) G_GNUC_CONST;
GtkWidget* ppg_log_window_new      (void);

G_END_DECLS

#endif /* __PPG_LOG_WINDOW_H__ */
