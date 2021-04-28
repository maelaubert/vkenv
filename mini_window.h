#ifndef VKENV_MINIWINDOW_H
#define VKENV_MINIWINDOW_H

#include "vulkan_loader.h"

typedef struct vkenv_MiniWindow_T *vkenv_MiniWindow;

typedef struct
{
  const char *name;
  const uint16_t width;
  const uint16_t height;
  bool fullscreen;
} vkenv_MiniWindowConfig;

// MiniWindow is a minimal window manager that can only create/destroy window, provide a drawable surface to Vulkan
// and detect window destruction event. Functions below must be implemented once per window manager type (xcb, wayland, win32, etc)
// in a separate file. Implementation codes are conditional to the "VK_USE_PLATFORM_impl_KHR" type of Vulkan defines so the type
// of window manager used is chosen at compile time by specifying the relevant define for the current platform.
//
// To use MiniWindow, the vulkan instance need to support at least the VK_KHR_SURFACE_EXTENSION_NAME extension and
// the return value of vkenv_getMiniWindowSurfaceExtensionName().

bool vkenv_createMiniWindow(vkenv_MiniWindow *wsi_ptr, vkenv_MiniWindowConfig *config);
bool vkenv_getMiniWindowSurface(vkenv_MiniWindow wsi, VkInstance instance, VkSurfaceKHR *surface);

// Return true if window was closed, false otherwise
bool vkenv_checkMiniWindowDestroyedEvent(vkenv_MiniWindow wsi);
void vkenv_destroyMiniWindow(vkenv_MiniWindow *wsi_ptr);

// Helper function to get the Vulkan instance extension required by the MiniWindow.
// MiniWindow will only work if the Vulkan instance was created with this extension.
const char *vkenv_getMiniWindowSurfaceExtensionName();

#endif // VKENV_MINIWINDOW_H