/* pkd-source-glue.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKD_SOURCE_GLUE_H__
#define __PKD_SOURCE_GLUE_H__

#include <perfkit-daemon/perfkit-daemon.h>

gboolean
pkd_source_dbus_get_plugin (PkdSource  *source,
                            gchar     **path,
                            GError    **error)
{
	PkdSourceInfo *info;

	g_return_val_if_fail(PKD_IS_SOURCE(source), FALSE);
	g_return_val_if_fail(path != NULL, FALSE);

	info = g_object_get_qdata(G_OBJECT(source),
	                          g_quark_from_static_string("pkd-source-info"));
	*path = g_strdup_printf("/com/dronelabs/Perfkit/Plugins/Sources/%s",
	                        pkd_source_info_get_uid(info));

	return TRUE;
}

#endif /* __PKD_SOURCE_GLUE_H__ */
