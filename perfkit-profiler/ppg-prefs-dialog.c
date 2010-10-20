/* ppg-prefs-dialog.c
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

#include "ppg-prefs.h"
#include "ppg-prefs-dialog.h"

G_DEFINE_TYPE(PpgPrefsDialog, ppg_prefs_dialog, GTK_TYPE_DIALOG)

struct _PpgPrefsDialogPrivate
{
	GtkWidget *notebook;
};

static PpgPrefsDialog *singleton = NULL;

static GObject*
ppg_prefs_dialog_constructor (GType type,
                              guint n_props,
                              GObjectConstructParam *props)
{
	GObject *object;

	if (!singleton) {
		object = G_OBJECT_CLASS(ppg_prefs_dialog_parent_class)->
			constructor(type, n_props, props);
		singleton = PPG_PREFS_DIALOG(object);
	} else {
		object = g_object_ref(G_OBJECT(singleton));
	}

	return object;
}

static gboolean
ppg_prefs_dialog_delete_event (GtkWidget *widget,
                               GdkEvent  *event,
                               gpointer   user_data)
{
	gtk_widget_hide(widget);
	return TRUE;
}

static void
ppg_prefs_dialog_response (GtkDialog *dialog,
                           gint response,
                           gpointer user_data)
{
	if (response == GTK_RESPONSE_CLOSE) {
		gtk_widget_hide(GTK_WIDGET(dialog));
	}
}

static void
ppg_prefs_dialog_add_group (GtkWidget *box,
                            GtkWidget *label,
                            GtkWidget *child)
{
	GtkWidget *align;

	align = g_object_new(GTK_TYPE_ALIGNMENT,
	                     "left-padding", 12,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(align), child);

	gtk_container_add_with_properties(GTK_CONTAINER(box), label,
	                                  "expand", FALSE,
	                                  NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(box), align,
	                                  "expand", FALSE,
	                                  NULL);
}

static GtkWidget*
ppg_prefs_dialog_create_project_page (PpgPrefsDialog *dialog)
{
	GtkWidget *vbox;
	GtkWidget *project_button;
	GtkWidget *l;
	gchar *project_dir;

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "border-width", 12,
	                    "spacing", 6,
	                    "visible", TRUE,
	                    NULL);

	project_dir = ppg_prefs_get_project_default_dir();
	project_button = g_object_new(GTK_TYPE_FILE_CHOOSER_BUTTON,
	                              "action", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                              "title", _("Default Project Directory"),
	                              "visible", TRUE,
	                              NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(project_button), project_dir);
	g_free(project_dir);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("<b>Default _Directory</b>"),
	                 "mnemonic-widget", project_button,
	                 "use-markup", TRUE,
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 NULL);
	ppg_prefs_dialog_add_group(vbox, l, project_button);

	return vbox;
}

/**
 * ppg_prefs_dialog_finalize:
 * @object: A #PpgPrefsDialog.
 *
 * Finalizer for a #PpgPrefsDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_prefs_dialog_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(ppg_prefs_dialog_parent_class)->finalize(object);
}

/**
 * ppg_prefs_dialog_class_init:
 * @klass: A #PpgPrefsDialogClass.
 *
 * Initializes the #PpgPrefsDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_prefs_dialog_class_init (PpgPrefsDialogClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->constructor = ppg_prefs_dialog_constructor;
	object_class->finalize = ppg_prefs_dialog_finalize;
	g_type_class_add_private(object_class, sizeof(PpgPrefsDialogPrivate));
}

/**
 * ppg_prefs_dialog_init:
 * @dialog: A #PpgPrefsDialog.
 *
 * Initializes the newly created #PpgPrefsDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_prefs_dialog_init (PpgPrefsDialog *dialog) /* IN */
{
	PpgPrefsDialogPrivate *priv;
	GtkWidget *content_area;
	GtkWidget *page;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog, PPG_TYPE_PREFS_DIALOG,
	                                   PpgPrefsDialogPrivate);
	dialog->priv = priv;

	g_signal_connect(dialog, "delete-event",
	                 G_CALLBACK(ppg_prefs_dialog_delete_event),
	                 NULL);
	g_signal_connect(dialog, "response",
	                 G_CALLBACK(ppg_prefs_dialog_response),
	                 NULL);

	g_object_set(dialog,
	             "border-width", 6,
	             "default-height", 400,
	             "default-width", 400,
	             "has-separator", FALSE,
	             "title", _("Preferences"),
	             NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	priv->notebook = g_object_new(GTK_TYPE_NOTEBOOK,
	                              "border-width", 6,
	                              "visible", TRUE,
	                              NULL);
	gtk_container_add(GTK_CONTAINER(content_area), priv->notebook);

	page = ppg_prefs_dialog_create_project_page(dialog);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->notebook), page,
	                                  "position", 0,
	                                  "tab-label", _("Projects"),
	                                  NULL);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE,
	                      GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
}
