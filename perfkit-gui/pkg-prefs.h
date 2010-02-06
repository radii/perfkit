/* pkg-prefs.h
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

#ifndef __PERFKIT_GUI_PREFS__
#define __PERFKIT_GUI_PREFS__

#include <glib.h>

G_BEGIN_DECLS

gboolean pkg_prefs_init (gint *argc, gchar ***argv, GError **error);

G_END_DECLS

#endif /* __PERFKIT_GUI_PREFS__ */
