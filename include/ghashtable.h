/*
 *   Copyright (c) 2025 Roi

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


#ifndef GHASHTABLE_H
#define GHASHTABLE_H

#include <glib.h>

extern GHashTable *ht;

void glib_new_hash_table();
gboolean glib_insert_new_value(char *, char *);
const char *fetch_vlevel_for_spid_ht(char *);

#ifdef DEBUG
void print_table_contents();
#endif /* DEBUG */

#endif /* GHASHTABLE_H */
