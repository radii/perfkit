/* ppg-status-actor.h
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

#ifndef __PPG_STATUS_ACTOR_H__
#define __PPG_STATUS_ACTOR_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define PPG_TYPE_STATUS_ACTOR            (ppg_status_actor_get_type())
#define PPG_STATUS_ACTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_STATUS_ACTOR, PpgStatusActor))
#define PPG_STATUS_ACTOR_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_STATUS_ACTOR, PpgStatusActor const))
#define PPG_STATUS_ACTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_STATUS_ACTOR, PpgStatusActorClass))
#define PPG_IS_STATUS_ACTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_STATUS_ACTOR))
#define PPG_IS_STATUS_ACTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_STATUS_ACTOR))
#define PPG_STATUS_ACTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_STATUS_ACTOR, PpgStatusActorClass))

typedef struct _PpgStatusActor        PpgStatusActor;
typedef struct _PpgStatusActorClass   PpgStatusActorClass;
typedef struct _PpgStatusActorPrivate PpgStatusActorPrivate;

struct _PpgStatusActor
{
	ClutterCairoTexture parent;

	/*< private >*/
	PpgStatusActorPrivate *priv;
};

struct _PpgStatusActorClass
{
	ClutterCairoTextureClass parent_class;
};

GType ppg_status_actor_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_STATUS_ACTOR_H__ */
