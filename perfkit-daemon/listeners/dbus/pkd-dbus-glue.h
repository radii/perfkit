#ifndef __PKD_DBUS_GLUE_H__
#define __PKD_DBUS_GLUE_H__

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <pkd-dbus.h>
#include <glib/gi18n.h>

#include <perfkit-daemon/perfkit-daemon.h>

static gboolean
pkd_channel_add_source_dbus (PkdChannel   *channel,
                             const gchar  *path,
                             gchar       **spath,
                             GError      **error)
{
	DBusGConnection *conn = pkd_dbus_get_connection();
	PkdSourceInfo *info = (gpointer)dbus_g_connection_lookup_g_object(conn, path);
	PkdSource *source;

	if (!info) {
		g_set_error(error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_STATE,
		            _("Invalid source path"));
		return FALSE;
	}

	source = pkd_channel_add_source(channel, info);
	if (!source) {
		g_set_error(error, PKD_CHANNEL_ERROR, PKD_CHANNEL_ERROR_STATE,
		            _("Couldn't create source"));
		return FALSE;
	}

	*spath = g_strdup_printf("/com/dronelabs/Perfkit/Sources/%d",
	                         pkd_source_get_id(source));

	return TRUE;
}

#endif /* __PKD_DBUS_GLUE_H__ */
