/* pka-source-glue.h
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

#ifndef __PKA_SOURCE_GLUE_H__
#define __PKA_SOURCE_GLUE_H__

#include <perfkit-agent/perfkit-agent.h>

#define SOURCE_INFO_PATH  "/org/perfkit/Agent/Plugins/Sources/%s"
#define SOURCE_INFO_QUARK g_quark_from_static_string("pka-source-info")

gboolean
pka_source_dbus_get_plugin (PkaSource  *source,
                            gchar     **path,
                            GError    **error)
{
	PkaSourceInfo *info;

	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);
	g_return_val_if_fail(path != NULL, FALSE);

	info = g_object_get_qdata(G_OBJECT(source), SOURCE_INFO_QUARK);
	*path = g_strdup_printf(SOURCE_INFO_PATH, pka_source_info_get_uid(info));

	return TRUE;
}

#endif /* __PKA_SOURCE_GLUE_H__ */
