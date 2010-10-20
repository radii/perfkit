/* ppg-add-instrument-dialog.c
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

#include "ppg-add-instrument-dialog.h"
#include "ppg-instruments.h"
#include "ppg-session.h"

G_DEFINE_TYPE(PpgAddInstrumentDialog, ppg_add_instrument_dialog, GTK_TYPE_DIALOG)

struct _PpgAddInstrumentDialogPrivate
{
	PpgSession *session;

	GtkWidget *entry;
	GtkWidget *icon_view;
	GtkWidget *add_button;

	GtkListStore       *model;
	GtkTreeModelFilter *filter;
};

enum
{
	PROP_0,
	PROP_SESSION,
};

static void
ppg_add_instrument_dialog_entry_changed (GtkWidget *entry,
                                         PpgAddInstrumentDialog *dialog)
{
	PpgAddInstrumentDialogPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;

	g_return_if_fail(PPG_IS_ADD_INSTRUMENT_DIALOG(dialog));

	priv = dialog->priv;
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(priv->filter));
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(priv->filter), NULL) == 1) {
		if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->filter), &iter)) {
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->filter), &iter);
			gtk_icon_view_select_path(GTK_ICON_VIEW(priv->icon_view), path);
			gtk_tree_path_free(path);
		}
	}
}

static void
ppg_add_instrument_dialog_item_activated (GtkWidget *icon_view,
                                          GtkTreePath *path,
                                          PpgAddInstrumentDialog *dialog)
{
	PpgAddInstrumentDialogPrivate *priv;
	PpgInstrument *instrument;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *name;

	g_return_if_fail(PPG_IS_ADD_INSTRUMENT_DIALOG(dialog));
	g_return_if_fail(dialog->priv->session);

	priv = dialog->priv;
	model = GTK_TREE_MODEL(priv->filter);

	/*
	 * FIXME: Add instrument.
	 */

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter,
	                   PPG_INSTRUMENTS_STORE_COLUMN_NAME, &name,
	                   -1);
	instrument = ppg_instruments_create(priv->session, name);
	g_free(name);
}

static void
ppg_add_instrument_dialog_selection_changed (GtkWidget *icon_view,
                                             PpgAddInstrumentDialog *dialog)
{
	PpgAddInstrumentDialogPrivate *priv;
	GList *list;

	g_return_if_fail(PPG_IS_ADD_INSTRUMENT_DIALOG(dialog));

	priv = dialog->priv;

	list = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(icon_view));
	gtk_widget_set_sensitive(priv->add_button, !!list);
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
}

static void
ppg_add_instrument_dialog_set_session (PpgAddInstrumentDialog *dialog,
                                       PpgSession *session)
{
	PpgAddInstrumentDialogPrivate *priv;

	g_return_if_fail(PPG_IS_ADD_INSTRUMENT_DIALOG(dialog));

	priv = dialog->priv;

	if (priv->session) {
		g_object_unref(priv->session);
		priv->session = NULL;
	}

	if (session) {
		priv->session = g_object_ref(session);
	}
}

static void
ppg_add_instrument_dialog_response (PpgAddInstrumentDialog *dialog,
                                    gint response_id,
                                    gpointer user_data)
{
	PpgAddInstrumentDialogPrivate *priv;
	GList *list;

	g_return_if_fail(PPG_IS_ADD_INSTRUMENT_DIALOG(dialog));

	priv = dialog->priv;

	if (response_id == GTK_RESPONSE_OK) {
		g_signal_stop_emission_by_name(dialog, "response");
		list = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(priv->icon_view));
		if (list) {
			gtk_icon_view_item_activated(GTK_ICON_VIEW(priv->icon_view),
			                             list->data);
		}
		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);
		gtk_widget_grab_focus(priv->entry);
	}
}

static gboolean
ppg_add_instrument_dialog_filter_func (GtkTreeModel *filter,
                                       GtkTreeIter *iter,
                                       gpointer user_data)
{
	PpgAddInstrumentDialog *dialog = (PpgAddInstrumentDialog *)user_data;
	PpgAddInstrumentDialogPrivate *priv = dialog->priv;
	const gchar *text = gtk_entry_get_text((GtkEntry *)priv->entry);
	gchar *lower;
	gchar *fulltext;
	gboolean ret;

	if (!text || !text[0]) {
		return TRUE;
	}

	gtk_tree_model_get(filter, iter,
	                   PPG_INSTRUMENTS_STORE_COLUMN_FULLTEXT, &fulltext,
	                   -1);
	lower = g_utf8_strdown(text, -1);
	ret = !!strstr(fulltext, lower);
	g_free(fulltext);
	g_free(lower);

	return ret;
}

/**
 * ppg_add_instrument_dialog_dispose:
 * @object: A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
ppg_add_instrument_dialog_dispose (GObject *object) /* IN */
{
	PpgAddInstrumentDialog *dialog = PPG_ADD_INSTRUMENT_DIALOG(object);

	ppg_add_instrument_dialog_set_session(dialog, NULL);

	G_OBJECT_CLASS(ppg_add_instrument_dialog_parent_class)->dispose(object);
}

/**
 * ppg_add_instrument_dialog_finalize:
 * @object: (in): A #PpgAddInstrumentDialog.
 *
 * Finalizer for a #PpgAddInstrumentDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_add_instrument_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_add_instrument_dialog_parent_class)->finalize(object);
}

static void
ppg_add_instrument_dialog_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
	PpgAddInstrumentDialog *dialog = PPG_ADD_INSTRUMENT_DIALOG(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_add_instrument_dialog_set_session(dialog,
		                                      g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_add_instrument_dialog_class_init:
 * @klass: (in): A #PpgAddInstrumentDialogClass.
 *
 * Initializes the #PpgAddInstrumentDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_add_instrument_dialog_class_init (PpgAddInstrumentDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_add_instrument_dialog_finalize;
	object_class->dispose = ppg_add_instrument_dialog_dispose;
	object_class->set_property = ppg_add_instrument_dialog_set_property;
	g_type_class_add_private(object_class, sizeof(PpgAddInstrumentDialogPrivate));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_WRITABLE));
}

/**
 * ppg_add_instrument_dialog_init:
 * @dialog: (in): A #PpgAddInstrumentDialog.
 *
 * Initializes the newly created #PpgAddInstrumentDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_add_instrument_dialog_init (PpgAddInstrumentDialog *dialog)
{
	PpgAddInstrumentDialogPrivate *priv;
	GtkWidget *content_area;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *l;
	GtkWidget *scroller;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog, PPG_TYPE_ADD_INSTRUMENT_DIALOG,
	                                   PpgAddInstrumentDialogPrivate);
	dialog->priv = priv;

	g_object_set(dialog,
	             "border-width", 6,
	             "default-width", 350,
	             "default-height", 400,
	             "has-separator", FALSE,
	             "title", _("Add Instrument"),
	             NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "border-width", 6,
	                    "spacing", 12,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(content_area), vbox);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "justify", GTK_JUSTIFY_CENTER,
	                 "label", _("Choose one or more instruments to\n"
	                            "add to your session."),
	                 "visible", TRUE,
	                 "wrap", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), l,
	                                  "expand", FALSE,
	                                  NULL);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "spacing", 12,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), hbox,
	                                  "expand", FALSE,
	                                  NULL);

	priv->entry = g_object_new(GTK_TYPE_ENTRY,
	                           "activates-default", TRUE,
	                           "visible", TRUE,
	                           NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), priv->entry,
	                                  "position", 1,
	                                  NULL);
	g_signal_connect(priv->entry,
	                 "changed",
	                 G_CALLBACK(ppg_add_instrument_dialog_entry_changed),
	                 dialog);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("_Search:"),
	                 "mnemonic-widget", priv->entry,
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), l,
	                                  "expand", FALSE,
	                                  "position", 0,
	                                  NULL);

	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW,
	                        "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                        "shadow-type", GTK_SHADOW_IN,
	                        "visible", TRUE,
	                        "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                        NULL);
	gtk_container_add(GTK_CONTAINER(vbox), scroller);

	priv->model = ppg_instruments_store_new();
	priv->filter = g_object_new(GTK_TYPE_TREE_MODEL_FILTER,
	                            "child-model", priv->model,
	                            NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(priv->filter),
	                                       ppg_add_instrument_dialog_filter_func,
	                                       dialog, NULL);

	priv->icon_view = g_object_new(GTK_TYPE_ICON_VIEW,
	                               "model", priv->filter,
	                               "pixbuf-column", PPG_INSTRUMENTS_STORE_COLUMN_PIXBUF,
	                               "visible", TRUE,
	                               "text-column", PPG_INSTRUMENTS_STORE_COLUMN_TITLE,
	                               NULL);
	gtk_container_add(GTK_CONTAINER(scroller), priv->icon_view);
	g_signal_connect(priv->icon_view,
	                 "selection-changed",
	                 G_CALLBACK(ppg_add_instrument_dialog_selection_changed),
	                 dialog);
	g_signal_connect(priv->icon_view,
	                 "item-activated",
	                 G_CALLBACK(ppg_add_instrument_dialog_item_activated),
	                 dialog);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE,
	                      GTK_RESPONSE_CLOSE);
	priv->add_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
	                                         GTK_STOCK_ADD,
	                                         GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	g_object_set(priv->add_button, "sensitive", FALSE, NULL);

	g_signal_connect(dialog,
	                 "response",
	                 G_CALLBACK(ppg_add_instrument_dialog_response),
	                 NULL);
}
