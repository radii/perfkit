/* controller.vala
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

using GLib;

namespace PerfkitGui {
    public delegate Controller? ControllerResolver(string path);

    public interface Controller: GLib.Object {
        static Gee.ArrayList<Resolver> _resolvers;
        static HashTable<string,Controller> _registered;

        /*
         *---------------------------------------------------------------------
         *
         * PerfkitGui::Controller::init --
         *
         *      Initialize the controller subsystem.
         *
         * Results:
         *      None.
         *
         * Side effects:
         *      Structures for controller resolution are allocated.
         *
         *---------------------------------------------------------------------
         */

        public static void init() {
            _resolvers = new Gee.ArrayList<Resolver>();
            _registered = new HashTable<string,Controller>(str_hash, str_equal);

            Controller.add_resolver((path) => {
            	if (path.has_prefix("/Connection/")) {
            		var hash = path.substring(12);
            		var conn = Connections.lookup(hash);
            		if (conn != null) {
            			var ctrl = new ConnectionController();
            			ctrl.connection = conn;
            			Controller.register(path, ctrl);
            			return ctrl;
            		}
            	}
            	return null;
            });
        }

		/*
		 *---------------------------------------------------------------------
		 *
		 * PerfkitGui::Controller::register --
		 *
		 * 		Registers a controller for a specific path.
		 *
		 * Results:
		 * 		None.
		 *
		 * Side effects:
		 *		None.
		 *
		 *---------------------------------------------------------------------
		 */

        public static void register(string path, Controller controller) {
        	_registered.insert(path, controller);
        }

        /*
         *---------------------------------------------------------------------
         *
         * PerfkitGui::Controller::resolve --
         *
         *        Resolves a controller for a given controller path.
         *
         * Results:
         *        A PerfkitGui.Controller if successful; otherwise null.
         *
         * Side effects:
         *        None.
         *
         *---------------------------------------------------------------------
         */

        public static Controller? resolve(string path) {
            Controller result = null;

            foreach (var resolver in _resolvers) {
                if (null != (result = resolver.callback(path))) {
                    break;
                }
            }

            return result;
        }

        /*
         *---------------------------------------------------------------------
         *
         * PerfkitGui::Controller::add_resolver --
         *
         *      Adds a controller resolver to the resolver-chain.
         *
         * Results:
         *      None.
         *
         * Side effects:
         *      None.
         *
         *---------------------------------------------------------------------
         */

        public static void add_resolver(ControllerResolver resolver) {
            var resolverObj = new Resolver();
            resolverObj.callback = resolver;
            _resolvers.add(resolverObj);
        }
    }

    private class Resolver {
        public ControllerResolver callback;
    }
}
