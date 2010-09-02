/* ppg-session-view.h
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

#ifndef __PPG_SESSION_VIEW_H__
#define __PPG_SESSION_VIEW_H__

#include "ppg-session.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_SESSION_VIEW            (ppg_session_view_get_type())
#define PPG_SESSION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_SESSION_VIEW, PpgSessionView))
#define PPG_SESSION_VIEW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_SESSION_VIEW, PpgSessionView const))
#define PPG_SESSION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_SESSION_VIEW, PpgSessionViewClass))
#define PPG_IS_SESSION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_SESSION_VIEW))
#define PPG_IS_SESSION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_SESSION_VIEW))
#define PPG_SESSION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_SESSION_VIEW, PpgSessionViewClass))

typedef struct _PpgSessionView        PpgSessionView;
typedef struct _PpgSessionViewClass   PpgSessionViewClass;
typedef struct _PpgSessionViewPrivate PpgSessionViewPrivate;

struct _PpgSessionView
{
	GtkAlignment parent;

	/*< private >*/
	PpgSessionViewPrivate *priv;
};

struct _PpgSessionViewClass
{
	GtkAlignmentClass parent_class;
};

GType        ppg_session_view_get_type    (void) G_GNUC_CONST;
GtkWidget *  ppg_session_view_new         (void);
void         ppg_session_view_set_session (PpgSessionView *view,
                                           PpgSession     *session);
PpgSession * ppg_session_view_get_session (PpgSessionView *view);

G_END_DECLS

#endif /* __PPG_SESSION_VIEW_H__ */
