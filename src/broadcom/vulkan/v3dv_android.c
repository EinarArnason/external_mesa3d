/*
 * Copyright © 2017, Google Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>

#include <hardware/hardware.h>
#include <hardware/hwvulkan.h>

#include <vulkan/vk_android_native_buffer.h>
#include <vulkan/vk_icd.h>

#include "drm-uapi/drm_fourcc.h"

#include "util/libsync.h"
#include "util/log.h"
#include "util/os_file.h"

#include "v3dv_private.h"

static int v3dv_hal_open(const struct hw_module_t *mod, const char *id, struct hw_device_t **dev);
static int v3dv_hal_close(struct hw_device_t *dev);

static void UNUSED
static_asserts(void)
{
   STATIC_ASSERT(HWVULKAN_DISPATCH_MAGIC == ICD_LOADER_MAGIC);
}

PUBLIC struct hwvulkan_module_t HAL_MODULE_INFO_SYM = {
   .common =
     {
       .tag = HARDWARE_MODULE_TAG,
       .module_api_version = HWVULKAN_MODULE_API_VERSION_0_1,
       .hal_api_version = HARDWARE_MAKE_API_VERSION(1, 0),
       .id = HWVULKAN_HARDWARE_MODULE_ID,
       .name = "Broadcom Vulkan HAL",
       .author = "Broadcom",
       .methods =
         &(hw_module_methods_t) {
           .open = v3dv_hal_open,
         },
     },
};

/* If any bits in test_mask are set, then unset them and return true. */
static inline bool
unmask32(uint32_t *inout_mask, uint32_t test_mask)
{
   uint32_t orig_mask = *inout_mask;
   *inout_mask &= ~test_mask;
   return *inout_mask != orig_mask;
}

static int
v3dv_hal_open(const struct hw_module_t *mod,
              const char *id,
              struct hw_device_t **dev)
{
   assert(mod == &HAL_MODULE_INFO_SYM.common);
   assert(strcmp(id, HWVULKAN_DEVICE_0) == 0);

   hwvulkan_device_t *hal_dev = malloc(sizeof(*hal_dev));
   if (!hal_dev)
      return -1;

   *hal_dev = (hwvulkan_device_t){
      .common =
        {
          .tag = HARDWARE_DEVICE_TAG,
          .version = HWVULKAN_DEVICE_API_VERSION_0_1,
          .module = &HAL_MODULE_INFO_SYM.common,
          .close = v3dv_hal_close,
        },
     .EnumerateInstanceExtensionProperties = v3dv_EnumerateInstanceExtensionProperties,
     .CreateInstance = v3dv_CreateInstance,
     .GetInstanceProcAddr = v3dv_GetInstanceProcAddr,
   };

   *dev = &hal_dev->common;
   return 0;
}

static int
v3dv_hal_close(struct hw_device_t *dev)
{
   /* hwvulkan.h claims that hw_device_t::close() is never called. */
   return -1;
}

/* get dma-buf and modifier from gralloc info */
VkResult
v3dv_gralloc_info(struct v3dv_device *device,
                  const VkNativeBufferANDROID *gralloc_info,
                  int *dma_buf,
                  uint64_t *modifier)
{
   const uint32_t *handle_fds = (uint32_t *)gralloc_info->handle->data;
   const uint32_t *handle_data = &handle_fds[gralloc_info->handle->numFds];

   if (gralloc_info->handle->numFds == 1) {
      /* gbm_gralloc.  TODO: modifiers support */
      *dma_buf = handle_fds[0];
   } else {
      return vk_errorf(device, VK_ERROR_INVALID_EXTERNAL_HANDLE,
                       "VkNativeBufferANDROID::handle::numFds is %d, expected 1",
                       gralloc_info->handle->numFds);
   }

   *modifier = DRM_FORMAT_MOD_LINEAR;
   return VK_SUCCESS;
}

/**
 * Creates the VkImage using the gralloc handle in *gralloc_info.
 */
VkResult
v3dv_import_memory_from_gralloc_handle(VkDevice device_h,
                                       int dma_buf,
                                       const VkAllocationCallbacks *alloc,
                                       VkImage image_h)
{
   struct v3dv_image *image = NULL;
   VkResult result;

   image = v3dv_image_from_handle(image_h);

   VkDeviceMemory memory_h;

   const VkMemoryDedicatedAllocateInfo ded_alloc = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
      .pNext = NULL,
      .buffer = VK_NULL_HANDLE,
      .image = image_h
   };

   const VkImportMemoryFdInfoKHR import_info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
      .pNext = &ded_alloc,
      .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
      .fd = os_dupfd_cloexec(dma_buf),
   };

   result =
      v3dv_AllocateMemory(device_h,
                        &(VkMemoryAllocateInfo) {
                           .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                           .pNext = &import_info,
                           .allocationSize = image->size,
                           .memoryTypeIndex = 0,
                        },
                        alloc, &memory_h);
   if (result != VK_SUCCESS)
      goto fail_create_image;

   VkBindImageMemoryInfo bind_info = {
      .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
      .image = image_h,
      .memory = memory_h,
      .memoryOffset = 0,
   };
   v3dv_BindImageMemory2(device_h, 1, &bind_info);

   image->owned_memory = memory_h;

   return VK_SUCCESS;

fail_create_image:
   v3dv_DestroyImage(device_h, image_h, alloc);

   return result;
}

static VkResult
format_supported_with_usage(VkDevice device_h, VkFormat format,
                            VkImageUsageFlags imageUsage)
{
   V3DV_FROM_HANDLE(v3dv_device, device, device_h);
   struct v3dv_physical_device *phys_dev = &device->instance->physicalDevice;
   VkPhysicalDevice phys_dev_h = v3dv_physical_device_to_handle(phys_dev);
   VkResult result;

   const VkPhysicalDeviceImageFormatInfo2 image_format_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
      .format = format,
      .type = VK_IMAGE_TYPE_2D,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = imageUsage,
   };

   VkImageFormatProperties2 image_format_props = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
   };

   /* Check that requested format and usage are supported. */
   result = v3dv_GetPhysicalDeviceImageFormatProperties2(
      phys_dev_h, &image_format_info, &image_format_props);
   if (result != VK_SUCCESS) {
      return vk_errorf(device, result,
                       "v3dv_GetPhysicalDeviceImageFormatProperties2 failed "
                       "inside %s",
                       __func__);
   }

   return VK_SUCCESS;
}

static VkResult
setup_gralloc0_usage(struct v3dv_device *device, VkFormat format,
                     VkImageUsageFlags imageUsage, int *grallocUsage)
{
   if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
      *grallocUsage |= GRALLOC_USAGE_HW_RENDER;

   if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_STORAGE_BIT |
                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
      *grallocUsage |= GRALLOC_USAGE_HW_TEXTURE;

   /* All VkImageUsageFlags not explicitly checked here are unsupported for
    * gralloc swapchains.
    */
   if (imageUsage != 0) {
      return vk_errorf(device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                       "unsupported VkImageUsageFlags(0x%x) for gralloc "
                       "swapchain", imageUsage);
   }

   /* The below formats support GRALLOC_USAGE_HW_FB (that is, display
    * scanout). This short list of formats is univserally supported on Intel
    * but is incomplete.  The full set of supported formats is dependent on
    * kernel and hardware.
    *
    * FINISHME: Advertise all display-supported formats.
    */
   switch (format) {
      case VK_FORMAT_R5G6B5_UNORM_PACK16:
      case VK_FORMAT_R8G8B8A8_UNORM:
      case VK_FORMAT_R8G8B8A8_SRGB:
         *grallocUsage |= GRALLOC_USAGE_HW_FB |
                          GRALLOC_USAGE_HW_COMPOSER |
                          GRALLOC_USAGE_EXTERNAL_DISP;
         break;
      default:
         mesa_logw("%s: unsupported format=%d", __func__, format);
   }

   if (*grallocUsage == 0)
      return VK_ERROR_FORMAT_NOT_SUPPORTED;

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
v3dv_GetSwapchainGrallocUsageANDROID(VkDevice device_h,
                                   VkFormat format,
                                   VkImageUsageFlags imageUsage,
                                   int *grallocUsage)
{
   V3DV_FROM_HANDLE(v3dv_device, device, device_h);
   VkResult result;

   result = format_supported_with_usage(device_h, format, imageUsage);
   if (result != VK_SUCCESS)
      return result;

   *grallocUsage = 0;
   return setup_gralloc0_usage(device, format, imageUsage, grallocUsage);
}

VKAPI_ATTR VkResult VKAPI_CALL
v3dv_GetSwapchainGrallocUsage2ANDROID(VkDevice device_h,
                                      VkFormat format,
                                      VkImageUsageFlags imageUsage,
                                      VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
                                      uint64_t *grallocConsumerUsage,
                                      uint64_t *grallocProducerUsage)
{
   V3DV_FROM_HANDLE(v3dv_device, device, device_h);
   VkResult result;

   *grallocConsumerUsage = 0;
   *grallocProducerUsage = 0;
   mesa_logd("%s: format=%d, usage=0x%x", __func__, format, imageUsage);

   result = format_supported_with_usage(device_h, format, imageUsage);
   if (result != VK_SUCCESS)
      return result;

   int32_t grallocUsage = 0;
   result = setup_gralloc0_usage(device, format, imageUsage, &grallocUsage);
   if (result != VK_SUCCESS)
      return result;

   /* Setup gralloc1 usage flags from gralloc0 flags. */

   if (grallocUsage & GRALLOC_USAGE_HW_RENDER) {
      *grallocProducerUsage |= GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
      *grallocConsumerUsage |= GRALLOC1_CONSUMER_USAGE_CLIENT_TARGET;
   }

   if (grallocUsage & GRALLOC_USAGE_HW_TEXTURE) {
      *grallocConsumerUsage |= GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE;
   }

   if (grallocUsage & (GRALLOC_USAGE_HW_FB |
                       GRALLOC_USAGE_HW_COMPOSER |
                       GRALLOC_USAGE_EXTERNAL_DISP)) {
      *grallocProducerUsage |= GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
      *grallocConsumerUsage |= GRALLOC1_CONSUMER_USAGE_HWCOMPOSER;
   }

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
v3dv_AcquireImageANDROID(VkDevice _device,
                         VkImage image_h,
                         int nativeFenceFd,
                         VkSemaphore semaphore_h,
                         VkFence fence_h)
{
   V3DV_FROM_HANDLE(v3dv_device, device, _device);
   VkResult result = VK_SUCCESS;

   if (nativeFenceFd != -1) {
      /* As a simple, firstpass implementation of VK_ANDROID_native_buffer, we
       * block on the nativeFenceFd. This may introduce latency and is
       * definitiely inefficient, yet it's correct.
       *
       * FINISHME(chadv): Import the nativeFenceFd into the VkSemaphore and
       * VkFence.
       */
      if (sync_wait(nativeFenceFd, /*timeout*/ -1) < 0) {
         result = vk_errorf(device, VK_ERROR_DEVICE_LOST,
                            "%s: failed to wait on nativeFenceFd=%d",
                            __func__, nativeFenceFd);
      }

      /* From VK_ANDROID_native_buffer's pseudo spec
       * (https://source.android.com/devices/graphics/implement-vulkan):
       *
       *    The driver takes ownership of the fence fd and is responsible for
       *    closing it [...] even if vkAcquireImageANDROID fails and returns
       *    an error.
       */
      close(nativeFenceFd);

      if (result != VK_SUCCESS)
         return result;
   }

   if (semaphore_h || fence_h) {
      /* Thanks to implicit sync, the image is ready for GPU access.  But we
       * must still put the semaphore into the "submit" state; otherwise the
       * client may get unexpected behavior if the client later uses it as
       * a wait semaphore.
       *
       * Because we blocked above on the nativeFenceFd, the image is also
       * ready for foreign-device access (including CPU access). But we must
       * still signal the fence; otherwise the client may get unexpected
       * behavior if the client later waits on it.
       *
       * For some values of anv_semaphore_type, we must submit the semaphore
       * to execbuf in order to signal it.  Likewise for anv_fence_type.
       * Instead of open-coding here the signal operation for each
       * anv_semaphore_type and anv_fence_type, we piggy-back on
       * vkQueueSubmit.
       */
      const VkSubmitInfo submit = {
         .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
         .waitSemaphoreCount = 0,
         .commandBufferCount = 0,
         .signalSemaphoreCount = (semaphore_h ? 1 : 0),
         .pSignalSemaphores = &semaphore_h,
      };

      result = v3dv_QueueSubmit(v3dv_queue_to_handle(&device->queue), 1,
                               &submit, fence_h);
      if (result != VK_SUCCESS) {
         return vk_errorf(device, result,
                          "anv_QueueSubmit failed inside %s", __func__);
      }
   }

   return VK_SUCCESS;
}

VkResult
v3dv_QueueSignalReleaseImageANDROID(VkQueue queue,
                                    uint32_t waitSemaphoreCount,
                                    const VkSemaphore* pWaitSemaphores,
                                    VkImage image,
                                    int* pNativeFenceFd)
{
   VkResult result;

   if (waitSemaphoreCount == 0)
      goto done;

   result = v3dv_QueueSubmit(queue, 1,
      &(VkSubmitInfo) {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = pWaitSemaphores,
      },
      (VkFence) VK_NULL_HANDLE);
   if (result != VK_SUCCESS)
      return result;

 done:
   if (pNativeFenceFd) {
      /* We can rely implicit on sync because above we submitted all
       * semaphores to the queue.
       */
      *pNativeFenceFd = -1;
   }

   return VK_SUCCESS;
}
