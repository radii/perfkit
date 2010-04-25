#ifndef __PKA_DBUS_GLUE_H__
#define __PKA_DBUS_GLUE_H__

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <pka-dbus.h>
#include <glib/gi18n.h>

#include <perfkit-agent/perfkit-agent.h>

static gboolean
pka_channel_add_source_dbus (PkaChannel   *channel,
                             const gchar  *path,
                             gchar       **spath,
                             GError      **error)
{
	DBusGConnection *conn = pka_dbus_get_connection();
	PkaSourceInfo *info = (gpointer)dbus_g_connection_lookup_g_object(conn, path);
	PkaSource *source;

	if (!info) {
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Invalid source path"));
		return FALSE;
	}

	source = pka_channel_add_source(channel, info);
	if (!source) {
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Couldn't create source"));
		return FALSE;
	}

	*spath = g_strdup_printf("/org/perfkit/Agent/Sources/%d",
	                         pka_source_get_id(source));

	return TRUE;
}

#endif /* __PKA_DBUS_GLUE_H__ */
