/* ppg-welcome-dialog.c
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

#include "ppg-runtime.h"
#include "ppg-sessions-store.h"
#include "ppg-welcome-dialog.h"
#include "ppg-window.h"

G_DEFINE_TYPE(PpgWelcomeDialog, ppg_welcome_dialog, GTK_TYPE_WINDOW)

static guint instances = 0;

struct _PpgWelcomeDialogPrivate
{
	GtkListStore *sessions;
};

/**
 * ppg_welcome_dialog_new:
 *
 * Creates a new instance of #PpgWelcomeDialog.
 *
 * Returns: the newly created instance of #PpgWelcomeDialog.
 * Side effects: None.
 */
GtkWidget*
ppg_welcome_dialog_new (void)
{
	return g_object_new(PPG_TYPE_WELCOME_DIALOG, NULL);
}

/**
 * ppg_welcome_dialog_row_separator_func:
 * @model: (in): A #GtkTreeModel.
 * @iter: (in): A #GtkTreeIter.
 * @user_data: (in): User data provided.
 *
 * Determines if the row is a tree sepatator.
 *
 * Returns: %TRUE if the row is a separator.
 * Side effects: None.
 */
static gboolean
ppg_welcome_dialog_row_separator_func (GtkTreeModel *model,
                                       GtkTreeIter  *iter,
                                       gpointer      user_data)
{
	gboolean sep;

	gtk_tree_model_get(model, iter,
	                   PPG_SESSIONS_STORE_COLUMN_SEPARATOR, &sep,
	                   -1);
	return sep;
}

static GtkWidget*
ppg_welcome_dialog_create_button (PpgWelcomeDialog *dialog,
                                  const gchar      *icon_name,
                                  gint              icon_size,
                                  const gchar      *label)
{
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *l;
	GtkWidget *hbox;

	button = g_object_new(GTK_TYPE_BUTTON,
	                      "relief", GTK_RELIEF_NONE,
	                      "visible", TRUE,
	                      NULL);
	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "spacing", 6,
	                    "visible", TRUE,
	                    NULL);
	image = g_object_new(GTK_TYPE_IMAGE,
	                     "icon-name", icon_name,
	                     "icon-size", icon_size,
	                     "visible", TRUE,
	                     NULL);
	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", label,
	                 "visible", TRUE,
	                 "wrap", TRUE,
	                 "width-request", 260,
	                 "xalign", 0.0f,
	                 NULL);

	gtk_container_add(GTK_CONTAINER(button), hbox);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), image,
	                                  "position", 0,
	                                  "expand", FALSE,
	                                  NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), l,
	                                  "position", 1,
	                                  NULL);

	return button;
}

/**
 * ppg_welcome_dialog_tutorials_clicked:
 * @button: (in): A #GtkButton.
 *
 * Handles the "show video tutorials" button clicked event. Opens
 * the tutorials website in a browser window.
 *
 * Returns: None.
 * Side effects: A webbrowser is launched.
 */
static void
ppg_welcome_dialog_tutorials_clicked (GtkWidget *button)
{
	GdkScreen *screen;

	screen = gtk_widget_get_screen(button);
	gtk_show_uri(screen, "http://perfkit.org/tutorials",
	             gtk_get_current_event_time(), NULL);
}

static void
ppg_welcome_dialog_local_clicked (GtkWidget        *button,
                                  PpgWelcomeDialog *dialog)
{
	PpgWelcomeDialogPrivate *priv;
	GtkWidget *window;

	g_return_if_fail(PPG_IS_WELCOME_DIALOG(dialog));

	priv = dialog->priv;

	window = g_object_new(PPG_TYPE_WINDOW,
	                      "uri", "dbus:///",
	                      "visible", TRUE,
	                      NULL);
	gtk_widget_destroy(GTK_WIDGET(dialog));
	gtk_window_present(GTK_WINDOW(window));
}

/**
 * ppg_welcome_dialog_count:
 *
 * Count the number of PpgWelcomeDialog widgets in use.
 *
 * Returns: None.
 * Side effects: None.
 */
guint
ppg_welcome_dialog_count (void)
{
	return instances;
}

/**
 * ppg_welcome_dialog_delete_event:
 * @dialog: (in): A #PpgWelcomeDialog.
 *
 * Handle the delete event for the window. Try to close the application
 * if no windows are left.
 *
 * Returns: %TRUE if the application should block the event.
 * Side effects: None.
 */
static gboolean
ppg_welcome_dialog_delete_event (GtkWidget   *widget,
                                 GdkEventAny *event)
{
	GtkWidgetClass *widget_class;
	gboolean ret = FALSE;

	widget_class = GTK_WIDGET_CLASS(ppg_welcome_dialog_parent_class);
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
 * ppg_welcome_dialog_finalize:
 * @object: (in): A #PpgWelcomeDialog.
 *
 * Finalizer for a #PpgWelcomeDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_welcome_dialog_finalize (GObject *object)
{
	instances --;

	G_OBJECT_CLASS(ppg_welcome_dialog_parent_class)->finalize(object);
}

/**
 * ppg_welcome_dialog_class_init:
 * @klass: (in): A #PpgWelcomeDialogClass.
 *
 * Initializes the #PpgWelcomeDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_welcome_dialog_class_init (PpgWelcomeDialogClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_welcome_dialog_finalize;
	g_type_class_add_private(object_class, sizeof(PpgWelcomeDialogPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->delete_event = ppg_welcome_dialog_delete_event;
}

/**
 * ppg_welcome_dialog_init:
 * @dialog: (in): A #PpgWelcomeDialog.
 *
 * Initializes the newly created #PpgWelcomeDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_welcome_dialog_init (PpgWelcomeDialog *dialog)
{
	PpgWelcomeDialogPrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *hbox;
	GtkWidget *scroller;
	GtkWidget *treeview;
	GtkWidget *l;
	GtkWidget *b;
	GtkWidget *hsep;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	instances ++;

	priv = dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog,
	                                                  PPG_TYPE_WELCOME_DIALOG,
	                                                  PpgWelcomeDialogPrivate);

	g_object_set(dialog,
	             "height-request", 440,
	             "resizable", FALSE,
	             "width-request", 640,
	             "window-position", GTK_WIN_POS_CENTER,
	             NULL);

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(dialog), vbox);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), hbox,
	                                  "position", 0,
	                                  NULL);

	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW,
	                        "hscrollbar-policy", GTK_POLICY_NEVER,
	                        "visible", TRUE,
	                        "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                        "width-request", 200,
	                        NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), scroller,
	                                  "expand", FALSE,
	                                  "position", 0,
	                                  NULL);

	priv->sessions = ppg_sessions_store_new();
	treeview = g_object_new(GTK_TYPE_TREE_VIEW,
	                        "headers-visible", FALSE,
	                        "model", priv->sessions,
	                        "visible", TRUE,
	                        NULL);
	gtk_tree_view_set_row_separator_func(GTK_TREE_VIEW(treeview),
	                                     ppg_welcome_dialog_row_separator_func,
	                                     NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroller), treeview);

	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	cell = g_object_new(GTK_TYPE_CELL_RENDERER_PIXBUF,
	                    "stock-size", GTK_ICON_SIZE_DND,
	                    "xpad", 3,
	                    "ypad", 3,
	                    NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), cell, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(column), cell, "icon-name",
	                              PPG_SESSIONS_STORE_COLUMN_ICON_NAME);

	cell = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
	                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                    NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(column), cell, "text",
	                              PPG_SESSIONS_STORE_COLUMN_TITLE);

	vbox2 = g_object_new(GTK_TYPE_VBOX,
	                     "border-width", 12,
	                     "spacing", 6,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), vbox2,
	                                  "position", 1,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("<span size=\"larger\" weight=\"bold\">"
	                            "<span size=\"larger\">"
	                            "Welcome to " PRODUCT_NAME
	                            "</span>"
	                            "</span>"),
	                 "use-markup", TRUE,
	                 "visible", TRUE,
	                 "ypad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox2), l,
	                                  "expand", FALSE,
	                                  NULL);

	b = ppg_welcome_dialog_create_button(dialog, "computer",
	                                     GTK_ICON_SIZE_DIALOG,
	                                     _("Start profiling a new or existing "
	                                       "process on your local machine"));
	gtk_container_add_with_properties(GTK_CONTAINER(vbox2), b,
	                                  "expand", FALSE,
	                                  NULL);
	g_signal_connect(b, "clicked",
	                 G_CALLBACK(ppg_welcome_dialog_local_clicked),
	                 dialog);

	b = ppg_welcome_dialog_create_button(dialog, "network-server",
	                                     GTK_ICON_SIZE_DIALOG,
	                                     _("Start profiling a new or existing "
	                                       "process on a remote machine"));
	gtk_container_add_with_properties(GTK_CONTAINER(vbox2), b,
	                                  "expand", FALSE,
	                                  NULL);

	b = ppg_welcome_dialog_create_button(dialog, GTK_STOCK_OPEN,
	                                     GTK_ICON_SIZE_DIALOG,
	                                     _("Open an existing profiling session"));
	gtk_container_add_with_properties(GTK_CONTAINER(vbox2), b,
	                                  "expand", FALSE,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add(GTK_CONTAINER(vbox2), l);

	hsep = g_object_new(GTK_TYPE_HSEPARATOR,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox2), hsep,
	                                  "expand", FALSE,
	                                  NULL);

	b = ppg_welcome_dialog_create_button(dialog, "document-import",
	                                     GTK_ICON_SIZE_MENU,
	                                     _("Import a profiling template"));
	gtk_container_add_with_properties(GTK_CONTAINER(vbox2), b,
	                                  "expand", FALSE,
	                                  NULL);

	b = ppg_welcome_dialog_create_button(dialog, "media-video",
	                                     GTK_ICON_SIZE_MENU,
	                                     _("View video tutorials"));
	gtk_container_add_with_properties(GTK_CONTAINER(vbox2), b,
	                                  "expand", FALSE,
	                                  NULL);
	g_signal_connect(b, "clicked",
	                 G_CALLBACK(ppg_welcome_dialog_tutorials_clicked),
	                 NULL);

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->sessions), &iter)) {
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_select_iter(selection, &iter);
	}
}
