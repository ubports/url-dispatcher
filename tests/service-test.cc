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
#include <liburl-dispatcher/url-dispatcher.h>
#include <libdbustest/dbus-test.h>

#include "url-db.h"
#include "systemd-mock.h"

#define CGROUP_DIR (CMAKE_BINARY_DIR "/systemd-service-test-cgroups")

class ServiceTest : public ::testing::Test
{
	protected:
		DbusTestService * service = nullptr;
		DbusTestDbusMock * dashmock = nullptr;
		DbusTestProcess * dispatcher = nullptr;
		std::shared_ptr<SystemdMock> systemd;
		GDBusConnection * bus = nullptr;

		virtual void SetUp() {
			g_setenv("UBUNTU_APP_LAUNCH_USE_SESSION", "1", TRUE);
			g_setenv("UBUNTU_APP_LAUNCH_SYSTEMD_CGROUP_ROOT", CGROUP_DIR, TRUE);
			g_setenv("UBUNTU_APP_LAUNCH_SYSTEMD_PATH", "/this/should/not/exist", TRUE);

			g_setenv("URL_DISPATCHER_DISABLE_RECOVERABLE_ERROR", "1", TRUE);
			g_setenv("URL_DISPATCHER_DISABLE_DIRECTORY_UPDATES", "1", TRUE);
			g_setenv("XDG_DATA_DIRS", XDG_DATA_DIRS, TRUE);
			g_setenv("LD_PRELOAD", MIR_MOCK_PATH, TRUE);

			SetUpDb();

			service = dbus_test_service_new(nullptr);

			dispatcher = dbus_test_process_new(URL_DISPATCHER_SERVICE);
			dbus_test_task_set_name(DBUS_TEST_TASK(dispatcher), "Dispatcher");
			dbus_test_service_add_task(service, DBUS_TEST_TASK(dispatcher));

			/* Systemd Mock */
			systemd = std::make_shared<SystemdMock>(
					std::list<SystemdMock::Instance>{
						{"application-legacy", "single", {}, getpid(), {getpid()}},
					}, CGROUP_DIR);
			dbus_test_service_add_task(service, *systemd);

			/* Dash Mock */
			dashmock = dbus_test_dbus_mock_new("com.canonical.UnityDash");

			DbusTestDbusMockObject * fdoobj = dbus_test_dbus_mock_get_object(dashmock, "/unity8_2ddash", "org.freedesktop.Application", nullptr);
			dbus_test_dbus_mock_object_add_method(dashmock, fdoobj,
												  "Open",
												  G_VARIANT_TYPE("(asa{sv})"),
												  nullptr, /* return */
												  "", /* python */
												  nullptr); /* error */
			dbus_test_task_set_name(DBUS_TEST_TASK(dashmock), "UnityDash");
			dbus_test_service_add_task(service, DBUS_TEST_TASK(dashmock));

			/* Start your engines! */
			dbus_test_service_start_tasks(service);

			bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
			g_dbus_connection_set_exit_on_close(bus, FALSE);
			g_object_add_weak_pointer(G_OBJECT(bus), (gpointer *)&bus);
			return;
		}

		virtual void TearDown() {
			kill(dbus_test_process_get_pid(dispatcher), SIGTERM);

			systemd.reset();
			g_clear_object(&dispatcher);
			g_clear_object(&dashmock);
			g_clear_object(&service);

			g_object_unref(bus);

			unsigned int cleartry = 0;
			while (bus != nullptr && cleartry < 100) {
				pause(100);
				cleartry++;
			}

			TearDownDb();
			return;
		}

		void SetUpDb () {
			const gchar * cachedir = CMAKE_BINARY_DIR "/service-test-cache";
			ASSERT_EQ(0, g_mkdir_with_parents(cachedir, 0700));

			g_setenv("XDG_CACHE_HOME", cachedir, TRUE);

			sqlite3 * db = url_db_create_database();

			GTimeVal time = {0, 0};
			time.tv_sec = 5;
			url_db_set_file_motification_time(db, "/unity8-dash.url-dispatcher", &time);
			url_db_insert_url(db, "/unity8-dash.url-dispatcher", "thingish", nullptr);
			sqlite3_close(db);
		}

		void TearDownDb () {
			g_spawn_command_line_sync("rm -rf " CMAKE_BINARY_DIR "/service-test-cache", nullptr, nullptr, nullptr, nullptr);
		}
		
		static gboolean quit_loop (gpointer ploop) {
			g_main_loop_quit((GMainLoop *)ploop);
			return FALSE;
		}

		void pause (int time) {
			GMainLoop * loop = g_main_loop_new(nullptr, FALSE);
			g_timeout_add(time, quit_loop, loop);
			g_main_loop_run(loop);
			g_main_loop_unref(loop);
		}
};

static void
simple_cb (const gchar * /*url*/, gboolean /*success*/, gpointer user_data)
{
	g_main_loop_quit(static_cast<GMainLoop *>(user_data));
}

TEST_F(ServiceTest, InvalidTest) {
	GMainLoop * main = g_main_loop_new(nullptr, FALSE);

	/* Send an invalid URL */
	url_dispatch_send("foo://bar/barish", simple_cb, main);

	/* Give it some time to send and reply */
	g_main_loop_run(main);
	g_main_loop_unref(main);

	auto calls = systemd->unitCalls();
	ASSERT_EQ(0u, calls.size());
}

TEST_F(ServiceTest, ApplicationTest) {
	GMainLoop * main = g_main_loop_new(nullptr, FALSE);

	/* Send an invalid URL */
	url_dispatch_send("application:///foo-bar.desktop", simple_cb, main);

	/* Give it some time to send and reply */
	g_main_loop_run(main);
	g_main_loop_unref(main);

	/* Check to see it called systemd */
	auto calls = systemd->unitCalls();
	ASSERT_EQ(1u, calls.size());
	EXPECT_EQ(SystemdMock::instanceName({"application-legacy", "foo-bar", "", 0, {}}), calls.begin()->name);
}

TEST_F(ServiceTest, TestURLTest) {
	/* Simple */
	const char * testurls[] = {
		"application:///foo-bar.desktop",
		nullptr
	};

	gchar ** appids = url_dispatch_url_appid(testurls);
	ASSERT_EQ(1u, g_strv_length(appids));

	EXPECT_STREQ("foo-bar", appids[0]);

	g_strfreev(appids);

	/* Multiple */
	const char * multiurls[] = {
		"application:///bar-foo.desktop",
		"application:///foo-bar.desktop",
		nullptr
	};

	gchar ** multiappids = url_dispatch_url_appid(multiurls);
	ASSERT_EQ(2u, g_strv_length(multiappids));

	EXPECT_STREQ("bar-foo", multiappids[0]);
	EXPECT_STREQ("foo-bar", multiappids[1]);

	g_strfreev(multiappids);

	/* Error URL */
	const char * errorurls[] = {
		"foo://bar/no/url",
		nullptr
	};

	gchar ** errorappids = url_dispatch_url_appid(errorurls);
	ASSERT_EQ(0u, g_strv_length(errorappids));

	g_strfreev(errorappids);
}

void
focus_signal_cb (GDBusConnection * /*connection*/, const gchar * /*sender_name*/, const gchar * /*object_path*/, const gchar * /*interface_name*/, const gchar * /*signal_name*/, GVariant * /*parameters*/, gpointer user_data)
{
	guint * focus_count = (guint *)user_data;
	*focus_count = *focus_count + 1;
	g_debug("Focus signaled: %d", *focus_count);
}

TEST_F(ServiceTest, Unity8DashTest) {
	guint focus_count = 0;
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);

	guint focus_signal = g_dbus_connection_signal_subscribe(bus,
	                                                        nullptr, /* sender */
	                                                        "com.canonical.UbuntuAppLaunch",
	                                                        "UnityFocusRequest",
	                                                        "/",
	                                                        "unity8-dash", /* arg0 */
	                                                        G_DBUS_SIGNAL_FLAGS_NONE,
	                                                        focus_signal_cb,
	                                                        &focus_count,
	                                                        nullptr); /* destroy func */


	DbusTestDbusMockObject * fdoobj = dbus_test_dbus_mock_get_object(dashmock, "/unity8_2ddash", "org.freedesktop.Application", nullptr);
	GMainLoop * main = g_main_loop_new(nullptr, FALSE);

	/* Send an invalid URL */
	url_dispatch_send("thingish://foo-bar", simple_cb, main);

	/* Give it some time to send and reply */
	g_main_loop_run(main);
	g_main_loop_unref(main);

	guint callslen = 0;
	auto calls = dbus_test_dbus_mock_object_get_method_calls(dashmock, fdoobj, "Open", &callslen, nullptr);

	EXPECT_EQ(1u, callslen);
	EXPECT_TRUE(g_variant_equal(calls[0].params, g_variant_new_parsed("(['thingish://foo-bar'], @a{sv} {})")));

	EXPECT_EQ(1u, focus_count);

	g_dbus_connection_signal_unsubscribe(bus, focus_signal);
	g_clear_object(&bus);
}
