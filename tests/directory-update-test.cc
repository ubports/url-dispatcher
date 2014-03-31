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

TEST_F(DirectoryUpdateTest, RerunAgain)
{
	gchar * cmdline = NULL;
	sqlite3 * db = url_db_create_database();

	cmdline = g_strdup_printf("%s \"%s\"", UPDATE_DIRECTORY_TOOL, UPDATE_DIRECTORY_URLS);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(1, get_file_count(db));
	EXPECT_EQ(1, get_url_count(db));

	cmdline = g_strdup_printf("%s \"%s\"", UPDATE_DIRECTORY_TOOL, UPDATE_DIRECTORY_URLS);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(1, get_file_count(db));
	EXPECT_EQ(1, get_url_count(db));

	cmdline = g_strdup_printf("%s \"%s\"", UPDATE_DIRECTORY_TOOL, UPDATE_DIRECTORY_URLS);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(1, get_file_count(db));
	EXPECT_EQ(1, get_url_count(db));

	sqlite3_close(db);
};

TEST_F(DirectoryUpdateTest, VariedItems)
{
	sqlite3 * db = url_db_create_database();

	gchar * cmdline = g_strdup_printf("%s \"%s\"", UPDATE_DIRECTORY_TOOL, UPDATE_DIRECTORY_VARIED);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(6, get_file_count(db));
	EXPECT_EQ(13, get_url_count(db));

	/* object-base.url-dispatcher */
	EXPECT_TRUE(has_file(db, UPDATE_DIRECTORY_VARIED "/object-base.url-dispatcher"));
	EXPECT_FALSE(has_url(db, "object", "object-base.com"));

	/* bad-filename-suffix.url-launcher */
	EXPECT_FALSE(has_file(db, UPDATE_DIRECTORY_VARIED "/bad-filename-suffix.url-launcher"));
	EXPECT_FALSE(has_url(db, "badsuffix", "bad.suffix.com"));

	/* not-json.url-dispatcher */
	EXPECT_TRUE(has_file(db, UPDATE_DIRECTORY_VARIED "/not-json.url-dispatcher"));
	EXPECT_FALSE(has_url(db, "notjson", "not.json.com"));

	/* lots-o-entries.url-dispatcher */
	EXPECT_TRUE(has_file(db, UPDATE_DIRECTORY_VARIED "/lots-o-entries.url-dispatcher"));
	EXPECT_TRUE(has_url(db, "lots0", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots1", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots2", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots3", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots4", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots5", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots6", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots7", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots8", "lots.com"));
	EXPECT_TRUE(has_url(db, "lots9", "lots.com"));

	/* duplicate.url-dispatcher */
	EXPECT_TRUE(has_file(db, UPDATE_DIRECTORY_VARIED "/duplicate.url-dispatcher"));
	EXPECT_TRUE(has_url(db, "duplicate", "dup.licate.com"));

	/* dup-file-1.url-dispatcher */
	/* dup-file-2.url-dispatcher */
	EXPECT_TRUE(has_file(db, UPDATE_DIRECTORY_VARIED "/dup-file-1.url-dispatcher"));
	EXPECT_TRUE(has_file(db, UPDATE_DIRECTORY_VARIED "/dup-file-2.url-dispatcher"));
	EXPECT_FALSE(has_url(db, "dupfile", "this.is.in.two.file.org"));

	sqlite3_close(db);
};

TEST_F(DirectoryUpdateTest, RemoveFile)
{
	gchar * cmdline;
	sqlite3 * db = url_db_create_database();

	/* A temporary directory to put files in */
	gchar * datadir = g_build_filename(CMAKE_BINARY_DIR, "remove-file-data", NULL);
	g_mkdir_with_parents(datadir,  1 << 6 | 1 << 7 | 1 << 8); // 700
	ASSERT_TRUE(g_file_test(datadir, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)));

	/* Copy the files */
	cmdline = g_strdup_printf("cp \"%s/%s\" \"%s\"", UPDATE_DIRECTORY_URLS, "single-good.url-dispatcher", datadir);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	/* Run the tool */
	cmdline = g_strdup_printf("%s \"%s\"", UPDATE_DIRECTORY_TOOL, datadir);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(1, get_file_count(db));
	EXPECT_EQ(1, get_url_count(db));

	/* Kill the files */
	cmdline = g_strdup_printf("rm \"%s/%s\"", datadir, "single-good.url-dispatcher");
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	/* Run the tool */
	cmdline = g_strdup_printf("%s \"%s\"", UPDATE_DIRECTORY_TOOL, datadir);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(0, get_file_count(db));
	EXPECT_EQ(0, get_url_count(db));

	/* Cleanup */
	cmdline = g_strdup_printf("rm -rf \"%s\"", datadir);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	sqlite3_close(db);
}

TEST_F(DirectoryUpdateTest, RemoveDirectory)
{
	gchar * cmdline;
	sqlite3 * db = url_db_create_database();

	/* A temporary directory to put files in */
	gchar * datadir = g_build_filename(CMAKE_BINARY_DIR, "remove-directory-data", NULL);
	g_mkdir_with_parents(datadir,  1 << 6 | 1 << 7 | 1 << 8); // 700
	ASSERT_TRUE(g_file_test(datadir, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)));

	/* Copy the files */
	cmdline = g_strdup_printf("cp \"%s/%s\" \"%s\"", UPDATE_DIRECTORY_URLS, "single-good.url-dispatcher", datadir);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	/* Run the tool */
	cmdline = g_strdup_printf("%s \"%s/\"", UPDATE_DIRECTORY_TOOL, datadir);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(1, get_file_count(db));
	EXPECT_EQ(1, get_url_count(db));

	/* Kill the dir */
	cmdline = g_strdup_printf("rm -rf \"%s\"", datadir);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	/* Run the tool */
	cmdline = g_strdup_printf("%s \"%s/\"", UPDATE_DIRECTORY_TOOL, datadir);
	g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
	g_free(cmdline);

	EXPECT_EQ(0, get_file_count(db));
	EXPECT_EQ(0, get_url_count(db));

	/* Cleanup */
	sqlite3_close(db);
}
