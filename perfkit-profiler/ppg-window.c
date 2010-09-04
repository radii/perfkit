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

#include "ppg-actions.h"
#include "ppg-add-source-dialog.h"
#include "ppg-log.h"
#include "ppg-log-window.h"
#include "ppg-panels.h"
#include "ppg-path.h"
#include "ppg-pause-action.h"
#include "ppg-record-action.h"
#include "ppg-restart-action.h"
#include "ppg-session.h"
#include "ppg-session-view.h"
#include "ppg-stop-action.h"
#include "ppg-util.h"
#include "ppg-window.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Window"

struct _PpgWindowPrivate
{
	PpgSession *session;

	GtkWidget *session_view;
	GtkWidget *session_status;
};

enum
{
	PROP_0,
	PROP_SESSION,
};

static guint window_count = 0;

G_DEFINE_TYPE(PpgWindow, ppg_window, GTK_TYPE_WINDOW)

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

static void
ppg_window_add_source_cb (PpgWindow *window,
                          GtkAction *action)
{
	GtkDialog *dialog;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(GTK_IS_ACTION(action));

	dialog = g_object_new(PPG_TYPE_ADD_SOURCE_DIALOG,
	                      "transient-for", window,
	                      NULL);
	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
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

static GtkActionGroup*
ppg_window_create_action_group (PpgWindow *window)
{
	GtkActionGroup *action_group;

	action_group = gtk_action_group_new(NULL);
	ADD_ACTION(action_group, "FileAction", _("Per_fkit"), NULL);
	ADD_STOCK_ACTION(action_group, "QuitAction", GTK_STOCK_QUIT, gtk_main_quit);
	ADD_ACTION(action_group, "EditAction", _("_Edit"), NULL);
	ADD_ACTION(action_group, "ProfileAction", _("_Profile"), NULL);
	ADD_ACTION(action_group, "AddSourceAction", _("Add source"),
	           ppg_window_add_source_cb);
	ADD_ACTION(action_group, "ViewAction", _("View"), NULL);
	ADD_ACTION(action_group, "HelpAction", _("Help"), NULL);

	return action_group;
}

static void
ppg_window_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
	PpgWindowPrivate *priv;

	priv = PPG_WINDOW(object)->priv;
	switch (prop_id) {
	case PROP_SESSION:
		g_value_set_object(value, priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
ppg_window_finalize (GObject *object)
{
	PpgWindowPrivate *priv = PPG_WINDOW(object)->priv;

	if (priv->session) {
		g_object_unref(priv->session);
	}

	G_OBJECT_CLASS(ppg_window_parent_class)->finalize(object);
}

static void
ppg_window_class_init (PpgWindowClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_window_finalize;
	object_class->get_property = ppg_window_get_property;
	g_type_class_add_private(object_class, sizeof(PpgWindowPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->delete_event = ppg_window_delete_event;

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READABLE));

	ppg_actions_register(PPG_TYPE_WINDOW, PPG_TYPE_PAUSE_ACTION);
	ppg_actions_register(PPG_TYPE_WINDOW, PPG_TYPE_RECORD_ACTION);
	ppg_actions_register(PPG_TYPE_WINDOW, PPG_TYPE_RESTART_ACTION);
	ppg_actions_register(PPG_TYPE_WINDOW, PPG_TYPE_STOP_ACTION);
}

static void
ppg_window_init (PpgWindow *window)
{
	PpgWindowPrivate *priv = INIT_PRIV(window, WINDOW, Window);
	GtkActionGroup *action_group;
	GtkWidget *menubar;
	GtkWidget *toolbar;
	GtkWidget *vbox;

	g_object_set(window,
	             "default-width", 1000,
	             "default-height", 620,
	             "window-position", GTK_WIN_POS_CENTER,
	             NULL);

	priv->session = g_object_new(PPG_TYPE_SESSION, NULL);

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

	action_group = ppg_window_create_action_group(window);
	ppg_actions_load_from_ui_string(GTK_WIDGET(window),
	                                action_group,
	                                "<ui>"
	                                " <menubar name=\"Menubar\">"
	                                "  <menu action=\"FileAction\">"
	                                "   <menuitem action=\"QuitAction\"/>"
	                                "  </menu>"
	                                "  <menu action=\"EditAction\">"
	                                "  </menu>"
	                                "  <menu action=\"ViewAction\">"
	                                "  </menu>"
	                                "  <menu action=\"ProfileAction\">"
	                                "   <menuitem action=\"AddSourceAction\"/>"
	                                "   <separator/>"
	                                "   <menuitem action=\"StopAction\"/>"
	                                "   <menuitem action=\"PauseAction\"/>"
	                                "   <menuitem action=\"RecordAction\"/>"
	                                "   <menuitem action=\"RestartAction\"/>"
	                                "  </menu>"
	                                "  <menu action=\"HelpAction\">"
	                                "  </menu>"
	                                " </menubar>"
	                                " <toolbar name=\"Toolbar\">"
	                                "  <toolitem action=\"StopAction\"/>"
	                                "  <toolitem action=\"PauseAction\"/>"
	                                "  <toolitem action=\"RecordAction\"/>"
	                                "  <toolitem action=\"RestartAction\"/>"
	                                "  <separator/>"
	                                " </toolbar>"
	                                "</ui>",
	                                -1,
	                                "/Menubar", &menubar,
	                                "/Toolbar", &toolbar,
	                                NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), menubar,
	                                  "expand", FALSE,
	                                  "fill", TRUE,
	                                  "position", 0,
	                                  NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), toolbar,
	                                  "expand", FALSE,
	                                  "fill", TRUE,
	                                  "position", 1,
	                                  NULL);
}
