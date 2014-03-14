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

#include <gio/gio.h>
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

	url_db_insert_url(urldata->db, urldata->filename, protocol, suffix);
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
check_file_uptodate (const gchar * filename, sqlite3 * db)
{
	g_debug("Processing file: %s", filename);

	GTimeVal dbtime = {0};
	GTimeVal filetime = {0};

	GFile * file = g_file_new_for_path(filename);
	g_return_if_fail(file != NULL);

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

	url_db_set_file_motification_time(db, filename, &filetime);

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

/* In the beginning, there was main, and that was good */
int
main (int argc, char * argv[])
{
	if (argc != 2) {
		g_printerr("Usage: %s <directory>\n", argv[0]);
		return 1;
	}

	sqlite3 * db = url_db_create_database();
	g_return_val_if_fail(db != NULL, -1);

	/* Check out what we got and recover */
	gchar * dirname = g_strdup(argv[1]);
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
	if (g_file_test(dirname, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		GDir * dir = g_dir_open(dirname, 0, NULL);
		g_return_val_if_fail(dir != NULL, -1);

		const gchar * name = NULL;
		while ((name = g_dir_read_name(dir)) != NULL) {
			if (g_str_has_suffix(name, ".url-dispatcher")) {
				gchar * fullname = g_build_filename(dirname, name, NULL);

				if (check_file_uptodate(fullname, db)) {
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

	sqlite3_close(db);

	g_debug("Directory '%s' is up-to-date", dirname);
	g_free(dirname);

	return 0;
}
