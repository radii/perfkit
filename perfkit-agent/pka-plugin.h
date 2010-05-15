/* pka-plugin.h
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

#ifndef __PKA_PLUGIN_H__
#define __PKA_PLUGIN_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKA_TYPE_PLUGIN            (pka_plugin_get_type())
#define PKA_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_PLUGIN, PkaPlugin))
#define PKA_PLUGIN_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_PLUGIN, PkaPlugin const))
#define PKA_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_PLUGIN, PkaPluginClass))
#define PKA_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_PLUGIN))
#define PKA_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_PLUGIN))
#define PKA_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_PLUGIN, PkaPluginClass))
#define PKA_PLUGIN_ERROR           (pka_plugin_error_quark())

typedef enum
{
	PKA_PLUGIN_INVALID = 0,
	PKA_PLUGIN_LISTENER,
	PKA_PLUGIN_SOURCE,
	PKA_PLUGIN_ENCODER,
} PkaPluginType;

typedef enum
{
	PKA_PLUGIN_ERROR_INVALID_MODULE,
	PKA_PLUGIN_ERROR_INVALID_TYPE,
} PkaPluginError;

typedef struct _PkaPlugin        PkaPlugin;
typedef struct _PkaPluginClass   PkaPluginClass;
typedef struct _PkaPluginPrivate PkaPluginPrivate;
typedef struct _PkaPluginInfo    PkaPluginInfo;

typedef GObject* (*PkaPluginFactory) (GError **error);

struct _PkaPluginInfo
{
	PkaPluginFactory  factory;
	PkaPluginType     plugin_type;
	const gchar      *id;
	const gchar      *name;
	const gchar      *version;
	const gchar      *description;
	const gchar      *copyright;
};

struct _PkaPlugin
{
	GObject parent;

	/*< private >*/
	PkaPluginPrivate *priv;
};

struct _PkaPluginClass
{
	GObjectClass parent_class;
};

GQuark        pka_plugin_error_quark      (void) G_GNUC_CONST;
GType         pka_plugin_get_type         (void) G_GNUC_CONST;
const gchar*  pka_plugin_get_copyright    (PkaPlugin    *plugin) G_GNUC_PURE;
const gchar*  pka_plugin_get_description  (PkaPlugin    *plugin) G_GNUC_PURE;
const gchar*  pka_plugin_get_id           (PkaPlugin    *plugin) G_GNUC_PURE;
const gchar*  pka_plugin_get_name         (PkaPlugin    *plugin) G_GNUC_PURE;
PkaPluginType pka_plugin_get_plugin_type  (PkaPlugin    *plugin) G_GNUC_PURE;
const gchar*  pka_plugin_get_version      (PkaPlugin    *plugin) G_GNUC_PURE;
PkaPlugin*    pka_plugin_new              (void);
gboolean      pka_plugin_load_from_file   (PkaPlugin    *plugin,
                                           const gchar  *filename,
                                           GError      **error);
GObject*      pka_plugin_create           (PkaPlugin    *plugin,
                                           GError      **error);
gboolean      pka_plugin_is_disabled      (PkaPlugin    *plugin);

G_END_DECLS

#endif /* __PKA_PLUGIN_H__ */
