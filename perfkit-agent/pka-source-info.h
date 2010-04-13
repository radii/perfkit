/* pka-source-info.h
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

#if !defined (__PERFKIT_AGENT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-agent/perfkit-agent.h> can be included directly."
#endif

#ifndef __PKA_SOURCE_INFO_H__
#define __PKA_SOURCE_INFO_H__

#include <glib-object.h>

#include "pka-source.h"

G_BEGIN_DECLS

#define PKA_TYPE_SOURCE_INFO            (pka_source_info_get_type())
#define PKA_SOURCE_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_SOURCE_INFO, PkaSourceInfo))
#define PKA_SOURCE_INFO_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_SOURCE_INFO, PkaSourceInfo const))
#define PKA_SOURCE_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_SOURCE_INFO, PkaSourceInfoClass))
#define PKA_IS_SOURCE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_SOURCE_INFO))
#define PKA_IS_SOURCE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_SOURCE_INFO))
#define PKA_SOURCE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_SOURCE_INFO, PkaSourceInfoClass))
#define PKA_SOURCE_INFO_ERROR           (pka_source_info_error_quark())

typedef struct _PkaSourceInfo        PkaSourceInfo;
typedef struct _PkaSourceInfoClass   PkaSourceInfoClass;
typedef struct _PkaSourceInfoPrivate PkaSourceInfoPrivate;

/**
 * PkaSourceFactory:
 *
 * The #PkaSourceFactory is a function prototype for creating new instances
 * of #PkaSource via their respective plugin.  When a shared-module containing
 * a source plugin is opened, a structure is read to extract meta-data on
 * the plugin.  One field within that structure is "factory".  This method
 * is executed to createa a new instance of the source to add to a #PkaChannel.
 *
 * Methods implmenting this prototype should simply create a new instance of
 * the #PkaSource.  Configuration can happen in a later stage.
 */
typedef PkaSource* (*PkaSourceFactory) (void);

/**
 * PkaStaticSourceInfo:
 *
 * #PkaStaticSourceInfo is a structure that should be exported by
 * source plugins under the symbol "pka_source_plugin".  This symbol is read
 * when the shared-module is loaded by the plugins subsystem.  The data is
 * then used to construct a #PkaSourceInfo.
 */
typedef struct
{
	const gchar      *uid;
	const gchar      *version;
	const gchar      *name;
	const gchar      *description;
	const gchar      *conflicts;
	PkaSourceFactory   factory;
} PkaStaticSourceInfo;

/**
 * PkaSourceInfoError:
 * @PKA_SOURCE_INFO_ERROR_FILENAME
 * @PKA_SOURCE_INFO_ERROR_MODULE
 * @PKA_SOURCE_INFO_ERROR_SYMBOL
 *
 * #PkaSourceInfo error enumeration.
 */
typedef enum
{
	PKA_SOURCE_INFO_ERROR_FILENAME,
	PKA_SOURCE_INFO_ERROR_MODULE,
	PKA_SOURCE_INFO_ERROR_SYMBOL,
} PkaSourceInfoError;

struct _PkaSourceInfo
{
	GObject parent;

	/*< private >*/
	PkaSourceInfoPrivate *priv;
};

struct _PkaSourceInfoClass
{
	GObjectClass parent_class;
};

GType           pka_source_info_get_type        (void) G_GNUC_CONST;
GQuark          pka_source_info_error_quark     (void) G_GNUC_CONST;
PkaSourceInfo*  pka_source_info_new             (void);
gboolean        pka_source_info_load_from_file  (PkaSourceInfo  *source_info,
                                                 const gchar    *filename,
                                                 GError        **error);
gboolean        pka_source_info_conflicts       (PkaSourceInfo  *source_info,
                                                 PkaSourceInfo  *other);
const gchar*    pka_source_info_get_uid         (PkaSourceInfo  *source_info);
const gchar*    pka_source_info_get_name        (PkaSourceInfo  *source_info);
const gchar*    pka_source_info_get_description (PkaSourceInfo  *source_info);
const gchar*    pka_source_info_get_version     (PkaSourceInfo  *source_info);
PkaSource*      pka_source_info_create          (PkaSourceInfo  *source_info);

G_END_DECLS

#endif /* __PKA_SOURCE_INFO_H__ */
