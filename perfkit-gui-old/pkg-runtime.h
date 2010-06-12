/* pkg-runtime.h
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

#ifndef __PKG_RUNTIME_H__
#define __PKG_RUNTIME_H__

#include <ethos/ethos.h>

#include "pkg-window.h"

G_BEGIN_DECLS

gboolean      pkg_runtime_initialize        (gint     *argc,
                                             gchar  ***argv,
                                             GError  **error);
void          pkg_runtime_shutdown          (void);
void          pkg_runtime_run               (void);
void          pkg_runtime_quit              (void);
EthosManager* pkg_runtime_get_manager       (void);
PkgWindow*    pkg_runtime_get_active_window (void);
void          pkg_runtime_set_active_window (PkgWindow *window);

G_END_DECLS

#endif /* __PKG_RUNTIME_H__ */
