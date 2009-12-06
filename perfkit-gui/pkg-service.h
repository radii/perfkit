/* pkg-service.h
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

#ifndef __PKG_SERVICE_H__
#define __PKG_SERVICE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKG_TYPE_SERVICE             (pkg_service_get_type())
#define PKG_SERVICE(o)               (G_TYPE_CHECK_INSTANCE_CAST((o),    PKG_TYPE_SERVICE, PkgService))
#define PKG_IS_SERVICE(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o),    PKG_TYPE_SERVICE))
#define PKG_SERVICE_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), PKG_TYPE_SERVICE, PkgServiceIface))

typedef struct _PkgService      PkgService;
typedef struct _PkgServiceIface PkgServiceIface;

struct _PkgServiceIface
{
	GTypeInterface parent;

	gboolean (*initialize) (PkgService *service, GError **error);
	gboolean (*start)      (PkgService *service, GError **error);
	void     (*stop)       (PkgService *service);
};

GType    pkg_service_get_type   (void) G_GNUC_CONST;
gboolean pkg_service_initialize (PkgService *service, GError **error);
gboolean pkg_service_start      (PkgService *service, GError **error);
void     pkg_service_stop       (PkgService *service);

G_END_DECLS

#endif /* __PKG_SERVICE_H__ */