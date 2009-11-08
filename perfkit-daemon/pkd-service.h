/* pkd-service.h
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

#ifndef __PKD_SERVICE_H__
#define __PKD_SERVICE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_SERVICE             (pkd_service_get_type())
#define PKD_SERVICE(o)               (G_TYPE_CHECK_INSTANCE_CAST((o),    PKD_TYPE_SERVICE, PkdService))
#define PKD_IS_SERVICE(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o),    PKD_TYPE_SERVICE))
#define PKD_SERVICE_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), PKD_TYPE_SERVICE, PkdServiceIface))

typedef struct _PkdService      PkdService;
typedef struct _PkdServiceIface PkdServiceIface;

struct _PkdServiceIface
{
	GTypeInterface parent;

	gboolean (*initialize) (PkdService *service, GError **error);
	gboolean (*start)      (PkdService *service, GError **error);
	void     (*stop)       (PkdService *service);
};

GType    pkd_service_get_type   (void) G_GNUC_CONST;
gboolean pkd_service_initialize (PkdService *service, GError **error);
gboolean pkd_service_start      (PkdService *service, GError **error);
void     pkd_service_stop       (PkdService *service);

G_END_DECLS

#endif /* __PKD_SERVICE_H__ */