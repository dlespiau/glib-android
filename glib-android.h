#ifndef __GLIB_ANDROID_H__
#define __GLIB_ANDROID_H__

#include <glib.h>

typedef enum
{
  G_ANDROID_INIT_FLAG_NONE,
  G_ANDROID_INIT_FLAG_LOG_HANDLER
} GAndroidInitFlags;


gboolean        g_android_init          (int                 *argc,
                                         gchar             ***arvg,
                                         GAndroidInitFlags    flags);

#endif /* __GLIB_ANDROID_H__ */
