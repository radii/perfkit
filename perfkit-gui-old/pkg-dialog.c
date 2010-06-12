/* pkg-dialog.c
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

#include "pkg-dialog.h"

static void
pkg_dialog_warning_response (GtkWidget *widget,
                             gint       response_id,
                             gpointer   user_data)
{
	gtk_widget_hide(widget);
	gtk_widget_destroy(widget);
}

/**
 * pkg_dialog_warning:
 * @parent: The parent #GtkWindow or %NULL.
 * @title: The title for the dialog.
 * @primary: The primary message for the dialog.
 * @secondary: The secondary message for the dialog.
 * @use_expander: If the secondary data should be shown in an expander.
 *
 * Creates a transient dialog which is shown to the user.
 *
 * Returns: the newly created #GtkWidget
 * Side effects: None.
 */
GtkWidget*
pkg_dialog_warning (GtkWidget   *parent,
                    const gchar *title,
                    const gchar *primary,
                    const gchar *secondary,
                    gboolean     use_expander)
{
	GtkWidget *dialog,
	          *label,
	          *vbox,
	          *hbox,
	          *warning,
	          *expander,
	          *scroller,
	          *text;
	gchar *markup;

	dialog = gtk_dialog_new_with_buttons(title,
	                                     GTK_WINDOW(parent),
	                                     GTK_DIALOG_NO_SEPARATOR,
	                                     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
	                                     NULL);

	/*
	 * Pack the hbox.
	 */
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
	                   hbox, TRUE, TRUE, 0);

	/*
	 * Pack the warning image.
	 */
	warning = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment(GTK_MISC(warning), .5, .0);
	gtk_box_pack_start(GTK_BOX(hbox), warning, FALSE, TRUE, 0);

	/*
	 * Pack the vbox containing primary and secondary text.
	 */
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	/*
	 * Pack the primary text.
	 */
	markup = g_strdup_printf("<span weight=\"bold\">%s</span>",
	                         primary);
	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0., 0.);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	g_free(markup);

	/*
	 * Add the secondary text if needed.
	 */
	if (use_expander) {
		expander = gtk_expander_new(_("More ..."));
		gtk_box_pack_start(GTK_BOX(vbox), expander, TRUE, TRUE, 0);

		scroller = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
		                               GTK_POLICY_AUTOMATIC,
		                               GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller),
	                                    GTK_SHADOW_IN);
		gtk_container_add(GTK_CONTAINER(expander), scroller);
		text = gtk_text_view_new();
		gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
		                         secondary, -1);
		gtk_container_add(GTK_CONTAINER(scroller), text);
	} else {
		label = gtk_label_new(secondary);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_misc_set_alignment(GTK_MISC(label), 0., 0.);
		gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	}

	g_signal_connect(dialog,
	                 "response",
	                 G_CALLBACK(pkg_dialog_warning_response),
	                 NULL);

	gtk_widget_show_all(dialog);

	return GTK_WIDGET(dialog);
}
