/* ppg-stop-action.c
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

#include "ppg-stop-action.h"
#include "ppg-window.h"
#include "ppg-util.h"


G_DEFINE_TYPE(PpgStopAction, ppg_stop_action, GTK_TYPE_TOGGLE_ACTION)


struct _PpgStopActionPrivate
{
	GtkWidget  *window;
};


enum
{
	PROP_0,
	PROP_WINDOW,
};


static void
ppg_stop_action_toggled (GtkToggleAction *toggle)
{
	GTK_TOGGLE_ACTION_CLASS(ppg_stop_action_parent_class)->toggled(toggle);
}


static void
ppg_stop_action_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_stop_action_parent_class)->finalize(object);
}


static void
ppg_stop_action_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	PpgStopActionPrivate *priv;

	priv = PPG_STOP_ACTION(object)->priv;
	switch (prop_id) {
	case PROP_WINDOW:
		g_value_set_object(value, priv->window);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


static void
ppg_stop_action_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	PpgStopActionPrivate *priv;

	priv = PPG_STOP_ACTION(object)->priv;
	switch (prop_id) {
	case PROP_WINDOW:
		priv->window = g_value_get_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


static void
ppg_stop_action_class_init (PpgStopActionClass *klass)
{
	GObjectClass *object_class;
	GtkToggleActionClass *toggle_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_stop_action_finalize;
	object_class->get_property = ppg_stop_action_get_property;
	object_class->set_property = ppg_stop_action_set_property;
	g_type_class_add_private(object_class, sizeof(PpgStopActionPrivate));

	toggle_class = GTK_TOGGLE_ACTION_CLASS(klass);
	toggle_class->toggled = ppg_stop_action_toggled;

	g_object_class_install_property(object_class,
	                                PROP_WINDOW,
	                                g_param_spec_object("window",
	                                                    "window",
	                                                    "window",
	                                                    GTK_TYPE_WIDGET,
	                                                    G_PARAM_READWRITE));
}


static void
ppg_stop_action_init (PpgStopAction *action)
{
	INIT_PRIV(action, STOP_ACTION, StopAction);

	g_object_set(action,
	             "active", FALSE,
	             "always-show-image", TRUE,
	             "icon-name", GTK_STOCK_MEDIA_STOP,
	             "label", _("Stop"),
	             "sensitive", FALSE,
	             "tooltip", _("Stop this profiling session"),
	             NULL);
}
