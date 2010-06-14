/* pkg-subscription-page.c
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

#include "pkg-closures.h"
#include "pkg-log.h"
#include "pkg-path.h"
#include "pkg-subscription-page.h"

/**
 * SECTION:pkg-subscription-page
 * @title: PkgSubscriptionPage
 * @short_description: 
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkgSubscriptionPage, pkg_subscription_page, GTK_TYPE_ALIGNMENT)

struct _PkgSubscriptionPagePrivate
{
	PkConnection *connection;
	gint          id;
	GtkBuilder   *builder;
	GtkTreeModel *model;
	GtkWidget    *container;
	GtkWidget    *sources;
	GtkWidget    *title;
};

enum
{
	PROP_0,
	PROP_CONNECTION,
	PROP_ID,
};

enum
{
	COLUMN_ID,
	COLUMN_IDSTR,
	COLUMN_PLUGIN,
};

/**
 * pkg_subscription_page_new:
 *
 * Creates a new instance of #PkgSubscriptionPage.
 *
 * Returns: the newly created instance of #PkgSubscriptionPage.
 * Side effects: None.
 */
GtkWidget*
pkg_subscription_page_new (PkConnection *connection,   /* IN */
                           gint          subscription) /* IN */
{
	GtkWidget *widget;

	ENTRY;
	widget = g_object_new(PKG_TYPE_SUBSCRIPTION_PAGE,
	                      "connection", connection,
	                      "id", subscription,
	                      NULL);
	RETURN(widget);
}

static void
pkg_subscription_page_get_plugin_cb (GObject      *object,    /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkgSubscriptionPagePrivate *priv;
	PkgSourceCall *call = user_data;
	gchar *plugin = NULL;
	GError *error = NULL;
	GtkTreeIter iter;
	gint id;

	g_return_if_fail(call != NULL);

	ENTRY;
	priv = PKG_SUBSCRIPTION_PAGE(call->user_data)->priv;
	if (!pk_connection_source_get_plugin_finish(call->connection, result,
	                                            &plugin, &error)) {
		GOTO(failed);
	}
	if (gtk_tree_model_get_iter_first(priv->model, &iter)) {
		do {
			gtk_tree_model_get(priv->model, &iter, COLUMN_ID, &id, -1);
			if (id == call->source) {
				gtk_list_store_set(GTK_LIST_STORE(priv->model), &iter,
				                   COLUMN_PLUGIN, plugin,
				                   -1);
				BREAK;
			}
		} while (gtk_tree_model_iter_next(priv->model, &iter));
	}
  failed:
  	g_free(plugin);
	pkg_source_call_free(call);
	EXIT;
}

static void
pkg_subscription_page_get_sources_cb (GObject      *object,    /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkgSubscriptionPagePrivate *priv;
	GError *error = NULL;
	gint *sources;
	gsize sources_len = 0;
	gchar *idstr;
	GtkTreeIter iter;
	gint i;

	g_return_if_fail(PKG_IS_SUBSCRIPTION_PAGE(user_data));

	ENTRY;
	priv = PKG_SUBSCRIPTION_PAGE(user_data)->priv;
	if (!pk_connection_subscription_get_sources_finish(priv->connection,
	                                                   result,
	                                                   &sources, &sources_len,
	                                                   &error)) {
	    WARNING(Subscription, "Error retrieving source list: %s",
	            error->message);
	    g_error_free(error);
		EXIT;
	}
	for (i = 0; i < sources_len; i++) {
		idstr = g_strdup_printf("%d", sources[i]);
		gtk_list_store_append(GTK_LIST_STORE(priv->model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(priv->model), &iter,
		                   COLUMN_ID, sources[i],
		                   COLUMN_IDSTR, idstr,
		                   -1);
		pk_connection_source_get_plugin_async(
				priv->connection, sources[i], NULL,
				pkg_subscription_page_get_plugin_cb,
				pkg_source_call_new(priv->connection, sources[i], user_data));
		g_free(idstr);
	}
	EXIT;
}

void
pkg_subscription_page_reload (PkgSubscriptionPage *page) /* IN */
{
	PkgSubscriptionPagePrivate *priv;
	gchar *markup;

	g_return_if_fail(PKG_IS_SUBSCRIPTION_PAGE(page));

	ENTRY;
	priv = page->priv;
	markup = g_markup_printf_escaped("<span weight=\"bold\">"
	                                 "Subscription %d"
	                                 "</span>",
	                                 priv->id);
	gtk_label_set_markup(GTK_LABEL(priv->title), markup);
	g_free(markup);
	pk_connection_subscription_get_sources_async(
			priv->connection, priv->id, NULL,
			pkg_subscription_page_get_sources_cb,
			page);
	EXIT;
}

static void
pkg_subscription_page_finalize (GObject *object)
{
	ENTRY;
	G_OBJECT_CLASS(pkg_subscription_page_parent_class)->finalize(object);
	EXIT;
}

static void
pkg_subscription_page_dispose (GObject *object) /* IN */
{
	PkgSubscriptionPagePrivate *priv;

	ENTRY;
	priv = PKG_SUBSCRIPTION_PAGE(object)->priv;
	if (priv->connection) {
		g_object_unref(priv->connection);
	}
	G_OBJECT_CLASS(pkg_subscription_page_parent_class)->dispose(object);
	EXIT;
}

static void
pkg_subscription_page_get_property (GObject    *object,  /* IN */
                                    guint       prop_id, /* IN */
                                    GValue     *value,   /* OUT */
                                    GParamSpec *pspec)   /* IN */
{
	PkgSubscriptionPagePrivate *priv;

	ENTRY;
	priv = PKG_SUBSCRIPTION_PAGE(object)->priv;
	switch (prop_id) {
	CASE(PROP_CONNECTION);
		g_value_set_object(value, priv->connection);
		BREAK;
	CASE(PROP_ID);
		g_value_set_int(value, priv->id);
		BREAK;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
	EXIT;
}

static void
pkg_subscription_page_set_property (GObject      *object,  /* IN */
                                    guint         prop_id, /* IN */
                                    const GValue *value,   /* IN */
                                    GParamSpec   *pspec)   /* IN */
{
	PkgSubscriptionPagePrivate *priv;

	ENTRY;
	priv = PKG_SUBSCRIPTION_PAGE(object)->priv;
	switch (prop_id) {
	CASE(PROP_CONNECTION);
		priv->connection = g_object_ref(g_value_get_object(value));
		BREAK;
	CASE(PROP_ID);
		priv->id = g_value_get_int(value);
		BREAK;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
	EXIT;
}

static void
pkg_subscription_page_class_init (PkgSubscriptionPageClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkg_subscription_page_finalize;
	object_class->dispose = pkg_subscription_page_dispose;
	object_class->set_property = pkg_subscription_page_set_property;
	object_class->get_property = pkg_subscription_page_get_property;
	g_type_class_add_private(object_class, sizeof(PkgSubscriptionPagePrivate));

	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "Connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

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
pkg_subscription_page_init (PkgSubscriptionPage *page)
{
	PkgSubscriptionPagePrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	gchar *path;

	ENTRY;
	page->priv = G_TYPE_INSTANCE_GET_PRIVATE(page,
	                                         PKG_TYPE_SUBSCRIPTION_PAGE,
	                                         PkgSubscriptionPagePrivate);
	priv = page->priv;

	#define EXTRACT_WIDGET(_n, _f)                         \
	    priv->_f = GTK_WIDGET(                             \
	    		gtk_builder_get_object(priv->builder, _n))

	path = pkg_path_for_data("ui", "pkg-subscription-page.ui", NULL);
	priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(priv->builder, path, NULL);
	g_free(path);

	EXTRACT_WIDGET("sources", sources);
	EXTRACT_WIDGET("title", title);
	EXTRACT_WIDGET("subscription-page", container);

	priv->model = GTK_TREE_MODEL(
			gtk_list_store_new(3,
			                   G_TYPE_INT,
			                   G_TYPE_STRING,
			                   G_TYPE_STRING));
	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->sources), priv->model);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("ID"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->sources), column);
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_add_attribute(column, cell, "text", COLUMN_IDSTR);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Plugin"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->sources), column);
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_add_attribute(column, cell, "text", COLUMN_PLUGIN);

	gtk_widget_unparent(priv->container);
	gtk_container_add(GTK_CONTAINER(page), priv->container);

	EXIT;
}
