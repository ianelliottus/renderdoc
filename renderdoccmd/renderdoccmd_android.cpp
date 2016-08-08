/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "renderdoccmd.h"
#include <locale.h>
#include <replay/renderdoc_replay.h>
#include <string.h>
#include <unistd.h>
#include <string>

#if 1
#include <android_native_app_glue.h>
#else
#include <android_native_app_glue.h>
#define ANativeActivity_onCreate __attribute__((visibility("default"))) ANativeActivity_onCreate
extern "C" {
#include <android_native_app_glue.c>
}
#endif

#include <android/log.h>
#define LOGCAT_TAG "renderdoc"

using std::string;

struct android_app *android_state;

void DisplayRendererPreview(ReplayRenderer *renderer, TextureDisplay &displayCfg, uint32_t width,
                            uint32_t height)
{
  ANativeWindow *connectionScreenWindow = android_state->window;

  ReplayOutput *out = ReplayRenderer_CreateOutput(renderer, eWindowingSystem_Android,
                                                  connectionScreenWindow, eOutputType_TexDisplay);

  OutputConfig c = {eOutputType_TexDisplay};

  ReplayOutput_SetOutputConfig(out, c);
  ReplayOutput_SetTextureDisplay(out, displayCfg);

  for(int i = 0; i < 100; i++)
  {
    ReplayRenderer_SetFrameEvent(renderer, 10000000, true);

    __android_log_print(ANDROID_LOG_INFO, LOGCAT_TAG, "Frame %i", i);
    ReplayOutput_Display(out);

    usleep(100000);
  }
}

void handle_cmd(android_app *app, int32_t cmd)
{
  switch(cmd)
  {
    case APP_CMD_INIT_WINDOW:
    {
      // The window is being shown, get it ready.

      __android_log_print(ANDROID_LOG_INFO, LOGCAT_TAG,
                          "APP_CMD_INIT_WINDOW: android_state->window: %p", android_state->window);

      const char *argv[] = {
          "renderdoccmd", "replay", "/sdcard/capture.rdc",
      };
      int argc = sizeof(argv) / sizeof(argv[0]);
      renderdoccmd(argc, (char **)argv);
      break;
    }
    case APP_CMD_TERM_WINDOW:
      // The window is being hidden or closed, clean it up.
      //	  DeleteVulkan();
      break;
    default: __android_log_print(ANDROID_LOG_INFO, LOGCAT_TAG, "event not handled: %d", cmd);
  }
}

void android_main(struct android_app *state)
{
  android_state = state;
  android_state->onAppCmd = handle_cmd;

  __android_log_print(ANDROID_LOG_INFO, LOGCAT_TAG, "android_main android_state->window: %p",
                      android_state->window);

  // Used to poll the events in the main loop
  int events;
  android_poll_source *source;
  do
  {
    if(ALooper_pollAll(1, nullptr, &events, (void **)&source) >= 0)
    {
      if(source != NULL)
        source->process(android_state, source);
    }
  } while(android_state->destroyRequested == 0);
}
