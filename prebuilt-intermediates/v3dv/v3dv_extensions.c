/*
 * Copyright 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "v3dv_private.h"

#include "v3dv_extensions.h"


#include "vk_util.h"

/* Convert the VK_USE_PLATFORM_* defines to booleans */
#ifdef VK_USE_PLATFORM_DISPLAY_KHR
#   undef VK_USE_PLATFORM_DISPLAY_KHR
#   define VK_USE_PLATFORM_DISPLAY_KHR true
#else
#   define VK_USE_PLATFORM_DISPLAY_KHR false
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
#   undef VK_USE_PLATFORM_XLIB_KHR
#   define VK_USE_PLATFORM_XLIB_KHR true
#else
#   define VK_USE_PLATFORM_XLIB_KHR false
#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
#   undef VK_USE_PLATFORM_XLIB_XRANDR_EXT
#   define VK_USE_PLATFORM_XLIB_XRANDR_EXT true
#else
#   define VK_USE_PLATFORM_XLIB_XRANDR_EXT false
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
#   undef VK_USE_PLATFORM_XCB_KHR
#   define VK_USE_PLATFORM_XCB_KHR true
#else
#   define VK_USE_PLATFORM_XCB_KHR false
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#   undef VK_USE_PLATFORM_WAYLAND_KHR
#   define VK_USE_PLATFORM_WAYLAND_KHR true
#else
#   define VK_USE_PLATFORM_WAYLAND_KHR false
#endif
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
#   undef VK_USE_PLATFORM_DIRECTFB_EXT
#   define VK_USE_PLATFORM_DIRECTFB_EXT true
#else
#   define VK_USE_PLATFORM_DIRECTFB_EXT false
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#   undef VK_USE_PLATFORM_ANDROID_KHR
#   define VK_USE_PLATFORM_ANDROID_KHR true
#else
#   define VK_USE_PLATFORM_ANDROID_KHR false
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
#   undef VK_USE_PLATFORM_WIN32_KHR
#   define VK_USE_PLATFORM_WIN32_KHR true
#else
#   define VK_USE_PLATFORM_WIN32_KHR false
#endif
#ifdef VK_USE_PLATFORM_VI_NN
#   undef VK_USE_PLATFORM_VI_NN
#   define VK_USE_PLATFORM_VI_NN true
#else
#   define VK_USE_PLATFORM_VI_NN false
#endif
#ifdef VK_USE_PLATFORM_IOS_MVK
#   undef VK_USE_PLATFORM_IOS_MVK
#   define VK_USE_PLATFORM_IOS_MVK true
#else
#   define VK_USE_PLATFORM_IOS_MVK false
#endif
#ifdef VK_USE_PLATFORM_MACOS_MVK
#   undef VK_USE_PLATFORM_MACOS_MVK
#   define VK_USE_PLATFORM_MACOS_MVK true
#else
#   define VK_USE_PLATFORM_MACOS_MVK false
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
#   undef VK_USE_PLATFORM_METAL_EXT
#   define VK_USE_PLATFORM_METAL_EXT true
#else
#   define VK_USE_PLATFORM_METAL_EXT false
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
#   undef VK_USE_PLATFORM_FUCHSIA
#   define VK_USE_PLATFORM_FUCHSIA true
#else
#   define VK_USE_PLATFORM_FUCHSIA false
#endif
#ifdef VK_USE_PLATFORM_GGP
#   undef VK_USE_PLATFORM_GGP
#   define VK_USE_PLATFORM_GGP true
#else
#   define VK_USE_PLATFORM_GGP false
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
#   undef VK_ENABLE_BETA_EXTENSIONS
#   define VK_ENABLE_BETA_EXTENSIONS true
#else
#   define VK_ENABLE_BETA_EXTENSIONS false
#endif
#ifdef VK_USE_PLATFORM_SCREEN_QNX
#   undef VK_USE_PLATFORM_SCREEN_QNX
#   define VK_USE_PLATFORM_SCREEN_QNX true
#else
#   define VK_USE_PLATFORM_SCREEN_QNX false
#endif

/* And ANDROID too */
#ifdef ANDROID
#   undef ANDROID
#   define ANDROID true
#else
#   define ANDROID false
#   define ANDROID_API_LEVEL 0
#endif

#define V3DV_HAS_SURFACE (VK_USE_PLATFORM_WIN32_KHR ||                                        VK_USE_PLATFORM_WAYLAND_KHR ||                                        VK_USE_PLATFORM_XCB_KHR ||                                        VK_USE_PLATFORM_XLIB_KHR ||                                        VK_USE_PLATFORM_DISPLAY_KHR)

static const uint32_t MAX_API_VERSION = VK_MAKE_VERSION(1, 1, 155);

VKAPI_ATTR VkResult VKAPI_CALL v3dv_EnumerateInstanceVersion(
    uint32_t*                                   pApiVersion)
{
    *pApiVersion = MAX_API_VERSION;
    return VK_SUCCESS;
}

const struct vk_instance_extension_table v3dv_instance_extensions_supported = {
   .KHR_display = VK_USE_PLATFORM_DISPLAY_KHR,
   .KHR_external_memory_capabilities = true,
   .KHR_get_physical_device_properties2 = true,
   .KHR_get_surface_capabilities2 = V3DV_HAS_SURFACE,
   .KHR_surface = V3DV_HAS_SURFACE,
   .KHR_wayland_surface = VK_USE_PLATFORM_WAYLAND_KHR,
   .KHR_xcb_surface = VK_USE_PLATFORM_XCB_KHR,
   .KHR_xlib_surface = VK_USE_PLATFORM_XLIB_KHR,
   .EXT_debug_report = true,
};

uint32_t
v3dv_physical_device_api_version(struct v3dv_physical_device *device)
{
    uint32_t version = 0;

    uint32_t override = vk_get_version_override();
    if (override)
        return MIN2(override, MAX_API_VERSION);

    if (!(true))
        return version;
    version = VK_MAKE_VERSION(1, 0, 155);

    if (!(false))
        return version;
    version = VK_MAKE_VERSION(1, 1, 155);

    return version;
}

void
v3dv_physical_device_get_supported_extensions(const struct v3dv_physical_device *device,
                                                   struct vk_device_extension_table *extensions)
{
   *extensions = (struct vk_device_extension_table) {
      .KHR_external_memory = true,
      .KHR_external_memory_fd = true,
      .KHR_maintenance1 = true,
      .KHR_swapchain = V3DV_HAS_SURFACE,
      .EXT_external_memory_dma_buf = true,
      .EXT_image_drm_format_modifier = false,
      .EXT_private_data = true,
      .ANDROID_native_buffer = ANDROID,
   };
}
