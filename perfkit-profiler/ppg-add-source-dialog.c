/* ppg-add-source-dialog.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "ppg-add-source-dialog.h"

/**
 * SECTION:ppg-add-source-dialog.h
 * @title: PpgAddSourceDialog
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PpgAddSourceDialog, ppg_add_source_dialog, GTK_TYPE_DIALOG)

struct _PpgAddSourceDialogPrivate
{
	GtkWidget *search_entry;
	GtkWidget *source_view;
	GtkWidget *add_button;
};

/**
 * ppg_add_source_dialog_finalize:
 * @object: A #PpgAddSourceDialog.
 *
 * Finalizer for a #PpgAddSourceDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_add_source_dialog_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(ppg_add_source_dialog_parent_class)->finalize(object);
}

/**
 * ppg_add_source_dialog_class_init:
 * @klass: A #PpgAddSourceDialogClass.
 *
 * Initializes the #PpgAddSourceDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_add_source_dialog_class_init (PpgAddSourceDialogClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_add_source_dialog_finalize;
	g_type_class_add_private(object_class, sizeof(PpgAddSourceDialogPrivate));
}

/**
 * ppg_add_source_dialog_init:
 * @dialog: A #PpgAddSourceDialog.
 *
 * Initializes the newly created #PpgAddSourceDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_add_source_dialog_init (PpgAddSourceDialog *add) /* IN */
{
	GtkDialog *dialog = GTK_DIALOG(add);
	PpgAddSourceDialogPrivate *priv;
	GtkWidget *content;
	GtkWidget *scroll;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *l;

	add->priv = G_TYPE_INSTANCE_GET_PRIVATE(add, PPG_TYPE_ADD_SOURCE_DIALOG,
	                                        PpgAddSourceDialogPrivate);
	priv = add->priv;

	g_object_set(add,
	             "border-width", 6,
	             "default-height", 400,
	             "has-separator", FALSE,
	             "title", "",
	             NULL);

	content = gtk_dialog_get_content_area(dialog);

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "border-width", 6,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(content), vbox);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("Choose one or more sources to add "
	                            "to your session."),
	                 "visible", TRUE,
	                 "xalign", 0.5,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), l,
	                                  "expand", FALSE,
	                                  "fill", TRUE,
	                                  NULL);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "border-width", 12,
	                    "spacing", 12,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), hbox,
	                                  "expand", FALSE,
	                                  "fill", TRUE,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("Search:"),
	                 "xalign", 1.0,
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), l,
	                                  "expand", FALSE,
	                                  "fill", TRUE,
	                                  NULL);

	priv->search_entry = g_object_new(GTK_TYPE_ENTRY,
	                                  "has-focus", TRUE,
	                                  "visible", TRUE,
	                                  NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), priv->search_entry,
	                                  "expand", TRUE,
	                                  "fill", TRUE,
	                                  NULL);

	scroll = g_object_new(GTK_TYPE_SCROLLED_WINDOW,
	                      "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                      "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                      "shadow-type", GTK_SHADOW_IN,
	                      "visible", TRUE,
	                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), scroll,
	                                  "expand", TRUE,
	                                  "fill", TRUE,
	                                  NULL);

	priv->source_view = g_object_new(GTK_TYPE_ICON_VIEW,
	                                 "visible", TRUE,
	                                 NULL);
	gtk_container_add(GTK_CONTAINER(scroll), priv->source_view);

	gtk_dialog_add_buttons(dialog,
	                       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
	                       GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT,
	                       NULL);
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_ACCEPT);

	priv->add_button =
		gtk_dialog_get_widget_for_response(dialog, GTK_RESPONSE_ACCEPT);
	g_object_set(priv->add_button,
	             "sensitive", FALSE,
	             NULL);
}
