/**
 * Copyright (C) 2013 Canonical, Ltd.
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


#include <gio/gio.h>
#include <gtest/gtest.h>
#include "dispatcher.h"
#include "ubuntu-app-launch-mock.h"
#include "url-db.h"

class DispatcherTest : public ::testing::Test
{
	private:
		GTestDBus * testbus = NULL;
		GMainLoop * mainloop = NULL;
		gchar * cachedir = NULL;

	protected:
		virtual void SetUp() {
			g_setenv("TEST_CLICK_DB", "click-db", TRUE);
			g_setenv("TEST_CLICK_USER", "test-user", TRUE);

			cachedir = g_build_filename(CMAKE_BINARY_DIR, "dispatcher-test-cache", NULL);
			g_setenv("URL_DISPATCHER_CACHE_DIR", cachedir, TRUE);

			sqlite3 * db = url_db_create_database();
			GTimeVal timestamp = { .tv_sec = 12345 };

			url_db_set_file_motification_time(db, "/testdir/com.ubuntu.calendar_calendar_9.8.2343.url-dispatcher", &timestamp);
			url_db_insert_url(db, "/testdir/com.ubuntu.calendar_calendar_9.8.2343.url-dispatcher", "calendar", NULL);

			sqlite3_close(db);

			testbus = g_test_dbus_new(G_TEST_DBUS_NONE);
			g_test_dbus_up(testbus);

			mainloop = g_main_loop_new(NULL, FALSE);
			dispatcher_init(mainloop);

			return;
		}

		virtual void TearDown() {
			dispatcher_shutdown();

			/* Clean up queued events */
			while (g_main_pending()) {
				g_main_iteration(TRUE);
			}

			g_main_loop_unref(mainloop);

			ubuntu_app_launch_mock_clear_last_app_id();

			/* let other threads settle */
			g_usleep(500000);

			g_test_dbus_down(testbus);
			g_object_unref(testbus);

			gchar * cmdline = g_strdup_printf("rm -rf \"%s\"", cachedir);
			g_spawn_command_line_sync(cmdline, NULL, NULL, NULL, NULL);
			g_free(cmdline);
			g_free(cachedir);
			return;
		}
};

TEST_F(DispatcherTest, ApplicationTest)
{
	/* Good sanity check */
	dispatch_url("application:///foo.desktop");
	ASSERT_STREQ("foo", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* No .desktop */
	dispatch_url("application:///foo");
	ASSERT_TRUE(NULL == ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Missing a / */
	dispatch_url("application://foo.desktop");
	ASSERT_TRUE(NULL == ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Good with hyphens */
	dispatch_url("application:///my-really-cool-app.desktop");
	ASSERT_STREQ("my-really-cool-app", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Good Click Style */
	dispatch_url("application:///com.test.foo_bar-app_0.3.4.desktop");
	ASSERT_STREQ("com.test.foo_bar-app_0.3.4", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	return;
}

TEST_F(DispatcherTest, CalendarTest)
{
	/* Base Calendar */
	dispatch_url("calendar:///?starttime=196311221830Z");
	ASSERT_STREQ("com.ubuntu.calendar_calendar_9.8.2343", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Two Slash, nothing else */
	dispatch_url("calendar://");
	ASSERT_STREQ("com.ubuntu.calendar_calendar_9.8.2343", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	return;
}

