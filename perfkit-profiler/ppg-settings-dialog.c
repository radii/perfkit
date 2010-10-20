/* ppg-settings-dialog.c
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

#include "ppg-session.h"
#include "ppg-settings-dialog.h"

G_DEFINE_TYPE(PpgSettingsDialog, ppg_settings_dialog, GTK_TYPE_DIALOG)

struct _PpgSettingsDialogPrivate
{
	PpgSession *session;
};

enum
{
	PROP_0,
	PROP_SESSION,
};

static void
ppg_settings_dialog_set_session (PpgSettingsDialog *dialog,
                                 PpgSession *session)
{
	PpgSettingsDialogPrivate *priv;

	g_return_if_fail(PPG_IS_SETTINGS_DIALOG(dialog));

	priv = dialog->priv;

	if (priv->session) {
		g_object_remove_weak_pointer(G_OBJECT(priv->session), (gpointer *)&priv->session);
		priv->session = NULL;
	}

	if (session) {
		priv->session = session;
		g_object_add_weak_pointer(G_OBJECT(priv->session), (gpointer *)&priv->session);
	}
}

/**
 * ppg_settings_dialog_finalize:
 * @object: A #PpgSettingsDialog.
 *
 * Finalizer for a #PpgSettingsDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_settings_dialog_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(ppg_settings_dialog_parent_class)->finalize(object);
}

/**
 * ppg_settings_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_settings_dialog_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
	PpgSettingsDialog *dialog = PPG_SETTINGS_DIALOG(object);

	switch (prop_id) {
	case PROP_SESSION:
		g_value_set_object(value, dialog->priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_settings_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_settings_dialog_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
	PpgSettingsDialog *dialog = PPG_SETTINGS_DIALOG(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_settings_dialog_set_session(dialog, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_settings_dialog_class_init:
 * @klass: A #PpgSettingsDialogClass.
 *
 * Initializes the #PpgSettingsDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_settings_dialog_class_init (PpgSettingsDialogClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_settings_dialog_finalize;
	object_class->get_property = ppg_settings_dialog_get_property;
	object_class->set_property = ppg_settings_dialog_set_property;
	g_type_class_add_private(object_class, sizeof(PpgSettingsDialogPrivate));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

/**
 * ppg_settings_dialog_init:
 * @dialog: A #PpgSettingsDialog.
 *
 * Initializes the newly created #PpgSettingsDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_settings_dialog_init (PpgSettingsDialog *dialog) /* IN */
{
	PpgSettingsDialogPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog, PPG_TYPE_SETTINGS_DIALOG,
	                                   PpgSettingsDialogPrivate);
	dialog->priv = priv;

	g_object_set(dialog,
	             "border-width", 6,
	             "default-width", 400,
	             "default-height", 400,
	             "has-separator", FALSE,
	             "title", _("Profiler Settings"),
	             NULL);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE,
	                      GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
}
