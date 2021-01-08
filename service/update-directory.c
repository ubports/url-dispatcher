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

#include "update-directory.h"

#include <json-glib/json-glib.h>
#include "url-db.h"

typedef struct {
	const gchar * filename;
	sqlite3 * db;
} urldata_t;

static void
each_url (JsonArray * array, guint index, JsonNode * value, gpointer user_data)
{
	urldata_t * urldata = (urldata_t *)user_data;

	if (!JSON_NODE_HOLDS_OBJECT(value)) {
		g_warning("File %s: Array entry %d not an object", urldata->filename, index);
		return;
	}

	JsonObject * obj = json_node_get_object(value);

	const gchar * protocol = NULL;
	const gchar * suffix = NULL;

	if (json_object_has_member(obj, "protocol")) {
		protocol = json_object_get_string_member(obj, "protocol");
	}

	if (json_object_has_member(obj, "domain-suffix")) {
		suffix = json_object_get_string_member(obj, "domain-suffix");
	}

	if (protocol == NULL) {
		g_warning("File %s: Array entry %d doesn't contain a 'protocol'", urldata->filename, index);
		return;
	}

	if (g_strcmp0(protocol, "intent") == 0) {
		/* Special handling for intent, we have to have a domain suffix
		   there because otherwise things will get crazy as we're handling
		   it by package lookup in the service. */
		if (suffix == NULL) {
			g_warning("File %s: Array entry %d is an 'intent' protocol but doesn't have a package name", urldata->filename, index);
			return;
		}
	}

	if (!url_db_insert_url(urldata->db, urldata->filename, protocol, suffix)) {
        g_warning("Unable to add protocol '%s' with suffix '%s' from '%s'.",
                  protocol, suffix, urldata->filename);
	}
}

static void
insert_urls_from_file (const gchar * filename, sqlite3 * db)
{
	GError * error = NULL;
	JsonParser * parser = json_parser_new();
	json_parser_load_from_file(parser, filename, &error);

	if (error != NULL) {
		g_warning("Unable to parse JSON in '%s': %s", filename, error->message);
		g_object_unref(parser);
		g_error_free(error);
		return;
	}

	JsonNode * rootnode = json_parser_get_root(parser);
	if (!JSON_NODE_HOLDS_ARRAY(rootnode)) {
		g_warning("File '%s' does not have an array as its root node", filename);
		g_object_unref(parser);
		return;
	}

	JsonArray * rootarray = json_node_get_array(rootnode);

	urldata_t urldata = {
		.filename = filename,
		.db = db
	};

	json_array_foreach_element(rootarray, each_url, &urldata);

	g_object_unref(parser);
}

static gboolean
check_file_outofdate (const gchar * filename, sqlite3 * db)
{
	g_debug("Processing file: %s", filename);

	GTimeVal dbtime = {0};
	GTimeVal filetime = {0};

	GFile * file = g_file_new_for_path(filename);
	g_return_val_if_fail(file != NULL, FALSE);

	GFileInfo * info = g_file_query_info(file, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_file_info_get_modification_time(info, &filetime);

	g_object_unref(info);
	g_object_unref(file);

	if (url_db_get_file_motification_time(db, filename, &dbtime)) {
		if (filetime.tv_sec <= dbtime.tv_sec) {
			g_debug("\tup-to-date: %s", filename);
			return FALSE;
		}
	}

	if (!url_db_set_file_motification_time(db, filename, &filetime)) {
        g_warning("Error updating SQLite with file '%s'.", filename);
		return FALSE;
	}

	return TRUE;
}

/* Remove a file from the database */
static void
remove_file (gpointer key, gpointer value, gpointer user_data)
{
	const gchar * filename = (const gchar *)key;
	g_debug("  Removing file: %s", filename);
	if (!url_db_remove_file((sqlite3*)user_data, filename)) {
		g_warning("Unable to remove file: %s", filename);
	}
}

static gchar *
sanitize_path (const gchar *directory)
{
	/* Check out what we got and recover */
	gchar * dirname = g_strdup(directory);
	if (!g_file_test(dirname, G_FILE_TEST_IS_DIR) && !g_str_has_suffix(dirname, "/")) {
		gchar * upone = g_path_get_dirname(dirname);
		/* Upstart will give us filenames a bit, let's handle them */
		if (g_file_test(upone, G_FILE_TEST_IS_DIR)) {
			g_free(dirname);
			dirname = upone;
		} else {
			/* If the dirname function doesn't help, stick with what
			   we were given, the whole thing coulda been deleted */
			g_free(upone);
		}
	}

	return dirname;
}

static int
update_directory (gchar *dirname)
{
	sqlite3 * db = url_db_create_database();
	g_return_val_if_fail(db != NULL, -1);

	/* Get the current files in the directory in the DB so we
	   know if any got dropped */
	GHashTable * startingdb = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	GList * files = url_db_files_for_dir(db, dirname);
	GList * cur;
	for (cur = files; cur != NULL; cur = g_list_next(cur)) {
		g_hash_table_add(startingdb, cur->data);
	}
	g_list_free(files);


	/* Open the directory on the file system and start going
	   through it */
	if (g_file_test(dirname, G_FILE_TEST_IS_DIR)) {
		GDir * dir = g_dir_open(dirname, 0, NULL);
		g_return_val_if_fail(dir != NULL, -1);

		const gchar * name = NULL;
		while ((name = g_dir_read_name(dir)) != NULL) {
			if (g_str_has_suffix(name, ".url-dispatcher")) {
				gchar * fullname = g_build_filename(dirname, name, NULL);

				if (check_file_outofdate(fullname, db)) {
					insert_urls_from_file(fullname, db);
				}

				g_hash_table_remove(startingdb, fullname);
				g_free(fullname);
			}
		}

		g_dir_close(dir);
	}

	/* Remove deleted files */
	g_hash_table_foreach(startingdb, remove_file, db);
	g_hash_table_destroy(startingdb);

	int close_status = sqlite3_close(db);
	if (close_status != SQLITE_OK) {
        g_critical("Error closing SQLite db: %d", close_status);
	}

	g_debug("Directory '%s' is up-to-date", dirname);
	g_free(dirname);

	return 0;
}

static void
file_changed_cb(GFileMonitor * monitor, GFile *file, GFile * other, GFileMonitorEvent evtype, gpointer user_data)
{
	gchar *fpath = g_file_get_path(file);
	g_debug("Directory '%s' changed, updating directory", fpath);
	update_directory(fpath);
	g_free(fpath);
}

// API
int
update_and_monitor_directory (const gchar *directory)
{
	gchar * dirname = sanitize_path(directory);

	if (!g_file_test(dirname, G_FILE_TEST_IS_DIR)) {
		g_debug("Directory not found: %s", dirname);
		return -1;
	}

	update_directory(dirname);

	GFile *f = g_file_new_for_path(dirname);

	GError *err = NULL;
	GFileMonitor *fm = g_file_monitor_directory(f, G_FILE_MONITOR_WATCH_MOVES, NULL, &err);
	if (err) {
		g_warning("unable to monitor %s: %s\n", dirname, err->message);
		g_error_free(err);
		return -1;
	}

	g_signal_connect(G_OBJECT(fm), "changed", G_CALLBACK(file_changed_cb), NULL);

	return 0;
}


int
update_directory_sanitize (const gchar *directory)
{
	return update_directory(sanitize_path(directory));
}
