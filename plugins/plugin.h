#include <gmodule.h>
#include <ethos/ethos.h>
#include <perfkit-daemon/perfkit-daemon.h>

#define PKD_SOURCE_SIMPLE_REGISTER(uid,name,version,description,callback) \
\
static PkdSource* \
pkd_factory (const gchar *type_name, \
             gpointer     user_data) \
{ \
	PkdSource *source; \
\
	source = pkd_source_simple_new (); \
	pkd_source_simple_set_sample_func (PKD_SOURCE_SIMPLE (source), \
	                                   (callback), NULL, NULL); \
\
	return source; \
} \
static void \
activate_plugin (EthosPlugin *plugin, \
                 gpointer     user_data) \
{ \
	PkdSources *sources; \
	sources = PKD_SOURCES (pkd_runtime_get_service ("Sources")); \
	pkd_sources_register (sources, (uid), (name), (version), \
	                      (description), pkd_factory, NULL); \
} \
\
G_MODULE_EXPORT EthosPlugin* \
ethos_plugin_register (void) \
{ \
	EthosPlugin *plugin; \
\
 	plugin = g_object_new (ETHOS_TYPE_PLUGIN, NULL); \
 	g_signal_connect (plugin, "activated", G_CALLBACK (activate_plugin), NULL); \
\
	return plugin; \
}

