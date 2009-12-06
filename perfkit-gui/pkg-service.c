/* pkg-service.c
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

#include "pkg-service.h"

/**
 * SECTION:pkg-service
 * @title: 
 * @short_description: 
 *
 * Overview of services here.
 */

GType
pkg_service_get_type (void)
{
	static GType g_type = 0;

	if (G_UNLIKELY (!g_type)) {
		const GTypeInfo g_type_info = {
			sizeof (PkgServiceIface),
			NULL, /* base_init      */
			NULL, /* base_finalize  */
			NULL, /* class_init     */
			NULL, /* class_finalize */
			NULL, /* class_data     */
			0,    /* instance_size  */
			0,    /* n_preallocs    */
			NULL, /* instance_init  */
			NULL  /* value_table    */
		};

		g_type = g_type_register_static (G_TYPE_INTERFACE,
		                                 "PkgService",
		                                 &g_type_info,
		                                 0);
		g_type_interface_add_prerequisite (g_type, G_TYPE_OBJECT);
	}

	return g_type;
}

/**
 * pkg_service_start:
 * @service: a #PkgService
 * @error: a location for a #GError or %NULL
 *
 * Starts the #PkgService.  Upon failure, @error is set and %FALSE is
 * returned.
 *
 * Return value: %TRUE on success.
 */
gboolean
pkg_service_start (PkgService *service,
                          GError        **error)
{
    if (PKG_SERVICE_GET_INTERFACE (service)->start)
        return PKG_SERVICE_GET_INTERFACE (service)->start (service, error);
    return TRUE;
}

/**
 * pkg_service_stop:
 * @service: a #PkgService
 *
 * Stops the #PkgService.
 */
void
pkg_service_stop (PkgService *service)
{
    if (PKG_SERVICE_GET_INTERFACE (service)->stop)
        PKG_SERVICE_GET_INTERFACE (service)->stop (service);
}

/**
 * pkg_service_initialize:
 * @service: a #PkgService
 * @error: a location for a #GError, or %NULL
 *
 * Stops the #PkgService.
 */
gboolean
pkg_service_initialize (PkgService *service,
                                GError    **error)
{
    if (PKG_SERVICE_GET_INTERFACE (service)->initialize)
        return PKG_SERVICE_GET_INTERFACE (service)->initialize (service, error);
    return TRUE;
}
