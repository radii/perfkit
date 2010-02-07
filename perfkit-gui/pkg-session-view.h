/* pkg-session-view.h
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

#ifndef __PKG_SESSION_VIEW_H__
#define __PKG_SESSION_VIEW_H__

#include <gtk/gtk.h>

#include "pkg-session.h"

G_BEGIN_DECLS

#define PKG_TYPE_SESSION_VIEW            (pkg_session_view_get_type())
#define PKG_SESSION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SESSION_VIEW, PkgSessionView))
#define PKG_SESSION_VIEW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_SESSION_VIEW, PkgSessionView const))
#define PKG_SESSION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_SESSION_VIEW, PkgSessionViewClass))
#define PKG_IS_SESSION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_SESSION_VIEW))
#define PKG_IS_SESSION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_SESSION_VIEW))
#define PKG_SESSION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_SESSION_VIEW, PkgSessionViewClass))

typedef struct _PkgSessionView        PkgSessionView;
typedef struct _PkgSessionViewClass   PkgSessionViewClass;
typedef struct _PkgSessionViewPrivate PkgSessionViewPrivate;

struct _PkgSessionView
{
	GtkVBox parent;

	/*< private >*/
	PkgSessionViewPrivate *priv;
};

struct _PkgSessionViewClass
{
	GtkVBoxClass parent_class;
};

GType       pkg_session_view_get_type    (void) G_GNUC_CONST;
GtkWidget*  pkg_session_view_new         (void);
void        pkg_session_view_set_session (PkgSessionView *session_view,
                                          PkgSession     *session);
PkgSession* pkg_session_view_get_session (PkgSessionView *session_view);

G_END_DECLS

#endif /* __PKG_SESSION_VIEW_H__ */
