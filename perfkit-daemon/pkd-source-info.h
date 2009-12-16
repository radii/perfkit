/* pkd-source-info.h
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

#ifndef __PKD_SOURCE_INFO_H__
#define __PKD_SOURCE_INFO_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_SOURCE_INFO            (pkd_source_info_get_type())
#define PKD_SOURCE_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCE_INFO, PkdSourceInfo))
#define PKD_SOURCE_INFO_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCE_INFO, PkdSourceInfo const))
#define PKD_SOURCE_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_SOURCE_INFO, PkdSourceInfoClass))
#define PKD_IS_SOURCE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_SOURCE_INFO))
#define PKD_IS_SOURCE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_SOURCE_INFO))
#define PKD_SOURCE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_SOURCE_INFO, PkdSourceInfoClass))

typedef struct _PkdSourceInfo        PkdSourceInfo;
typedef struct _PkdSourceInfoClass   PkdSourceInfoClass;
typedef struct _PkdSourceInfoPrivate PkdSourceInfoPrivate;

struct _PkdSourceInfo
{
	GObject parent;

	/*< private >*/
	PkdSourceInfoPrivate *priv;
};

struct _PkdSourceInfoClass
{
	GObjectClass parent_class;
};

GType                 pkd_source_info_get_type        (void) G_GNUC_CONST;
G_CONST_RETURN gchar* pkd_source_info_get_uid         (PkdSourceInfo *source_info);
G_CONST_RETURN gchar* pkd_source_info_get_name        (PkdSourceInfo *source_info);
G_CONST_RETURN gchar* pkd_source_info_get_description (PkdSourceInfo *source_info);
G_CONST_RETURN gchar* pkd_source_info_get_version     (PkdSourceInfo *source_info);

G_END_DECLS

#endif /* __PKD_SOURCE_INFO_H__ */
