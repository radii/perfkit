/* pkg-gui-service.h
 * 
 * Copyright (C) 2009 Christian Hergert
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

#ifndef __PKG_GUI_SERVICE_H__
#define __PKG_GUI_SERVICE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKG_TYPE_GUI_SERVICE            (pkg_gui_service_get_type ())
#define PKG_GUI_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_GUI_SERVICE, PkgGuiService))
#define PKG_GUI_SERVICE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKG_TYPE_GUI_SERVICE, PkgGuiService const))
#define PKG_GUI_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKG_TYPE_GUI_SERVICE, PkgGuiServiceClass))
#define PKG_IS_GUI_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKG_TYPE_GUI_SERVICE))
#define PKG_IS_GUI_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKG_TYPE_GUI_SERVICE))
#define PKG_GUI_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKG_TYPE_GUI_SERVICE, PkgGuiServiceClass))

typedef struct _PkgGuiService        PkgGuiService;
typedef struct _PkgGuiServiceClass   PkgGuiServiceClass;
typedef struct _PkgGuiServicePrivate PkgGuiServicePrivate;

struct _PkgGuiService
{
	GObject parent;

	/*< private >*/
	PkgGuiServicePrivate *priv;
};

struct _PkgGuiServiceClass
{
	GObjectClass parent_class;
};

GType      pkg_gui_service_get_type    (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PKG_GUI_SERVICE_H__ */
