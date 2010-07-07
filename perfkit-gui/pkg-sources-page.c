/* pkg-sources-page.c
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
#include <perfkit/perfkit.h>

#include "pkg-closures.h"
#include "pkg-path.h"
#include "pkg-log.h"
#include "pkg-sources-page.h"

/**
 * SECTION:pkg-sources-page.h
 * @title: PkgSourcesPage
 * @short_description: 
 *
 * Section overview.
 */

static void pkg_page_init (PkgPageIface *iface);

G_DEFINE_TYPE_EXTENDED(PkgSourcesPage,
                       pkg_sources_page,
                       GTK_TYPE_ALIGNMENT,
                       0,
                       G_IMPLEMENT_INTERFACE(PKG_TYPE_PAGE, pkg_page_init));

struct _PkgSourcesPagePrivate
{
	PkConnection *connection;
	GtkBuilder   *builder;
	GtkListStore *model;
	GtkListStore *types;
	GtkWidget    *container;
	GtkWidget    *treeview;
	GtkWidget    *add;
	GtkWidget    *remove;
	GtkWidget    *combo;
};

enum
{
	COLUMN_ID,
	COLUMN_IDSTR,
	COLUMN_PLUGIN,
	COLUMN_STATE,
};

enum
{
	PROP_0,
	PROP_CONNECTION,
	PROP_ID,
};

/**
 * pkg_sources_page_new:
 *
 * Creates a new instance of #PkgSourcesPage.
 *
 * Returns: the newly created instance of #PkgSourcesPage.
 * Side effects: None.
 */
GtkWidget*
pkg_sources_page_new (void)
{
	PkgSourcesPage *page;

	ENTRY;
	page = g_object_new(PKG_TYPE_SOURCES_PAGE, NULL);
	RETURN(GTK_WIDGET(page));
}

static void
pkg_sources_page_get_plugins_cb (PkConnection *connection, /* IN */
                                 GAsyncResult *result,     /* IN */
                                 gpointer      user_data)  /* IN */
{
	PkgSourcesPagePrivate *priv;
	PkgSourcesPage *page = user_data;
	gchar **plugins = NULL;
	GtkTreeIter iter;
	gint i;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = page->priv;
	/* TODO: Show error */
	if (pk_connection_manager_get_plugins_finish(connection, result, &plugins, NULL)) {
		for (i = 0; plugins[i]; i++) {
			gtk_list_store_append(priv->types, &iter);
			gtk_list_store_set(priv->types, &iter, 0, plugins[i], -1);
		}
		g_strfreev(plugins);
	}
	g_object_unref(page);
	EXIT;
}

static gboolean
pkg_sources_page_get_source_iter (PkgSourcesPage *page,   /* IN */
                                  gint            source, /* IN */
                                  GtkTreeIter    *iter)   /* OUT */
{
	PkgSourcesPagePrivate *priv;
	gint id = 0;

	ENTRY;
	priv = page->priv;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->model), iter)) {
		do {
			gtk_tree_model_get(GTK_TREE_MODEL(priv->model), iter, COLUMN_ID, &id, -1);
			if (id == source) {
				RETURN(TRUE);
			}
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->model), iter));
	}
	RETURN(FALSE);
}

static void
pkg_sources_page_get_plugin_cb (PkConnection *connection, /* IN */
                                GAsyncResult *result,     /* IN */
                                gpointer      user_data)  /* IN */
{
	PkgSourceCall *call = user_data;
	PkgSourcesPagePrivate *priv;
	PkgSourcesPage *page;
	GtkTreeIter iter;
	GError *error = NULL;
	gchar *plugin = NULL;

	g_return_if_fail(call != NULL);

	ENTRY;
	page = PKG_SOURCES_PAGE(call->user_data);
	priv = page->priv;
	if (!pk_connection_source_get_plugin_finish(connection, result, &plugin, &error)) {
		/* TODO: show error */
		g_error_free(error);
		GOTO(cleanup);
	}
	if (!pkg_sources_page_get_source_iter(call->user_data, call->source, &iter)) {
		GOTO(cleanup);
	}
	gtk_list_store_set(priv->model, &iter, COLUMN_PLUGIN, plugin, -1);
  cleanup:
	g_free(plugin);
	g_object_unref(page);
	pkg_source_call_free(call);
	EXIT;
}

static void
pkg_sources_page_add_source (PkgSourcesPage *page,   /* IN */
                             gint            source) /* IN */
{
	PkgSourcesPagePrivate *priv;
	GtkTreeIter iter;
	gchar *id_str;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = page->priv;
	id_str = g_strdup_printf("%d", source);
	gtk_list_store_append(priv->model, &iter);
	gtk_list_store_set(priv->model, &iter,
	                   COLUMN_ID, source,
	                   COLUMN_IDSTR, id_str,
	                   -1);
	g_free(id_str);
	pk_connection_source_get_plugin_async(
			priv->connection, source, NULL,
			(GAsyncReadyCallback)pkg_sources_page_get_plugin_cb,
			pkg_source_call_new(priv->connection, source, g_object_ref(page)));
	EXIT;
}

static void
pkg_sources_page_get_sources_cb (PkConnection *connection, /* IN */
                                 GAsyncResult *result,     /* IN */
                                 gpointer      user_data)  /* IN */
{
	PkgSourcesPagePrivate *priv;
	PkgSourcesPage *page = user_data;
	gint *sources = NULL;
	gsize sources_len = 0;
	GError *error = NULL;
	gint i;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = page->priv;
	if (!pk_connection_manager_get_sources_finish(connection, result, &sources, &sources_len, &error)) {
		/* TODO: Show error */
		g_error_free(error);
		GOTO(cleanup);
	}
	for (i = 0; i < sources_len; i++) {
		pkg_sources_page_add_source(page, sources[i]);
	}
  cleanup:
	g_free(sources);
	g_object_unref(page);
	EXIT;
}

/**
 * pkg_sources_page_load:
 * @page: A #PkgPage.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkg_sources_page_load (PkgPage *page) /* IN */
{
	PkgSourcesPagePrivate *priv;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = PKG_SOURCES_PAGE(page)->priv;
	pk_connection_manager_get_plugins_async(
			priv->connection,
			NULL,
			(GAsyncReadyCallback)pkg_sources_page_get_plugins_cb,
			g_object_ref(page));
	pk_connection_manager_get_sources_async(
			priv->connection,
			NULL,
			(GAsyncReadyCallback)pkg_sources_page_get_sources_cb,
			g_object_ref(page));
	EXIT;
}

/**
 * pkg_sources_page_unload:
 * @page: A #PkgPage.
 *
 * XXX
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkg_sources_page_unload (PkgPage *page) /* IN */
{
	PkgSourcesPagePrivate *priv;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = PKG_SOURCES_PAGE(page)->priv;
	EXIT;
}

static void
pkg_sources_page_combo_changed_cb (GtkWidget *combo,     /* IN */
                                   gpointer   user_data) /* IN */
{
	PkgSourcesPagePrivate *priv;
	PkgSourcesPage *page = user_data;
	GtkTreeIter iter;
	gboolean sensitive = FALSE;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = page->priv;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->combo), &iter)) {
		sensitive = TRUE;
	}
	gtk_widget_set_sensitive(priv->add, sensitive);
	EXIT;
}

static void
pkg_sources_page_add_source_cb (PkConnection *connection, /* IN */
                                GAsyncResult *result,     /* IN */
                                gpointer      user_data)  /* IN */
{
	PkgSourcesPagePrivate *priv;
	PkgSourcesPage *page = user_data;
	GError *error = NULL;
	gint id = 0;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = page->priv;
	if (!pk_connection_manager_add_source_finish(
				connection, result, &id, &error)) {
		/* TODO: Show error */
		g_error_free(error);
	}
	EXIT;
}

static void
pkg_sources_page_selection_changed_cb (GtkTreeSelection *selection, /* IN */
                                       PkgSourcesPage   *page)      /* IN */
{
	PkgSourcesPagePrivate *priv;
	gboolean sensitive = FALSE;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = page->priv;
	if (gtk_tree_selection_count_selected_rows(selection)) {
		sensitive = TRUE;
	}
	gtk_widget_set_sensitive(priv->remove, sensitive);
	EXIT;
}

static void
pkg_sources_page_add_clicked_cb (GtkWidget *add,       /* IN */
                                 gpointer   user_data) /* IN */
{
	PkgSourcesPagePrivate *priv;
	PkgSourcesPage *page = user_data;
	const gchar *plugin = NULL;
	GtkTreeIter iter;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = page->priv;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(priv->combo), &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(priv->types), &iter, 0, &plugin, -1);
		g_assert(plugin);

		pk_connection_manager_add_source_async(
				priv->connection, plugin, NULL,
				(GAsyncReadyCallback)pkg_sources_page_add_source_cb,
				NULL);
	}
	EXIT;
}

static void
pkg_sources_page_remove_clicked_cb (GtkWidget *widget,    /* IN */
                                    gpointer   user_data) /* IN */
{
	PkgSourcesPagePrivate *priv;
	PkgSourcesPage *page = user_data;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint id = 0;

	g_return_if_fail(PKG_IS_SOURCES_PAGE(page));

	ENTRY;
	priv = page->priv;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview));
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, COLUMN_ID, &id, -1);
		INFO(Sources, "Removing source %d", id);
		pk_connection_manager_remove_source(priv->connection, id, NULL);
		/* TODO: Remove after attaching signals */
		gtk_list_store_remove(priv->model, &iter);
	}
	EXIT;
}

static void
pkg_sources_page_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	PkgSourcesPagePrivate *priv;

	ENTRY;
	priv = PKG_SOURCES_PAGE(object)->priv;
	switch (prop_id) {
	CASE(PROP_CONNECTION);
		priv->connection = g_object_ref(g_value_get_object(value));
		BREAK;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
	EXIT;
}

static void
pkg_sources_page_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	PkgSourcesPagePrivate *priv;

	ENTRY;
	priv = PKG_SOURCES_PAGE(object)->priv;
	switch (prop_id) {
	CASE(PROP_CONNECTION);
		g_value_set_object(value, priv->connection);
		BREAK;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
	EXIT;
}

/**
 * pkg_sources_page_finalize:
 * @object: A #PkgSourcesPage.
 *
 * Finalizer for a #PkgSourcesPage instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkg_sources_page_finalize (GObject *object) /* IN */
{
	G_OBJECT_CLASS(pkg_sources_page_parent_class)->finalize(object);
}

/**
 * pkg_sources_page_class_init:
 * @klass: A #PkgSourcesPageClass.
 *
 * Initializes the #PkgSourcesPageClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkg_sources_page_class_init (PkgSourcesPageClass *klass) /* IN */
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkg_sources_page_finalize;
	object_class->set_property = pkg_sources_page_set_property;
	object_class->get_property = pkg_sources_page_get_property;
	g_type_class_add_private(object_class, sizeof(PkgSourcesPagePrivate));

	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "Connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

/**
 * pkg_sources_page_init:
 * @page: A #PkgSourcesPage.
 *
 * Initializes the newly created #PkgSourcesPage instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pkg_sources_page_init (PkgSourcesPage *page) /* IN */
{
	PkgSourcesPagePrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	gchar *path;

	ENTRY;
	page->priv = G_TYPE_INSTANCE_GET_PRIVATE(page,
	                                         PKG_TYPE_SOURCES_PAGE,
	                                         PkgSourcesPagePrivate);
	priv = page->priv;

	#define EXTRACT_WIDGET(_n, _f) \
		priv->_f = GTK_WIDGET(gtk_builder_get_object(priv->builder, _n))

	path = pkg_path_for_data("ui", "pkg-sources-page.ui", NULL);
	priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(priv->builder, path, NULL);
	g_free(path);

	EXTRACT_WIDGET("sources-page", container);
	EXTRACT_WIDGET("treeview", treeview);
	EXTRACT_WIDGET("add", add);
	EXTRACT_WIDGET("remove", remove);
	EXTRACT_WIDGET("combo", combo);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("ID"));
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_add_attribute(column, cell, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Type"));
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_add_attribute(column, cell, "text", 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	priv->model = gtk_list_store_new(4,
	                                 G_TYPE_INT,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->treeview),
	                        GTK_TREE_MODEL(priv->model));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->combo), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(priv->combo), cell, "text", 0);

	priv->types = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_combo_box_set_model(GTK_COMBO_BOX(priv->combo),
	                        GTK_TREE_MODEL(priv->types));

	g_signal_connect(priv->combo,
	                 "changed",
	                 G_CALLBACK(pkg_sources_page_combo_changed_cb),
	                 page);

	g_signal_connect(priv->add,
	                 "clicked",
	                 G_CALLBACK(pkg_sources_page_add_clicked_cb),
	                 page);

	g_signal_connect(priv->remove,
	                 "clicked",
	                 G_CALLBACK(pkg_sources_page_remove_clicked_cb),
	                 page);

	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->treeview)),
	                 "changed",
	                 G_CALLBACK(pkg_sources_page_selection_changed_cb),
	                 page);

	gtk_widget_unparent(priv->container);
	gtk_container_add(GTK_CONTAINER(page), priv->container);

	EXIT;
}

static void
pkg_page_init (PkgPageIface *iface) /* IN */
{
	ENTRY;
	iface->load = pkg_sources_page_load;
	iface->unload = pkg_sources_page_unload;
	EXIT;
}
