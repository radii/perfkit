/* pkg-runtime.h
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

#ifndef __PKG_RUNTIME_H__
#define __PKG_RUNTIME_H__

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "pkg-service.h"

G_BEGIN_DECLS

void             pkg_runtime_initialize     (void);
void             pkg_runtime_shutdown       (void);
void             pkg_runtime_run            (void);
void             pkg_runtime_quit           (void);
void             pkg_runtime_add_service    (const gchar *name, PkgService *service);
PkgService*      pkg_runtime_get_service    (const gchar *name);
void             pkg_runtime_remove_service (const gchar *name);
DBusGConnection* pkg_runtime_get_connection (void);

G_END_DECLS

#endif /* __PKG_RUNTIME_H__ */
