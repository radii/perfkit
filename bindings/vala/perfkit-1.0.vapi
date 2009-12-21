/* perfkit-1.0.vapi
 *
 * Copyright (C) 2009 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

using GLib;

[CCode (cheader_filename="perfkit/perfkit.h")]
namespace Perfkit {
	[CCode (cname="PkConnectionError",
	        cprefix="pk_connection_error_",
	        type_id="PK_TYPE_CONNECTION_ERROR")]
	public errordomain ConnectionError {
		INVALID,
	}

	[CCode (cname="PkConnection",
	        cprefix="pk_connection_",
	        type_id="PK_TYPE_CONNECTION")]
	public class Connection: GLib.Object {
		public signal void connected ();
		public signal void disconnected ();

		public Connection.for_uri (string uri);
		public Channels get_channels();
		public Sources get_sources();

		public bool is_connected();
		public void connect() throws Perfkit.ConnectionError;
		public void disconnect();

		public string uri { get; }
	}

	[CCode (cname="PkChannels",
	        cprefix="pk_channels_",
	        type_id="PK_TYPE_CHANNELS")]
	public class Channels: GLib.Object {
	}

	[CCode (cname="PkSources",
	        cprefix="pk_sources_",
	        type_id="PK_TYPE_SOURCES")]
	public class Sources: GLib.Object {
	}

	[CCode (cname="PkSourceInfo",
	        cprefix="pk_source_info_",
	        type_id="PK_TYPE_SOURCE_INFO")]
	public class SourceInfo: GLib.Object {
		public string uid { get; }
		public string name { get; }
	}
}
