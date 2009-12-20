/* pkd-listener.c
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

#include "pkd-listener.h"

/**
 * SECTION:pkd-listener
 * @title: PkdListener
 * @short_description: External communication
 *
 * #PkdListener<!-- -->'s are responsible for providing communication
 * with external resources.  Be it over TCP, DBus, or a Unix socket.
 */

GType
pkd_listener_get_type (void)
{
	static GType g_type = 0;

	if (G_UNLIKELY (!g_type)) {
		const GTypeInfo g_type_info = {
			sizeof (PkdListenerIface),
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
		                                 "PkdListener",
		                                 &g_type_info,
		                                 0);
		g_type_interface_add_prerequisite (g_type, G_TYPE_OBJECT);
	}

	return g_type;
}

/**
 * pkd_listener_listen:
 * @listener: a #PkdListener
 * @error: a location for a #GError or %NULL
 *
 * 
 *
 * Return value:
 *       %TRUE on success.
 *
 * Side effects:
 *       
 */
gboolean
pkd_listener_listen (PkdListener  *listener,
                     GError     **error)
{
	if (PKD_LISTENER_GET_INTERFACE (listener)->listen)
		return PKD_LISTENER_GET_INTERFACE (listener)->listen (listener, error);
	return TRUE;
}

/**
 * pkd_listener_shutdown:
 * @listener: a #PkdListener
 *
 * 
 *
 * Side effects: 
 */
void
pkd_listener_shutdown (PkdListener *listener)
{
	if (PKD_LISTENER_GET_INTERFACE (listener)->shutdown)
		PKD_LISTENER_GET_INTERFACE (listener)->shutdown (listener);
}

/**
 * pkd_listener_initialize:
 * @listener: a #PkdListener
 * @error: a location for a #GError, or %NULL
 *
 * Initializes the #PkdListener.
 *
 * Return value:
 *       %TRUE if successful; otherwise %FALSE and @error is set.
 *
 * Side effects:
 *       Initial resources are allocated for the listener.
 */
gboolean
pkd_listener_initialize (PkdListener  *listener,
                        GError     **error)
{
	if (PKD_LISTENER_GET_INTERFACE (listener)->initialize)
		return PKD_LISTENER_GET_INTERFACE (listener)->
				initialize (listener, error);
	return TRUE;
}
