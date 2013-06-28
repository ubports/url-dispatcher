
#include <glib.h>

#ifndef __URL_DISPATCH_H__
#define __URL_DISPATCH_H__ 1

#pragma GCC visibility push(default)

G_BEGIN_DECLS

typedef void (*URLDispatchCallback) (const gchar * url, gboolean success, gpointer user_data);

void   url_dispatch_send    (const gchar *         url,
                             URLDispatchCallback   cb,
                             gpointer              user_data);

G_END_DECLS

#pragma GCC visibility pop

#endif /* __URL_DISPATCH_H__ */
