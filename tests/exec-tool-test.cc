/**
 * Copyright © 2013-2015 Canonical, Ltd.
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
#include <libdbustest/dbus-test.h>

class ExecToolTest : public ::testing::Test
{
	protected:
		DbusTestService * service = nullptr;
		DbusTestDbusMock * mock = nullptr;
		DbusTestDbusMockObject * obj = nullptr;
		GDBusConnection * bus = nullptr;

		virtual void SetUp() {
			g_setenv("URL_DISPATCHER_DISABLE_RECOVERABLE_ERROR", "1", TRUE);
			g_setenv("URL_DISPATCHER_OVERLAY_DIR", OVERLAY_TEST_DIR, TRUE);

			service = dbus_test_service_new(nullptr);

			/* Upstart Mock */
			mock = dbus_test_dbus_mock_new("com.ubuntu.Upstart");
			obj = dbus_test_dbus_mock_get_object(mock, "/com/ubuntu/Upstart", "com.ubuntu.Upstart0_6", nullptr);

			dbus_test_dbus_mock_object_add_method(mock, obj,
				"SetEnv",
				G_VARIANT_TYPE("(assb)"),
				NULL,
				"",
				NULL);

			dbus_test_task_set_name(DBUS_TEST_TASK(mock), "Upstart");
			dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));

			/* Start your engines! */
			dbus_test_service_start_tasks(service);

			bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
			g_dbus_connection_set_exit_on_close(bus, FALSE);
			g_object_add_weak_pointer(G_OBJECT(bus), (gpointer *)&bus);
			return;
		}

		virtual void TearDown() {
			/* dbustest should probably do this, not sure */

			g_clear_object(&mock);
			g_clear_object(&service);

			g_object_unref(bus);

			unsigned int cleartry = 0;
			while (bus != nullptr && cleartry < 100) {
				pause(100);
				cleartry++;
			}

			return;
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

TEST_F(ExecToolTest, SetOverlay)
{
	g_unsetenv("APP_ID");
	gint retval = 0;
	EXPECT_TRUE(g_spawn_command_line_sync(EXEC_TOOL, nullptr, nullptr, &retval, nullptr));
	EXPECT_NE(0, retval);

	g_setenv("APP_ID", "mock-overlay", TRUE);
	g_setenv("UPSTART_JOB", "fubar", TRUE);
	EXPECT_TRUE(g_spawn_command_line_sync(EXEC_TOOL, nullptr, nullptr, &retval, nullptr));
	EXPECT_EQ(0, retval);

	guint len = 0;
	const DbusTestDbusMockCall * calls = dbus_test_dbus_mock_object_get_method_calls(mock, obj, "SetEnv", &len, NULL);
	ASSERT_NE(nullptr, calls);
	ASSERT_EQ(1, len);

	GVariant * appexecenv = g_variant_get_child_value(calls[0].params, 1);
	EXPECT_STREQ("APP_EXEC=overlay", g_variant_get_string(appexecenv, nullptr));
	g_variant_unref(appexecenv);
}
