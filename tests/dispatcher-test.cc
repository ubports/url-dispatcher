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
#include "upstart-app-launch-mock.h"

class DispatcherTest : public ::testing::Test
{
	private:
		GTestDBus * testbus = NULL;
		GMainLoop * mainloop = NULL;

	protected:
		virtual void SetUp() {
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

			upstart_app_launch_mock_clear_last_app_id();

			g_test_dbus_down(testbus);
			g_object_unref(testbus);
			return;
		}
};

TEST_F(DispatcherTest, ApplicationTest)
{
	/* Good sanity check */
	dispatch_url("application:///foo.desktop");
	ASSERT_STREQ("foo", upstart_app_launch_mock_get_last_app_id());
	upstart_app_launch_mock_clear_last_app_id();

	/* No .desktop */
	dispatch_url("application:///foo");
	ASSERT_TRUE(NULL == upstart_app_launch_mock_get_last_app_id());
	upstart_app_launch_mock_clear_last_app_id();

	/* Missing a / */
	dispatch_url("application://foo.desktop");
	ASSERT_TRUE(NULL == upstart_app_launch_mock_get_last_app_id());
	upstart_app_launch_mock_clear_last_app_id();

	/* Good with hyphens */
	dispatch_url("application:///my-really-cool-app.desktop");
	ASSERT_STREQ("my-really-cool-app", upstart_app_launch_mock_get_last_app_id());
	upstart_app_launch_mock_clear_last_app_id();

	/* Good Click Style */
	dispatch_url("application:///com.test.foo_bar-app_0.3.4.desktop");
	ASSERT_STREQ("com.test.foo_bar-app_0.3.4", upstart_app_launch_mock_get_last_app_id());
	upstart_app_launch_mock_clear_last_app_id();

	return;
}
