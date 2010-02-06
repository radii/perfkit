/* pkg-window.h
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

#ifndef __PKG_WINDOW_H__
#define __PKG_WINDOW_H__

#include <gtk/gtk.h>
#include <perfkit/perfkit.h>

G_BEGIN_DECLS

#define PKG_TYPE_WINDOW            (pkg_window_get_type())
#define PKG_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_WINDOW, PkgWindow))
#define PKG_WINDOW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_WINDOW, PkgWindow const))
#define PKG_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_WINDOW, PkgWindowClass))
#define PKG_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_WINDOW))
#define PKG_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_WINDOW))
#define PKG_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_WINDOW, PkgWindowClass))

typedef struct _PkgWindow        PkgWindow;
typedef struct _PkgWindowClass   PkgWindowClass;
typedef struct _PkgWindowPrivate PkgWindowPrivate;

struct _PkgWindow
{
	GtkWindow parent;

	/*< private >*/
	PkgWindowPrivate *priv;
};

struct _PkgWindowClass
{
	GtkWindowClass parent_class;
};

GType      pkg_window_get_type       (void) G_GNUC_CONST;
GtkWidget* pkg_window_new            (void);
GtkWidget* pkg_window_new_for_uri    (const gchar *uri);
gint       pkg_window_count_windows  (void);
void       pkg_window_set_connection (PkgWindow    *window,
                                      PkConnection *connection);

G_END_DECLS

#endif /* __PKG_WINDOW_H__ */
