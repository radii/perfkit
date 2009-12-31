/* pkd-source-info.h
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

#if !defined (__PERFKIT_DAEMON_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-daemon/perfkit-daemon.h> can be included directly."
#endif

#ifndef __PKD_SOURCE_INFO_H__
#define __PKD_SOURCE_INFO_H__

#include <glib-object.h>

#include "pkd-source.h"

G_BEGIN_DECLS

#define PKD_TYPE_SOURCE_INFO            (pkd_source_info_get_type())
#define PKD_SOURCE_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCE_INFO, PkdSourceInfo))
#define PKD_SOURCE_INFO_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKD_TYPE_SOURCE_INFO, PkdSourceInfo const))
#define PKD_SOURCE_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKD_TYPE_SOURCE_INFO, PkdSourceInfoClass))
#define PKD_IS_SOURCE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKD_TYPE_SOURCE_INFO))
#define PKD_IS_SOURCE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKD_TYPE_SOURCE_INFO))
#define PKD_SOURCE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKD_TYPE_SOURCE_INFO, PkdSourceInfoClass))
#define PKD_SOURCE_INFO_ERROR           (pkd_source_info_error_quark())

typedef struct _PkdSourceInfo        PkdSourceInfo;
typedef struct _PkdSourceInfoClass   PkdSourceInfoClass;
typedef struct _PkdSourceInfoPrivate PkdSourceInfoPrivate;

/**
 * PkdSourceFactory:
 *
 * The #PkdSourceFactory is a function prototype for creating new instances
 * of #PkdSource via their respective plugin.  When a shared-module containing
 * a source plugin is opened, a structure is read to extract meta-data on
 * the plugin.  One field within that structure is "factory".  This method
 * is executed to createa a new instance of the source to add to a #PkdChannel.
 *
 * Methods implmenting this prototype should simply create a new instance of
 * the #PkdSource.  Configuration can happen in a later stage.
 */
typedef PkdSource* (*PkdSourceFactory) (void);

/**
 * PkdStaticSourceInfo:
 *
 * #PkdStaticSourceInfo is a structure that should be exported by
 * source plugins under the symbol "pkd_source_plugin".  This symbol is read
 * when the shared-module is loaded by the plugins subsystem.  The data is
 * then used to construct a #PkdSourceInfo.
 */
typedef struct
{
	const gchar      *uid;
	const gchar      *version;
	const gchar      *name;
	const gchar      *description;
	const gchar      *conflicts;
	PkdSourceFactory   factory;
} PkdStaticSourceInfo;

/**
 * PkdSourceInfoError:
 * @PKD_SOURCE_INFO_ERROR_FILENAME
 * @PKD_SOURCE_INFO_ERROR_MODULE
 * @PKD_SOURCE_INFO_ERROR_SYMBOL
 *
 * #PkdSourceInfo error enumeration.
 */
typedef enum
{
	PKD_SOURCE_INFO_ERROR_FILENAME,
	PKD_SOURCE_INFO_ERROR_MODULE,
	PKD_SOURCE_INFO_ERROR_SYMBOL,
} PkdSourceInfoError;

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

GType           pkd_source_info_get_type        (void) G_GNUC_CONST;
GQuark          pkd_source_info_error_quark     (void) G_GNUC_CONST;
PkdSourceInfo*  pkd_source_info_new             (void);
gboolean        pkd_source_info_load_from_file  (PkdSourceInfo  *source_info,
                                                 const gchar    *filename,
                                                 GError        **error);
gboolean        pkd_source_info_conflicts       (PkdSourceInfo  *source_info,
                                                 PkdSourceInfo  *other);
const gchar*    pkd_source_info_get_uid         (PkdSourceInfo  *source_info);
const gchar*    pkd_source_info_get_name        (PkdSourceInfo  *source_info);
const gchar*    pkd_source_info_get_description (PkdSourceInfo  *source_info);
const gchar*    pkd_source_info_get_version     (PkdSourceInfo  *source_info);

G_END_DECLS

#endif /* __PKD_SOURCE_INFO_H__ */
