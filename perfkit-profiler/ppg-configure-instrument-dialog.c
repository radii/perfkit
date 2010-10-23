/* ppg-configure-instrument-dialog.c
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

#include "ppg-configure-instrument-dialog.h"
#include "ppg-instrument.h"
#include "ppg-session.h"

G_DEFINE_TYPE(PpgConfigureInstrumentDialog,
              ppg_configure_instrument_dialog,
              GTK_TYPE_DIALOG)

struct _PpgConfigureInstrumentDialogPrivate
{
	PpgSession    *session;
	PpgInstrument *instrument;

	GtkNotebook *notebook;
};

enum
{
	PROP_0,
	PROP_INSTRUMENT,
	PROP_SESSION,
};

static void
ppg_configure_instrument_dialog_set_instrument (PpgConfigureInstrumentDialog *dialog,
                                                PpgInstrument *instrument)
{
	PpgConfigureInstrumentDialogPrivate *priv;

	g_return_if_fail(PPG_IS_CONFIGURE_INSTRUMENT_DIALOG(dialog));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = dialog->priv;
	priv->instrument = instrument;
}

static void
ppg_configure_instrument_dialog_set_session (PpgConfigureInstrumentDialog *dialog,
                                             PpgSession *session)
{
	PpgConfigureInstrumentDialogPrivate *priv;

	g_return_if_fail(PPG_IS_CONFIGURE_INSTRUMENT_DIALOG(dialog));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = dialog->priv;
	priv->session = session;
}

/**
 * ppg_configure_instrument_dialog_finalize:
 * @object: (in): A #PpgConfigureInstrumentDialog.
 *
 * Finalizer for a #PpgConfigureInstrumentDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_configure_instrument_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_configure_instrument_dialog_parent_class)->finalize(object);
}

/**
 * ppg_configure_instrument_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_configure_instrument_dialog_get_property (GObject    *object,
                                              guint       prop_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
	PpgConfigureInstrumentDialog *dialog = PPG_CONFIGURE_INSTRUMENT_DIALOG(object);

	switch (prop_id) {
	case PROP_INSTRUMENT:
		g_value_set_object(value, dialog->priv->instrument);
		break;
	case PROP_SESSION:
		g_value_set_object(value, dialog->priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_configure_instrument_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_configure_instrument_dialog_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
	PpgConfigureInstrumentDialog *dialog = PPG_CONFIGURE_INSTRUMENT_DIALOG(object);

	switch (prop_id) {
	case PROP_INSTRUMENT:
		ppg_configure_instrument_dialog_set_instrument(dialog, g_value_get_object(value));
		break;
	case PROP_SESSION:
		ppg_configure_instrument_dialog_set_session(dialog, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_configure_instrument_dialog_class_init:
 * @klass: (in): A #PpgConfigureInstrumentDialogClass.
 *
 * Initializes the #PpgConfigureInstrumentDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_configure_instrument_dialog_class_init (PpgConfigureInstrumentDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_configure_instrument_dialog_finalize;
	object_class->get_property = ppg_configure_instrument_dialog_get_property;
	object_class->set_property = ppg_configure_instrument_dialog_set_property;
	g_type_class_add_private(object_class, sizeof(PpgConfigureInstrumentDialogPrivate));

	g_object_class_install_property(object_class,
	                                PROP_INSTRUMENT,
	                                g_param_spec_object("instrument",
	                                                    "instrument",
	                                                    "instrument",
	                                                    PPG_TYPE_INSTRUMENT,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READWRITE));
}

/**
 * ppg_configure_instrument_dialog_init:
 * @dialog: (in): A #PpgConfigureInstrumentDialog.
 *
 * Initializes the newly created #PpgConfigureInstrumentDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_configure_instrument_dialog_init (PpgConfigureInstrumentDialog *dialog)
{
	PpgConfigureInstrumentDialogPrivate *priv;
	GtkContainer *box;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog,
	                                   PPG_TYPE_CONFIGURE_INSTRUMENT_DIALOG,
	                                   PpgConfigureInstrumentDialogPrivate);
	dialog->priv = priv;

	g_object_set(dialog,
	             "border-width", 6,
	             "default-width", 300,
	             "default-height", 350,
	             "title", _("Configure Instrument"),
	             NULL);

	box = GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
	g_object_set(box,
	             "border-width", 6,
	             NULL);

	priv->notebook = g_object_new(GTK_TYPE_NOTEBOOK,
	                              "visible", TRUE,
	                              NULL);
	gtk_container_add(box, GTK_WIDGET(priv->notebook));

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
}
