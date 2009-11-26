#include <glib-object.h>
#include <ethos/ethos.h>
#include <perfkit-daemon/pkd-sources.h>
#include <perfkit-daemon/pkd-runtime.h>
#include <gmodule.h>

#define SOURCE_PLUGIN(Name,name,NAME,TYPE) \
 \
typedef struct _Name##Plugin		Name##Plugin; \
typedef struct _Name##PluginClass	Name##PluginClass; \
typedef struct _Name##PluginPrivate	Name##PluginPrivate; \
 \
struct _Name##Plugin \
{ \
	EthosPlugin parent; \
 \
	Name##PluginPrivate *priv; \
}; \
 \
struct _Name##PluginClass \
{ \
	EthosPluginClass parent_class; \
}; \
 \
G_DEFINE_TYPE (Name##Plugin, name##_plugin, ETHOS_TYPE_PLUGIN) \
 \
static void \
name##_plugin_finalize (GObject *object) \
{ \
	G_OBJECT_CLASS (name##_plugin_parent_class)->finalize (object); \
} \
 \
static void \
name##_plugin_class_init (Name##PluginClass *klass) \
{ \
	GObjectClass *object_class; \
\
	object_class = G_OBJECT_CLASS (klass); \
	object_class->finalize = name##_plugin_finalize; \
} \
 \
static void \
name##_plugin_init (Name##Plugin *plugin) \
{ \
	PkdSources *sources; \
\
	sources = PKD_SOURCES (pkd_runtime_get_service ("Sources")); \
	pkd_sources_register (sources, (TYPE)); \
} \
 \
EthosPlugin* \
name##_plugin_new (void) \
{ \
	return g_object_new (name##_plugin_get_type(), NULL); \
} \
 \
G_MODULE_EXPORT EthosPlugin* \
ethos_plugin_register (void) \
{ \
	return name##_plugin_new (); \
} \

