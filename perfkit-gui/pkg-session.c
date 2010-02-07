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
pkg_session_finalize (GObject *object)
{
	PkgSessionPrivate *priv;

	g_return_if_fail(PKG_IS_SESSION (object));

	priv = PKG_SESSION(object)->priv;

	G_OBJECT_CLASS(pkg_session_parent_class)->finalize(object);
}

static void
pkg_session_class_init (PkgSessionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pkg_session_finalize;
	g_type_class_add_private(object_class, sizeof(PkgSessionPrivate));
}

static void
pkg_session_init (PkgSession *session)
{
	session->priv = G_TYPE_INSTANCE_GET_PRIVATE(session,
	                                            PKG_TYPE_SESSION,
	                                            PkgSessionPrivate);
}
