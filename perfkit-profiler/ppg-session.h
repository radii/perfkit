/* ppg-session.h
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

#ifndef __PPG_SESSION_H__
#define __PPG_SESSION_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PPG_TYPE_SESSION            (ppg_session_get_type())
#define PPG_SESSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_SESSION, PpgSession))
#define PPG_SESSION_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_SESSION, PpgSession const))
#define PPG_SESSION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_SESSION, PpgSessionClass))
#define PPG_IS_SESSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_SESSION))
#define PPG_IS_SESSION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_SESSION))
#define PPG_SESSION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_SESSION, PpgSessionClass))

typedef struct _PpgSession        PpgSession;
typedef struct _PpgSessionClass   PpgSessionClass;
typedef struct _PpgSessionPrivate PpgSessionPrivate;
typedef enum   _PpgSessionState   PpgSessionState;

enum _PpgSessionState
{
	PPG_SESSION_STOPPED = 1,
	PPG_SESSION_STARTED = 2,
	PPG_SESSION_PAUSED  = 3,
};

struct _PpgSession
{
	GObject parent;

	/*< private >*/
	PpgSessionPrivate *priv;
};

struct _PpgSessionClass
{
	GObjectClass parent_class;
};

GType    ppg_session_get_type (void) G_GNUC_CONST;
gboolean ppg_session_start    (PpgSession  *session,
                               GError     **error);
gboolean ppg_session_stop     (PpgSession  *session,
                               GError     **error);
gboolean ppg_session_pause    (PpgSession  *session,
                               GError     **error);
gboolean ppg_session_unpause  (PpgSession  *session,
                               GError     **error);

G_END_DECLS

#endif /* __PPG_SESSION_H__ */
