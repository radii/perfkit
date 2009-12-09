/* sources.js
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

const Gtk = imports.gi.Gtk;
const PerfkitGui = imports.gi.PerfkitGui;


/*
 *-----------------------------------------------------------------------------
 *
 * SourcesView -
 *       Creates an instance of SourcesView.
 *
 * Return value:
 *       The newly created instance.
 *
 * Side effects:
 *       The view is created and attached to controller.
 *
 *-----------------------------------------------------------------------------
 */

function SourcesView(controller) {
   this._init(controller);
}


SourcesView.prototype = {

   /*
    *--------------------------------------------------------------------------
    *
    * SourcesView::_init -
    *       Constructor for SourcesView
    *
    * Return value:
    *       None.
    *
    * Side effects:
    *       The view is attached to the controller.
    *
    *--------------------------------------------------------------------------
    */

   _init: function(controller) {
      this.controller = controller;
      this._create_gui();
      this._wire_gui();
      this._window.show();
   },


   /*
    *--------------------------------------------------------------------------
    *
    * SourcesView::_wire_gui -
    *       Create the user interface widgets.
    *
    * Return value:
    *       None.
    *
    * Side effects:
    *       The gtk+ widgets are wired to the controller.
    *
    *--------------------------------------------------------------------------
    */
 
   _wire_gui: function() {
      this._window.connect('delete-event', function() {
         this._window.hide();
         return true;
      });

      this.controller.connect('connected', function() {
         this.refresh();
      });

      this.controller.connect('disconnect', function() {
         this._model.clear();
      });
   },


   /*
    *--------------------------------------------------------------------------
    *
    * SourcesView::_create_gui -
    *       Create the user interface widgets.
    *
    * Return value:
    *       None.
    *
    * Side effects:
    *       The gtk+ widgets are created.
    *
    *--------------------------------------------------------------------------
    */

   _create_gui: function() {
      /*
       * Setup window.
       */
      this._window = new Gtk.Window();
      this._window.set_title("Perfkit Sources");
      this._window.set_type_hint(Gtk.WindowTypeHint.UTILITY);
      this._window.set_default_size(200, 350);

      /*
       * Setup TreeModel for sources.
       */
      this._model = new Gtk.ListStore(string);

      /*
       * Create window widgets.
       */
      let vbox = new Gtk.VBox();
      this._window.add(vbox);
      vbox.show();

      let scroller = new Gtk.ScrolledWindow();
      scroller.set_policy(Gtk.POLICY_AUTOMATIC, Gtk.POLICY_AUTOMATIC);
      vbox.pack_start(scroller, true, true, 0);
      scroller.show();

      let treeview = new Gtk.TreeView();
      treeview.set_headers_visible(false);
      scroller.add(treeview);
      treeview.show();

      let hbox = new Gtk.HBox();
      vbox.pack_start(hbox, false, true, 0);
      hbox.show();

      let refresh_img = new Gtk.Image();
      refresh_img.set_from_stock(Gtk.STOCK_REFRESH, Gtk.IconSize.MENU);
      let refresh = new Gtk.Button();
      refresh.connect('clicked', function() { this.refresh(); });
      refresh.set_relief(Gtk.ReliefStyle.NONE);
      refresh.add(refresh_img);
      hbox.pack_start(refresh, false, true, 0);
      refresh_img.show();
      refresh.show();
   },


   /*
    *--------------------------------------------------------------------------
    *
    * SourcesView::refresh
    *       Refresh the list of data sources.
    *
    * Return value:
    *       None.
    *
    * Side effects:
    *       Current list is cleared and reloaded.
    *
    *--------------------------------------------------------------------------
    */

   refresh: function() {
      this._model.clear();
      $(controller.sources).each(function(sourceInfo) {
         this._model.append();
         this._model.set(iter, 0, sourceInfo.name, -1);
      });
   },
};

PerfkitGui.Runtime.Gui.register_view('sources', function(controller) {
   return SourcesView(controller);
});
