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
 * Author: Ted Gould <ted@canonical.com>
 *
 */

#ifndef URL_DB_H
#define URL_DB_H 1

#include <glib.h>
#include <sqlite3.h>

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
gchar *       url_db_find_url                       (sqlite3 *      db,
                                                     const gchar *  protocol,
                                                     const gchar *  domainsuffix);
GList *       url_db_files_for_dir                  (sqlite3 *      db,
                                                     const gchar *  dir);
gboolean      url_db_remove_file                    (sqlite3 *      db,
                                                     const gchar *  path);

G_END_DECLS

#endif /* URL_DB_H */
