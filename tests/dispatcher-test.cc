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

			url_db_set_file_motification_time(db, "/testdir/com.ubuntu.dialer_dialer_1234.url-dispatcher", &timestamp);
			url_db_insert_url(db, "/testdir/com.ubuntu.dialer_dialer_1234.url-dispatcher", "tel", NULL);

			url_db_set_file_motification_time(db, "/testdir/magnet-test.url-dispatcher", &timestamp);
			url_db_insert_url(db, "/testdir/magnet-test.url-dispatcher", "magnet", NULL);

			url_db_set_file_motification_time(db, "/testdir/browser.url-dispatcher", &timestamp);
			url_db_insert_url(db, "/testdir/browser.url-dispatcher", "http", NULL);

			url_db_set_file_motification_time(db, "/testdir/webapp.url-dispatcher", &timestamp);
			url_db_insert_url(db, "/testdir/webapp.url-dispatcher", "http", "foo.com");

			url_db_set_file_motification_time(db, "/testdir/intenter.url-dispatcher", &timestamp);
			url_db_insert_url(db, "/testdir/intenter.url-dispatcher", "intent", "my.android.package");

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
	gchar * out_appid = NULL;
	const gchar * out_url = NULL;

	/* Good sanity check */
	dispatcher_url_to_appid("application:///foo.desktop", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_STREQ("foo", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* No .desktop */
	dispatcher_url_to_appid("application:///foo", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_TRUE(NULL == ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Missing a / */
	dispatcher_url_to_appid("application://foo.desktop", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_TRUE(NULL == ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Good with hyphens */
	dispatcher_url_to_appid("application:///my-really-cool-app.desktop", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_STREQ("my-really-cool-app", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Good Click Style */
	dispatcher_url_to_appid("application:///com.test.foo_bar-app_0.3.4.desktop", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_STREQ("com.test.foo_bar-app_0.3.4", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	return;
}

TEST_F(DispatcherTest, RestrictionTest)
{
	/* NULL cases */
	EXPECT_EQ(false, dispatcher_appid_restrict("foo-bar", NULL));
	EXPECT_EQ(false, dispatcher_appid_restrict("foo-bar", ""));
	/* Legacy case, full match */
	EXPECT_EQ(false, dispatcher_appid_restrict("foo-bar", "foo-bar"));
	EXPECT_EQ(false, dispatcher_appid_restrict("foo_bar", "foo_bar"));
	EXPECT_EQ(true,  dispatcher_appid_restrict("foo_bar", "foo-bar"));
	/* Click case, match package */
	EXPECT_EQ(false, dispatcher_appid_restrict("com.test.foo_bar-app_0.3.4", "com.test.foo"));
	EXPECT_EQ(true,  dispatcher_appid_restrict("com.test.foo_bar-app_0.3.4", "com.test.bar"));
	EXPECT_EQ(true,  dispatcher_appid_restrict("com.test.foo_bar-app", "com.test.bar"));
}

TEST_F(DispatcherTest, CalendarTest)
{
	gchar * out_appid = NULL;
	const gchar * out_url = NULL;

	/* Base Calendar */
	dispatcher_url_to_appid("calendar:///?starttime=196311221830Z", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_STREQ("com.ubuntu.calendar_calendar_9.8.2343", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Two Slash, nothing else */
	dispatcher_url_to_appid("calendar://", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_STREQ("com.ubuntu.calendar_calendar_9.8.2343", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	return;
}

TEST_F(DispatcherTest, DialerTest)
{
	gchar * out_appid = NULL;
	const gchar * out_url = NULL;

	/* Base Telephone */
	EXPECT_TRUE(dispatcher_url_to_appid("tel:+442031485000", &out_appid, &out_url));
	EXPECT_STREQ("com.ubuntu.dialer_dialer_1234", out_appid);
	g_free(out_appid);

	/* Tel with bunch of commas */
	EXPECT_TRUE(dispatcher_url_to_appid("tel:911,,,1,,1,,2", &out_appid, &out_url));
	EXPECT_STREQ("com.ubuntu.dialer_dialer_1234", out_appid);
	g_free(out_appid);

	/* Telephone with slashes */
	EXPECT_TRUE(dispatcher_url_to_appid("tel:///+442031485000", &out_appid, &out_url));
	EXPECT_STREQ("com.ubuntu.dialer_dialer_1234", out_appid);
	g_free(out_appid);

	return;
}

TEST_F(DispatcherTest, MagnetTest)
{
	gchar * out_appid = NULL;
	const gchar * out_url = NULL;

	EXPECT_TRUE(dispatcher_url_to_appid("magnet:?xt=urn:ed2k:31D6CFE0D16AE931B73C59D7E0C089C0&xl=0&dn=zero_len.fil&xt=urn:bitprint:3I42H3S6NNFQ2MSVX7XZKYAYSCX5QBYJ.LWPNACQDBZRYXW3VHJVCJ64QBZNGHOHHHZWCLNQ&xt=urn:md5:D41D8CD98F00B204E9800998ECF8427E", &out_appid, &out_url));
	EXPECT_STREQ("magnet-test", out_appid);
	g_free(out_appid);

	return;
}

TEST_F(DispatcherTest, WebappTest)
{
	gchar * out_appid = NULL;
	const gchar * out_url = NULL;

	/* Browser test */
	EXPECT_TRUE(dispatcher_url_to_appid("http://ubuntu.com", &out_appid, &out_url));
	EXPECT_STREQ("browser", out_appid);
	g_free(out_appid);

	/* Webapp test */
	EXPECT_TRUE(dispatcher_url_to_appid("http://foo.com", &out_appid, &out_url));
	EXPECT_STREQ("webapp", out_appid);
	g_free(out_appid);

	EXPECT_TRUE(dispatcher_url_to_appid("http://m.foo.com", &out_appid, &out_url));
	EXPECT_STREQ("webapp", out_appid);
	g_free(out_appid);

	return;
}

TEST_F(DispatcherTest, IntentTest)
{
	gchar * out_appid = NULL;
	const gchar * out_url = NULL;

	/* Intent basic test */
	EXPECT_TRUE(dispatcher_url_to_appid("intent://foo.google.com/maps#Intent;scheme=http;package=my.android.package;end", &out_appid, &out_url));
	EXPECT_STREQ("intenter", out_appid);
	g_free(out_appid);

	/* Not our intent test */
	out_appid = NULL;
	EXPECT_FALSE(dispatcher_url_to_appid("intent://foo.google.com/maps#Intent;scheme=http;package=not.android.package;end", &out_appid, &out_url));
	EXPECT_EQ(NULL, out_appid);

	/* Ensure domain is ignored */
	out_appid = NULL;
	EXPECT_FALSE(dispatcher_url_to_appid("intent://my.android.package/maps#Intent;scheme=http;package=not.android.package;end", &out_appid, &out_url));
	EXPECT_EQ(NULL, out_appid);

	return;
}
