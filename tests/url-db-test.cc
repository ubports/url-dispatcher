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

#include <gtest/gtest.h>
#include "url-db.h"

class UrlDBTest : public ::testing::Test
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
};

TEST_F(UrlDBTest, CreateTest) {
	sqlite3 * db = url_db_create_database();

	ASSERT_TRUE(db != NULL);

	gchar * dbfile = g_build_filename(cachedir, "url-dispatcher", "urls.db", NULL);
	EXPECT_TRUE(g_file_test(dbfile, G_FILE_TEST_EXISTS));
	g_free(dbfile);

	const char * type = NULL;

	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, NULL, "configfiles", "name", &type, NULL, NULL, NULL, NULL));
	EXPECT_STREQ("text", type);
	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, NULL, "configfiles", "timestamp", &type, NULL, NULL, NULL, NULL));
	EXPECT_STREQ("bigint", type);

	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, NULL, "urls", "sourcefile", &type, NULL, NULL, NULL, NULL));
	EXPECT_STREQ("integer", type);
	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, NULL, "urls", "protocol", &type, NULL, NULL, NULL, NULL));
	EXPECT_STREQ("text", type);
	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, NULL, "urls", "domainsuffix", &type, NULL, NULL, NULL, NULL));
	EXPECT_STREQ("text", type);

	sqlite3_close(db);
}
