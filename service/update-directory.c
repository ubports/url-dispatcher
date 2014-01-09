/**
 * Copyright © 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <glib.h>
#include "url-db.h"

static void
process_file (const gchar * filename, sqlite3 * db)
{
	g_debug("Processing file: %s", filename);

}

int
main (int argc, char * argv[])
{
	if (argc != 2) {
		g_printerr("Usage: %s <directory>\n", argv[0]);
		return 1;
	}

	if (!g_file_test(argv[1], G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		g_print("Directory '%s' is up-to-date because it doesn't exist", argv[1]);
		return 0;
	}

	sqlite3 * db = url_db_create_database();
	g_return_val_if_fail(db != NULL, -1);

	GDir * dir = g_dir_open(argv[1], 0, NULL);
	g_return_val_if_fail(dir != NULL, -1);

	const gchar * name = NULL;
	while ((name = g_dir_read_name(dir)) != NULL) {
		if (g_str_has_suffix(name, ".url-dispatcher")) {
			process_file(name, db);
		}
	}

	g_dir_close(dir);
	sqlite3_close(db);

	g_debug("Directory '%s' is up-to-date", argv[1]);
	return 0;
}
