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

#include <glib.h>
#include <sqlite3.h>

#ifndef __URL_DB_H__
#define __URL_DB_H__ 1

G_BEGIN_DECLS

sqlite3 *     url_db_create_database                (void);
gboolean      url_db_get_file_motification_time     (sqlite3 *      db,
                                                     const gchar *  filename,
                                                     GTimeVal *     timeval);
gboolean      url_db_set_file_motification_time     (sqlite3 *      db,
                                                     const gchar *  filename,
                                                     GTimeVal *     timeval);
gboolean      url_db_insert_url                     (sqlite3 *      db,
                                                     const gchar *  filename,
                                                     const gchar *  protocol,
                                                     const gchar *  domainsuffix);

G_END_DECLS

#endif /* __URL_DB_H__ */
