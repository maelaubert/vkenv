#ifdef VK_USE_PLATFORM_XCB_KHR

#include "logger.h"
#include "mini_window.h"

#include <xcb/xcb.h>
//
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_xcb.h>
/////////////////////////////////////////////////
// Implementation for X server (xcb)
/////////////////////////////////////////////////

static char LOG_TAG[] = "MiniWindow_xcb";

typedef struct vkenv_MiniWindow_T
{
  xcb_connection_t *wsi_connection;
  xcb_screen_t *wsi_screen;
  xcb_window_t wsi_window;
  xcb_intern_atom_reply_t *wm_delete_window_reply;
} vkenv_MiniWindow_T;

bool vkenv_createMiniWindow(vkenv_MiniWindow *window_ptr, vkenv_MiniWindowConfig *config)
{
  assert(window_ptr != NULL);  // pointer should be valid
  assert(*window_ptr == NULL); // Provided MiniWindow should be NULL to avoid overwriting data
  assert(config != NULL);

  *window_ptr = (vkenv_MiniWindow)malloc(sizeof(vkenv_MiniWindow_T));
  vkenv_MiniWindow window = *window_ptr;
  memset(window, 0, sizeof(vkenv_MiniWindow_T));

  // Connect to the X server
  int screen_number;
  window->wsi_connection = xcb_connect(NULL, &screen_number);
  if (xcb_connection_has_error(window->wsi_connection))
  {
    logError(LOG_TAG, "Failed to connect to X server (xcb_connection_has_error)");
    return false;
  }

  // Get the screen selected during the connection
  const xcb_setup_t *setup = xcb_get_setup(window->wsi_connection);
  xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(setup);
  for (int i = 0; i < screen_number; i++)
  {
    xcb_screen_next(&screen_iter);
  }
  window->wsi_screen = screen_iter.data;

  uint16_t window_width = config->width;
  uint16_t window_height = config->height;
  if (config->fullscreen)
  {
    window_width = window->wsi_screen->width_in_pixels;
    window_height = window->wsi_screen->height_in_pixels;
  }

  // Setup window and map to screen
  window->wsi_window = xcb_generate_id(window->wsi_connection);
  uint32_t value_list[32];
  value_list[0] = window->wsi_screen->black_pixel;
  value_list[1] = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_EXPOSURE;

  xcb_create_window(window->wsi_connection, XCB_COPY_FROM_PARENT, window->wsi_window, window->wsi_screen->root, 0, 0, window_width, window_height, 0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, window->wsi_screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, value_list);

  // Set window name
  xcb_change_property(window->wsi_connection, XCB_PROP_MODE_REPLACE, window->wsi_window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(config->name),
                      config->name);

  // Allow catching window destroyed events
  const char *wm_protocols_name = "WM_PROTOCOLS";
  const char *wm_delete_window_name = "WM_DELETE_WINDOW";
  xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(window->wsi_connection, true, strlen(wm_protocols_name), wm_protocols_name);
  xcb_intern_atom_reply_t *wm_protocols_reply = xcb_intern_atom_reply(window->wsi_connection, wm_protocols_cookie, NULL);
  xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom(window->wsi_connection, false, strlen(wm_delete_window_name), wm_delete_window_name);
  window->wm_delete_window_reply = xcb_intern_atom_reply(window->wsi_connection, wm_delete_window_cookie, NULL);
  xcb_change_property(window->wsi_connection, XCB_PROP_MODE_REPLACE, window->wsi_window, wm_protocols_reply->atom, XCB_ATOM_ATOM, 32, 1,
                      &window->wm_delete_window_reply->atom);
  free(wm_protocols_reply);

  if (config->fullscreen)
  {

    // Setup fullscreen mode (xcb will take the primary screen)
    const char *wm_state_name = "_NET_WM_STATE";
    xcb_intern_atom_cookie_t wm_state_cookie = xcb_intern_atom(window->wsi_connection, false, strlen(wm_state_name), wm_state_name);
    xcb_intern_atom_reply_t *wm_state_reply = xcb_intern_atom_reply(window->wsi_connection, wm_state_cookie, NULL);
    const char *wm_fullscreen_name = "_NET_WM_STATE_FULLSCREEN";
    xcb_intern_atom_cookie_t wm_fullscreen_cookie = xcb_intern_atom(window->wsi_connection, false, strlen(wm_fullscreen_name), wm_fullscreen_name);
    xcb_intern_atom_reply_t *wm_fullscreen_reply = xcb_intern_atom_reply(window->wsi_connection, wm_fullscreen_cookie, NULL);
    xcb_change_property(window->wsi_connection, XCB_PROP_MODE_REPLACE, window->wsi_window, wm_state_reply->atom, XCB_ATOM_ATOM, 32, 1,
                        &(wm_fullscreen_reply->atom));
    free(wm_fullscreen_reply);
    free(wm_state_reply);
  }

  xcb_map_window(window->wsi_connection, window->wsi_window);
  xcb_flush(window->wsi_connection);

  return true;
}

bool vkenv_getMiniWindowSurface(vkenv_MiniWindow window, VkInstance instance, VkSurfaceKHR *surface)
{
  // Get function address
  if (vkCreateXcbSurfaceKHR == NULL)
  {
    logError(LOG_TAG, " Function vkCreateXcbSurfaceKHR was not loaded by the instance. Make sure the instance extensions VK_KHR_surface and "
                      "VK_KHR_xcb_surface were added in the instance configuration.");
    return false;
  }
  VkXcbSurfaceCreateInfoKHR surface_create_info = {.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
                                                   .pNext = NULL,
                                                   .flags = 0,
                                                   .connection = window->wsi_connection,
                                                   .window = window->wsi_window};
  if (vkCreateXcbSurfaceKHR(instance, &surface_create_info, NULL, surface) != VK_SUCCESS)
  {
    logError(LOG_TAG, " Failed to acquire surface from vkCreateXcbSurfaceKHR. Make sure that the instance extensions VK_KHR_surface and "
                      "VK_KHR_xcb_surface are requested.");
    return false;
  }

  return true;
}

bool vkenv_checkMiniWindowDestroyedEvent(vkenv_MiniWindow window)
{
  xcb_generic_event_t *event;
  bool window_destroyed = false;
  while ((event = xcb_poll_for_event(window->wsi_connection)) != NULL)
  {
    if ((event->response_type & 0x7f) == XCB_CLIENT_MESSAGE)
    {
      xcb_client_message_event_t *client_msg_event = (xcb_client_message_event_t *)event;
      if (client_msg_event->data.data32[0] == window->wm_delete_window_reply->atom)
      {
        // If window destroyed event
        window_destroyed = true;
      }
    }
    free(event);
  }
  return window_destroyed;
}

void vkenv_destroyMiniWindow(vkenv_MiniWindow *window_ptr)
{
  assert(window_ptr != NULL);  // pointer should be valid
  assert(*window_ptr != NULL); // WSI_destroy shouldn't be called on NULL MiniWindow

  vkenv_MiniWindow window = *window_ptr;
  free(window->wm_delete_window_reply);
  xcb_destroy_window(window->wsi_connection, window->wsi_window);
  xcb_disconnect(window->wsi_connection);
  free(*window_ptr);
  *window_ptr = NULL;
}

const char *vkenv_getMiniWindowSurfaceExtensionName() { return VK_KHR_XCB_SURFACE_EXTENSION_NAME; }

#endif
