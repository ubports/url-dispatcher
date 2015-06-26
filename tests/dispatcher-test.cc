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

#include "test-config.h"

#include <gio/gio.h>
#include <gtest/gtest.h>
#include "dispatcher.h"
#include "ubuntu-app-launch-mock.h"
#include "overlay-tracker-mock.h"
#include "url-db.h"
#include "scope-mock.h"

class DispatcherTest : public ::testing::Test
{
	private:
		GTestDBus * testbus = nullptr;
		GMainLoop * mainloop = nullptr;
		gchar * cachedir = nullptr;

	protected:
		OverlayTrackerMock tracker;
		RuntimeMock scope_runtime;
		GDBusConnection * session = nullptr;

		virtual void SetUp() {
			g_setenv("TEST_CLICK_DB", "click-db", TRUE);
			g_setenv("TEST_CLICK_USER", "test-user", TRUE);
			g_setenv("URL_DISPATCHER_OVERLAY_DIR", OVERLAY_TEST_DIR, TRUE);

			cachedir = g_build_filename(CMAKE_BINARY_DIR, "dispatcher-test-cache", nullptr);
			g_setenv("URL_DISPATCHER_CACHE_DIR", cachedir, TRUE);

			sqlite3 * db = url_db_create_database();
			GTimeVal timestamp;
			timestamp.tv_sec = 12345;
			timestamp.tv_usec = 0;

			url_db_set_file_motification_time(db, "/testdir/com.ubuntu.calendar_calendar_9.8.2343.url-dispatcher", &timestamp);
			url_db_insert_url(db, "/testdir/com.ubuntu.calendar_calendar_9.8.2343.url-dispatcher", "calendar", nullptr);

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

			url_db_set_file_motification_time(db, "/testdir/scoper.url-dispatcher", &timestamp);
			url_db_insert_url(db, "/testdir/scoper.url-dispatcher", "scope", nullptr);

			sqlite3_close(db);

			testbus = g_test_dbus_new(G_TEST_DBUS_NONE);
			g_test_dbus_up(testbus);

			session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

			mainloop = g_main_loop_new(nullptr, FALSE);
			dispatcher_init(mainloop, reinterpret_cast<OverlayTracker *>(&tracker), reinterpret_cast<ScopeChecker *>(&scope_runtime));

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
			scope_runtime.clearExceptions();

			/* let other threads settle */
			g_usleep(500000);

			g_clear_object(&session);

			g_test_dbus_down(testbus);
			g_object_unref(testbus);

			gchar * cmdline = g_strdup_printf("rm -rf \"%s\"", cachedir);
			g_spawn_command_line_sync(cmdline, nullptr, nullptr, nullptr, nullptr);
			g_free(cmdline);
			g_free(cachedir);
			return;
		}
};

TEST_F(DispatcherTest, ApplicationTest)
{
	gchar * out_appid = nullptr;
	const gchar * out_url = nullptr;

	/* Good sanity check */
	dispatcher_url_to_appid("application:///foo.desktop", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_STREQ("foo", ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* No .desktop */
	dispatcher_url_to_appid("application:///foo", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_TRUE(nullptr == ubuntu_app_launch_mock_get_last_app_id());
	ubuntu_app_launch_mock_clear_last_app_id();

	/* Missing a / */
	dispatcher_url_to_appid("application://foo.desktop", &out_appid, &out_url);
	dispatcher_send_to_app(out_appid, out_url);
	ASSERT_TRUE(nullptr == ubuntu_app_launch_mock_get_last_app_id());
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
	/* nullptr cases */
	EXPECT_FALSE(dispatcher_appid_restrict("foo-bar", nullptr));
	EXPECT_FALSE(dispatcher_appid_restrict("foo-bar", ""));
	/* Legacy case, full match */
	EXPECT_FALSE(dispatcher_appid_restrict("foo-bar", "foo-bar"));
	EXPECT_FALSE(dispatcher_appid_restrict("foo_bar", "foo_bar"));
	EXPECT_TRUE(dispatcher_appid_restrict("foo_bar", "foo-bar"));
	/* Click case, match package */
	EXPECT_FALSE(dispatcher_appid_restrict("com.test.foo_bar-app_0.3.4", "com.test.foo"));
	EXPECT_TRUE(dispatcher_appid_restrict("com.test.foo_bar-app_0.3.4", "com.test.bar"));
	EXPECT_TRUE(dispatcher_appid_restrict("com.test.foo_bar-app", "com.test.bar"));
}

TEST_F(DispatcherTest, CalendarTest)
{
	gchar * out_appid = nullptr;
	const gchar * out_url = nullptr;

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
	gchar * out_appid = nullptr;
	const gchar * out_url = nullptr;

	/* Intent basic test */
	EXPECT_TRUE(dispatcher_url_to_appid("intent://foo.google.com/maps#Intent;scheme=http;package=my.android.package;end", &out_appid, &out_url));
	EXPECT_STREQ("intenter", out_appid);
	g_free(out_appid);

	/* Not our intent test */
	out_appid = nullptr;
	EXPECT_FALSE(dispatcher_url_to_appid("intent://foo.google.com/maps#Intent;scheme=http;package=not.android.package;end", &out_appid, &out_url));
	EXPECT_EQ(nullptr, out_appid);

	/* Ensure domain is ignored */
	out_appid = nullptr;
	EXPECT_FALSE(dispatcher_url_to_appid("intent://my.android.package/maps#Intent;scheme=http;package=not.android.package;end", &out_appid, &out_url));
	EXPECT_EQ(nullptr, out_appid);

	return;
}

TEST_F(DispatcherTest, OverlayTest)
{
	EXPECT_TRUE(dispatcher_is_overlay("com.test.good_application_1.2.3"));
	EXPECT_FALSE(dispatcher_is_overlay("com.test.bad_application_1.2.3"));

	EXPECT_TRUE(dispatcher_send_to_overlay ("com.test.good_application_1.2.3", "overlay://ubuntu.com", session, g_dbus_connection_get_unique_name(session)));

	ASSERT_EQ(1, tracker.addedOverlays.size());
	EXPECT_EQ("com.test.good_application_1.2.3", std::get<0>(tracker.addedOverlays[0]));
	EXPECT_EQ(getpid(), std::get<1>(tracker.addedOverlays[0]));
	EXPECT_EQ("overlay://ubuntu.com", std::get<2>(tracker.addedOverlays[0]));

	return;
}

TEST_F(DispatcherTest, ScopeTest)
{
	gchar * out_appid = nullptr;

	unity::scopes::NotFoundException scopeException("test", "badscope");
	scope_runtime.addException("badscope.scopemaster_badscope", scopeException);
	std::invalid_argument invalidException("confused");
	scope_runtime.addException("confusedscope.scopemaster_confusedscope", invalidException);

	/* Good sanity check */
	dispatcher_url_to_appid("scope://simplescope.scopemaster_simplescope", &out_appid, nullptr);
	EXPECT_STREQ("scoper", out_appid);
	g_free(out_appid);

	/* Bad scope */
	dispatcher_url_to_appid("scope://badscope.scopemaster_badscope", &out_appid, nullptr);
	EXPECT_STRNE("scoper", out_appid);
	g_free(out_appid);

	/* Confused scope */
	dispatcher_url_to_appid("scope://confusedscope.scopemaster_confusedscope", &out_appid, nullptr);
	EXPECT_STRNE("scoper", out_appid);
	g_free(out_appid);
}
