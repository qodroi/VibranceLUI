/*
 *   Copyright (c) 2024 Roi

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ghashtable.h"
#include "vibrancelui.h"

GHashTable *ht;

void glib_new_hash_table()
{
    ht = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

/*  Add key and value to GHashTable _or_ replace (if `key` already exists)
    Returns false if key wasn't already in the table, and true if it was.

    NOTE: This function duplicates a string and allocates memory for it.
*/
gboolean glib_insert_new_value(char *spid, char *vlevel)
{
    if (!ht || !spid || !vlevel)
        return false;

    return g_hash_table_insert(ht, g_strdup(spid), g_strdup(vlevel));
}

/* Fetch the Vibrance level for the given Process ID `spid` */
const char *fetch_vlevel_for_spid_ht(char *spid)
{
    return g_hash_table_lookup(ht, spid); /* NULL if spid isn't present. */
}

/* Remove all keys and values off the table */
__always_inline void glib_clear_hash_table()
{
    return g_hash_table_remove_all(ht);
}

/* Return the number of elements contained in the GHashTable, i.e.
    Number of processes tracked. */
static __always_inline guint glib_pids_in_ht()
{
    return g_hash_table_size(ht);
}

#ifdef DEBUG
void ghfunc_callback(gpointer key,
    gpointer value, gpointer user_data)
{
    guint total_pairs_in_ht = glib_pids_in_ht();
    DEBUG_PRINTF("PAIRS TOTAL - %u : KEY - %s ; VALUE - %s\n",
        total_pairs_in_ht, (char *)key, (char *)value);
}

void print_table_contents()
{
    g_hash_table_foreach(ht, ghfunc_callback, NULL);
}
#endif


