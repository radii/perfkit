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
		Session _session;
		GenericArray<RowGroup> _row_groups;

		/* Gtk widgets */
		UIManager        _ui_manager;
		Ppg.Timer        _timer;
		GtkClutter.Embed _embed;
		Gtk.Statusbar    _statusbar;
		Gtk.MenuBar      _menubar;
		Gtk.Toolbar      _toolbar;
		Ppg.Ruler        _ruler;
		Gtk.Label        _pointer_label;
		Gtk.Adjustment   _hadj;
		Gtk.Adjustment   _vadj;
		Gtk.Adjustment   _zadj;

		/* Clutter actors */
		Clutter.Actor     _pos_actor;
		Clutter.Rectangle _bg_actor;
		Clutter.Rectangle _bg_stripe;
		Clutter.Box       _row_groups_box;

		/* Current row selection */
		RowGroup _selected_row_group;
		int _selected_offset = -1;

		/* GLib timeouts */
		uint _update_pos_handler;

		/* grab state */
		bool _in_move;

		/* track window sizing */
		int _last_width;
		int _last_height;

		/* signals */
		public signal void row_changed ();

		construct {
			window_count++;

			set("title", _("Perfkit Profiler"),
			    "default-width", 1000,
			    "default-height", 620,
			    "window-position", WindowPosition.CENTER,
			    null);

			_session = new Session();
			_row_groups = new GenericArray<RowGroup>();

			_hadj = new Adjustment(0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
			_vadj = new Adjustment(0.0f, 0.0f, 1.0f, 5.0f, 1.0f, 1.0f);
			_zadj = new Adjustment(1.0f, 0.0f, 2.0f, 0.025f, 0.5f, 0.0f);

			_session.state_changed.connect(on_state_changed);
			_session.instrument_added.connect(on_instrument_added);
			_session.instrument_removed.connect(on_instrument_removed);

			var vbox = new VBox(false, 0);
			this.add(vbox);
			vbox.show();

			var table = new Table(4, 3, false);
			vbox.pack_start(table, true, true, 0);
			table.show();

			var header = new Header();
			header.corners = CairoUtil.CornerType.NONE;
			header.show();
			table.add_with_properties(header,
			                          "left-attach", 0,
			                          "right-attach", 1,
			                          "top-attach", 0,
			                          "bottom-attach", 1,
			                          "y-options", AttachOptions.FILL,
			                          "x-options", AttachOptions.FILL,
			                          null);

			_ruler = new Ppg.Ruler();
			_ruler.corners = CairoUtil.CornerType.TOP_RIGHT;
			_ruler.show();
			table.add_with_properties(_ruler,
			                          "left-attach", 1,
			                          "right-attach", 2,
			                          "top-attach", 0,
			                          "bottom-attach", 1,
			                          "y-options", AttachOptions.FILL,
			                          null);

			var vscrollbar = new VScrollbar(_vadj);
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
			                          "right-attach", 2,
			                          "top-attach", 2,
			                          "bottom-attach", 3,
			                          "y-options", AttachOptions.FILL,
			                          null);

			var hscrollbar = new HScrollbar(_hadj);
			hscrollbar.show();
			halign.add(hscrollbar);

			_embed = new GtkClutter.Embed();
			_embed.can_focus = true;
			_embed.add_events(Gdk.EventMask.KEY_PRESS_MASK | Gdk.EventMask.POINTER_MOTION_MASK);
			_embed.key_press_event.connect(on_embed_key_press);
			_embed.motion_notify_event.connect(on_embed_motion_notity);
			_embed.show();
			create_actors();
			_embed.size_allocate.connect((_, alloc) => {
				_row_groups.foreach((data) => {
					var group = (RowGroup)data;
					group.update_width(_embed);
				});
			});
			table.add_with_properties(_embed,
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

			var hscale = new HScale(_zadj);
			hscale.draw_value = false;
			hscale.digits = 3;
			hscale.add_mark(1.0, PositionType.BOTTOM, null);
			hscale.show();
			hbox.pack_start(hscale, true, true, 0);

			var zin = new Gtk.Image.from_stock(STOCK_ZOOM_IN, IconSize.MENU);
			zin.show();
			hbox.pack_start(zin, false, true, 6);

			_statusbar = new Statusbar();
			_statusbar.push(0, "Ready");
			vbox.pack_start(_statusbar, false, true, 0);
			_statusbar.show();

			var frame = new Gtk.Frame(null);
			frame.shadow = ShadowType.ETCHED_IN;
			frame.show();
			_statusbar.pack_start(frame, false, true, 0);

			_pointer_label = new Label("00:00:00.0000");
			_pointer_label.use_markup = true;
			_pointer_label.xpad = 3;
			var attrlist = new Pango.AttrList();
			attrlist.insert(Pango.attr_family_new("Monospace"));
			attrlist.insert(new Pango.AttrSize(8 * Pango.SCALE));
			_pointer_label.set_attributes(attrlist);
			_ruler.notify["position"].connect(() => {
				_pointer_label.label = format_time(_ruler.position);
			});
			_pointer_label.show();
			frame.add(_pointer_label);

			var action_group = new ActionGroup("PpgWindow");
			Actions.load(this, action_group);
			_ui_manager = new UIManager();
			_ui_manager.insert_action_group(action_group, 0);
			try {
				_ui_manager.add_ui_from_string(ui_data, -1);
			} catch (Error err) {
				assert_not_reached();
			}
			_menubar = (Gtk.MenuBar)_ui_manager.get_widget("/Menubar");
			_toolbar = (Gtk.Toolbar)_ui_manager.get_widget("/Toolbar");

			vbox.add_with_properties(_menubar,
			                         "expand", false,
			                         "position", 0,
			                         null);
			vbox.add_with_properties(_toolbar,
			                         "expand", false,
			                         "position", 1,
			                         null);

			var tool_item = new ToolItem();
			tool_item.set_expand(true);
			_toolbar.insert(tool_item, -1);
			tool_item.show();

			_timer = new Timer();
			_timer.session = _session;
			_timer.set_size_request(200, -1);
			tool_item = new ToolItem();
			tool_item.add(_timer);
			_toolbar.insert(tool_item, -1);
			tool_item.show();
			_timer.show();

			var target = new TargetToolButton();
			target.set_expand(true);
			_toolbar.insert(target, -1);
			target.show();

			this.delete_event.connect((event) => {
				window_count--;
				if (window_count < 1) {
					Gtk.main_quit();
				}
				return false;
			});

			_zadj.value_changed.connect(on_zoom_changed);
			_vadj.value_changed.connect(on_vadj_changed);
			_row_groups_box.notify["allocation"].connect(box_allocation_notify);

			/*
			 * Gtk likes to select tool items as the main focus. Go ahead
			 * and force the focus on the clutter widget.
			 */
			_embed.grab_focus();
		}

		public Gtk.Adjustment zoom {
			get { return _zadj; }
		}

		public Toolbar toolbar {
			get { return _toolbar; }
		}

		public MenuBar menubar {
			get { return _menubar; }
		}

		public Statusbar statusbar {
			get { return _statusbar; }
		}

		public Stage stage {
			get {
				return (Stage)_embed.get_stage();
			}
		}

		public int selected_row {
			get { return this._selected_offset; }
		}

		public Session session {
			get { return _session; }
		}

		public static int get_window_count () {
			return window_count;
		}

		void on_zoom_changed (Adjustment zoom) {
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
			_ruler.set_range(lower, upper, lower, 0);
		}

		void on_vadj_changed (Adjustment adj) {
			_in_move = true;
			_row_groups_box.y = -(int)adj.value;
			_in_move = false;
		}

		void box_allocation_notify () {
			Clutter.ActorBox box;
			double upper;
			double value;

			if (!_in_move) {
				_row_groups_box.get_allocation_box(out box);
				upper = box.get_height();
				value = -(int)box.y1;
				_vadj.set("upper", upper,
				         "value", value,
				         null);
			}
		}

		public override void style_set (Gtk.Style? old_style) {
			Clutter.Color dark;
			Clutter.Color mid;
			Clutter.Color light;

			base.style_set(old_style);

			GtkClutter.get_light_color(this, StateType.NORMAL, out light);
			GtkClutter.get_dark_color(this, StateType.NORMAL, out dark);
			GtkClutter.get_mid_color(this, StateType.NORMAL, out mid);

			stage.color = light;
			_bg_actor.color = mid;
			_bg_stripe.color = dark;

			_row_groups.foreach((data) => {
				var group = (RowGroup)data;
				group.style = this.style;
			});
		}

		public override void size_allocate (Gdk.Rectangle alloc) {
			Gtk.Allocation embed_alloc;

			base.size_allocate(alloc);

			if (_last_width != alloc.width || _last_height != alloc.height) {
				_embed.get_allocation(out embed_alloc);
				on_zoom_changed(this._zadj);
				_vadj.page_size = embed_alloc.height;
				_bg_actor.height = embed_alloc.height;
				_bg_stripe.height = embed_alloc.height;
				_pos_actor.height = embed_alloc.height;

				_row_groups.foreach((data) => {
					RowGroup group = (RowGroup)data;
					group.width = embed_alloc.width;
				});
			}

			_last_width = alloc.width;
			_last_height = alloc.height;
		}

		bool on_embed_key_press (Gdk.EventKey event) {
			bool retval = true;

			switch (event.keyval) {
			case Gdk.KeySym.Down:
				move_next();
				break;
			case Gdk.KeySym.Page_Down:
				move_next_group();
				break;
			case Gdk.KeySym.Up:
				move_previous();
				break;
			case Gdk.KeySym.Page_Up:
				move_previous_group();
				break;
			default:
				retval = false;
				break;
			}

			return retval;
		}

		void on_state_changed (SessionState state) {
			switch (state) {
			case SessionState.STARTED:
				enable_pos_tracker();
				break;
			case SessionState.PAUSED:
				disable_pos_tracker();
				break;
			case SessionState.STOPPED:
				disable_pos_tracker();
				break;
			default:
				break;
			}
		}

		void enable_pos_tracker () {
			_update_pos_handler = Timeout.add((1000 / 20), () => {
				if (_session.timer != null) {
					ulong usec;
					var seconds = _session.timer.elapsed(out usec);

					if (_ruler.contains(seconds)) {
						Gtk.Allocation alloc;

						_embed.get_allocation(out alloc);
						var width = alloc.width - 200;
						var offset = width / (_ruler.upper - _ruler.lower);
						var x = (seconds - _ruler.lower) * offset;
						_pos_actor.x = 200.0f + (float)x;
						_pos_actor.show();
					} else {
						_pos_actor.hide();
					}
				}
				return true;
			});
		}

		void disable_pos_tracker () {
			if (_update_pos_handler != 0) {
				GLib.Source.remove(_update_pos_handler);
				_update_pos_handler = 0;
			}
		}

		bool on_embed_motion_notity (Gdk.EventMotion motion) {
			Gtk.Allocation alloc;
			double offset;

			if (motion.x > 200) {
				_embed.get_allocation(out alloc);
				offset = (motion.x - 200.0f) / (alloc.width - 200.0f);
				_ruler.position = _ruler.lower + ((_ruler.upper - _ruler.lower) * offset);
			} else {
				_ruler.position = _ruler.lower;
			}
			return false;
		}

		public void scroll_to_row (RowGroup row_group,
		                           Row? row)
		{
			Gtk.Allocation alloc;
			float y = row != null ? row.y : row_group.y;
			float h = row != null ? row.height : row_group.height;
			float box_y = _row_groups_box.y;

			/*
			 * FIXME: Try to fit the entire rowgroup into view if possible.
			 */

			_embed.get_allocation(out alloc);

			if (box_y < -y) {
				_row_groups_box.y = -y;
			} else if ((y + h + box_y) > alloc.height) {
				_row_groups_box.y = -(y + h - alloc.height);
			}
		}

		public bool move_next_group () {
			return false;
		}

		public bool move_previous_group () {
			return false;
		}

		public bool move_next () {
			return false;
		}

		public bool move_previous () {
			return false;
		}

		void create_actors () {
			Clutter.Color black = Clutter.Color.from_string("#000");

			_bg_actor = new Clutter.Rectangle.with_color(black);
			_bg_actor.set_size(200, 100);
			stage.add_actor(_bg_actor);

			var layout = new Clutter.BoxLayout();
			layout.easing_duration = 250;
			layout.easing_mode = AnimationMode.EASE_IN_QUAD;
			layout.pack_start = false;
			layout.use_animations = true;
			layout.vertical = true;
			_row_groups_box = new Clutter.Box(layout);
			stage.add_actor(_row_groups_box);

			_bg_stripe = new Clutter.Rectangle.with_color(black);
			_bg_stripe.set_size(1, 200);
			_bg_stripe.set_position(200, 0);
			stage.add_actor(_bg_stripe);

			_pos_actor = new Clutter.Rectangle.with_color(black);
			_pos_actor.hide();
			_pos_actor.set_size(1, 200);
			_pos_actor.set_position(201, 0);
			stage.add_actor(_pos_actor);
		}

		public override void destroy () {
			this._session.teardown();
			base.destroy();
		}

		static string format_time (double time) {
			return "%02d:%02d:%02d.%04d".printf((int)(time / 3600.0),
			                                    (int)((time % 3600.0) / 60.0),
			                                    (int)(time % 60.0),
			                                    (int)((time % 1.0) * 10000));
		}

		void on_instrument_added (Instrument instrument) {
			var group = new RowGroup() {
				instrument = instrument,
				style = this.style
			};

			group.button_press_event.connect((event) => {
				_embed.grab_focus();

				if (event.button == 1) {
					if (event.click_count == 1) {
						if ((event.modifier_state & ModifierType.CONTROL_MASK) != 0) {
							this.select_row_group(null);
						} else {
							this.select_row_group(group);
						}
					}
				}

				return false;
			});

			group.update_width(_embed);
			_row_groups.add(group);
			_row_groups_box.pack(group, false, true, true,
			                     BoxAlignment.START,
			                     BoxAlignment.START);
		}

		void on_instrument_removed (Instrument instrument) {
		}

		public void select_row_group (RowGroup? row_group) {
			if (_selected_row_group != null) {
				_selected_row_group.state = StateType.NORMAL;
				_selected_row_group = null;
			}

			if (row_group != null) {
				_selected_row_group = row_group;
				_selected_row_group.state = StateType.SELECTED;
			}
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
  <menu action="ViewAction">
   <menuitem action="ZoomInAction"/>
   <menuitem action="ZoomOutAction"/>
   <menuitem action="ZoomOneAction"/>
   <separator/>
   <menuitem action="FullscreenAction"/>
  </menu>
  <menu action="ProfileAction">
   <menuitem action="StopAction"/>
   <menuitem action="PauseAction"/>
   <menuitem action="RunAction"/>
   <menuitem action="RestartAction"/>
   <separator/>
  </menu>
  <menu action="InstrumentsAction">
   <menuitem action="AddInstrumentAction"/>
   <menuitem action="RemoveInstrumentAction"/>
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
