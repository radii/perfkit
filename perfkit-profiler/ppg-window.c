/* ppg-window.c
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <perfkit/perfkit.h>

#include "ppg-add-source-dialog.h"
#include "ppg-log.h"
#include "ppg-log-window.h"
#include "ppg-panels.h"
#include "ppg-path.h"
#include "ppg-session-view.h"
#include "ppg-util.h"
#include "ppg-window.h"


#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Window"


G_DEFINE_TYPE(PpgWindow, ppg_window, GTK_TYPE_WINDOW)


struct _PpgWindowPrivate
{
	GtkWidget *session_view;
	GtkWidget *session_status;
};


static guint window_count = 0;


/**
 * ppg_window_count:
 *
 * Returns the count of active windows.
 *
 * Returns: The number of #PpgWindow instances.
 * Side effects: None.
 */
guint
ppg_window_count (void)
{
	return window_count;
}


/**
 * ppg_window_delete_event:
 * @widget: (in): A #PpgWindow.
 *
 * Handle #GtkWidget "delete-event". Shut down if all windows are closed.
 *
 * Returns: None.
 * Side effects: None.
 */
static gboolean
ppg_window_delete_event (GtkWidget   *widget,
                         GdkEventAny *any)
{

	if (!ppg_window_count()) {
		gtk_main_quit();
	}
	return FALSE;
}


static void
ppg_window_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_window_parent_class)->finalize(object);
}


static void
ppg_window_class_init (PpgWindowClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_window_finalize;
	g_type_class_add_private(object_class, sizeof(PpgWindowPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->delete_event = ppg_window_delete_event;
}


static void
ppg_window_init (PpgWindow *window)
{
	PpgWindowPrivate *priv = INIT_PRIV(window, WINDOW, Window);
	GtkWidget *vbox;

	g_object_set(window,
	             "default-width", 770,
	             "default-height", 550,
	             "window-position", GTK_WIN_POS_CENTER,
	             NULL);

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "homogeneous", FALSE,
	                    "spacing", 0,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	priv->session_view = g_object_new(PPG_TYPE_SESSION_VIEW,
	                                  "visible", TRUE,
	                                  NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox),
	                                  priv->session_view,
	                                  "expand", TRUE,
	                                  "fill", TRUE,
	                                  NULL);

	priv->session_status = g_object_new(GTK_TYPE_STATUSBAR,
	                                    "has-resize-grip", TRUE,
	                                    "visible", TRUE,
	                                    NULL);
	gtk_statusbar_push(GTK_STATUSBAR(priv->session_status), 0, _("Ready"));
	gtk_container_add_with_properties(GTK_CONTAINER(vbox),
	                                  priv->session_status,
	                                  "expand", FALSE,
	                                  "fill", TRUE,
	                                  NULL);

}
