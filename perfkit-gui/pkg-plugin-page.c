/* pkg-plugin-page.c
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

#include "pkg-log.h"
#include "pkg-page.h"
#include "pkg-path.h"
#include "pkg-plugin-page.h"

/**
 * SECTION:pkg-plugin-page
 * @title: PkgPluginPage
 * @short_description: 
 *
 * Section overview.
 */

static void pkg_page_init (PkgPageIface *iface);

G_DEFINE_TYPE_EXTENDED(PkgPluginPage,
                       pkg_plugin_page,
                       GTK_TYPE_ALIGNMENT,
                       0,
                       G_IMPLEMENT_INTERFACE(PKG_TYPE_PAGE, pkg_page_init));

struct _PkgPluginPagePrivate
{
	PkConnection *connection;
	gchar        *id;
	GtkBuilder   *builder;
	GtkWidget    *container;
	GtkWidget    *title;
	GtkWidget    *name;
	GtkWidget    *description;
	GtkWidget    *version;
	GtkWidget    *copyright;
};

enum
{
	PROP_0,
	PROP_CONNECTION,
	PROP_ID,
};

/**
 * pkg_plugin_page_new:
 *
 * Creates a new instance of #PkgPluginPage.
 *
 * Returns: the newly created instance of #PkgPluginPage.
 * Side effects: None.
 */
GtkWidget*
pkg_plugin_page_new (PkConnection *connection, /* IN */
                     const gchar  *plugin)     /* IN */
{
	PkgPluginPage *page;

	ENTRY;
	page = g_object_new(PKG_TYPE_PLUGIN_PAGE,
	                    "connection", connection,
	                    "id", plugin,
	                    NULL);
	RETURN(GTK_WIDGET(page));
}

static void
pkg_plugin_page_get_name_cb (GObject      *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
	PkgPluginPagePrivate *priv;
	GError *error = NULL;
	gchar *name = NULL;
	gchar *markup;

	g_return_if_fail(PKG_IS_PLUGIN_PAGE(user_data));

	ENTRY;
	priv = PKG_PLUGIN_PAGE(user_data)->priv;
	if (!pk_connection_plugin_get_name_finish(PK_CONNECTION(object), result,
	                                          &name, &error)) {
		EXIT;
	}
	markup = g_markup_printf_escaped("<span size=\"smaller\">%s</span>", name);
	gtk_label_set_markup(GTK_LABEL(priv->name), markup);
	g_free(markup);
	g_free(name);
	EXIT;
}

static void
pkg_plugin_page_get_version_cb (GObject      *object,
                                GAsyncResult *result,
                                gpointer      user_data)
{
	PkgPluginPagePrivate *priv;
	GError *error = NULL;
	gchar *version = NULL;
	gchar *markup;

	g_return_if_fail(PKG_IS_PLUGIN_PAGE(user_data));

	ENTRY;
	priv = PKG_PLUGIN_PAGE(user_data)->priv;
	if (!pk_connection_plugin_get_version_finish(PK_CONNECTION(object), result,
	                                             &version, &error)) {
		EXIT;
	}
	markup = g_markup_printf_escaped("<span size=\"smaller\">%s</span>",
	                                 version);
	gtk_label_set_markup(GTK_LABEL(priv->version), markup);
	g_free(markup);
	g_free(version);
	EXIT;
}

static void
pkg_plugin_page_get_copyright_cb (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
	PkgPluginPagePrivate *priv;
	GError *error = NULL;
	gchar *copyright = NULL;
	gchar *markup;

	g_return_if_fail(PKG_IS_PLUGIN_PAGE(user_data));

	ENTRY;
	priv = PKG_PLUGIN_PAGE(user_data)->priv;
	if (!pk_connection_plugin_get_copyright_finish(PK_CONNECTION(object), result,
	                                               &copyright, &error)) {
		EXIT;
	}
	markup = g_markup_printf_escaped("<span size=\"smaller\">%s</span>",
	                                 copyright);
	gtk_label_set_markup(GTK_LABEL(priv->copyright), markup);
	g_free(markup);
	g_free(copyright);
	EXIT;
}

static void
pkg_plugin_page_get_description_cb (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
	PkgPluginPagePrivate *priv;
	GError *error = NULL;
	gchar *description = NULL;
	gchar *markup;

	g_return_if_fail(PKG_IS_PLUGIN_PAGE(user_data));

	ENTRY;
	priv = PKG_PLUGIN_PAGE(user_data)->priv;
	if (!pk_connection_plugin_get_description_finish(PK_CONNECTION(object), result,
	                                                 &description, &error)) {
		EXIT;
	}
	markup = g_markup_printf_escaped("<span size=\"smaller\">%s</span>",
	                                 description);
	gtk_label_set_markup(GTK_LABEL(priv->description), markup);
	g_free(markup);
	g_free(description);
	EXIT;
}

void
pkg_plugin_page_load (PkgPage *page) /* IN */
{
	PkgPluginPagePrivate *priv;
	gchar *markup;

	ENTRY;
	priv = PKG_PLUGIN_PAGE(page)->priv;
	markup = g_markup_printf_escaped(
			_("<span weight=\"bold\">%s Plugin</span>"),
			priv->id);
	gtk_label_set_markup(GTK_LABEL(priv->title), markup);
	g_free(markup);
	pk_connection_plugin_get_name_async(
			priv->connection, priv->id, NULL,
			pkg_plugin_page_get_name_cb, page);
	pk_connection_plugin_get_version_async(
			priv->connection, priv->id, NULL,
			pkg_plugin_page_get_version_cb, page);
	pk_connection_plugin_get_copyright_async(
			priv->connection, priv->id, NULL,
			pkg_plugin_page_get_copyright_cb, page);
	pk_connection_plugin_get_description_async(
			priv->connection, priv->id, NULL,
			pkg_plugin_page_get_description_cb, page);
	EXIT;
}

static void
pkg_plugin_page_finalize (GObject *object)
{
	PkgPluginPagePrivate *priv;

	ENTRY;
	priv = PKG_PLUGIN_PAGE(object)->priv;
	g_free(priv->id);
	G_OBJECT_CLASS(pkg_plugin_page_parent_class)->finalize(object);
	EXIT;
}

static void
pkg_plugin_page_dispose (GObject *object)
{
	PkgPluginPagePrivate *priv;

	ENTRY;
	priv = PKG_PLUGIN_PAGE(object)->priv;
	if (priv->connection) {
		g_object_unref(priv->connection);
	}
	G_OBJECT_CLASS(pkg_plugin_page_parent_class)->dispose(object);
	EXIT;
}

static void
pkg_plugin_page_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	PkgPluginPagePrivate *priv;

	ENTRY;
	priv = PKG_PLUGIN_PAGE(object)->priv;
	switch (prop_id) {
	CASE(PROP_CONNECTION);
		priv->connection = g_object_ref(g_value_get_object(value));
		BREAK;
	CASE(PROP_ID);
		priv->id = g_value_dup_string(value);
		BREAK;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
	EXIT;
}

static void
pkg_plugin_page_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	PkgPluginPagePrivate *priv;

	ENTRY;
	priv = PKG_PLUGIN_PAGE(object)->priv;
	switch (prop_id) {
	CASE(PROP_CONNECTION);
		g_value_set_object(value, priv->connection);
		BREAK;
	CASE(PROP_ID);
		g_value_set_string(value, priv->id);
		BREAK;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
	EXIT;
}

static void
pkg_plugin_page_class_init (PkgPluginPageClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkg_plugin_page_finalize;
	object_class->dispose = pkg_plugin_page_dispose;
	object_class->set_property = pkg_plugin_page_set_property;
	object_class->get_property = pkg_plugin_page_get_property;
	g_type_class_add_private(object_class, sizeof(PkgPluginPagePrivate));

	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "Connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_ID,
	                                g_param_spec_string("id",
	                                                    "Id",
	                                                    "Id",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pkg_plugin_page_init (PkgPluginPage *page)
{
	PkgPluginPagePrivate *priv;
	gchar *path;

	ENTRY;
	page->priv = G_TYPE_INSTANCE_GET_PRIVATE(page,
	                                         PKG_TYPE_PLUGIN_PAGE,
	                                         PkgPluginPagePrivate);
	priv = page->priv;

	#define EXTRACT_WIDGET(_n, _f) \
		priv->_f = GTK_WIDGET(gtk_builder_get_object(priv->builder, _n))

	path = pkg_path_for_data("ui", "pkg-plugin-page.ui", NULL);
	priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(priv->builder, path, NULL);
	g_free(path);

	EXTRACT_WIDGET("plugin-page", container);
	EXTRACT_WIDGET("title", title);
	EXTRACT_WIDGET("name", name);
	EXTRACT_WIDGET("description", description);
	EXTRACT_WIDGET("version", version);
	EXTRACT_WIDGET("copyright", copyright);

	gtk_widget_unparent(priv->container);
	gtk_container_add(GTK_CONTAINER(page), priv->container);

	EXIT;
}

static void
pkg_page_init (PkgPageIface *iface) /* IN */
{
	ENTRY;
	iface->load = pkg_plugin_page_load;
	EXIT;
}
