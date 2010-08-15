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

#include "ppg-dialog.h"

G_BEGIN_DECLS

#define EXTRACT_WIDGET(b, n, w)                       \
    G_STMT_START {                                    \
        w = GTK_WIDGET(gtk_builder_get_object(b, n)); \
    } G_STMT_END
#define EXTRACT_OBJECT(b, t, n, o)                    \
    G_STMT_START {                                    \
        o = (t)(gtk_builder_get_object(b, n));        \
    } G_STMT_END
#define EXTRACT_MENU_ITEM(b, n, f)                    \
    G_STMT_START {                                    \
        GtkWidget *w;                                 \
        w = GTK_WIDGET(gtk_builder_get_object(b, n)); \
        g_signal_connect(w, "activate",               \
                         G_CALLBACK(f), NULL);        \
    } G_STMT_END
#define EXTRACT_ACTION(b, n, o, f, d)                 \
    G_STMT_START {                                    \
        o = GTK_ACTION(gtk_builder_get_object(b, n)); \
        g_signal_connect((o), "activate",             \
                         G_CALLBACK(f), d);           \
    } G_STMT_END
#define DISPLAY_ERROR(p, f1, f2, ...)                 \
    G_STMT_START {                                    \
        gchar *m;                                     \
        m = g_strdup_printf(f2, ## __VA_ARGS__);      \
        ppg_dialog_warning(GTK_WIDGET(p), "", f1, m,  \
                           TRUE);                     \
    } G_STMT_END
#define G_ASYNC(f) ((GAsyncReadyCallback)f)

G_END_DECLS

#endif /* __PPG_UTIL_H__ */
