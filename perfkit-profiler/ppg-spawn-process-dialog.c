/* ppg-spawn-process-dialog.c
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

#include "ppg-edit-channel-task.h"
#include "ppg-session.h"
#include "ppg-spawn-process-dialog.h"
#include "ppg-task.h"

#define APPEND_DUMMY_ROW(m)                                \
    G_STMT_START {                                         \
        GtkTreeIter iter;                                  \
        gtk_list_store_append((m), &iter);                 \
        gtk_list_store_set((m), &iter,                     \
                           0, _("Click to add variable"),  \
                           -1);                            \
    } G_STMT_END

G_DEFINE_TYPE(PpgSpawnProcessDialog, ppg_spawn_process_dialog, GTK_TYPE_DIALOG)

struct _PpgSpawnProcessDialogPrivate
{
	PpgSession   *session;

	GtkWidget    *target_entry;
	GtkWidget    *args_entry;
	GtkWidget    *dir_button;
	GtkWidget    *env_treeview;
	GtkWidget    *ok_button;
	GtkListStore *env_model;
};

enum
{
	PROP_0,
	PROP_SESSION,
	PROP_TASK,
};

static void
ppg_spawn_process_dialog_args_changed (GtkWidget             *entry,
                                       PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	const gchar *text;
	gint argc = 0;
	gchar **argv = NULL;
	GError *error = NULL;
	gboolean empty;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));

	priv = dialog->priv;
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	empty = !text || !text[0];

	if (!empty && !g_shell_parse_argv(text, &argc, &argv, &error)) {
		gtk_widget_set_tooltip_text(entry, error->message);
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
		                                  GTK_ENTRY_ICON_SECONDARY,
		                                  GTK_STOCK_DIALOG_ERROR);
		gtk_widget_set_sensitive(priv->ok_button, FALSE);
		g_error_free(error);
	} else {
		gtk_widget_set_tooltip_text(entry, NULL);
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
		                                  GTK_ENTRY_ICON_SECONDARY,
		                                  NULL);
		gtk_widget_set_sensitive(priv->ok_button, TRUE);
	}

	g_strfreev(argv);
}

static void
ppg_spawn_process_dialog_set_session (PpgSpawnProcessDialog *dialog,
                                      PpgSession            *session)
{
	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));
	g_return_if_fail(PPG_IS_SESSION(session));

	dialog->priv->session = session;
}

static PpgTask*
ppg_spawn_process_dialog_get_task (PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	const gchar *target;
	const gchar *args;
	const gchar *working_dir;
	gchar **argv = NULL;
	gint argc = 0;
	gboolean empty;
	PpgTask *task;

	g_return_val_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog), NULL);

	priv = dialog->priv;
	target = gtk_entry_get_text(GTK_ENTRY(priv->target_entry));
	args = gtk_entry_get_text(GTK_ENTRY(priv->args_entry));
	empty = !args || !args[0];
	working_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(priv->dir_button));

	if (!empty && !g_shell_parse_argv(args, &argc, &argv, NULL)) {
		return NULL;
	}

	task = g_object_new(PPG_TYPE_EDIT_CHANNEL_TASK,
	                    "args", argv,
	                    "session", priv->session,
	                    "target", target,
	                    "working-dir", working_dir,
	                    NULL);

	/*
	 * FIXME: Environment variables.
	 */

	g_strfreev(argv);

	return task;
}

/**
 * ppg_spawn_process_dialog_finalize:
 * @object: (in): A #PpgSpawnProcessDialog.
 *
 * Finalizer for a #PpgSpawnProcessDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_spawn_process_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_spawn_process_dialog_parent_class)->finalize(object);
}

/**
 * ppg_spawn_process_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_spawn_process_dialog_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
	PpgSpawnProcessDialog *dialog = PPG_SPAWN_PROCESS_DIALOG(object);

	switch (prop_id) {
	case PROP_SESSION:
		g_value_set_object(value, dialog->priv->session);
		break;
	case PROP_TASK:
		g_value_set_object(value, ppg_spawn_process_dialog_get_task(dialog));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_spawn_process_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_spawn_process_dialog_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
	PpgSpawnProcessDialog *dialog = PPG_SPAWN_PROCESS_DIALOG(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_spawn_process_dialog_set_session(dialog,
		                                     g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_spawn_process_dialog_class_init:
 * @klass: (in): A #PpgSpawnProcessDialogClass.
 *
 * Initializes the #PpgSpawnProcessDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_spawn_process_dialog_class_init (PpgSpawnProcessDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_spawn_process_dialog_finalize;
	object_class->get_property = ppg_spawn_process_dialog_get_property;
	object_class->set_property = ppg_spawn_process_dialog_set_property;
	g_type_class_add_private(object_class, sizeof(PpgSpawnProcessDialogPrivate));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_TASK,
	                                g_param_spec_object("task",
	                                                    "task",
	                                                    "task",
	                                                    PPG_TYPE_TASK,
	                                                    G_PARAM_READABLE));
}

/**
 * ppg_spawn_process_dialog_init:
 * @dialog: (in): A #PpgSpawnProcessDialog.
 *
 * Initializes the newly created #PpgSpawnProcessDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_spawn_process_dialog_init (PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkWidget *content_area;
	GtkWidget *vbox;
	GtkWidget *l;
	GtkWidget *table;
	GtkWidget *align;
	GtkWidget *scroller;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog,
	                                   PPG_TYPE_SPAWN_PROCESS_DIALOG,
	                                   PpgSpawnProcessDialogPrivate);
	dialog->priv = priv;

	g_object_set(dialog,
	             "border-width", 6,
	             "default-width", 350,
	             "default-height", 400,
	             "has-separator", FALSE,
	             "title", _("Spawn a new process"),
	             NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "border-width", 6,
	                    "spacing", 12,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(content_area), vbox);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("Choose a target executable to spawn and "
	                 	        "configure the environment."),
	                 "justify", GTK_JUSTIFY_CENTER,
	                 "visible", TRUE,
	                 "wrap", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), l,
	                                  "expand", FALSE,
	                                  NULL);

	table = g_object_new(GTK_TYPE_TABLE,
	                     "n-columns", 3,
	                     "n-rows", 6,
	                     "row-spacing", 3,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(vbox), table);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "use-markup", TRUE,
	                 "label", _("<b>Process</b>"),
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 "ypad", 3,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 1,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 0,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	priv->target_entry = g_object_new(GTK_TYPE_ENTRY,
	                                  "activates-default", TRUE,
	                                  "visible", TRUE,
	                                  NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->target_entry,
	                                  "bottom-attach", 2,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 1,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("_Executable:"),
	                 "mnemonic-widget", priv->target_entry,
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 2,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 1,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	priv->args_entry = g_object_new(GTK_TYPE_ENTRY,
	                                "activates-default", TRUE,
	                                "visible", TRUE,
	                                NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->args_entry,
	                                  "bottom-attach", 3,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 2,
	                                  "y-options", GTK_FILL,
	                                  NULL);
	g_signal_connect(priv->args_entry, "changed",
	                 G_CALLBACK(ppg_spawn_process_dialog_args_changed),
	                 dialog);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("_Arguments:"),
	                 "mnemonic-widget", priv->args_entry,
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 3,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 2,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	/*
	 * FIXME: Remote profiling should use a GtkEntry here.
	 */
	priv->dir_button = g_object_new(GTK_TYPE_FILE_CHOOSER_BUTTON,
	                                "action", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                "visible", TRUE,
	                                NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->dir_button,
	                                  "bottom-attach", 4,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 3,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("Working _Directory:"),
	                 "mnemonic-widget", priv->dir_button,
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 4,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 3,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "use-markup", TRUE,
	                 "label", _("<b>Environment</b>"),
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 "ypad", 3,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 5,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 4,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	align = g_object_new(GTK_TYPE_ALIGNMENT,
	                     "visible", TRUE,
	                     "left-padding", 12,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), align,
	                                  "bottom-attach", 6,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 5,
	                                  NULL);

	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW,
	                        "hscrollbar-policy", GTK_POLICY_NEVER,
	                        "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                        "shadow-type", GTK_SHADOW_IN,
	                        "visible", TRUE,
	                        NULL);
	gtk_container_add(GTK_CONTAINER(align), scroller);

	priv->env_model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	APPEND_DUMMY_ROW(priv->env_model);

	priv->env_treeview = g_object_new(GTK_TYPE_TREE_VIEW,
	                                  "model", priv->env_model,
	                                  "visible", TRUE,
	                                  NULL);
	gtk_container_add(GTK_CONTAINER(scroller), priv->env_treeview);

	column = g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
	                      "expand", TRUE,
	                      "title", _("Key"),
	                      NULL);
	cell = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
	                    "editable", TRUE,
	                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                    NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(column), cell, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->env_treeview), column);

	column = g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
	                      "expand", TRUE,
	                      "title", _("Value"),
	                      NULL);
	cell = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
	                    "editable", TRUE,
	                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                    NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(column), cell, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->env_treeview), column);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL,
	                      GTK_RESPONSE_CANCEL);
	priv->ok_button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK,
	                                        GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}
