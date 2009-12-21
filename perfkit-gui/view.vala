/* view.vala
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
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

using Gee;
using GLib;
using Gtk;

namespace PerfkitGui {
	public delegate View? ViewResolver(string path);

	public interface View: Gtk.Widget {
		static Gee.ArrayList<ViewResolverObj> _resolvers;
		static HashTable<string,View> _registered;

		public static void init() {
			_resolvers = new Gee.ArrayList<ViewResolverObj>();
			_registered = new HashTable<string,View>(str_hash, str_equal);

			/* Register default views */
			View.register("/SourceTypes", new SourceTypesView());

			/* Register default resolvers */
			View.add_resolver((path) => {
				if (path == "/Connection") {
					return new ConnectionView();
				}
				return null;
			});
		}

		public static View? resolve(string path) {
			View view = null;

			view = _registered.lookup(path);
			if (view != null) {
				return view;
			}

			foreach (var resolver in _resolvers) {
				view = resolver.callback(path);
				if (view != null) {
					return view;
				}
			}

			return null;
		}

		public static void add_resolver(ViewResolver resolver) {
			var resolverObj = new ViewResolverObj();
			resolverObj.callback = resolver;
			_resolvers.add(resolverObj);
		}

		public static void register(string path, View view) {
			_registered.insert(path, view);
		}

		public abstract void set_controller(Controller controller);
	}

	private class ViewResolverObj {
		public ViewResolver callback;
	}
}
