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

#include <clutter-gtk/clutter-gtk.h>
#include <glib/gi18n.h>
#include <gobject/gvaluecollector.h>
#include <math.h>
#include <uber.h>

#include "ppg-about-dialog.h"
#include "ppg-add-instrument-dialog.h"
#include "ppg-actions.h"
#include "ppg-configure-instrument-dialog.h"
#include "ppg-header.h"
#include "ppg-instrument.h"
#include "ppg-log.h"
#include "ppg-menu-tool-item.h"
#include "ppg-monitor.h"
#include "ppg-prefs-dialog.h"
#include "ppg-restart-task.h"
#include "ppg-row.h"
#include "ppg-ruler.h"
#include "ppg-runtime.h"
#include "ppg-session.h"
#include "ppg-settings-dialog.h"
#include "ppg-spawn-process-dialog.h"
#include "ppg-status-actor.h"
#include "ppg-timer-tool-item.h"
#include "ppg-util.h"
#include "ppg-visualizer-menu.h"
#include "ppg-window.h"
#include "ppg-window-actions.h"
#include "ppg-window-ui.h"

#define COLUMN_WIDTH      (200.0f)
#define COLUMN_WIDTH_INT  ((gint)COLUMN_WIDTH)
#define PIXELS_PER_SECOND (20.0)
#define LARGER(_s)        ("<span size=\"larger\">" _s "</span>")
#define BOLD(_s)          ("<span weight=\"bold\">" _s "</span>")
#define UPDATE_TIMEOUT    1000
#define SET_ACTION_INSENSITIVE(n)                        \
    G_STMT_START {                                       \
        g_object_set(ppg_window_get_action(window, (n)), \
                     "sensitive", FALSE,                 \
                     NULL);                              \
    } G_STMT_END
#define SET_ACTION_ACTIVE(n)                             \
    G_STMT_START {                                       \
        g_object_set(ppg_window_get_action(window, (n)), \
                     "active", TRUE,                     \
                     NULL);                              \
    } G_STMT_END
#define BEGIN_ACTION_UPDATE                              \
	G_STMT_START {                                       \
		if (window->priv->in_action_update) {            \
			return;                                      \
		}                                                \
		window->priv->in_action_update = TRUE;           \
	} G_STMT_END

#define END_ACTION_UPDATE                                \
	G_STMT_START {                                       \
		window->priv->in_action_update = FALSE;          \
	} G_STMT_END

G_DEFINE_TYPE(PpgWindow, ppg_window, GTK_TYPE_WINDOW)

struct _PpgWindowPrivate
{
	PpgSession *session;

	GtkActionGroup *actions;

	gboolean in_action_update;
	gboolean ignore_ruler;
	gint last_width;
	gint last_height;

	GtkWidget *menubar;
	GtkWidget *toolbar;
	GtkWidget *target_tool_item;
	GtkWidget *timer_tool_item;
	GtkWidget *paned;
	GtkWidget *statusbar;
	GtkWidget *position_label;
	GtkWidget *zoom_scale;
	GtkWidget *clutter_embed;
	GtkWidget *ruler;
	GtkWidget *settings_dialog;
	GtkWidget *instrument_popup;
	GtkWidget *visualizers_menu;

	GtkAdjustment *hadj;
	GtkAdjustment *vadj;
	GtkAdjustment *zadj;

	ClutterActor *stage;
	ClutterActor *header_bg;
	ClutterActor *header_sep;
	ClutterActor *rows_box;
	ClutterActor *selected;
	ClutterActor *add_instrument_actor;
	ClutterActor *timer_sep;
	ClutterActor *status_actor;

	ClutterLayoutManager *box_layout;
};

enum
{
	PROP_0,
	PROP_SESSION,
	PROP_URI,
	PROP_STATUS_LABEL,
};

static guint instances = 0;

/**
 * ppg_window_count:
 *
 * Counts the number of active PpgWindows that are active.
 *
 * Returns: An unsigned intenger.
 * Side effects: None.
 */
guint
ppg_window_count (void)
{
	return instances;
}

/**
 * ppg_window_get_action:
 * @window: (in): A #PpgWindow.
 * @name: (in): The actions name.
 *
 * Retrieves the first #GtkAction with a name matching @name. If no action
 * can be found matching @name, then %NULL is returned.
 *
 * Returns: A #GtkAction if successful; otherwise %NULL.
 * Side effects: None.
 */
GtkAction*
ppg_window_get_action (PpgWindow   *window,
                       const gchar *name)
{
	PpgWindowPrivate *priv;

	g_return_val_if_fail(PPG_IS_WINDOW(window), NULL);
	g_return_val_if_fail(name != NULL, NULL);

	priv = window->priv;
	return gtk_action_group_get_action(priv->actions, name);
}

/**
 * ppg_window_stop_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "stop" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_stop_activate (GtkAction *action,
                          PpgWindow *window)
{
	PpgWindowPrivate *priv;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	BEGIN_ACTION_UPDATE;

	g_object_set(ppg_window_get_action(window, "run"),
	             "active", FALSE,
	             "sensitive", TRUE,
	             NULL);
	g_object_set(ppg_window_get_action(window, "stop"),
	             "active", TRUE,
	             "sensitive", FALSE,
	             NULL);
	g_object_set(ppg_window_get_action(window, "pause"),
	             "active", FALSE,
	             "sensitive", FALSE,
	             NULL);
	g_object_set(ppg_window_get_action(window, "restart"),
	             "sensitive", FALSE,
	             NULL);

	END_ACTION_UPDATE;

	if (priv->session) {
		ppg_session_stop(priv->session);
	}
}

/**
 * ppg_window_pause_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "pause" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_pause_activate (GtkAction *action,
                           PpgWindow *window)
{
	gboolean active;

	g_object_get(action, "active", &active, NULL);

	if (active) {
		ppg_session_pause(window->priv->session);
	} else {
		ppg_session_unpause(window->priv->session);
	}
}

/**
 * ppg_window_run_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "run" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_run_activate (GtkAction *action,
                         PpgWindow *window)
{
	PpgWindowPrivate *priv;

	ENTRY;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;
	g_object_set(priv->timer_sep,
	             "visible", TRUE,
	             NULL);
	BEGIN_ACTION_UPDATE;
	g_object_set(ppg_window_get_action(window, "run"),
	             "sensitive", FALSE,
	             NULL);
	g_object_set(ppg_window_get_action(window, "stop"),
	             "active", FALSE,
	             "sensitive", TRUE,
	             NULL);
	g_object_set(ppg_window_get_action(window, "pause"),
	             "active", FALSE,
	             "sensitive", TRUE,
	             NULL);
	g_object_set(ppg_window_get_action(window, "restart"),
	             "sensitive", TRUE,
	             NULL);
	END_ACTION_UPDATE;
	ppg_session_start(priv->session);
	EXIT;
}

/**
 * ppg_window_zoom_in_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "zoom-in" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_zoom_in_activate (GtkAction *action,
                             PpgWindow *window)
{
	PpgWindowPrivate *priv;
	gdouble step;
	gdouble value;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	g_object_get(priv->zadj,
	             "step-increment", &step,
	             "value", &value,
	             NULL);
	value += step;
	g_object_set(priv->zadj,
	             "value", value,
	             NULL);
}

/**
 * ppg_window_zoom_out_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "zoom-out" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_zoom_out_activate (GtkAction *action,
                             PpgWindow *window)
{
	PpgWindowPrivate *priv;
	gdouble step;
	gdouble value;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	g_object_get(priv->zadj,
	             "step-increment", &step,
	             "value", &value,
	             NULL);
	value -= step;
	g_object_set(priv->zadj,
	             "value", value,
	             NULL);
}

/**
 * ppg_window_zoom_one_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "zoom-one" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_zoom_one_activate (GtkAction *action,
                             PpgWindow *window)
{
	PpgWindowPrivate *priv;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;
	g_object_set(priv->zadj,
	             "value", 1.0,
	             NULL);
}

/**
 * ppg_window_spawn_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "spawn" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_target_spawn_activate (GtkAction *action,
                                  PpgWindow *window)
{
	PpgWindowPrivate *priv;
	GtkDialog *dialog;
	PpgTask *task;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	dialog = g_object_new(PPG_TYPE_SPAWN_PROCESS_DIALOG,
	                      "session", priv->session,
	                      "transient-for", window,
	                      "visible", TRUE,
	                      NULL);

	if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
		g_object_get(dialog, "task", &task, NULL);
		if (task != NULL) {
			ppg_task_schedule(task);
			g_object_unref(task);
		}
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

/**
 * ppg_window_fullscreen_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "fullscreen" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_fullscreen_activate (GtkAction *action,
                                PpgWindow *window)
{
	PpgWindowPrivate *priv;
	gboolean active;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(GTK_IS_ACTION(action));

	priv = window->priv;

	g_object_get(action,
	             "active", &active,
	             NULL);

	if (active) {
		gtk_window_fullscreen(GTK_WINDOW(window));
	} else {
		gtk_window_unfullscreen(GTK_WINDOW(window));
	}
}

/**
 * ppg_window_about_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "about" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_about_activate (GtkAction *action,
                           PpgWindow *window)
{
	GtkWidget *dialog;

	dialog = g_object_new(PPG_TYPE_ABOUT_DIALOG,
	                      "transient-for", window,
	                      NULL);
	gtk_window_present(GTK_WINDOW(dialog));
}

/**
 * ppg_window_add_instrument_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "add-instrument" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_add_instrument_activate (GtkAction *action,
                                    PpgWindow *window)
{
	PpgWindowPrivate *priv;
	GtkDialog *dialog;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	dialog = g_object_new(PPG_TYPE_ADD_INSTRUMENT_DIALOG,
	                      "session", priv->session,
	                      "transient-for", window,
	                      NULL);

	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

/**
 * ppg_window_configure_instrument_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "configure-instrument" #GtkAction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_configure_instrument_activate (GtkAction *action,
                                          PpgWindow *window)
{
	PpgWindowPrivate *priv;
	PpgInstrument *instrument;
	GtkDialog *dialog;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	if (!priv->selected) {
		CRITICAL(Window, "No selected instrument to configure");
		return;
	}

	g_object_get(priv->selected,
	             "instrument", &instrument,
	             NULL);

	dialog = g_object_new(PPG_TYPE_CONFIGURE_INSTRUMENT_DIALOG,
	                      "session", priv->session,
	                      "instrument", instrument,
	                      "transient-for", window,
	                      NULL);

	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_object_unref(instrument);
}

/**
 * ppg_window_show_graph:
 * @title: (in): The title for the created window.
 * @graph: (in): An #UberGraph.
 * @parent: (in): The GtkWindow transient-for parent.
 *
 * Prepares the widgetry and shows a new GtkWindow containing the graph.
 *
 * Returns: None.
 * Side effects: A new #GtkWindow is presented to the user.
 */
static void
ppg_window_show_graph (const gchar *title,
                       GtkWidget   *graph,
                       GtkWindow   *parent)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *labels;

	window = g_object_new(GTK_TYPE_WINDOW,
	                      "border-width", 12,
	                      "default-height", 300,
	                      "default-width", 640,
	                      "title", title,
	                      "transient-for", parent,
	                      NULL);
	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "spacing", 6,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_container_add(GTK_CONTAINER(vbox), graph);
	labels = uber_graph_get_labels(UBER_GRAPH(graph));
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), labels,
	                                  "expand", FALSE,
	                                  NULL);
	gtk_widget_show(labels);
	gtk_window_present(GTK_WINDOW(window));
}

/**
 * ppg_window_monitor_cpu_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "monitor-cpu" action.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_monitor_cpu_activate (GtkAction *action,
                                 PpgWindow *window)
{
	GtkWidget *graph;

	g_return_if_fail(PPG_IS_WINDOW(window));

	graph = ppg_monitor_cpu_new();
	ppg_window_show_graph(_("CPU Usage"), graph, GTK_WINDOW(window));
}

/**
 * ppg_window_monitor_mem_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "monitor-mem" action.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_monitor_mem_activate (GtkAction *action,
                                 PpgWindow *window)
{
	GtkWidget *graph;

	g_return_if_fail(PPG_IS_WINDOW(window));

	graph = ppg_monitor_mem_new();
	ppg_window_show_graph(_("Memory Usage"), graph, GTK_WINDOW(window));
}

/**
 * ppg_window_monitor_net_activate:
 * @action: (in): A #GtkAction.
 * @window: (in): A #PpgWindow.
 *
 * Handles the "activate" signal for the "monitor-net" action.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_monitor_net_activate (GtkAction *action,
                                 PpgWindow *window)
{
	GtkWidget *graph;

	g_return_if_fail(PPG_IS_WINDOW(window));

	graph = ppg_monitor_net_new();
	ppg_window_show_graph(_("Network Usage"), graph, GTK_WINDOW(window));
}

/**
 * ppg_window_delete_event:
 * @window: (in): A #PpgWindow.
 * @event: (in): A #GdkEvent.  *
 * Handle the delete event for the window and adjust the active window count.
 *
 * Returns: TRUE if the delete-event should be blocked.
 * Side effects: None.
 */
static gboolean
ppg_window_delete_event (GtkWidget   *widget,
                         GdkEventAny *event)
{
	GtkWidgetClass *widget_class;
	gboolean ret = FALSE;

	widget_class = GTK_WIDGET_CLASS(ppg_window_parent_class);
	if (widget_class->delete_event) {
		ret = widget_class->delete_event(widget, event);
	}

	if (!ret) {
		instances --;
		ppg_runtime_try_quit();
	}

	return ret;
}

/**
 * ppg_window_notify_target:
 * @session: (in): A #PpgSssion.
 * @pspec: (in): A #GParamSpec.
 * @user_data: (in): A #PpgWindow.
 *
 * Handles the notification that the sessions "target" property has changed.
 * The various widgets are updated to reflect this.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_notify_target (PpgSession *session,
                          GParamSpec *pspec,
                          gpointer    user_data)
{
	PpgWindow *window = (PpgWindow *)user_data;
	PpgWindowPrivate *priv;
	gchar *target;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	g_object_get(session,
	             "target", &target,
	             NULL);
	g_object_set(priv->target_tool_item,
	             "label", target,
	             NULL);
	g_free(target);
}

static void
ppg_window_close_activate (GtkAction *action,
                           PpgWindow *window)
{
	PpgWindowPrivate *priv;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(GTK_IS_ACTION(action));

	priv = window->priv;

	instances--;
	gtk_widget_destroy(GTK_WIDGET(window));
	ppg_runtime_try_quit();
}

static void
ppg_window_preferences_activate (GtkAction *action,
                                 PpgWindow *window)
{
	GtkWidget *dialog;

	g_return_if_fail(PPG_IS_WINDOW(window));

	/*
	 * We don't run the preferences dialog like a modal dialog.
	 * It can be edited while the main window is used.
	 */

	dialog = g_object_new(PPG_TYPE_PREFS_DIALOG,
	                      "modal", FALSE,
	                      "transient-for", window,
	                      "visible", TRUE,
	                      NULL);
	gtk_window_present(GTK_WINDOW(dialog));
}

static void
ppg_window_settings_activate (GtkAction *action,
                              PpgWindow *window)
{
	PpgWindowPrivate *priv;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;
	gtk_window_present(GTK_WINDOW(priv->settings_dialog));
}

static void
ppg_window_restart_activate (GtkAction *action,
                             PpgWindow *window)
{
	PpgWindowPrivate *priv;
	PpgTask *task;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(GTK_IS_ACTION(action));

	priv = window->priv;

	if (priv->session) {
		task = g_object_new(PPG_TYPE_RESTART_TASK,
		                    "session", priv->session,
		                    NULL);
		ppg_task_schedule(task);
	}
}

static GList*
ppg_window_get_visualizers (PpgWindow *window)
{
	PpgWindowPrivate *priv;
	PpgInstrument *instrument;
	GList *visualizers = NULL;
	GList *viz_list;
	GList *rows;
	GList *iter;

	g_return_val_if_fail(PPG_IS_WINDOW(window), NULL);

	priv = window->priv;

	rows = clutter_container_get_children(CLUTTER_CONTAINER(priv->rows_box));
	for (iter = rows; iter; iter = iter->next) {
		instrument = ppg_row_get_instrument(PPG_ROW(iter->data));
		viz_list = ppg_instrument_get_visualizers(instrument);
		visualizers = g_list_concat(visualizers, g_list_copy(viz_list));
	}

	return visualizers;
}

/**
 * ppg_window_visualizers_set:
 * @window: (in): A #PpgWindow.
 * @first_property: (in): first property name.
 *
 * Set properties on all visualizers in the window.
 *
 * Returns: None.
 * Side effects: Properties set.
 */
static void
ppg_window_visualizers_set (PpgWindow   *window,
                            const gchar *first_property,
                            ...)
{
	PpgWindowPrivate *priv;
	const gchar *name = first_property;
	GObjectClass *klass;
	GParamSpec *pspec;
	GValue value = { 0 };
	va_list args;
	gchar *error = NULL;
	GList *list;
	GList *iter;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(first_property != NULL);

	priv = window->priv;
	list = ppg_window_get_visualizers(window);
	if (!(klass = g_type_class_peek(PPG_TYPE_VISUALIZER))) {
		return;
	}

	va_start(args, first_property);

	do {
		pspec = g_object_class_find_property(klass, name);
		if (!pspec) {
			g_critical("Failed to find property %s of class %s", name,
			           g_type_name(G_TYPE_FROM_INSTANCE(iter->data)));
			return;
		}
		G_VALUE_COLLECT_INIT(&value, pspec->value_type, args, 0, &error);
		if (error != NULL) {
			g_critical("Failed to extract property %s from var_args: %s",
			           name, error);
			g_free(error);
			goto cleanup;
		}
		for (iter = list; iter; iter = iter->next) {
			g_object_set_property(G_OBJECT(iter->data), name, &value);
		}
		g_value_unset(&value);
	} while ((name = va_arg(args, const gchar*)));

  cleanup:
	va_end(args);
}

static void
ppg_window_position_notify (PpgSession *session,
                            GParamSpec *pspec,
                            PpgWindow  *window)
{
	PpgWindowPrivate *priv;
	gdouble position;
	gdouble lower;
	gdouble upper;
	gdouble ratio;
	gfloat width;
	gfloat x;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	g_object_get(priv->ruler,
	             "lower", &lower,
	             "upper", &upper,
	             NULL);
	position = ppg_session_get_position(session);
	g_object_get(priv->rows_box, "width", &width, NULL);
	width -= 200.0f;

	upper -= lower;
	position -= lower;

	if (position < 0 || position > upper) {
		/* FIXME: need to adjust hadjustment */
	}

	ratio = (position / upper);
	x = (gint)(200.0f + (ratio * width));

	g_object_set(priv->timer_sep, "x", x, NULL);
}

static void
ppg_window_zoom_value_changed (GtkAdjustment *adjustment,
                               PpgWindow     *window)
{
	PpgWindowPrivate *priv;
	GtkAllocation alloc;
	gdouble lower;
	gdouble upper;
	gdouble scale;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	if (!GTK_WIDGET_REALIZED(priv->clutter_embed)) {
		return;
	}

	gtk_widget_get_allocation(priv->clutter_embed, &alloc);
	g_object_get(adjustment,
	             "value", &scale,
	             NULL);

	scale = CLAMP(scale, 0.0, 2.0);
	if (scale > 1.0) {
		scale = (scale - 1.0) * 100.0;
	}
	scale = CLAMP(scale, 0.001, 100.0);
	lower = 0.0f; /* FIXME: */
	upper = lower + ((alloc.width - 200.0) / (scale * PIXELS_PER_SECOND));

	priv->ignore_ruler = TRUE;
	ppg_ruler_set_range(PPG_RULER(priv->ruler), lower, upper, lower);
	priv->ignore_ruler = FALSE;

	ppg_window_visualizers_set(window,
	                           "begin", lower,
	                           "end", upper,
	                           NULL);

	ppg_window_position_notify(priv->session, NULL, window);
}

static void
ppg_window_vadj_value_changed (GtkAdjustment *adj,
                               PpgWindow *window)
{
	PpgWindowPrivate *priv;
	gfloat value;

	g_return_if_fail(GTK_IS_ADJUSTMENT(adj));
	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	value = gtk_adjustment_get_value(adj);
	clutter_actor_set_y(priv->rows_box, -(gint)value);
}

static void
ppg_window_size_allocate (GtkWidget     *widget,
                          GtkAllocation *alloc)
{
	PpgWindow *window = (PpgWindow *)widget;
	PpgWindowPrivate *priv;
	GtkWidgetClass *widget_class;
	GtkAllocation embed_alloc;
	gfloat width;
	gfloat height;

	g_return_if_fail(PPG_IS_WINDOW(widget));

	priv = window->priv;

	widget_class = GTK_WIDGET_CLASS(ppg_window_parent_class);
	if (widget_class->size_allocate) {
		widget_class->size_allocate(widget, alloc);
	}

	if (priv->last_width != alloc->width || priv->last_height != alloc->height) {
		gtk_widget_get_allocation(priv->clutter_embed, &embed_alloc);

		clutter_actor_get_size(priv->add_instrument_actor, &width, &height);
		clutter_actor_set_x(priv->add_instrument_actor,
		                    200.0f + floor((embed_alloc.width - 200.0f - width) / 2));
		clutter_actor_set_y(priv->add_instrument_actor,
		                    floor((embed_alloc.height - height) / 2));

		clutter_actor_set_height(priv->header_bg, embed_alloc.height);
		clutter_actor_set_height(priv->header_sep, embed_alloc.height);
		clutter_actor_set_width(priv->rows_box, embed_alloc.width);
		clutter_actor_set_height(priv->timer_sep, embed_alloc.height);
		clutter_actor_set_y(priv->status_actor,
		                    embed_alloc.height - clutter_actor_get_height(priv->status_actor));

		g_object_set(priv->vadj,
		             "page-size", (gdouble)embed_alloc.height,
		             NULL);

		ppg_window_zoom_value_changed(priv->zadj, window);
	}

	priv->last_width = alloc->width;
	priv->last_height = alloc->height;
}

static void
ppg_window_unselect_row (PpgWindow *window)
{
	PpgWindowPrivate *priv;
	GtkAction *action;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	if (priv->selected) {
		g_object_set(priv->selected,
		             "selected", FALSE,
		             NULL);
		priv->selected = NULL;
	}

	action = ppg_window_get_action(window, "configure-instrument");
	g_object_set(action, "sensitive", FALSE, NULL);
}

static void
ppg_window_select_row (PpgWindow *window,
                       PpgRow    *row)
{
	PpgWindowPrivate *priv;
	PpgInstrument *instrument;
	GtkAction *action;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(!row || PPG_IS_ROW(row));

	priv = window->priv;

	if (priv->selected == CLUTTER_ACTOR(row)) {
		return;
	}

	ppg_window_unselect_row(window);

	if (row) {
		priv->selected = CLUTTER_ACTOR(row);
		g_object_set(priv->selected,
		             "selected", TRUE,
		             NULL);

		action = ppg_window_get_action(window, "configure-instrument");
		g_object_set(action,
		             "sensitive", TRUE,
		             NULL);

		g_object_get(priv->selected,
		             "instrument", &instrument,
		             NULL);
		g_object_set(priv->visualizers_menu,
		             "instrument", instrument,
		             NULL);
		g_object_unref(instrument);
	}
}

static gboolean
ppg_window_row_button_press (ClutterActor       *actor,
                             ClutterButtonEvent *event,
                             PpgWindow          *window)
{
	PpgWindowPrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(PPG_IS_WINDOW(window), FALSE);

	priv = window->priv;

	switch (event->button) {
	case 1:
		if (event->click_count == 1) {
			if (event->modifier_state & CLUTTER_CONTROL_MASK) {
				ppg_window_unselect_row(window);
			} else {
				ppg_window_select_row(window, PPG_ROW(actor));
			}
		}
		break;
	case 3:
		if (event->click_count == 1) {
			ppg_window_select_row(window, PPG_ROW(actor));
			gtk_menu_popup(GTK_MENU(priv->instrument_popup), NULL, NULL,
			               NULL, NULL, 3, event->time);
		}
		break;
	default:
		break;
	}

	if (ret) {
		gtk_widget_grab_focus(priv->clutter_embed);
	}

	return ret;
}

static void
ppg_window_set_status_label (PpgWindow   *window,
                             const gchar *label)
{
	PpgWindowPrivate *priv;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;
	label = label ? label : "";

	g_object_set(priv->status_actor,
	             "label", label,
	             NULL);
}

static void
ppg_window_add_row (PpgWindow *window,
                    PpgRow    *row)
{
	PpgWindowPrivate *priv;
	GtkStyle *style;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(PPG_IS_ROW(row));

	priv = window->priv;
	style = gtk_widget_get_style(GTK_WIDGET(window));

	clutter_box_layout_pack(CLUTTER_BOX_LAYOUT(priv->box_layout),
	                        CLUTTER_ACTOR(row), TRUE, TRUE, FALSE,
	                        CLUTTER_BOX_ALIGNMENT_START,
	                        CLUTTER_BOX_ALIGNMENT_START);

	if (style) {
		g_object_set(row, "style", style, NULL);
	}

	g_signal_connect(row,
	                 "button-press-event",
	                 G_CALLBACK(ppg_window_row_button_press),
	                 window);
}

/**
 * ppg_window_instrument_added:
 * @window: (in): A #PpgWindow.
 *
 * Handle the "instrument-added" event. Add the instruments visualizers to
 * the data view.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_instrument_added (PpgSession *session,
                             PpgInstrument *instrument,
                             PpgWindow *window)
{
	PpgWindowPrivate *priv;
	gdouble lower;
	gdouble upper;
	PpgRow *row;

	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;
	row = g_object_new(PPG_TYPE_ROW,
	                   "instrument", instrument,
	                   "window", window,
	                   NULL);
	ppg_window_add_row(window, row);
	g_object_get(priv->ruler,
	             "lower", &lower,
	             "upper", &upper,
	             NULL);
	ppg_window_visualizers_set(window,
	                           "begin", lower,
	                           "end", upper,
	                           NULL);
	if (CLUTTER_ACTOR_IS_VISIBLE(priv->add_instrument_actor)) {
		clutter_actor_hide(priv->add_instrument_actor);
	}
}

static void
ppg_window_set_row_style (ClutterActor *actor,
                          gpointer      style)
{
	g_object_set(actor, "style", style, NULL);
}

static void
ppg_window_style_set (GtkWidget *widget,
                      GtkStyle  *old_style)
{
	PpgWindowPrivate *priv;
	PpgWindow *window = (PpgWindow *)widget;
	GtkWidgetClass *widget_class;
	ClutterColor mid;
	ClutterColor dark;
	ClutterColor bg;
	GtkStyle *style;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	widget_class = GTK_WIDGET_CLASS(ppg_window_parent_class);
	if (widget_class->style_set) {
		widget_class->style_set(widget, old_style);
	}

	style = gtk_widget_get_style(widget);

	g_object_set(priv->status_actor,
	             "style", style,
	             NULL);

	bg.alpha = 0xFF;
	bg.red = style->bg[GTK_STATE_NORMAL].red / 256;
	bg.green = style->bg[GTK_STATE_NORMAL].green / 256;
	bg.blue = style->bg[GTK_STATE_NORMAL].blue / 256;
	g_object_set(priv->stage,
	             "color", &bg,
	             NULL);

	gtk_clutter_get_mid_color(widget, GTK_STATE_NORMAL, &mid);
	gtk_clutter_get_dark_color(widget, GTK_STATE_NORMAL, &dark);

	g_object_set(priv->header_bg, "color", &mid, NULL);
	g_object_set(priv->header_sep, "color", &dark, NULL);

	clutter_container_foreach(CLUTTER_CONTAINER(priv->rows_box),
	                          ppg_window_set_row_style,
	                          gtk_widget_get_style(widget));
}

static gboolean
force_redraw (gpointer data)
{
	PpgWindow *window = (PpgWindow *)data;
	PpgWindowPrivate *priv = window->priv;
	PpgInstrument *instrument;
	GList *rows;
	GList *iter;
	GList *viz;
	GList *viter;

	rows = clutter_container_get_children(CLUTTER_CONTAINER(priv->rows_box));
	for (iter = rows; iter; iter = iter->next) {
		g_object_get(iter->data,
		             "instrument", &instrument,
		             NULL);
		viz = ppg_instrument_get_visualizers(instrument);
		for (viter = viz; viter; viter = viter->next) {
			ppg_visualizer_queue_draw(viter->data);
		}
		g_object_unref(instrument);
	}

	return TRUE;
}

/**
 * ppg_window_set_uri:
 * @window: (in): A #PpgWindow.
 * @uri: (in): The uri to the agent.
 *
 * Sets the URI for the session.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_set_uri (PpgWindow   *window,
                    const gchar *uri)
{
	PpgWindowPrivate *priv;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(uri != NULL);

	priv = window->priv;

	priv->session = g_object_new(PPG_TYPE_SESSION,
	                             "uri", uri,
	                             NULL);

	/*
	 * FIXME: Check for reported errors on the session.
	 */

	g_signal_connect(priv->session, "notify::target",
	                 G_CALLBACK(ppg_window_notify_target),
	                 window);
	g_signal_connect(priv->session, "instrument-added",
	                 G_CALLBACK(ppg_window_instrument_added),
	                 window);
	g_signal_connect(priv->session, "notify::position",
	                 G_CALLBACK(ppg_window_position_notify),
	                 window);

	g_object_set(priv->timer_tool_item,
	             "session", priv->session,
	             NULL);

	g_timeout_add(UPDATE_TIMEOUT, force_redraw, window);
}

static gboolean
ppg_window_embed_key_press (GtkWidget   *embed,
                            GdkEventKey *key,
                            PpgWindow   *window)
{
	return FALSE;
}

static void
ppg_window_ruler_position_notify (PpgRuler   *ruler,
                                  GParamSpec *pspec,
                                  PpgWindow  *window)
{
	PpgWindowPrivate *priv;
	gdouble pos;
	gdouble frac;
	gdouble dummy;
	gchar *label;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	if (priv->ignore_ruler) {
		return;
	}

	g_object_get(priv->ruler, "position", &pos, NULL);
	frac = modf(pos, &dummy);
	label = g_strdup_printf("%02d:%02d:%02d.%04d",
	                        (gint)(pos / 3600.0),
	                        (gint)(((gint)pos % 3600) / 60.0),
	                        (gint)((gint)pos % 60),
	                        (gint)(frac * 10000));
	g_object_set(priv->position_label, "label", label, NULL);
	g_free(label);
}

static gboolean
ppg_window_embed_motion_notify (GtkWidget      *embed,
                                GdkEventMotion *motion,
                                PpgWindow      *window)
{
	PpgWindowPrivate *priv;
	GtkAllocation alloc;
	gdouble x = motion->x - 200.0f;
	gdouble width;
	gdouble ratio;
	gdouble lower;
	gdouble upper;
	gdouble pos;

	priv = window->priv;

	if (x > 0) {
		gtk_widget_get_allocation(embed, &alloc);
		width = alloc.width - 200.0;
		g_object_get(priv->ruler,
		             "lower", &lower,
		             "upper", &upper,
		             NULL);
		ratio = x / width;
		pos = lower + ((upper - lower) * ratio);
		g_object_set(priv->ruler, "position", pos, NULL);
	} else {
		g_object_get(priv->ruler, "lower", &lower, NULL);
		g_object_set(priv->ruler, "position", lower, NULL);
	}

	return FALSE;
}

static void
ppg_window_show (GtkWidget *widget)
{
	PpgWindow *window = (PpgWindow *)widget;
	PpgWindowPrivate *priv;

	g_return_if_fail(PPG_IS_WINDOW(window));

	GTK_WIDGET_CLASS(ppg_window_parent_class)->show(widget);

	priv = window->priv;
	ppg_window_zoom_value_changed(priv->zadj, window);
}

static void
ppg_window_realize (GtkWidget *widget)
{
	GtkWidgetClass *klass;
	GdkGeometry geom = { 640, 300 };

	klass = GTK_WIDGET_CLASS(ppg_window_parent_class);
	klass->realize(widget);

	gtk_window_set_geometry_hints(GTK_WINDOW(widget), widget,
	                              &geom, GDK_HINT_MIN_SIZE);
}

static void
ppg_window_rows_notify_allocation (ClutterActor *actor,
                                   GParamSpec *pspec,
                                   PpgWindow *window)
{
	PpgWindowPrivate *priv;
	gfloat height;
	gfloat y;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	g_object_get(actor,
	             "height", &height,
	             "y", &y,
	             NULL);

	if (height > 0.0) {
		g_object_set(priv->vadj,
		             "upper", (gdouble)height,
		             NULL);
	}

	g_object_set(priv->vadj,
	             "value", (gdouble)-y,
	             NULL);
}

/**
 * ppg_window_finalize:
 * @object: (in): A #PpgWindow.
 *
 * Finalizer for a #PpgWindow instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_window_parent_class)->finalize(object);
}

/**
 * ppg_window_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_window_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
	PpgWindow *window = PPG_WINDOW(object);

	switch (prop_id) {
	case PROP_SESSION:
		g_value_set_object(value, window->priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_window_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_window_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
	PpgWindow *window = PPG_WINDOW(object);

	switch (prop_id) {
	case PROP_URI:
		ppg_window_set_uri(window, g_value_get_string(value));
		break;
	case PROP_STATUS_LABEL:
		ppg_window_set_status_label(window, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_window_class_init:
 * @klass: (in): A #PpgWindowClass.
 *
 * Initializes the #PpgWindowClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_class_init (PpgWindowClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_window_finalize;
	object_class->get_property = ppg_window_get_property;
	object_class->set_property = ppg_window_set_property;
	g_type_class_add_private(object_class, sizeof(PpgWindowPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->delete_event = ppg_window_delete_event;
	widget_class->realize = ppg_window_realize;
	widget_class->show = ppg_window_show;
	widget_class->size_allocate = ppg_window_size_allocate;
	widget_class->style_set = ppg_window_style_set;

	g_object_class_install_property(object_class,
	                                PROP_URI,
	                                g_param_spec_string("uri",
	                                                    "uri",
	                                                    "uri",
	                                                    NULL,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READABLE));

	g_object_class_install_property(object_class,
	                                PROP_STATUS_LABEL,
	                                g_param_spec_string("status-label",
	                                                    "status-label",
	                                                    "status-label",
	                                                    NULL,
	                                                    G_PARAM_WRITABLE));

	ppg_actions_register_entries(PPG_TYPE_WINDOW,
	                             ppg_window_action_entries,
	                             G_N_ELEMENTS(ppg_window_action_entries));

	ppg_actions_register_toggle_entries(PPG_TYPE_WINDOW,
	                                    ppg_window_toggle_action_entries,
	                                    G_N_ELEMENTS(ppg_window_toggle_action_entries));
}

/**
 * ppg_window_init:
 * @window: (in): A #PpgWindow.
 *
 * Initializes the newly created #PpgWindow instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_init (PpgWindow *window)
{
	PpgWindowPrivate *priv;
	PangoAttrList *attrs;
	ClutterColor black;
	GtkWidget *align;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *table;
	GtkWidget *scroll;
	GtkWidget *header;
	GtkWidget *image;
	GtkWidget *target_menu;
	GtkWidget *status_vbox;
	GtkWidget *status_hbox;
	GtkWidget *visualizers;
	GtkWidget *mb_visualizers;

	instances++;

	window->priv = G_TYPE_INSTANCE_GET_PRIVATE(window, PPG_TYPE_WINDOW,
	                                           PpgWindowPrivate);
	priv = window->priv;

	g_object_set(window,
	             "title", _(PRODUCT_NAME),
	             "default-width", 800,
	             "default-height", 494,
	             "window-position", GTK_WIN_POS_CENTER,
	             NULL);

	ppg_util_load_ui(GTK_WIDGET(window), &priv->actions, ppg_window_ui,
	                 "/menubar", &priv->menubar,
	                 "/menubar/instrument/visualizers", &mb_visualizers,
	                 "/toolbar", &priv->toolbar,
	                 "/target-popup", &target_menu,
	                 "/instrument-popup", &priv->instrument_popup,
	                 "/instrument-popup/visualizers", &visualizers,
	                 NULL);

	SET_ACTION_ACTIVE("stop");
	SET_ACTION_INSENSITIVE("stop");
	SET_ACTION_INSENSITIVE("pause");
	SET_ACTION_INSENSITIVE("restart");
	SET_ACTION_INSENSITIVE("cut");
	SET_ACTION_INSENSITIVE("copy");
	SET_ACTION_INSENSITIVE("paste");
	SET_ACTION_INSENSITIVE("configure-instrument");

	priv->vadj = g_object_new(GTK_TYPE_ADJUSTMENT,
	                          "lower", 0.0,
	                          "page-size", 1.0,
	                          "upper", 1.0,
	                          "value", 0.0,
	                          NULL);
	g_signal_connect(priv->vadj, "value-changed",
	                 G_CALLBACK(ppg_window_vadj_value_changed),
	                 window);

	priv->hadj = g_object_new(GTK_TYPE_ADJUSTMENT, NULL);

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), priv->menubar,
	                                  "position", 0,
	                                  "expand", FALSE,
	                                  NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), priv->toolbar,
	                                  "position", 1,
	                                  "expand", FALSE,
	                                  NULL);

	priv->timer_tool_item = g_object_new(PPG_TYPE_TIMER_TOOL_ITEM,
	                                     "visible", TRUE,
	                                     "width-request", 250,
	                                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->toolbar),
	                                  priv->timer_tool_item,
	                                  "expand", TRUE,
	                                  NULL);

	priv->target_tool_item = g_object_new(PPG_TYPE_MENU_TOOL_ITEM,
	                                      "label", _("Select target ..."),
	                                      "menu", target_menu,
	                                      "visible", TRUE,
	                                      "width-request", 175,
	                                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->toolbar),
	                                  priv->target_tool_item,
	                                  "expand", FALSE,
	                                  "homogeneous", FALSE,
	                                  NULL);

	priv->paned = g_object_new(GTK_TYPE_HPANED,
	                           "visible", TRUE,
	                           NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), priv->paned,
	                                  "position", 2,
	                                  NULL);

	table = g_object_new(GTK_TYPE_TABLE,
	                     "n-columns", 3,
	                     "n-rows", 4,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(priv->paned), table);

	scroll = g_object_new(GTK_TYPE_VSCROLLBAR,
	                      "adjustment", priv->vadj,
	                      "visible", TRUE,
	                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), scroll,
	                                  "bottom-attach", 2,
	                                  "left-attach", 2,
	                                  "right-attach", 3,
	                                  "top-attach", 0,
	                                  "x-options", GTK_FILL,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	header = g_object_new(PPG_TYPE_HEADER,
	                      "bottom-separator", TRUE,
	                      "height-request", 32,
	                      "right-separator", FALSE,
	                      "visible", TRUE,
	                      "width-request", COLUMN_WIDTH_INT,
	                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), header,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 0,
	                                  "bottom-attach", 1,
	                                  "x-options", GTK_FILL,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	priv->ruler = g_object_new(PPG_TYPE_RULER,
	                           "bottom-separator", TRUE,
	                           "right-separator", FALSE,
	                           "visible", TRUE,
	                           "lower", 0.0,
	                           "upper", 100.0,
	                           "position", 20.0,
	                           NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->ruler,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 0,
	                                  "bottom-attach", 1,
	                                  "y-options", GTK_FILL,
	                                  NULL);
	g_signal_connect(priv->ruler,
	                 "notify::position",
	                 G_CALLBACK(ppg_window_ruler_position_notify),
	                 window);

	status_vbox = g_object_new(GTK_TYPE_VBOX,
	                           "visible", TRUE,
	                           NULL);
	align = g_object_new(GTK_TYPE_ALIGNMENT,
	                     "child", status_vbox,
	                     "xscale", 1.0f,
	                     "xalign", 0.5f,
	                     "yalign", 0.0f,
	                     "yscale", 1.0f,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), align,
	                                  "bottom-attach", 5,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 2,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	scroll = g_object_new(GTK_TYPE_HSCROLLBAR,
	                      "visible", TRUE,
	                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(status_vbox), scroll,
	                                  "expand", FALSE,
	                                  NULL);

	status_hbox = g_object_new(GTK_TYPE_HBOX,
	                           "spacing", 3,
	                           "visible", TRUE,
	                           NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(status_vbox), status_hbox,
	                                  "expand", TRUE,
	                                  "padding", 3,
	                                  NULL);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "spacing", 3,
	                    "visible", TRUE,
	                    "width-request", COLUMN_WIDTH_INT,
	                    NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), hbox,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 2,
	                                  "bottom-attach", 4,
	                                  "x-options", GTK_FILL,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	image = g_object_new(GTK_TYPE_IMAGE,
	                     "icon-name", GTK_STOCK_ZOOM_OUT,
	                     "icon-size", GTK_ICON_SIZE_MENU,
	                     "visible", TRUE,
	                     "xpad", 3,
	                     "yalign", 0.0f,
	                     "ypad", 6,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), image,
	                                  "expand", FALSE,
	                                  "padding", 3,
	                                  "position", 0,
	                                  NULL);

	priv->zadj = g_object_new(GTK_TYPE_ADJUSTMENT,
	                          "lower", 0.0,
	                          "upper", 2.0,
	                          "step-increment", 0.025,
	                          "page-increment", 0.5,
	                          "page-size", 0.0,
	                          "value", 1.0,
	                          NULL);
	g_signal_connect(priv->zadj, "value-changed",
	                 G_CALLBACK(ppg_window_zoom_value_changed),
	                 window);

	priv->zoom_scale = g_object_new(GTK_TYPE_HSCALE,
	                                "adjustment", priv->zadj,
	                                "draw-value", FALSE,
	                                "visible", TRUE,
	                                NULL);
	gtk_widget_set_tooltip_text(priv->zoom_scale,
	                            _("Adjust the zoom level of the displayed data"));
	gtk_scale_add_mark(GTK_SCALE(priv->zoom_scale), 1.0, GTK_POS_BOTTOM, NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), priv->zoom_scale,
	                                  "position", 1,
	                                  NULL);

	image = g_object_new(GTK_TYPE_IMAGE,
	                     "icon-name", GTK_STOCK_ZOOM_IN,
	                     "icon-size", GTK_ICON_SIZE_MENU,
	                     "visible", TRUE,
	                     "xpad", 3,
	                     "yalign", 0.0f,
	                     "ypad", 6,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), image,
	                                  "expand", FALSE,
	                                  "padding", 3,
	                                  "position", 2,
	                                  NULL);

	priv->statusbar = g_object_new(GTK_TYPE_STATUSBAR,
	                               "visible", FALSE,
	                               NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), priv->statusbar,
	                                  "expand", FALSE,
	                                  "position", 3,
	                                  NULL);

	attrs = pango_attr_list_new();
	pango_attr_list_insert(attrs, pango_attr_size_new(8 * PANGO_SCALE));
	pango_attr_list_insert(attrs, pango_attr_family_new("Monospace"));
	priv->position_label = g_object_new(GTK_TYPE_LABEL,
	                                    "attributes", attrs,
	                                    "label", "00:00:00.0000",
	                                    "single-line-mode", TRUE,
	                                    "use-markup", FALSE,
	                                    "visible", TRUE,
	                                    "xpad", 3,
	                                    "ypad", 3,
	                                    "yalign", 0.5f,
	                                    NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(status_hbox),
	                                  priv->position_label,
	                                  "expand", FALSE,
	                                  "pack-type", GTK_PACK_END,
	                                  NULL);
	pango_attr_list_unref(attrs);

	priv->clutter_embed = g_object_new(GTK_CLUTTER_TYPE_EMBED,
	                                   "visible", TRUE,
	                                   NULL);
	gtk_widget_add_events(priv->clutter_embed,
	                      GDK_POINTER_MOTION_MASK);
	priv->stage = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(priv->clutter_embed));
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->clutter_embed,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 1,
	                                  "bottom-attach", 2,
	                                  NULL);
	g_signal_connect(priv->clutter_embed, "key-press-event",
	                 G_CALLBACK(ppg_window_embed_key_press),
	                 window);
	g_signal_connect(priv->clutter_embed, "motion-notify-event",
	                 G_CALLBACK(ppg_window_embed_motion_notify),
	                 window);

	clutter_color_from_string(&black, "#000000");

	priv->header_bg = g_object_new(CLUTTER_TYPE_RECTANGLE,
	                               "color", &black,
	                               "width", COLUMN_WIDTH,
	                               "height", 1.0f,
	                               NULL);
	priv->header_sep = g_object_new(CLUTTER_TYPE_RECTANGLE,
	                                "color", &black,
	                                "width", 1.0f,
	                                "height", 1.0f,
	                                "x", (COLUMN_WIDTH - 1.0f),
	                                NULL);
	priv->box_layout = g_object_new(CLUTTER_TYPE_BOX_LAYOUT,
	                                "use-animations", FALSE,
	                                "vertical", TRUE,
	                                NULL);
	priv->rows_box = g_object_new(CLUTTER_TYPE_BOX,
	                              "layout-manager", priv->box_layout,
	                              NULL);
	g_signal_connect(priv->rows_box, "notify::allocation",
	                 G_CALLBACK(ppg_window_rows_notify_allocation),
	                 window);
	priv->add_instrument_actor = g_object_new(CLUTTER_TYPE_TEXT,
	                                          "text", _(LARGER("Add an instrument to get started")),
	                                          "use-markup", TRUE,
	                                          NULL);
	priv->timer_sep = g_object_new(CLUTTER_TYPE_RECTANGLE,
	                               "color", &black,
	                               "height", 1.0f,
	                               "visible", FALSE,
	                               "width", 1.0f,
	                               "x", 200.0f,
	                               NULL);
	priv->status_actor = g_object_new(PPG_TYPE_STATUS_ACTOR,
	                                  "width", 320.0f,
	                                  "height", 24.0f,
	                                  "opacity", 0,
	                                  "y", 200.0f,
	                                  NULL);

	clutter_container_add(CLUTTER_CONTAINER(priv->stage),
	                      priv->header_bg,
	                      priv->header_sep,
	                      priv->rows_box,
	                      priv->add_instrument_actor,
	                      priv->timer_sep,
	                      priv->status_actor,
	                      NULL);

	priv->settings_dialog = g_object_new(PPG_TYPE_SETTINGS_DIALOG,
	                                     "session", priv->session,
	                                     "visible", FALSE,
	                                     NULL);
	g_signal_connect(priv->settings_dialog, "response",
	                 G_CALLBACK(gtk_widget_hide),
	                 NULL);
	g_signal_connect(priv->settings_dialog, "delete-event",
	                 G_CALLBACK(gtk_widget_hide_on_delete),
	                 NULL);

	priv->visualizers_menu = g_object_new(PPG_TYPE_VISUALIZER_MENU, NULL);
	g_object_set(visualizers,
	             "submenu", priv->visualizers_menu,
	             "visible", TRUE,
	             NULL);
	g_object_set(mb_visualizers,
	             "submenu", priv->visualizers_menu,
	             "visible", TRUE,
	             NULL);

	gtk_widget_grab_focus(priv->clutter_embed);
}
