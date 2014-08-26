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
#include <liburl-dispatcher/url-dispatcher.h>
#include <libdbustest/dbus-test.h>

class ServiceTest : public ::testing::Test
{
	protected:
		DbusTestService * service = NULL;
		DbusTestDbusMock * mock = NULL;
		DbusTestDbusMockObject * obj = NULL;
		DbusTestDbusMockObject * jobobj = NULL;
		DbusTestProcess * dispatcher = NULL;
		GDBusConnection * bus = NULL;

		virtual void SetUp() {
			g_setenv("UBUNTU_APP_LAUNCH_USE_SESSION", "1", TRUE);
			g_setenv("URL_DISPATCHER_DISABLE_RECOVERABLE_ERROR", "1", TRUE);

			service = dbus_test_service_new(NULL);

			dispatcher = dbus_test_process_new(URL_DISPATCHER_SERVICE);
			dbus_test_task_set_name(DBUS_TEST_TASK(dispatcher), "Dispatcher");
			dbus_test_service_add_task(service, DBUS_TEST_TASK(dispatcher));

			mock = dbus_test_dbus_mock_new("com.ubuntu.Upstart");
			obj = dbus_test_dbus_mock_get_object(mock, "/com/ubuntu/Upstart", "com.ubuntu.Upstart0_6", NULL);

			dbus_test_dbus_mock_object_add_method(mock, obj,
				"GetJobByName",
				G_VARIANT_TYPE_STRING,
				G_VARIANT_TYPE_OBJECT_PATH, /* out */
				"ret = dbus.ObjectPath('/job')", /* python */
				NULL); /* error */

			jobobj = dbus_test_dbus_mock_get_object(mock, "/job", "com.ubuntu.Upstart0_6.Job", NULL);

			dbus_test_dbus_mock_object_add_method(mock, jobobj,
				"Start",
				G_VARIANT_TYPE("(asb)"),
				G_VARIANT_TYPE_OBJECT_PATH, /* out */
				"ret = dbus.ObjectPath('/instance')", /* python */
				NULL); /* error */

			dbus_test_task_set_name(DBUS_TEST_TASK(mock), "Upstart");
			dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
			dbus_test_service_start_tasks(service);

			bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
			g_dbus_connection_set_exit_on_close(bus, FALSE);
			g_object_add_weak_pointer(G_OBJECT(bus), (gpointer *)&bus);
			return;
		}

		virtual void TearDown() {
			/* dbustest should probably do this, not sure */
			kill(dbus_test_process_get_pid(dispatcher), SIGTERM);
			g_usleep(50000);

			g_clear_object(&dispatcher);
			g_clear_object(&mock);
			g_clear_object(&service);

			g_object_unref(bus);

			unsigned int cleartry = 0;
			while (bus != NULL && cleartry < 100) {
				g_usleep(100000);
				while (g_main_pending())
					g_main_iteration(TRUE);
				cleartry++;
			}
			return;
		}
};

static void
simple_cb (const gchar * url, gboolean success, gpointer user_data)
{
	g_main_loop_quit(static_cast<GMainLoop *>(user_data));
}

TEST_F(ServiceTest, InvalidTest) {
	GMainLoop * main = g_main_loop_new(NULL, FALSE);

	/* Send an invalid URL */
	url_dispatch_send("foo://bar/barish", simple_cb, main);

	/* Give it some time to send and reply */
	g_main_loop_run(main);
	g_main_loop_unref(main);

	guint callslen = 0;
	const DbusTestDbusMockCall * calls = dbus_test_dbus_mock_object_get_method_calls(mock, jobobj, "Start", &callslen, NULL);

	ASSERT_EQ(callslen, 0);
}

TEST_F(ServiceTest, ApplicationTest) {
	GMainLoop * main = g_main_loop_new(NULL, FALSE);

	/* Send an invalid URL */
	url_dispatch_send("application:///foo-bar.desktop", simple_cb, main);

	/* Give it some time to send and reply */
	g_main_loop_run(main);
	g_main_loop_unref(main);

	guint callslen = 0;
	const DbusTestDbusMockCall * calls = dbus_test_dbus_mock_object_get_method_calls(mock, jobobj, "Start", &callslen, NULL);

	ASSERT_EQ(callslen, 1);

	/* Making sure the APP_ID is here.  We're not testing more to
	   make it so the tests break less, that should be tested in
	   Upstart App Launch, we don't need to retest */
	GVariant * env = g_variant_get_child_value(calls->params, 0);
	GVariantIter iter;
	bool found_appid = false;
	g_variant_iter_init(&iter, env);
	gchar * var = NULL;

	while (g_variant_iter_loop(&iter, "s", &var)) {
		if (g_strcmp0(var, "APP_ID=foo-bar") == 0) {
			ASSERT_FALSE(found_appid);
			found_appid = true;
		}
	}

	g_variant_unref(env);

	ASSERT_TRUE(found_appid);
}

TEST_F(ServiceTest, TestURLTest) {
	/* Simple */
	const char * testurls[] = {
		"application:///foo-bar.desktop",
		NULL
	};

	gchar ** appids = url_dispatch_url_appid(testurls);
	ASSERT_EQ(1, g_strv_length(appids));

	EXPECT_STREQ("foo-bar", appids[0]);

	g_strfreev(appids);

	/* Multiple */
	const char * multiurls[] = {
		"application:///bar-foo.desktop",
		"application:///foo-bar.desktop",
		NULL
	};

	gchar ** multiappids = url_dispatch_url_appid(multiurls);
	ASSERT_EQ(2, g_strv_length(multiappids));

	EXPECT_STREQ("bar-foo", multiappids[0]);
	EXPECT_STREQ("foo-bar", multiappids[1]);

	g_strfreev(multiappids);

	/* Error URL */
	const char * errorurls[] = {
		"foo://bar/no/url",
		NULL
	};

	gchar ** errorappids = url_dispatch_url_appid(errorurls);
	ASSERT_EQ(0, g_strv_length(errorappids));

	g_strfreev(errorappids);
}

TEST_F(ServiceTest, Unity8DashTest) {
	DbusTestDbusMock * dashmock = dbus_test_dbus_mock_new("com.canonical.UnityDash");
	DbusTestDbusMockObject * fdoobj = dbus_test_dbus_mock_get_object(mock, "/unity8_2ddash", "org.freedesktop.Application", NULL);

	dbus_test_dbus_mock_object_add_method(dashmock, fdoobj,
	                                      "Open",
	                                      G_VARIANT_TYPE("(asa{sv})"),
	                                      NULL, /* return */
	                                      "", /* python */
	                                      NULL); /* error */

	dbus_test_task_set_name(DBUS_TEST_TASK(dashmock), "UnityDash");
	dbus_test_service_add_task(service, DBUS_TEST_TASK(dashmock));


	g_object_unref(dashmock);
}
