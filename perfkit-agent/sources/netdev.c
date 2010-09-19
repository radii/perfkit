/* netdev.c
 *
 * Copyright (C) 2010 Andrew Stiegmann <andrew.stiegmann@gmail.com>
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

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include <perfkit-agent/perfkit-agent.h>
#include "src-utils.h"

/*
Entries looks like this...
------------------------------------------------------------------------------------------------------------------------------
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo:   10410     104    0    0    0     0          0         0    10410     104    0    0    0     0       0          0
  eth0:1069675772 4197670    0    0    0     0          0         0 3221945712 3290571    0    0    0     0       0          0
------------------------------------------------------------------------------------------------------------------------------
*/


typedef struct
{
	PkaManifest *manifest;
} NetDevData;


/*
 * Read /proc/net/dev and store to the list of data in
 * NetDevData.  Returns the number of devices read.
 */
static gint
netdev_read (PkaSourceSimple *source) /* IN */
{
   gchar *content;
   gint devicesRead = 0;
   gchar filebuf[1024];
   gint delim_index = -1;

   ENTRY;
   content = src_utils_read_file("/proc/net/dev", filebuf, 1024);

   while (content != NULL) {
      /*
       * So this file can't be directly parsed using sscanf due to lack of
       * separation of some of the fields.  Hence we have to separate it by
       * using some string magic.
       */

      gchar *next_line = src_utils_str_tok('\n', content);
      gchar iface_name[64];
      gint val[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
      gint data_read = 0;

      //g_printf("In the Loop\n");
      if (delim_index < 0) {
         /* Find the index points of each field by searching for the | char. */
         src_utils_str_tok('|', content);
         delim_index = strlen(content);
      } else if (content[delim_index] == ':') {
         gchar *ptr = content;
         content[delim_index] = '\0';

         // First scan in the device name
         while(!g_ascii_isalnum(*ptr))
            ptr++;
         data_read += sscanf(ptr, "%s", iface_name);
         //g_printf("1:Read %d entries. %s\n", data_read, content);

         // Now scan in the first set of data removing trailing whitespace.
         ptr = &content[delim_index + 1];
         while(!g_ascii_isdigit(*ptr))
            ptr++;
         data_read += sscanf(ptr,
                             "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                             &val[0], &val[1], &val[2], &val[3],
                             &val[4], &val[5], &val[6], &val[7],
                             &val[8], &val[9], &val[10], &val[11],
                             &val[12], &val[13], &val[14], &val[15]);
         //g_printf("2:Read %d entries. %d:%d\n", data_read, val[0], val[8]);
         devicesRead++;

         if (data_read == 17) {
            gint i;
            PkaSample *s = pka_sample_new();

            pka_sample_append_string(s, 1, iface_name);
            for (i = 0; i < 16; i++) {
               pka_sample_append_int(s, i+2, val[i]);
            }

            pka_source_deliver_sample(PKA_SOURCE(source), s);
            pka_sample_unref(s);
         }
      }
      content = next_line;
   }
   RETURN(devicesRead);
}

/*
 * Handle a sample callback from the PkaSourceSimple.
 */
static inline void
netdev_sample (PkaSourceSimple *source,    /* IN */
               gpointer         user_data) /* IN */
{
	NetDevData *ndd = user_data;

	/*
	 * Create and deliver our manifest if it has not yet been done.
	 */
	if (G_UNLIKELY(!ndd->manifest)) {
		ndd->manifest = pka_manifest_sized_new(17);
		pka_manifest_append(ndd->manifest, "InterfaceName", G_TYPE_STRING);
		pka_manifest_append(ndd->manifest, "rxBytes", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "rxPackets", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "rxErrors", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "rxDropped", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "rxFifo", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "rxFrame", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "rxCompressed", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "rxMulticast", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "txBytes", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "txPackets", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "txErrors", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "txDropped", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "txFifo", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "txCollisions", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "txCarrier", G_TYPE_UINT);
		pka_manifest_append(ndd->manifest, "txCompressed", G_TYPE_UINT);
		pka_source_deliver_manifest(PKA_SOURCE(source), ndd->manifest);
	}

	/*
	 * Retrieve the sample.
	 */
	netdev_read(source);
}


/*
 * Handle a spawn event from the PkaSourceSimple.
 */
static void
netdev_spawn (PkaSourceSimple *source,
              PkaSpawnInfo    *spawn_info,
              gpointer         user_data)
{
	// No parameters are used
	ENTRY;
	EXIT;
}


/*
 * Free the netdev state when source is destroyed.
 */
static void
netdev_free (gpointer data)
{
	NetDevData *ndd = data;

	ENTRY;
	if (ndd->manifest) {
		pka_manifest_unref(ndd->manifest);
	}
	g_slice_free(NetDevData, ndd);
	EXIT;
}


/*
 * Create a new PkaSourceSimple for netdev sampling.
 */
GObject*
netdev_new (GError **error) /* OUT */
{
	PkaSource *source;
	NetDevData *netdev;

	ENTRY;
	netdev = g_slice_new0(NetDevData);
	source = pka_source_simple_new_full(netdev_sample,
	                                    netdev_spawn,
	                                    netdev,
	                                    netdev_free);
	RETURN(G_OBJECT(source));
}

const PkaPluginInfo pka_plugin_info = {
	.id          = "NetDev",
	.name        = "Network usage",
	.description = "This source provides network usage of a given network interface.",
	.version     = "0.1.1",
	.copyright   = "2010 Andrew Stiegmann",
	.factory     = netdev_new,
	.plugin_type = PKA_PLUGIN_SOURCE,
};
