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

#define RENDERDOC_WINDOWING_XLIB 1
#define RENDERDOC_WINDOWING_XCB 1

#include "api/replay/renderdoc_replay.h"

#include "vk_core.h"
#include "vk_replay.h"

void VulkanReplay::OutputWindow::SetWindowHandle(WindowingSystem system, void *data)
{
  m_WindowSystem = system;

  if(system == eWindowingSystem_Xlib)
  {
    XlibWindowData *xdata = (XlibWindowData *)data;
    xlib.display = xdata->display;
    xlib.window = xdata->window;
  }
  else if(system == eWindowingSystem_XCB)
  {
    XCBWindowData *xdata = (XCBWindowData *)data;
    xcb.connection = xdata->connection;
    xcb.window = xdata->window;
  }
}

void VulkanReplay::OutputWindow::CreateSurface(VkInstance inst)
{
  if(m_WindowSystem == eWindowingSystem_Xlib)
  {
    VkXlibSurfaceCreateInfoKHR createInfo;

    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.dpy = xlib.display;
    createInfo.window = xlib.window;

    VkResult vkr = ObjDisp(inst)->CreateXlibSurfaceKHR(Unwrap(inst), &createInfo, NULL, &surface);
    RDCASSERTEQUAL(vkr, VK_SUCCESS);
  }
  else if(m_WindowSystem == eWindowingSystem_XCB)
  {
    VkXcbSurfaceCreateInfoKHR createInfo;

    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = xcb.connection;
    createInfo.window = xcb.window;

    VkResult vkr = ObjDisp(inst)->CreateXcbSurfaceKHR(Unwrap(inst), &createInfo, NULL, &surface);
    RDCASSERTEQUAL(vkr, VK_SUCCESS);
  }
}

void VulkanReplay::GetOutputWindowDimensions(uint64_t id, int32_t &w, int32_t &h)
{
  if(id == 0 || m_OutputWindows.find(id) == m_OutputWindows.end())
    return;

  OutputWindow &outw = m_OutputWindows[id];

#ifdef ANDROID
  w = 500;
  h = 500;//FIXME
  __android_log_print(ANDROID_LOG_INFO, LOGCAT_TAG, "GetOutputWindowDimensions: %i, %i", w, h);
#else
  if(outw.m_WindowSystem == eWindowingSystem_Xlib)
  {
    XWindowAttributes attr = {};
    XGetWindowAttributes(outw.xlib.display, outw.xlib.window, &attr);

    w = (int32_t)attr.width;
    h = (int32_t)attr.height;
  }
  else if(outw.m_WindowSystem == eWindowingSystem_XCB)
  {
    xcb_get_geometry_cookie_t geomCookie =
        xcb_get_geometry(outw.xcb.connection, outw.xcb.window);    // window is a xcb_drawable_t
    xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(outw.xcb.connection, geomCookie, NULL);

    w = (int32_t)geom->width;
    h = (int32_t)geom->height;

    free(geom);
  }
#endif
}

const char *VulkanLibraryName = "libvulkan.so.1";
