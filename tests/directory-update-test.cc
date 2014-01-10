/**
 * Copyright © 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 * * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <gtest/gtest.h>
#include "url-db.h"
#include <glib.h>

class DirectoryUpdateTest : public ::testing::Test
{
	protected:
		gchar * cachedir = NULL;

		virtual void SetUp() {
			cachedir = g_build_filename(CMAKE_BINARY_DIR, "url-db-test-cache", NULL);
			g_setenv("URL_DISPATCHER_CACHE_DIR", cachedir, TRUE);
		}

		virtual void TearDown() {
			gchar * cmdline = g_strdup_printf("rm -rf \"%s\"", cachedir);
			g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
			g_free(cmdline);
			g_free(cachedir);
		}

		int get_file_count (sqlite3 * db) {
			sqlite3_stmt * stmt;
			if (sqlite3_prepare_v2(db,
					"select count(*) from configfiles",
					-1, /* length */
					&stmt,
					NULL) != SQLITE_OK) {
				g_warning("Unable to parse SQL to get file times");
				return 0;
			}

			int retval = 0;
			int exec_status = SQLITE_ROW;
			while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {
				retval = sqlite3_column_int(stmt, 0);
			}

			sqlite3_finalize(stmt);

			if (exec_status != SQLITE_DONE) {
				g_warning("Unable to execute insert");
				return 0;
			}

			return retval;
		}

		int get_url_count (sqlite3 * db) {
			sqlite3_stmt * stmt;
			if (sqlite3_prepare_v2(db,
					"select count(*) from urls",
					-1, /* length */
					&stmt,
					NULL) != SQLITE_OK) {
				g_warning("Unable to parse SQL to get file times");
				return 0;
			}

			int retval = 0;
			int exec_status = SQLITE_ROW;
			while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {
				retval = sqlite3_column_int(stmt, 0);
			}

			sqlite3_finalize(stmt);

			if (exec_status != SQLITE_DONE) {
				g_warning("Unable to execute insert");
				return 0;
			}

			return retval;
		}

		bool has_file (sqlite3 * db, const char * filename) {
			sqlite3_stmt * stmt;
			if (sqlite3_prepare_v2(db,
					"select count(*) from configfiles where name = ?1",
					-1, /* length */
					&stmt,
					NULL) != SQLITE_OK) {
				g_warning("Unable to parse SQL to get file times");
				return false;
			}

			sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_TRANSIENT);

			int retval = 0;
			int exec_status = SQLITE_ROW;
			while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {
				retval = sqlite3_column_int(stmt, 0);
			}

			sqlite3_finalize(stmt);

			if (exec_status != SQLITE_DONE) {
				g_warning("Unable to execute insert");
				return false;
			}

			if (retval > 1) {
				g_warning("Database contains more than one instance of '%s'", filename);
				return false;
			}

			return retval == 1;
		}

		bool has_url (sqlite3 * db, const char * protocol, const char * domainsuffix) {
			sqlite3_stmt * stmt;
			if (sqlite3_prepare_v2(db,
					"select count(*) from urls where protocol = ?1 and domainsuffix = ?2",
					-1, /* length */
					&stmt,
					NULL) != SQLITE_OK) {
				g_warning("Unable to parse SQL to get file times");
				return false;
			}

			sqlite3_bind_text(stmt, 1, protocol, -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 2, domainsuffix, -1, SQLITE_TRANSIENT);

			int retval = 0;
			int exec_status = SQLITE_ROW;
			while ((exec_status = sqlite3_step(stmt)) == SQLITE_ROW) {
				retval = sqlite3_column_int(stmt, 0);
			}

			sqlite3_finalize(stmt);

			if (exec_status != SQLITE_DONE) {
				g_warning("Unable to execute insert");
				return false;
			}

			if (retval > 1) {
				g_warning("Database contains more than one instance of prtocol '%s'", protocol);
				return false;
			}

			return retval == 1;
		}
};

TEST_F(DirectoryUpdateTest, DirDoesntExist)
{
	sqlite3 * db = url_db_create_database();

	gchar * cmdline = g_strdup_printf("%s \"%s\"", UPDATE_DIRECTORY_TOOL, CMAKE_SOURCE_DIR "/this-does-not-exist");
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(0, get_file_count(db));
	EXPECT_EQ(0, get_url_count(db));

	sqlite3_close(db);
};

TEST_F(DirectoryUpdateTest, SingleGoodItem)
{
	sqlite3 * db = url_db_create_database();

	gchar * cmdline = g_strdup_printf("%s \"%s\"", UPDATE_DIRECTORY_TOOL, UPDATE_DIRECTORY_URLS);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(1, get_file_count(db));
	EXPECT_EQ(1, get_url_count(db));

	EXPECT_TRUE(has_file(db, UPDATE_DIRECTORY_URLS "/single-good.url-dispatcher"));
	EXPECT_TRUE(has_url(db, "http", "ubuntu.com"));

	sqlite3_close(db);
};


