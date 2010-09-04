/* ppg-restart-action.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "ppg-restart-action.h"
#include "ppg-session.h"
#include "ppg-window.h"
#include "ppg-util.h"

struct _PpgRestartActionPrivate
{
	GtkWidget  *window;
	PpgSession *session;
};

enum
{
	PROP_0,
	PROP_WIDGET,
};

G_DEFINE_TYPE(PpgRestartAction, ppg_restart_action, GTK_TYPE_ACTION)

static void
ppg_restart_action_activate (GtkAction *action)
{
	PpgRestartActionPrivate *priv;
	GtkDialog *dialog;

	g_return_if_fail(PPG_IS_RESTART_ACTION(action));

	priv = PPG_RESTART_ACTION(action)->priv;
	if (priv->session) {
		dialog = g_object_new(GTK_TYPE_MESSAGE_DIALOG,
		                      "buttons", GTK_BUTTONS_OK_CANCEL,
		                      "text", _("Would you like to restart the profiling session?"),
		                      "message-type", GTK_MESSAGE_QUESTION,
		                      "secondary-text", "SECONDARY TEXT",
		                      "transient-for", priv->window,
		                      "visible", TRUE,
		                      NULL);
		gtk_dialog_set_default_response(dialog, GTK_RESPONSE_CANCEL);
		if (gtk_dialog_run(dialog) == GTK_RESPONSE_OK) {
			ppg_session_stop(priv->session, NULL);
			ppg_session_start(priv->session, NULL);
		}
		gtk_widget_destroy(GTK_WIDGET(dialog));
	}
}

static void
ppg_restart_action_state_changed (PpgRestartAction *action,
                                  guint             state,
                                  PpgSession       *session)
{
	gboolean sensitive;

	g_return_if_fail(PPG_IS_RESTART_ACTION(action));
	g_return_if_fail(PPG_IS_SESSION(session));

	switch (state) {
	case PPG_SESSION_STARTED:
	case PPG_SESSION_PAUSED:
		sensitive = TRUE;
		break;
	case PPG_SESSION_STOPPED:
		sensitive = FALSE;
		break;
	default:
		g_assert_not_reached();
		break;
	}

	g_object_set(action,
	             "sensitive", sensitive,
	             NULL);
}

static void
ppg_restart_action_set_widget (PpgRestartAction *action,
                               GtkWidget        *widget)
{
	PpgRestartActionPrivate *priv;

	g_return_if_fail(PPG_IS_RESTART_ACTION(action));
	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(action->priv->session == NULL);

	priv = action->priv;
	priv->window = widget;
	g_object_get(widget,
	             "session", &priv->session,
	             NULL);
	g_signal_connect_swapped(priv->session, "state-changed",
	                         G_CALLBACK(ppg_restart_action_state_changed),
	                         action);
}

static void
ppg_restart_action_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_restart_action_parent_class)->finalize(object);
}

static void
ppg_restart_action_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
	PpgRestartActionPrivate *priv;

	priv = PPG_RESTART_ACTION(object)->priv;
	switch (prop_id) {
	case PROP_WIDGET:
		g_value_set_object(value, priv->window);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
ppg_restart_action_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_WIDGET:
		ppg_restart_action_set_widget(PPG_RESTART_ACTION(object),
		                              g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static GObject*
ppg_restart_action_constructor (GType                  type,
                                guint                  n_props,
                                GObjectConstructParam *props)
{
	GObject *object;

	object = PARENT_CTOR(ppg_restart_action)(type, n_props, props);
	g_object_set(object,
	             "icon-name", GTK_STOCK_REFRESH,
	             "label", _("Restart"),
	             "name", "RestartAction",
	             "sensitive", FALSE,
	             "tooltip", _("Restart this profiling session"),
	             NULL);
	return object;
}

static void
ppg_restart_action_class_init (PpgRestartActionClass *klass)
{
	GObjectClass *object_class;
	GtkActionClass *action_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->constructor = ppg_restart_action_constructor;
	object_class->finalize = ppg_restart_action_finalize;
	object_class->get_property = ppg_restart_action_get_property;
	object_class->set_property = ppg_restart_action_set_property;
	g_type_class_add_private(object_class, sizeof(PpgRestartActionPrivate));

	action_class = GTK_ACTION_CLASS(klass);
	action_class->activate = ppg_restart_action_activate;

	g_object_class_install_property(object_class,
	                                PROP_WIDGET,
	                                g_param_spec_object("widget",
	                                                    "widget",
	                                                    "widget",
	                                                    GTK_TYPE_WIDGET,
	                                                    G_PARAM_READWRITE));
}

static void
ppg_restart_action_init (PpgRestartAction *action)
{
	INIT_PRIV(action, RESTART_ACTION, RestartAction);
}
