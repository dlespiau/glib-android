/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2011 Damien Lespiau
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/*
 * This file is derived from the "native-activity" sample of the android NDK
 * r5b. The coding style has been adapted to the code style most commonly found
 * in glib/gobject based projects.
 */

#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <glib.h>
#include <glib-android/glib-android.h>

/**
 * Shared state for our app.
 */
struct engine
{
  struct android_app* app;

  int animating;
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
  int32_t width;
  int32_t height;
};

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display (struct engine* engine)
{
  /*
   * Here specify the attributes of the desired configuration.
   * Below, we select an EGLConfig with at least 8 bits per color
   * component compatible with on-screen windows
   */
  const EGLint attribs[] =
  {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_NONE
  };
  EGLint w, h, format;
  EGLint numConfigs;
  EGLConfig config;
  EGLSurface surface;
  EGLContext context;

  EGLDisplay display = eglGetDisplay (EGL_DEFAULT_DISPLAY);

  eglInitialize (display, 0, 0);

  /* Here, the application chooses the configuration it desires. In this
   * sample, we have a very simplified selection process, where we pick
   * the first EGLConfig that matches our criteria */
  eglChooseConfig (display, attribs, &config, 1, &numConfigs);

  /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
   * guaranteed to be accepted by ANativeWindow_setBuffersGeometry ().
   * As soon as we picked a EGLConfig, we can safely reconfigure the
   * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
  eglGetConfigAttrib (display, config, EGL_NATIVE_VISUAL_ID, &format);

  ANativeWindow_setBuffersGeometry (engine->app->window, 0, 0, format);

  surface = eglCreateWindowSurface (display, config, engine->app->window, NULL);
  context = eglCreateContext (display, config, NULL, NULL);

  if (eglMakeCurrent (display, surface, surface, context) == EGL_FALSE)
    {
      g_warning ("Unable to eglMakeCurrent");
      return -1;
    }

  eglQuerySurface (display, surface, EGL_WIDTH, &w);
  eglQuerySurface (display, surface, EGL_HEIGHT, &h);

  engine->display = display;
  engine->context = context;
  engine->surface = surface;
  engine->width = w;
  engine->height = h;

  // Initialize GL state.
  glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glEnable (GL_CULL_FACE);
  glShadeModel (GL_SMOOTH);
  glDisable (GL_DEPTH_TEST);

  return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame (struct engine* engine)
{
  if (engine->display == NULL)
    {
      // No display.
      return;
    }

  // Just fill the screen with a color.
  glClearColor (0.5, 0, 0.5, 1);
  glClear (GL_COLOR_BUFFER_BIT);

  eglSwapBuffers (engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display (struct engine* engine)
{
  if (engine->display != EGL_NO_DISPLAY)
    {
      eglMakeCurrent (engine->display,
                      EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

      if (engine->context != EGL_NO_CONTEXT)
        eglDestroyContext (engine->display, engine->context);

      if (engine->surface != EGL_NO_SURFACE)
        eglDestroySurface (engine->display, engine->surface);

      eglTerminate (engine->display);
    }
  engine->display = EGL_NO_DISPLAY;
  engine->context = EGL_NO_CONTEXT;
  engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next main command.
 */
static void
engine_handle_cmd (struct android_app* app,
                   int32_t             cmd)
{
  struct engine* engine = (struct engine*)app->userData;

  switch (cmd)
    {
  case APP_CMD_SAVE_STATE:
    // The system has asked us to save our current state.  Do so.
    g_message ("command: SAVE_STATE");
    break;

  case APP_CMD_INIT_WINDOW:
    // The window is being shown, get it ready.
    g_message ("command: INIT_WINDOW");
    if (engine->app->window != NULL)
      {
        engine_init_display (engine);
        engine_draw_frame (engine);
      }
    break;

  case APP_CMD_TERM_WINDOW:
    // The window is being hidden or closed, clean it up.
    g_message ("command: TERM_WINDOW");
    engine_term_display (engine);
    break;

  case APP_CMD_GAINED_FOCUS:
    // When our app gains focus, we start monitoring the accelerometer.
    g_message ("command: GAINED_FOCUS");
    break;

  case APP_CMD_LOST_FOCUS:
    // When our app loses focus, we stop monitoring the accelerometer.
    // This is to avoid consuming battery while not being used.
    g_message ("command: LOST_FOCUS");
    engine_draw_frame (engine);
    break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void
android_main (struct android_app* state)
{
  struct engine engine;

  // Make sure glue isn't stripped.
  app_dummy ();

  g_android_init ();

  memset (&engine, 0, sizeof (engine));
  state->userData = &engine;
  state->onAppCmd = engine_handle_cmd;
  engine.app = state;

  // loop waiting for stuff to do.

  while (1)
    {
      // Read all pending events.
      int ident;
      int events;
      struct android_poll_source* source;

      while ((ident = ALooper_pollAll (-1, NULL, &events,
                                       (void**) &source)) >= 0)
        {
          // Process this event.
          if (source != NULL)
            {
              source->process (state, source);
            }

          // Check if we are exiting.
          if  (state->destroyRequested != 0)
            {
              engine_term_display (&engine);
              return;
            }
      }
    }
}
