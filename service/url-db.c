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
 * Author: Ted Gould <ted@canonical.com>
 *
 */

#include <glib.h>
#include "url-db.h"
#include "create-db-sql.h"

#define DB_SCHEMA_VERSION "1"

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

	gchar * dbfilename = g_build_filename(urldispatchercachedir, "urls-" DB_SCHEMA_VERSION ".db", NULL);
	g_free(urldispatchercachedir);

	gboolean dbexists = g_file_test(dbfilename, G_FILE_TEST_EXISTS);
	int open_status = SQLITE_ERROR;
	sqlite3 * db = NULL;

	open_status = sqlite3_open(dbfilename, &db);
	if (open_status != SQLITE_OK) {
		g_warning("Unable to open URL database: %s", sqlite3_errmsg(db));
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
			sqlite3_free(failstring);
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

	timeval->tv_sec = 0;
	timeval->tv_usec = 0;

	sqlite3_stmt * stmt;
	if (sqlite3_prepare_v2(db,
			"select timestamp from configfiles where name = ?1",
			-1, /* length */
			&stmt,
			NULL) != SQLITE_OK) {
		g_warning("Unable to parse SQL to get file times: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_TRANSIENT);

	gboolean valueset = FALSE;
	int exec_status = SQLITE_ROW;
	while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {
		if (timeval->tv_sec != 0) {
			g_warning("Seemingly two timestamps for the same file");
		}

		timeval->tv_sec = sqlite3_column_int(stmt, 0);
		valueset = TRUE;
	}

	sqlite3_finalize(stmt);

	if (exec_status != SQLITE_DONE) {
		g_warning("Unable to execute insert");
		return FALSE;
	}

	return valueset;
}

gboolean
url_db_set_file_motification_time (sqlite3 * db, const gchar * filename, GTimeVal * timeval)
{
	g_return_val_if_fail(db != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(timeval != NULL, FALSE);

	sqlite3_stmt * stmt;
	if (sqlite3_prepare_v2(db,
			"insert or replace into configfiles values (?1, ?2)",
			-1, /* length */
			&stmt,
			NULL) != SQLITE_OK) {
		g_warning("Unable to parse SQL to set file times: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 2, timeval->tv_sec);

	int exec_status = SQLITE_ROW;
	while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {}

	sqlite3_finalize(stmt);

	if (exec_status != SQLITE_DONE) {
		g_warning("Unable to execute insert");
		return FALSE;
	}

	return TRUE;
}

gboolean
url_db_insert_url (sqlite3 * db, const gchar * filename, const gchar * protocol, const gchar * domainsuffix)
{
	g_return_val_if_fail(db != NULL, FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(protocol != NULL, FALSE);

	if (domainsuffix == NULL) {
		domainsuffix = "";
	}

	sqlite3_stmt * stmt;
	if (sqlite3_prepare_v2(db,
			"insert or replace into urls select rowid, ?2, ?3 from configfiles where name = ?1",
			-1, /* length */
			&stmt,
			NULL) != SQLITE_OK) {
		g_warning("Unable to parse SQL to insert");
		return FALSE;
	}

	sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, protocol, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, domainsuffix, -1, SQLITE_TRANSIENT);

	int exec_status = SQLITE_ROW;
	while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {}

	sqlite3_finalize(stmt);

	if (exec_status != SQLITE_DONE) {
		g_warning("Unable to execute insert: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	return TRUE;
}

gchar *
url_db_find_url (sqlite3 * db, const gchar * protocol, const gchar * domainsuffix)
{
	g_return_val_if_fail(db != NULL, NULL);
	g_return_val_if_fail(protocol != NULL, NULL);

	if (domainsuffix == NULL) {
		domainsuffix = "";
	}

	sqlite3_stmt * stmt;
	if (sqlite3_prepare_v2(db,
			"select configfiles.name from configfiles, urls where urls.sourcefile = configfiles.rowid and urls.protocol = ?1 and ?2 like '%' || urls.domainsuffix order by length(urls.domainsuffix) desc limit 1",
			-1, /* length */
			&stmt,
			NULL) != SQLITE_OK) {
		g_warning("Unable to parse SQL to find url: %s", sqlite3_errmsg(db));
		return NULL;
	}

	sqlite3_bind_text(stmt, 1, protocol, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, domainsuffix, -1, SQLITE_TRANSIENT);

	gchar * filename = NULL;
	int exec_status = SQLITE_ROW;
	while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW && filename == NULL) {
		filename = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));
	}

	gchar * output = NULL;
	if (filename != NULL) {
		g_debug("Found file: '%s'", filename);
		gchar * basename = g_path_get_basename(filename);
		gchar * suffix = g_strrstr(basename, ".url-dispatcher");
		if (suffix != NULL) /* This should never not happen, but it's too scary not to throw this 'if' in */
			suffix[0] = '\0';
		output = g_strdup(basename);
		g_free(basename);
		g_free(filename);
	}

	sqlite3_finalize(stmt);

	if (exec_status != SQLITE_DONE) {
		g_warning("Unable to execute insert: %s", sqlite3_errmsg(db));
		g_free(output);
		return NULL;
	}

	return output;
}

GList *
url_db_files_for_dir (sqlite3 * db, const gchar * dir)
{
	g_return_val_if_fail(db != NULL, NULL);

	if (dir == NULL) {
		dir = "";
	}

	sqlite3_stmt * stmt;
	if (sqlite3_prepare_v2(db,
			"select name from configfiles where name like ?1",
			-1, /* length */
			&stmt,
			NULL) != SQLITE_OK) {
		g_warning("Unable to parse SQL to find files: %s", sqlite3_errmsg(db));
		return NULL;
	}

	gchar * dir_search = g_strdup_printf("%s%%", dir);
	sqlite3_bind_text(stmt, 1, dir_search, -1, SQLITE_TRANSIENT);

	GList * filelist = NULL;
	int exec_status = SQLITE_ROW;
	while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {
		gchar * name = g_strdup((const gchar *)sqlite3_column_text(stmt, 0));
		filelist = g_list_prepend(filelist, name);
	}

	sqlite3_finalize(stmt);
	g_free(dir_search);

	if (exec_status != SQLITE_DONE) {
		g_warning("Unable to execute insert: %s", sqlite3_errmsg(db));
		g_list_free_full(filelist, g_free);
		return NULL;
	}

	return filelist;
}

/* Remove a file from the database along with all URLs that were
   built because of it. */
gboolean
url_db_remove_file (sqlite3 * db, const gchar * path)
{
	g_return_val_if_fail(db != NULL, FALSE);
	g_return_val_if_fail(path != NULL, FALSE);

	/* Start a transaction so the database doesn't end up
	   in an inconsistent state */
	if (sqlite3_exec(db, "begin", NULL, NULL, NULL) != SQLITE_OK) {
		g_warning("Unable to start transaction to delete: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	/* Remove all URLs for file */
	sqlite3_stmt * stmt;
	if (sqlite3_prepare_v2(db,
			"delete from urls where sourcefile in (select rowid from configfiles where name = ?1);",
			-1, /* length */
			&stmt,
			NULL) != SQLITE_OK) {
		g_warning("Unable to parse SQL to remove urls: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);

	int exec_status = SQLITE_ROW;
	while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {
	}

	sqlite3_finalize(stmt);

	if (exec_status != SQLITE_DONE) {
		g_warning("Unable to execute removal of URLs: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	/* Remove references to the file */
	stmt = NULL;

	if (sqlite3_prepare_v2(db,
			"delete from configfiles where name = ?1;",
			-1, /* length */
			&stmt,
			NULL) != SQLITE_OK) {
		g_warning("Unable to parse SQL to remove urls: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);

	while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {
	}

	sqlite3_finalize(stmt);

	if (exec_status != SQLITE_DONE) {
		g_warning("Unable to execute removal of file: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	/* Commit the full transaction */
	if (sqlite3_exec(db, "commit", NULL, NULL, NULL) != SQLITE_OK) {
		g_warning("Unable to commit transaction to delete: %s", sqlite3_errmsg(db));
		return FALSE;
	}

	return TRUE;
}
