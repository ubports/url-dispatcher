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
#include "create-db-sql.h"

sqlite3 *
url_db_create_database (void)
{
	const gchar * cachedir = g_getenv("URL_DISPATCHER_CACHE_DIR"); /* Mostly for testing */

	if (G_LIKELY(cachedir == NULL)) {
		cachedir = g_get_user_cache_dir();
	}

	gchar * urldispatchercachedir = g_build_filename(cachedir, "url-dispatcher", NULL);
	if (!g_file_test(urldispatchercachedir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		gint cachedirokay = g_mkdir_with_parents(urldispatchercachedir, 1 << 6 | 1 << 7 | 1 << 8); // 700

		if (cachedirokay != 0) {
			g_warning("Unable to make or find cache directory '%s'", urldispatchercachedir);
			g_free(urldispatchercachedir);
			return NULL;
		}
	}

	gchar * dbfilename = g_build_filename(urldispatchercachedir, "urls.db", NULL);
	g_free(urldispatchercachedir);

	gboolean dbexists = g_file_test(dbfilename, G_FILE_TEST_EXISTS);
	int open_status = SQLITE_ERROR;
	sqlite3 * db = NULL;

	open_status = sqlite3_open(dbfilename, &db);
	if (open_status != SQLITE_OK) {
		g_warning("Unable to open URL database");
		g_free(dbfilename);
		if (db != NULL) {
			sqlite3_close(db);
		}
		return NULL;
	}

	g_free(dbfilename);

	if (!dbexists) { /* First usage */
		int exec_status = SQLITE_ERROR;
		char * failstring = NULL;

		exec_status = sqlite3_exec(db, create_db_sql, NULL, NULL, &failstring);

		if (exec_status != SQLITE_OK) {
			g_warning("Unable to create tables: %s", failstring);
			sqlite3_close(db);
			return NULL;
		}

	}

	return db;
}

gboolean
url_db_get_file_motification_time (sqlite3 * db, const gchar * filename, GTimeVal * timeval)
{
	g_return_val_if_fail(db != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(timeval != NULL, FALSE);



	return FALSE;
}

gboolean
url_db_set_file_motification_time (sqlite3 * db, const gchar * filename, GTimeVal * timeval)
{
	g_return_val_if_fail(db != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(timeval != NULL, FALSE);



	return FALSE;
}

gboolean
url_db_insert_url (sqlite3 * db, const gchar * filename, const gchar * protocol, const gchar * domainsuffix)
{
	g_return_val_if_fail(db != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(protocol != NULL, FALSE);


	return FALSE;
}
