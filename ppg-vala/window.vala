/* window.vala
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

using Clutter;
using GLib;
using Gtk;
using GtkClutter;

namespace Ppg {
	static const float ROW_HEIGHT = 45.0f;
	static const float PIXELS_PER_SECOND = 20.0f;
	static int window_count = 0;

	public class Window: Gtk.Window {
		Session _session = new Session();
		GenericArray<Row> rows = new GenericArray<Row>();

		Embed     embed;
		Statusbar statusbar;
		MenuBar   menubar;
		Toolbar   toolbar;
		UIManager ui_manager;
		Ruler     ruler;

		Clutter.Rectangle bg_actor;
		Clutter.Rectangle bg_stripe;
		Clutter.Box       rows_box;

		Row selected;
		int selected_offset = -1;

		Adjustment hadj;
		Adjustment vadj;
		Adjustment zadj;

		construct {
			window_count++;

			set("title", _("Perfkit Profiler"),
			    "default-width", 1000,
			    "default-height", 620,
			    "window-position", WindowPosition.CENTER,
			    null);

			hadj = new Adjustment(0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
			vadj = new Adjustment(0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
			zadj = new Adjustment(1.0f, 0.001f, 2.0f, 0.1f, 0.5f, 0.0f);

			var vbox = new VBox(false, 0);
			this.add(vbox);
			vbox.show();

			var table = new Table(4, 3, false);
			vbox.pack_start(table, true, true, 0);
			table.show();

			ruler = new HRuler();
			ruler.show();
			table.add_with_properties(ruler,
			                          "left-attach", 1,
			                          "right-attach", 2,
			                          "top-attach", 0,
			                          "bottom-attach", 1,
			                          "y-options", AttachOptions.FILL,
			                          null);

			var vscrollbar = new VScrollbar(vadj);
			vscrollbar.show();
			table.add_with_properties(vscrollbar,
			                          "left-attach", 2,
			                          "right-attach", 3,
			                          "top-attach", 0,
			                          "bottom-attach", 2,
			                          "x-options", AttachOptions.FILL,
			                          null);

			var halign = new Alignment(0.5f, 0.0f, 1.0f, 0.0f);
			halign.show();
			table.add_with_properties(halign,
			                          "left-attach", 1,
			                          "right-attach", 3,
			                          "top-attach", 2,
			                          "bottom-attach", 3,
			                          "y-options", AttachOptions.FILL,
			                          null);

			var hscrollbar = new HScrollbar(hadj);
			hscrollbar.show();
			halign.add(hscrollbar);

			embed = new Embed();
			create_actors();
			embed.can_focus = true;
			embed.add_events(Gdk.EventMask.KEY_PRESS_MASK);
			embed.key_press_event.connect(embed_key_press);
			embed.show();
			table.add_with_properties(embed,
			                          "left-attach", 0,
			                          "right-attach", 2,
			                          "top-attach", 1,
			                          "bottom-attach", 2,
			                          null);

			var hbox = new HBox(false, 3);
			hbox.width_request = 200;
			hbox.show();
			table.add_with_properties(hbox,
			                          "left-attach", 0,
			                          "right-attach", 1,
			                          "top-attach", 2,
			                          "bottom-attach", 4,
			                          "x-options", AttachOptions.FILL,
			                          "y-options", AttachOptions.FILL,
			                          null);

			var zout = new Gtk.Image.from_stock(STOCK_ZOOM_OUT, IconSize.MENU);
			zout.show();
			hbox.pack_start(zout, false, true, 6);

			var hscale = new HScale(zadj);
			hscale.draw_value = false;
			hscale.digits = 3;
			hscale.add_mark(1.0, PositionType.BOTTOM, null);
			hscale.show();
			hbox.pack_start(hscale, true, true, 0);

			var zin = new Gtk.Image.from_stock(STOCK_ZOOM_IN, IconSize.MENU);
			zin.show();
			hbox.pack_start(zin, false, true, 6);

			statusbar = new Statusbar();
			statusbar.push(0, "Ready");
			vbox.pack_start(statusbar, false, true, 0);
			statusbar.show();

			var action_group = new ActionGroup("PpgWindow");
			Actions.load(this, action_group);
			ui_manager = new UIManager();
			ui_manager.insert_action_group(action_group, 0);
			try {
				ui_manager.add_ui_from_string(ui_data, -1);
			} catch (Error err) {
				assert_not_reached();
			}
			menubar = (MenuBar)ui_manager.get_widget("/Menubar");
			toolbar = (Toolbar)ui_manager.get_widget("/Toolbar");

			vbox.add_with_properties(menubar,
			                         "expand", false,
			                         "position", 0,
			                         null);
			vbox.add_with_properties(toolbar,
			                         "expand", false,
			                         "position", 1,
			                         null);

			this.delete_event.connect((event) => {
				window_count--;
				if (window_count < 1) {
					Gtk.main_quit();
				}
				return false;
			});

			this.zadj.value_changed.connect(zoom_changed);

			add_row("Row 1");
			add_row("Row 2");
			add_row("Row 3");
			add_row("Row 4");

			embed.grab_focus();
		}

		public Stage stage {
			get {
				return (Stage)embed.get_stage();
			}
		}

		public Session session {
			get { return _session; }
		}

		public static int count_windows () {
			return window_count;
		}

		void zoom_changed (Adjustment zoom) {
			Gtk.Allocation alloc;
			double upper;
			double lower;
			double scale;

			this.get_allocation(out alloc);
			scale = zoom.value;
			scale.clamp(0.0, 2.0);
			if (scale > 1.0) {
				scale = (scale - 1.0) * 100.0;
			}
			scale.clamp(0.001, 100.0);

			upper = (alloc.width - 200.0) / (scale * PIXELS_PER_SECOND);
			lower = 0.0f; /* FIXME */
			ruler.set_range(lower, upper, lower, 0);
		}

		public override void style_set (Gtk.Style? old_style) {
			Clutter.Color dark;
			Clutter.Color mid;

			base.style_set(old_style);

			GtkClutter.get_dark_color(this, StateType.NORMAL, out dark);
			GtkClutter.get_mid_color(this, StateType.NORMAL, out mid);

			bg_actor.color = mid;
			bg_stripe.color = dark;

			rows.foreach((data) => {
				Row row = (Row)data;
				row.paint(embed);
			});
		}

		public override void size_allocate (Gdk.Rectangle alloc) {
			Gtk.Allocation embed_alloc;

			base.size_allocate(alloc);

			embed.get_allocation(out embed_alloc);
			bg_actor.height = embed_alloc.height;
			bg_stripe.height = embed_alloc.height;

			rows.foreach((data) => {
				Row row = (Row)data;
				row.update_size(embed);
			});
		}

		bool embed_key_press (Gdk.EventKey event) {
			bool retval = true;

			switch (event.keyval) {
			case Gdk.KeySym.Down:
				select_next(1);
				break;
			case Gdk.KeySym.Page_Down:
				select_next(5);
				break;
			case Gdk.KeySym.Up:
				select_previous(1);
				break;
			case Gdk.KeySym.Page_Up:
				select_previous(5);
				break;
			default:
				retval = false;
				break;
			}

			return retval;
		}

		void select_row (Row? row) {
			if (row == this.selected) {
				return;
			}

			if (this.selected != null) {
				this.selected.selected = false;
				this.selected.paint(embed);
			}

			this.selected = row;
			this.selected_offset = -1;

			if (this.selected != null) {
				this.selected.selected = true;
				this.selected.paint(embed);
				for (int i = 0; i < rows.length; i++) {
					if (rows.get(i) == row) {
						this.selected_offset = i;
					}
				}
			}
		}

		void select_next (int count) {
			if ((selected_offset + count) < rows.length) {
				select_row(rows.get(selected_offset + count));
			} else if (rows.length > 0) {
				select_row(rows.get(rows.length - 1));
			}
		}

		void select_previous (int count) {
			if ((selected_offset - count) >= 0) {
				select_row(rows.get(selected_offset - count));
			} else if (rows.length > 0) {
				select_row(rows.get(0));
			}
		}

		void create_actors () {
			Clutter.Color black = Clutter.Color.from_string("#000");

			bg_actor = new Clutter.Rectangle.with_color(black);
			bg_actor.set_size(200, 100);
			stage.add_actor(bg_actor);

			var layout = new Clutter.BoxLayout();
			layout.easing_duration = 250;
			layout.easing_mode = AnimationMode.EASE_IN_QUAD;
			layout.pack_start = false;
			layout.use_animations = true;
			layout.vertical = true;
			rows_box = new Clutter.Box(layout);
			stage.add_actor(rows_box);

			bg_stripe = new Clutter.Rectangle.with_color(black);
			bg_stripe.set_size(1, 200);
			bg_stripe.set_position(200, 0);
			stage.add_actor(bg_stripe);
		}

		void add_row (string title) {
			var row = new Row();
			row.title = title;
			row.update_size(embed);

			row.group.button_press_event.connect((event) => {
				embed.grab_focus();
				if (event.button == 1) {
					if (event.click_count == 1) {
						if ((event.modifier_state & ModifierType.CONTROL_MASK) != 0) {
							this.select_row(null);
						} else {
							this.select_row(row);
						}
					}
				}
				return false;
			});

			row.attach(rows_box);
			rows.add(row);
		}
	}

	class Row {
		public Clutter.Group group;

		Clutter.CairoTexture hdr_bg;
		Clutter.Text         hdr_text;
		Clutter.Rectangle    data_bg;
		Clutter.CairoTexture data_fg;

		public Row () {
			group = new Clutter.Group();
			group.reactive = true;
			hdr_bg = new Clutter.CairoTexture(200, (uint)ROW_HEIGHT);
			hdr_text = new Clutter.Text();
			hdr_text.x = 15.0f;
			data_bg = new Clutter.Rectangle();
			data_bg.height = 1.0f;
			data_bg.x = 200.0f;
			data_bg.y = ROW_HEIGHT - 1.0f;
			data_fg = new Clutter.CairoTexture(1, 1);
			data_fg.height = ROW_HEIGHT - 1.0f;
			data_fg.x = 200.0f;
			group.add(hdr_bg, hdr_text, data_bg, data_fg);
		}

		public void attach (Clutter.Box box) {
			BoxLayout layout = (BoxLayout)box.layout_manager;

			layout.pack(this.group, true, true, false,
			            BoxAlignment.START,
			            BoxAlignment.START);
		}

		public void paint (Widget widget) {
			StateType state = StateType.NORMAL;
			Clutter.Color mid;
			Clutter.Color dark;
			Gtk.Allocation alloc;

			if (this.selected) {
				state = StateType.SELECTED;
			}

			widget.get_allocation(out alloc);
			GtkClutter.get_dark_color(widget, state, out dark);
			GtkClutter.get_mid_color(widget, state, out mid);

			var cr = hdr_bg.create();
			var p = new Cairo.Pattern.linear(0, 0, 0, group.height);
			add_color_stop(p, 0.0, mid);
			add_color_stop(p, 1.0, dark);
			cr.rectangle(0, 0, group.width, group.height);
			cr.set_source(p);
			cr.fill();

			data_bg.color = mid;
		}

		void add_color_stop (Cairo.Pattern pattern,
		                     double offset,
		                     Clutter.Color color) {
			pattern.add_color_stop_rgb(offset,
			                           color.red / 255.0f,
			                           color.green / 255.0f,
			                           color.blue / 255.0f);
		}

		public string title {
			set {
				hdr_text.text = value;
				hdr_text.y = (float)Math.floor((ROW_HEIGHT - hdr_text.height) / 2.0f);
			}
			get {
				return hdr_text.text;
			}
		}

		public bool selected { get; set; }

		public void update_size (Widget widget) {
			Gtk.Allocation alloc;

			widget.get_allocation(out alloc);
			data_bg.width = alloc.width - 200.0f;
			data_fg.width = alloc.width - 200.0f;
			data_fg.set_surface_size((uint)data_fg.width, (uint)data_fg.height);
			this.paint(widget);
		}
	}

	static const string ui_data =
"""
<ui>
 <menubar name="Menubar">
  <menu action="PerfkitAction">
   <menuitem action="QuitAction"/>
  </menu>
  <menu action="EditAction">
   <menuitem action="PreferencesAction"/>
  </menu>
  <menu action="ProfileAction">
   <menuitem action="AddSourceAction"/>
   <menuitem action="RemoveSourceAction"/>
   <separator/>
   <menuitem action="StopAction"/>
   <menuitem action="PauseAction"/>
   <menuitem action="RunAction"/>
   <menuitem action="RestartAction"/>
   <separator/>
  </menu>
 </menubar>
 <toolbar name="Toolbar">
  <toolitem action="StopAction"/>
  <toolitem action="PauseAction"/>
  <toolitem action="RunAction"/>
  <toolitem action="RestartAction"/>
  <separator/>
  <toolitem action="AddSourceAction"/>
 </toolbar>
</ui>
""";
}
