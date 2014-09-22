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

class LibTest : public ::testing::Test
{
	protected:
		DbusTestService * service = NULL;
		DbusTestDbusMock * mock = NULL;
		DbusTestDbusMockObject * obj = NULL;
		GDBusConnection * bus = NULL;

		virtual void SetUp() {
			service = dbus_test_service_new(NULL);

			mock = dbus_test_dbus_mock_new("com.canonical.URLDispatcher");
			obj = dbus_test_dbus_mock_get_object(mock, "/com/canonical/URLDispatcher", "com.canonical.URLDispatcher", NULL);

			dbus_test_dbus_mock_object_add_method(mock, obj,
				"DispatchURL",
				G_VARIANT_TYPE("(ss)"),
				NULL, /* out */
				"", /* python */
				NULL); /* error */

			dbus_test_dbus_mock_object_add_method(mock, obj,
				"TestURL",
				G_VARIANT_TYPE("as"),
				G_VARIANT_TYPE("as"),
				"ret = ['appid']", /* python */
				NULL); /* error */

			dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));
			dbus_test_service_start_tasks(service);

			bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
			g_dbus_connection_set_exit_on_close(bus, FALSE);
			g_object_add_weak_pointer(G_OBJECT(bus), (gpointer *)&bus);
			return;
		}

		virtual void TearDown() {
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

TEST_F(LibTest, BaseTest) {
	GMainLoop * main = g_main_loop_new(NULL, FALSE);

	url_dispatch_send("foo://bar/barish", simple_cb, main);

	/* Give it some time to send and reply */
	g_main_loop_run(main);
	g_main_loop_unref(main);

	guint callslen = 0;
	const DbusTestDbusMockCall * calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, "DispatchURL", &callslen, NULL);

	// ASSERT_NE(calls, nullptr);
	ASSERT_EQ(callslen, 1);
	GVariant * check = g_variant_new_parsed("('foo://bar/barish', '')");
	g_variant_ref_sink(check);
	ASSERT_TRUE(g_variant_equal(calls->params, check));
	g_variant_unref(check);
}

TEST_F(LibTest, NoMain) {
	/* Spawning a non-main caller */
	g_spawn_command_line_sync(LIB_TEST_NO_MAIN_HELPER, NULL, NULL, NULL, NULL);

	guint callslen = 0;
	const DbusTestDbusMockCall * calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, "DispatchURL", &callslen, NULL);

	// ASSERT_NE(calls, nullptr);
	ASSERT_EQ(callslen, 1);
	GVariant * check = g_variant_new_parsed("('foo://bar/barish', '')");
	g_variant_ref_sink(check);
	ASSERT_TRUE(g_variant_equal(calls->params, check));
	g_variant_unref(check);
}

TEST_F(LibTest, RestrictedTest) {
	GMainLoop * main = g_main_loop_new(NULL, FALSE);

	url_dispatch_send_restricted("foo://bar/barish", "bar-package", simple_cb, main);

	/* Give it some time to send and reply */
	g_main_loop_run(main);
	g_main_loop_unref(main);

	guint callslen = 0;
	const DbusTestDbusMockCall * calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, "DispatchURL", &callslen, NULL);

	// ASSERT_NE(calls, nullptr);
	ASSERT_EQ(callslen, 1);
	GVariant * check = g_variant_new_parsed("('foo://bar/barish', 'bar-package')");
	g_variant_ref_sink(check);
	ASSERT_TRUE(g_variant_equal(calls->params, check));
	g_variant_unref(check);
}

TEST_F(LibTest, TestTest) {
	const gchar * urls[2] = {
		"foo://bar/barish",
		NULL
	};

	gchar ** appids = url_dispatch_url_appid(urls);

	EXPECT_EQ(1, g_strv_length(appids));
	EXPECT_STREQ("appid", appids[0]);
	g_strfreev(appids);

	guint callslen = 0;
	const DbusTestDbusMockCall * calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, "TestURL", &callslen, NULL);

	// ASSERT_NE(calls, nullptr);
	ASSERT_EQ(callslen, 1);
	GVariant * check = g_variant_new_parsed("(['foo://bar/barish'],)");
	g_variant_ref_sink(check);
	ASSERT_TRUE(g_variant_equal(calls->params, check));
	g_variant_unref(check);
}
