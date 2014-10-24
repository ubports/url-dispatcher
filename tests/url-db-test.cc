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
		gchar * cachedir = nullptr;

		virtual void SetUp() {
			cachedir = g_build_filename(CMAKE_BINARY_DIR, "url-db-test-cache", nullptr);
			g_setenv("URL_DISPATCHER_CACHE_DIR", cachedir, TRUE);
		}

		virtual void TearDown() {
			gchar * cmdline = g_strdup_printf("rm -rf \"%s\"", cachedir);
			g_spawn_command_line_sync(cmdline, nullptr, nullptr, nullptr, nullptr);
			g_free(cmdline);
			g_free(cachedir);
		}

		bool file_list_has (GList * list, const gchar * filename) {
			GList * cur;

			for (cur = list; cur != nullptr; cur = g_list_next(cur)) {
				const gchar * path = (const gchar *)cur->data;
				gchar * basename = g_path_get_basename(path);
				gint same = g_strcmp0(basename, filename);
				g_free(basename);

				if (same == 0) {
					return true;
				}
			}

			return false;
		}
};

TEST_F(UrlDBTest, CreateTest) {
	sqlite3 * db = url_db_create_database();

	ASSERT_TRUE(db != nullptr);

	gchar * dbfile = g_build_filename(cachedir, "url-dispatcher", "urls-1.db", nullptr);
	EXPECT_TRUE(g_file_test(dbfile, G_FILE_TEST_EXISTS));
	g_free(dbfile);

	const char * type = nullptr;

	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, nullptr, "configfiles", "name", &type, nullptr, nullptr, nullptr, nullptr));
	EXPECT_STREQ("text", type);
	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, nullptr, "configfiles", "timestamp", &type, nullptr, nullptr, nullptr, nullptr));
	EXPECT_STREQ("bigint", type);

	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, nullptr, "urls", "sourcefile", &type, nullptr, nullptr, nullptr, nullptr));
	EXPECT_STREQ("integer", type);
	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, nullptr, "urls", "protocol", &type, nullptr, nullptr, nullptr, nullptr));
	EXPECT_STREQ("text", type);
	EXPECT_EQ(SQLITE_OK, sqlite3_table_column_metadata(db, nullptr, "urls", "domainsuffix", &type, nullptr, nullptr, nullptr, nullptr));
	EXPECT_STREQ("text", type);

	sqlite3_close(db);
}

TEST_F(UrlDBTest, TimestampTest) {
	sqlite3 * db = url_db_create_database();

	ASSERT_TRUE(db != nullptr);

	GTimeVal timeval = {0};
	EXPECT_FALSE(url_db_get_file_motification_time(db, "/foo", &timeval));

	timeval.tv_sec = 12345;
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/foo", &timeval));

	timeval.tv_sec = 0;
	EXPECT_TRUE(url_db_get_file_motification_time(db, "/foo", &timeval));
	EXPECT_EQ(12345, timeval.tv_sec);

	sqlite3_close(db);
}

TEST_F(UrlDBTest, UrlTest) {
	sqlite3 * db = url_db_create_database();

	ASSERT_TRUE(db != nullptr);

	GTimeVal timeval = {0};
	timeval.tv_sec = 12345;
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/foo.url-dispatcher", &timeval));

	/* Insert and find */
	EXPECT_TRUE(url_db_insert_url(db, "/foo.url-dispatcher", "bar", "foo.com"));
	EXPECT_STREQ("foo", url_db_find_url(db, "bar", "foo.com"));
	EXPECT_STREQ("foo", url_db_find_url(db, "bar", "www.foo.com"));

	/* Two to compete */
	timeval.tv_sec = 67890;
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/bar.url-dispatcher", &timeval));
	EXPECT_TRUE(url_db_insert_url(db, "/bar.url-dispatcher", "bar", "more.foo.com"));
	EXPECT_STREQ("foo", url_db_find_url(db, "bar", "www.foo.com"));
	EXPECT_STREQ("bar", url_db_find_url(db, "bar", "more.foo.com"));
	EXPECT_STREQ("bar", url_db_find_url(db, "bar", "www.more.foo.com"));

	sqlite3_close(db);
}

TEST_F(UrlDBTest, FileListTest) {
	sqlite3 * db = url_db_create_database();

	ASSERT_TRUE(db != nullptr);

	GTimeVal timeval = {0};
	timeval.tv_sec = 12345;

	/* One Dir */
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/base/directory/for/us/one.url-dispatcher", &timeval));
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/base/directory/for/us/two.url-dispatcher", &timeval));
	/* No three! */
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/base/directory/for/us/four.url-dispatcher", &timeval));
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/base/directory/for/us/five.url-dispatcher", &timeval));

	/* Another Dir */
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/base/directory/for/them/six.url-dispatcher", &timeval));

	GList * files = url_db_files_for_dir(db, "/base/directory/for/us");
	EXPECT_TRUE(file_list_has(files, "one.url-dispatcher"));
	EXPECT_TRUE(file_list_has(files, "two.url-dispatcher"));
	EXPECT_FALSE(file_list_has(files, "three.url-dispatcher"));
	EXPECT_TRUE(file_list_has(files, "four.url-dispatcher"));
	EXPECT_TRUE(file_list_has(files, "five.url-dispatcher"));
	EXPECT_FALSE(file_list_has(files, "six.url-dispatcher"));

	g_list_free_full(files, g_free);

	files = url_db_files_for_dir(db, "/base/directory/for/them");
	EXPECT_FALSE(file_list_has(files, "five.url-dispatcher"));
	EXPECT_TRUE(file_list_has(files, "six.url-dispatcher"));

	g_list_free_full(files, g_free);

	files = url_db_files_for_dir(db, "/dir/not/there");
	EXPECT_EQ(0, g_list_length(files));

	sqlite3_close(db);
}

TEST_F(UrlDBTest, RemoveFile) {
	sqlite3 * db = url_db_create_database();

	ASSERT_TRUE(db != nullptr);

	GTimeVal timeval = {0};
	timeval.tv_sec = 12345;
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/foo.url-dispatcher", &timeval));

	/* Insert and find */
	EXPECT_TRUE(url_db_insert_url(db, "/foo.url-dispatcher", "bar", "foo.com"));
	EXPECT_STREQ("foo", url_db_find_url(db, "bar", "foo.com"));

	/* Remove and not find */
	EXPECT_TRUE(url_db_remove_file(db, "/foo.url-dispatcher"));
	EXPECT_FALSE(url_db_get_file_motification_time(db, "/foo.url-dispatcher", &timeval));
	EXPECT_STREQ(nullptr, url_db_find_url(db, "bar", "foo.com"));

	sqlite3_close(db);
}

TEST_F(UrlDBTest, ReplaceTest) {
	sqlite3 * db = url_db_create_database();

	ASSERT_TRUE(db != nullptr);

	GTimeVal timeval = {0};
	GTimeVal timevaltest = {0};

	timeval.tv_sec = 12345;
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/foo.url-dispatcher", &timeval));

	url_db_get_file_motification_time(db, "/foo.url-dispatcher", &timevaltest);
	EXPECT_EQ(12345, timevaltest.tv_sec);

	/* Replace it */
	timeval.tv_sec = 67890;
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/foo.url-dispatcher", &timeval));

	url_db_get_file_motification_time(db, "/foo.url-dispatcher", &timevaltest);
	EXPECT_EQ(67890, timevaltest.tv_sec);

	/* Replace it again with the same value */
	EXPECT_TRUE(url_db_set_file_motification_time(db, "/foo.url-dispatcher", &timeval));

	url_db_get_file_motification_time(db, "/foo.url-dispatcher", &timevaltest);
	EXPECT_EQ(67890, timevaltest.tv_sec);

	sqlite3_close(db);
}

