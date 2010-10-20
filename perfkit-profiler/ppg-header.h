/* ppg-header.h
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

#ifndef __PPG_HEADER_H__
#define __PPG_HEADER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_HEADER            (ppg_header_get_type())
#define PPG_HEADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_HEADER, PpgHeader))
#define PPG_HEADER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_HEADER, PpgHeader const))
#define PPG_HEADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_HEADER, PpgHeaderClass))
#define PPG_IS_HEADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_HEADER))
#define PPG_IS_HEADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_HEADER))
#define PPG_HEADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_HEADER, PpgHeaderClass))

typedef struct _PpgHeader        PpgHeader;
typedef struct _PpgHeaderClass   PpgHeaderClass;
typedef struct _PpgHeaderPrivate PpgHeaderPrivate;

struct _PpgHeader
{
	GtkDrawingArea parent;

	/*< private >*/
	PpgHeaderPrivate *priv;
};

struct _PpgHeaderClass
{
	GtkDrawingAreaClass parent_class;
};

GType ppg_header_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_HEADER_H__ */
