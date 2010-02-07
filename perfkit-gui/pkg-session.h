/* pkg-session.h
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-gui/perfkit-gui.h> can be included directly."
#endif

#ifndef __PKG_SESSION_H__
#define __PKG_SESSION_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKG_TYPE_SESSION            (pkg_session_get_type())
#define PKG_SESSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SESSION, PkgSession))
#define PKG_SESSION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SESSION, PkgSession const))
#define PKG_SESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_SESSION, PkgSessionClass))
#define PKG_IS_SESSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_SESSION))
#define PKG_IS_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_SESSION))
#define PKG_SESSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_SESSION, PkgSessionClass))

typedef struct _PkgSession        PkgSession;
typedef struct _PkgSessionClass   PkgSessionClass;
typedef struct _PkgSessionPrivate PkgSessionPrivate;

struct _PkgSession
{
	GObject parent;

	/*< private >*/
	PkgSessionPrivate *priv;
};

struct _PkgSessionClass
{
	GObjectClass parent_class;
};

GType       pkg_session_get_type (void) G_GNUC_CONST;
PkgSession* pkg_session_new      (void);

G_END_DECLS

#endif /* __PKG_SESSION_H__ */
