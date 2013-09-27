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

#include <glib.h>

#ifndef __UPSTART_APP_LAUNCH_MOCK__
#define __UPSTART_APP_LAUNCH_MOCK__ 1

G_BEGIN_DECLS

void upstart_app_launch_mock_clear_last_app_id (void);
gchar * upstart_app_launch_mock_get_last_app_id (void);

G_END_DECLS

#endif /* __UPSTART_APP_LAUNCH_MOCK__ */
