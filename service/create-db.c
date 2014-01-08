
#include <sqlite3.h>
#include <glib.h>
#include "create-db-sql.h"

sqlite3 *
create_database (void)
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
