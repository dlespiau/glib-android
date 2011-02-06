
#include <android/log.h>

#include "glib-android.h"

static android_LogPriority
g_log_to_android_log (GLogLevelFlags flags)
{
  guint i;
  static long levels[8] =
    {
      0,                      /* (unused) ANDROID_LOG_UNKNOWN */
      0,                      /* (unused) ANDROID_LOG_DEFAULT */
      G_LOG_LEVEL_MESSAGE,    /* ANDROID_LOG_VERBOSE */
      G_LOG_LEVEL_DEBUG,      /* ANDROID_LOG_DEBUG */
      G_LOG_LEVEL_INFO,       /* ANDROID_LOG_INFO */
      G_LOG_LEVEL_WARNING,    /* ANDROID_LOG_WARN */
      G_LOG_LEVEL_CRITICAL,   /* ANDROID_LOG_ERROR */
      G_LOG_LEVEL_ERROR       /* ANDROID_LOG_FATAL */
    };

  for (i = 3; i < G_N_ELEMENTS (levels); i++)
    if (flags & levels [i])
      break;

  if (i == G_N_ELEMENTS (levels))
    return ANDROID_LOG_INFO;

  return i;
}

static void
g_android_log_handler (const gchar    *log_domain,
                       GLogLevelFlags  log_level,
                       const gchar    *message,
                       gpointer        user_data)
{
  gboolean is_fatal = (log_level & G_LOG_FLAG_FATAL);
  android_LogPriority android_level;

  if (is_fatal)
    android_level = ANDROID_LOG_FATAL;
  else
    android_level = g_log_to_android_log (log_level);

  __android_log_write (android_level, log_domain, message);
}

gboolean
g_android_init (int                 *argc,
                gchar             ***arvg,
                GAndroidInitFlags    flags)
{
  if (flags & G_ANDROID_INIT_FLAG_LOG_HANDLER)
    g_log_set_default_handler (g_android_log_handler, NULL);

  return TRUE;
}
