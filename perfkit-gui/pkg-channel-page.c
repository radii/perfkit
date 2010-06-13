/* pkg-channel-page.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "pkg-channel-page.h"
#include "pkg-log.h"
#include "pkg-path.h"

#define IS_NULL_OR_EMPTY(_s) ((!(_s)) || (g_strcmp0(_s, "") == 0))

G_DEFINE_TYPE(PkgChannelPage, pkg_channel_page, GTK_TYPE_ALIGNMENT)

/**
 * SECTION:pkg-channel-page
 * @title: PkgChannelPage
 * @short_description: 
 *
 * Section overview.
 */

struct _PkgChannelPagePrivate
{
	PkConnection *connection;
	gint          id;
	GtkBuilder   *builder;

	GtkWidget *page_label;
	GtkWidget *container;
	GtkWidget *target;
	GtkWidget *args;
	GtkWidget *working_dir;
	GtkWidget *env;
	GtkWidget *pid;
	GtkWidget *kill_pid;
	GtkWidget *exit_status;
	GtkWidget *muted;
};

enum
{
	PROP_0,
	PROP_CONNECTION,
	PROP_ID,
};

/**
 * pkg_channel_page_new:
 * @connection: A #PkConnection.
 * @channel: The channel id.
 *
 * Creates a new instance of #PkgChannelPage.
 *
 * Returns: the newly created instance of #PkgChannelPage.
 * Side effects: None.
 */
GtkWidget*
pkg_channel_page_new (PkConnection *connection,  /* IN */
                      gint          channel)     /* IN */
{
	PkgChannelPage *page;

	ENTRY;
	page = g_object_new(PKG_TYPE_CHANNEL_PAGE,
	                    "connection", connection,
	                    "id", channel,
	                    NULL);
	RETURN(GTK_WIDGET(page));
}

static void
pkg_channel_page_get_target_cb (GObject      *object,    /* IN */
                                GAsyncResult *result,    /* IN */
                                gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(object);
	PkgChannelPagePrivate *priv;
	gchar *target = NULL;
	gchar *markup = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKG_CHANNEL_PAGE(user_data)->priv;
	if (!pk_connection_channel_get_target_finish(connection, result,
	                                             &target, &error)) {
		DEBUG(Channel, "Error retrieving channel target: %s",
		      error->message);
		g_error_free(error);
		EXIT;
	}
	if (!IS_NULL_OR_EMPTY(target)) {
		markup = g_markup_printf_escaped("<span size=\"smaller\">%s</span>",
										 target);
		gtk_label_set_markup(GTK_LABEL(priv->target), markup);
		g_free(markup);
	}
	g_free(target);
	EXIT;
}

static void
pkg_channel_page_get_working_dir_cb (GObject      *object,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(object);
	PkgChannelPagePrivate *priv;
	gchar *working_dir = NULL;
	gchar *markup;
	GError *error = NULL;

	ENTRY;
	priv = PKG_CHANNEL_PAGE(user_data)->priv;
	if (!pk_connection_channel_get_working_dir_finish(connection, result,
	                                                  &working_dir, &error)) {
		DEBUG(Channel, "Error retrieving channel working-dir: %s",
		      error->message);
		g_error_free(error);
		EXIT;
	}
	if (!IS_NULL_OR_EMPTY(working_dir)) {
		markup = g_markup_printf_escaped("<span size=\"smaller\">%s</span>",
										 working_dir);
		gtk_label_set_markup(GTK_LABEL(priv->working_dir), markup);
		g_free(markup);
	}
	g_free(working_dir);
	EXIT;
}

static void
pkg_channel_page_get_pid_cb (GObject      *object,    /* IN */
                             GAsyncResult *result,    /* IN */
                             gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(object);
	PkgChannelPagePrivate *priv;
	GError *error = NULL;
	gchar *str;
	gint pid;

	ENTRY;
	priv = PKG_CHANNEL_PAGE(user_data)->priv;
	if (!pk_connection_channel_get_pid_finish(connection, result,
	                                          &pid, &error)) {
		DEBUG(Channel, "Error retrieving channel pid: %s",
		      error->message);
		g_error_free(error);
		EXIT;
	}
	str = g_markup_printf_escaped("<span size=\"smaller\">%d</span>", pid);
	gtk_label_set_markup(GTK_LABEL(priv->pid), str);
	g_free(str);
	EXIT;
}

static void
pkg_channel_page_get_exit_status_cb (GObject      *object,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(object);
	PkgChannelPagePrivate *priv;
	gint exit_status;
	gchar *str;

	ENTRY;
	priv = PKG_CHANNEL_PAGE(user_data)->priv;
	if (!pk_connection_channel_get_exit_status_finish(connection, result,
	                                                  &exit_status, NULL)) {
		gtk_label_set_text(GTK_LABEL(priv->exit_status), "-");
		EXIT;
	}
	str = g_markup_printf_escaped("<span size=\"smaller\">%d</span>",
	                              exit_status);
	gtk_label_set_markup(GTK_LABEL(priv->exit_status), str);
	g_free(str);
	EXIT;
}

static void
pkg_channel_page_get_args_cb (GObject      *object,    /* IN */
                              GAsyncResult *result,    /* IN */
                              gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(object);
	PkgChannelPagePrivate *priv;
	gchar **args;
	gchar *markup;
	gchar *str;

	ENTRY;
	priv = PKG_CHANNEL_PAGE(user_data)->priv;
	if (!pk_connection_channel_get_args_finish(connection, result,
	                                           &args, NULL)) {
		gtk_label_set_text(GTK_LABEL(priv->args), "-");
		EXIT;
	}
	if (g_strv_length(args)) {
		str = g_strjoinv(" ", args);
		markup = g_markup_printf_escaped("<span size=\"smaller\">%s</span>", str);
		gtk_label_set_markup(GTK_LABEL(priv->args), markup);
		g_free(markup);
		g_free(str);
	}
	g_strfreev(args);
	EXIT;
}

static void
pkg_channel_page_get_env_cb (GObject      *object,    /* IN */
                             GAsyncResult *result,    /* IN */
                             gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(object);
	PkgChannelPagePrivate *priv;
	gchar **env;
	gchar *markup;
	gchar *str;

	ENTRY;
	priv = PKG_CHANNEL_PAGE(user_data)->priv;
	if (!pk_connection_channel_get_env_finish(connection, result,
	                                          &env, NULL)) {
		gtk_label_set_text(GTK_LABEL(priv->env), "-");
		EXIT;
	}
	if (g_strv_length(env)) {
		str = g_strjoinv("\n", env);
		markup = g_markup_printf_escaped("<span size=\"smaller\">%s</span>", str);
		gtk_label_set_markup(GTK_LABEL(priv->env), markup);
		g_free(markup);
		g_free(str);
	}
	g_strfreev(env);
	EXIT;
}

static void
pkg_channel_page_get_kill_pid_cb (GObject      *object,    /* IN */
                                  GAsyncResult *result,    /* IN */
                                  gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(object);
	PkgChannelPagePrivate *priv;
	gboolean kill_pid;

	ENTRY;
	priv = PKG_CHANNEL_PAGE(user_data)->priv;
	if (!pk_connection_channel_get_kill_pid_finish(connection, result,
	                                               &kill_pid, NULL)) {
		EXIT;
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->kill_pid), kill_pid);
	EXIT;
}

void
pkg_channel_page_reload (PkgChannelPage *page) /* IN */
{
	PkgChannelPagePrivate *priv;
	gchar *markup;

	g_return_if_fail(PKG_IS_CHANNEL_PAGE(page));

	ENTRY;
	priv = page->priv;
	markup = g_markup_printf_escaped(
			_("<span weight=\"bold\">Channel %d</span>"),
			priv->id);
	gtk_label_set_markup(GTK_LABEL(priv->page_label), markup);
	g_free(markup);
	pk_connection_channel_get_target_async(
			priv->connection, priv->id, NULL,
			pkg_channel_page_get_target_cb, page);
	pk_connection_channel_get_working_dir_async(
			priv->connection, priv->id, NULL,
			pkg_channel_page_get_working_dir_cb, page);
	pk_connection_channel_get_pid_async(
			priv->connection, priv->id, NULL,
			pkg_channel_page_get_pid_cb, page);
	pk_connection_channel_get_exit_status_async(
			priv->connection, priv->id, NULL,
			pkg_channel_page_get_exit_status_cb, page);
	pk_connection_channel_get_args_async(
			priv->connection, priv->id, NULL,
			pkg_channel_page_get_args_cb, page);
	pk_connection_channel_get_env_async(
			priv->connection, priv->id, NULL,
			pkg_channel_page_get_env_cb, page);
	pk_connection_channel_get_kill_pid_async(
			priv->connection, priv->id, NULL,
			pkg_channel_page_get_kill_pid_cb, page);
	EXIT;
}

static void
pkg_channel_page_set_property (GObject      *object,  /* IN */
                               guint         prop_id, /* IN */
                               const GValue *value,   /* IN */
                               GParamSpec   *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_CONNECTION:
		PKG_CHANNEL_PAGE(object)->priv->connection = g_object_ref(g_value_get_object(value));
		break;
	case PROP_ID:
		PKG_CHANNEL_PAGE(object)->priv->id = g_value_get_int(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pkg_channel_page_get_property (GObject    *object,  /* IN */
                               guint       prop_id, /* IN */
                               GValue     *value,   /* OUT */
                               GParamSpec *pspec)   /* IN */
{
	switch (prop_id) {
	case PROP_CONNECTION:
		g_value_set_object(value,
		                   PKG_CHANNEL_PAGE(object)->priv->connection);
		break;
	case PROP_ID:
		g_value_set_int(value,
		                PKG_CHANNEL_PAGE(object)->priv->id);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
pkg_channel_page_dispose (GObject *object)
{
	PkgChannelPagePrivate *priv;

	ENTRY;
	priv = PKG_CHANNEL_PAGE(object)->priv;
	g_object_unref(priv->connection);
	G_OBJECT_CLASS(pkg_channel_page_parent_class)->dispose(object);
	EXIT;
}

static void
pkg_channel_page_finalize (GObject *object)
{
	ENTRY;
	G_OBJECT_CLASS(pkg_channel_page_parent_class)->finalize(object);
	EXIT;
}

static void
pkg_channel_page_class_init (PkgChannelPageClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkg_channel_page_finalize;
	object_class->dispose = pkg_channel_page_dispose;
	object_class->set_property = pkg_channel_page_set_property;
	object_class->get_property = pkg_channel_page_get_property;
	g_type_class_add_private(object_class, sizeof(PkgChannelPagePrivate));

	/**
	 * PkgChannelPage:connection:
	 *
	 * The "connection" to the agent.
	 */
	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "Connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * PkgChannelPage:id:
	 *
	 * The "id" of the channel.
	 */
	g_object_class_install_property(object_class,
	                                PROP_ID,
	                                g_param_spec_int("id",
	                                                 "Id",
	                                                 "Id",
	                                                 0,
	                                                 G_MAXINT,
	                                                 0,
	                                                 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pkg_channel_page_init (PkgChannelPage *page)
{
	PkgChannelPagePrivate *priv;
	gchar *path;

	ENTRY;
	page->priv = G_TYPE_INSTANCE_GET_PRIVATE(page,
	                                         PKG_TYPE_CHANNEL_PAGE,
	                                         PkgChannelPagePrivate);
	priv = page->priv;

	#define EXTRACT_WIDGET(_n, _w)                                          \
	    G_STMT_START {                                                      \
	        priv->_w = (gpointer)gtk_builder_get_object(priv->builder, _n); \
	    } G_STMT_END

	path = pkg_path_for_data("ui", "perfkit-gui.ui", NULL);
	priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(priv->builder, path, NULL);
	g_free(path);

	EXTRACT_WIDGET("page-label", page_label);
	EXTRACT_WIDGET("channel-page", container);
	EXTRACT_WIDGET("target", target);
	EXTRACT_WIDGET("args", args);
	EXTRACT_WIDGET("working-dir", working_dir);
	EXTRACT_WIDGET("env", env);
	EXTRACT_WIDGET("pid", pid);
	EXTRACT_WIDGET("kill-pid", kill_pid);
	EXTRACT_WIDGET("exit-status", exit_status);
	EXTRACT_WIDGET("muted", muted);

	gtk_widget_unparent(priv->container);
	gtk_container_add(GTK_CONTAINER(page), priv->container);

	EXIT;
}
