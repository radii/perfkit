/* pkd-service.c
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

#include "pkd-service.h"

/**
 * SECTION:pkd-service
 * @title: PkdService
 * @short_description: Application services
 *
 * #PkdService provides a way to add application services to the Perfkit
 * Daemon.  Plugins can implement services and have them managed by the
 * daemon.
 */

GType
pkd_service_get_type (void)
{
	static GType g_type = 0;

	if (G_UNLIKELY (!g_type)) {
		const GTypeInfo g_type_info = {
			sizeof (PkdServiceIface),
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
		                                 "PkdService",
		                                 &g_type_info,
		                                 0);
		g_type_interface_add_prerequisite (g_type, G_TYPE_OBJECT);
	}

	return g_type;
}

/**
 * pkd_service_start:
 * @service: a #PkdService
 * @error: a location for a #GError or %NULL
 *
 * Starts the #PkdService.  Upon failure, @error is set and %FALSE is
 * returned.
 *
 * Return value: %TRUE on success.
 */
gboolean
pkd_service_start (PkdService  *service,
                   GError     **error)
{
	if (PKD_SERVICE_GET_INTERFACE (service)->start)
		return PKD_SERVICE_GET_INTERFACE (service)->start (service, error);
	return TRUE;
}

/**
 * pkd_service_stop:
 * @service: a #PkdService
 *
 * Stops the #PkdService.
 */
void
pkd_service_stop (PkdService *service)
{
	if (PKD_SERVICE_GET_INTERFACE (service)->stop)
		PKD_SERVICE_GET_INTERFACE (service)->stop (service);
}

/**
 * pkd_service_initialize:
 * @service: a #PkdService
 * @error: a location for a #GError, or %NULL
 *
 * Stops the #PkdService.
 *
 * Return value: %TRUE on success.  Upon failure, %FALSE is returned and
 *   @error is set.
 */
gboolean
pkd_service_initialize (PkdService  *service,
                        GError     **error)
{
	if (PKD_SERVICE_GET_INTERFACE (service)->initialize)
		return PKD_SERVICE_GET_INTERFACE (service)->initialize (service, error);
	return TRUE;
}
