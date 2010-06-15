/* pkg-util.h
 *
 * Copyright (C) 2007 David Zeuthen
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "pkg-log.h"
#include "pkg-util.h"

void
pkg_util_get_mix_color (GtkWidget    *widget,
                        GtkStateType  state,
                        gchar        *color_buf,
                        gsize         color_buf_size)
{
	GtkStyle *style;
	GdkColor color = {0};

	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (color_buf != NULL);

	/* This color business shouldn't be this hard... */
	style = gtk_widget_get_style (widget);
#define BLEND_FACTOR 0.7
	color.red   = style->text[state].red   * BLEND_FACTOR +
				  style->base[state].red   * (1.0 - BLEND_FACTOR);
	color.green = style->text[state].green * BLEND_FACTOR +
				  style->base[state].green * (1.0 - BLEND_FACTOR);
	color.blue  = style->text[state].blue  * BLEND_FACTOR +
				  style->base[state].blue  * (1.0 - BLEND_FACTOR);
#undef BLEND_FACTOR
	snprintf (color_buf,
	          color_buf_size, "#%02x%02x%02x",
	          (color.red >> 8),
	          (color.green >> 8),
	          (color.blue >> 8));
}


/**
 * pkg_util_dialog_warning:
 * @parent: The parent #GtkWindow or %NULL.
 * @title: The title for the dialog.
 * @primary: The primary message for the dialog.
 * @secondary: The secondary message for the dialog.
 * @use_expander: If the secondary data should be shown in an expander.
 *
 * Creates a transient dialog which is shown to the user.
 *
 * Returns: the newly created #GtkWidget.
 * Side effects: None.
 */
GtkWidget*
pkg_util_dialog_warning (GtkWidget   *parent,       /* IN */
                         const gchar *title,        /* IN */
                         const gchar *primary,      /* IN */
                         const gchar *secondary,    /* IN */
                         gboolean     use_expander) /* IN */
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *warning;
	GtkWidget *expander;
	GtkWidget *scroller;
	GtkWidget *text;
	gchar *markup;

	g_return_val_if_fail(!parent || GTK_IS_WIDGET(parent), NULL);

	ENTRY;
	dialog = gtk_dialog_new_with_buttons(title,
	                                     GTK_WINDOW(parent),
	                                     GTK_DIALOG_NO_SEPARATOR,
	                                     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
	                                     NULL);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	hbox = gtk_hbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 12);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
	                   hbox, TRUE, TRUE, 0);
	warning = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment(GTK_MISC(warning), .5, .0);
	gtk_box_pack_start(GTK_BOX(hbox), warning, FALSE, TRUE, 0);
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
	markup = g_strdup_printf("<span weight=\"bold\">%s</span>",
	                         primary);
	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0., 0.);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	g_free(markup);
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
	g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_widget_show_all(dialog);
	RETURN(GTK_WIDGET(dialog));
}
