/* pkg-session.c
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

#include <perfkit/perfkit.h>

#include "pkg-session.h"

G_DEFINE_TYPE(PkgSession, pkg_session, G_TYPE_OBJECT)

/**
 * SECTION:pkg-session
 * @title: PkgSession
 * @short_description: 
 *
 * 
 */

struct _PkgSessionPrivate
{
	PkConnection *conn;
};

enum
{
	PROP_0,
	PROP_CONNECTION,
};

/**
 * pkg_session_new:
 *
 * Creates a new instance of #PkgSession.
 *
 * Return value: the newly created #PkgSession instance.
 */
PkgSession*
pkg_session_new (void)
{
	return g_object_new(PKG_TYPE_SESSION, NULL);
}

static void
pkg_session_set_connection (PkgSession   *session,
                            PkConnection *connection)
{
	g_return_if_fail(PKG_IS_SESSION(session));
	session->priv->conn = g_object_ref(connection);
}

PkConnection*
pkg_session_get_connection (PkgSession *session)
{
	g_return_val_if_fail(PKG_IS_SESSION(session), NULL);
	return session->priv->conn;
}

static void
pkg_session_finalize (GObject *object)
{
	PkgSessionPrivate *priv;

	g_return_if_fail(PKG_IS_SESSION (object));

	priv = PKG_SESSION(object)->priv;

	G_OBJECT_CLASS(pkg_session_parent_class)->finalize(object);
}

static void
pkg_session_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_CONNECTION:
		pkg_session_set_connection(PKG_SESSION(object),
		                           PK_CONNECTION(g_value_get_object(value)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pkg_session_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_CONNECTION:
		g_value_set_object(value,
		                   pkg_session_get_connection(PKG_SESSION(object)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pkg_session_class_init (PkgSessionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_session_finalize;
	object_class->set_property = pkg_session_set_property;
	object_class->get_property = pkg_session_get_property;
	g_type_class_add_private(object_class, sizeof(PkgSessionPrivate));

	/**
	 * PkgSession:connection:
	 *
	 * The "connection" property.
	 */
	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pkg_session_init (PkgSession *session)
{
	session->priv = G_TYPE_INSTANCE_GET_PRIVATE(session,
	                                            PKG_TYPE_SESSION,
	                                            PkgSessionPrivate);
}
