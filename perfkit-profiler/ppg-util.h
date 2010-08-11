/* ppg-util.h
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

#if !defined (__PERFKIT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-profiler/perfkit-profiler.h> can be included directly."
#endif

#ifndef __PPG_UTIL_H__
#define __PPG_UTIL_H__

#include <glib.h>

G_BEGIN_DECLS

#define EXTRACT_WIDGET(b, n, w)                       \
    G_STMT_START {                                    \
        w = GTK_WIDGET(gtk_builder_get_object(b, n)); \
    } G_STMT_END
#define EXTRACT_OBJECT(b, t, n, o)                    \
    G_STMT_START {                                    \
        o = (t)(gtk_builder_get_object(b, n));        \
    } G_STMT_END

G_END_DECLS

#endif /* __PPG_UTIL_H__ */
