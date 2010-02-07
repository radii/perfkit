/* pkg-session-view.c
 *
 * Copyright (C) 2010 Christian Hergert
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

#include "pkg-session-view.h"

G_DEFINE_TYPE(PkgSessionView, pkg_session_view, GTK_TYPE_VBOX)

/**
 * SECTION:pkg-session_view
 * @title: PkgSessionView
 * @short_description: 
 *
 * 
 */

struct _PkgSessionViewPrivate
{
	PkgSession *session;
	GtkWidget  *label;
};

enum
{
    LAST_SIGNAL
};

//static guint signals [LAST_SIGNAL];

enum
{
    PROP_0,
};

/**
 * pkg_session_view_new:
 *
 * Creates a new instance of #PkgSessionView.
 *
 * Return value: the newly created #PkgSessionView instance.
 */
GtkWidget*
pkg_session_view_new (void)
{
	return g_object_new(PKG_TYPE_SESSION_VIEW, NULL);
}

void
pkg_session_view_set_session (PkgSessionView *session_view,
                              PkgSession     *session)
{
}

GtkWidget*
pkg_session_view_get_label (PkgSessionView *session_view)
{
	return session_view->priv->label;
}

static void
pkg_session_view_finalize (GObject *object)
{
	PkgSessionViewPrivate *priv;

	g_return_if_fail (PKG_IS_SESSION_VIEW (object));

	priv = PKG_SESSION_VIEW (object)->priv;

	G_OBJECT_CLASS (pkg_session_view_parent_class)->finalize (object);
}

static void
pkg_session_view_class_init (PkgSessionViewClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_session_view_finalize;
	g_type_class_add_private(object_class, sizeof(PkgSessionViewPrivate));
}

static void
pkg_session_view_init (PkgSessionView *session_view)
{
	PkgSessionViewPrivate *priv;
	GtkWidget *icon,
	          *text;

	session_view->priv = G_TYPE_INSTANCE_GET_PRIVATE(session_view,
	                                                 PKG_TYPE_SESSION_VIEW,
	                                                 PkgSessionViewPrivate);
	priv = session_view->priv;

	priv->label = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(priv->label);

	icon = gtk_image_new_from_icon_name("computer", GTK_ICON_SIZE_MENU);
	gtk_box_pack_start(GTK_BOX(priv->label), icon, FALSE, TRUE, 0);
	gtk_widget_show(icon);

	text = gtk_label_new("Session 0");
	gtk_box_pack_start(GTK_BOX(priv->label), text, TRUE, TRUE, 0);
	gtk_widget_show(text);
}
