/**
 * SPDX-License-Identifier: (WTFPL OR CC0-1.0) AND Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/egl.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */

#ifdef __cplusplus
extern "C" {
#endif



int GLAD_EGL_VERSION_1_0 = 0;
int GLAD_EGL_VERSION_1_1 = 0;
int GLAD_EGL_VERSION_1_2 = 0;
int GLAD_EGL_VERSION_1_3 = 0;
int GLAD_EGL_VERSION_1_4 = 0;
int GLAD_EGL_VERSION_1_5 = 0;
int GLAD_EGL_ANGLE_d3d_share_handle_client_buffer = 0;
int GLAD_EGL_ANGLE_device_d3d = 0;
int GLAD_EGL_ANGLE_query_surface_pointer = 0;
int GLAD_EGL_ANGLE_surface_d3d_texture_2d_share_handle = 0;
int GLAD_EGL_ANGLE_sync_control_rate = 0;
int GLAD_EGL_ANGLE_window_fixed_size = 0;
int GLAD_EGL_EXT_bind_to_front = 0;
int GLAD_EGL_EXT_buffer_age = 0;
int GLAD_EGL_EXT_client_extensions = 0;
int GLAD_EGL_EXT_client_sync = 0;
int GLAD_EGL_EXT_compositor = 0;
int GLAD_EGL_EXT_config_select_group = 0;
int GLAD_EGL_EXT_create_context_robustness = 0;
int GLAD_EGL_EXT_device_base = 0;
int GLAD_EGL_EXT_device_drm = 0;
int GLAD_EGL_EXT_device_drm_render_node = 0;
int GLAD_EGL_EXT_device_enumeration = 0;
int GLAD_EGL_EXT_device_openwf = 0;
int GLAD_EGL_EXT_device_persistent_id = 0;
int GLAD_EGL_EXT_device_query = 0;
int GLAD_EGL_EXT_device_query_name = 0;
int GLAD_EGL_EXT_display_alloc = 0;
int GLAD_EGL_EXT_explicit_device = 0;
int GLAD_EGL_EXT_gl_colorspace_bt2020_hlg = 0;
int GLAD_EGL_EXT_gl_colorspace_bt2020_linear = 0;
int GLAD_EGL_EXT_gl_colorspace_bt2020_pq = 0;
int GLAD_EGL_EXT_gl_colorspace_display_p3 = 0;
int GLAD_EGL_EXT_gl_colorspace_display_p3_linear = 0;
int GLAD_EGL_EXT_gl_colorspace_display_p3_passthrough = 0;
int GLAD_EGL_EXT_gl_colorspace_scrgb = 0;
int GLAD_EGL_EXT_gl_colorspace_scrgb_linear = 0;
int GLAD_EGL_EXT_image_dma_buf_import = 0;
int GLAD_EGL_EXT_image_dma_buf_import_modifiers = 0;
int GLAD_EGL_EXT_image_gl_colorspace = 0;
int GLAD_EGL_EXT_image_implicit_sync_control = 0;
int GLAD_EGL_EXT_multiview_window = 0;
int GLAD_EGL_EXT_output_base = 0;
int GLAD_EGL_EXT_output_drm = 0;
int GLAD_EGL_EXT_output_openwf = 0;
int GLAD_EGL_EXT_pixel_format_float = 0;
int GLAD_EGL_EXT_platform_base = 0;
int GLAD_EGL_EXT_platform_device = 0;
int GLAD_EGL_EXT_platform_wayland = 0;
int GLAD_EGL_EXT_platform_x11 = 0;
int GLAD_EGL_EXT_platform_xcb = 0;
int GLAD_EGL_EXT_present_opaque = 0;
int GLAD_EGL_EXT_protected_content = 0;
int GLAD_EGL_EXT_protected_surface = 0;
int GLAD_EGL_EXT_query_reset_notification_strategy = 0;
int GLAD_EGL_EXT_stream_consumer_egloutput = 0;
int GLAD_EGL_EXT_surface_CTA861_3_metadata = 0;
int GLAD_EGL_EXT_surface_SMPTE2086_metadata = 0;
int GLAD_EGL_EXT_surface_compression = 0;
int GLAD_EGL_EXT_swap_buffers_with_damage = 0;
int GLAD_EGL_EXT_sync_reuse = 0;
int GLAD_EGL_EXT_yuv_surface = 0;
int GLAD_EGL_IMG_context_priority = 0;
int GLAD_EGL_KHR_cl_event = 0;
int GLAD_EGL_KHR_cl_event2 = 0;
int GLAD_EGL_KHR_client_get_all_proc_addresses = 0;
int GLAD_EGL_KHR_config_attribs = 0;
int GLAD_EGL_KHR_context_flush_control = 0;
int GLAD_EGL_KHR_create_context = 0;
int GLAD_EGL_KHR_create_context_no_error = 0;
int GLAD_EGL_KHR_debug = 0;
int GLAD_EGL_KHR_display_reference = 0;
int GLAD_EGL_KHR_fence_sync = 0;
int GLAD_EGL_KHR_get_all_proc_addresses = 0;
int GLAD_EGL_KHR_gl_colorspace = 0;
int GLAD_EGL_KHR_gl_renderbuffer_image = 0;
int GLAD_EGL_KHR_gl_texture_2D_image = 0;
int GLAD_EGL_KHR_gl_texture_3D_image = 0;
int GLAD_EGL_KHR_gl_texture_cubemap_image = 0;
int GLAD_EGL_KHR_image = 0;
int GLAD_EGL_KHR_image_base = 0;
int GLAD_EGL_KHR_image_pixmap = 0;
int GLAD_EGL_KHR_lock_surface = 0;
int GLAD_EGL_KHR_lock_surface2 = 0;
int GLAD_EGL_KHR_lock_surface3 = 0;
int GLAD_EGL_KHR_mutable_render_buffer = 0;
int GLAD_EGL_KHR_no_config_context = 0;
int GLAD_EGL_KHR_partial_update = 0;
int GLAD_EGL_KHR_platform_android = 0;
int GLAD_EGL_KHR_platform_gbm = 0;
int GLAD_EGL_KHR_platform_wayland = 0;
int GLAD_EGL_KHR_platform_x11 = 0;
int GLAD_EGL_KHR_reusable_sync = 0;
int GLAD_EGL_KHR_stream = 0;
int GLAD_EGL_KHR_stream_attrib = 0;
int GLAD_EGL_KHR_stream_consumer_gltexture = 0;
int GLAD_EGL_KHR_stream_cross_process_fd = 0;
int GLAD_EGL_KHR_stream_fifo = 0;
int GLAD_EGL_KHR_stream_producer_aldatalocator = 0;
int GLAD_EGL_KHR_stream_producer_eglsurface = 0;
int GLAD_EGL_KHR_surfaceless_context = 0;
int GLAD_EGL_KHR_swap_buffers_with_damage = 0;
int GLAD_EGL_KHR_vg_parent_image = 0;
int GLAD_EGL_KHR_wait_sync = 0;
int GLAD_EGL_NV_context_priority_realtime = 0;



PFNEGLBINDAPIPROC glad_eglBindAPI = NULL;
PFNEGLBINDTEXIMAGEPROC glad_eglBindTexImage = NULL;
PFNEGLCHOOSECONFIGPROC glad_eglChooseConfig = NULL;
PFNEGLCLIENTSIGNALSYNCEXTPROC glad_eglClientSignalSyncEXT = NULL;
PFNEGLCLIENTWAITSYNCPROC glad_eglClientWaitSync = NULL;
PFNEGLCLIENTWAITSYNCKHRPROC glad_eglClientWaitSyncKHR = NULL;
PFNEGLCOMPOSITORBINDTEXWINDOWEXTPROC glad_eglCompositorBindTexWindowEXT = NULL;
PFNEGLCOMPOSITORSETCONTEXTATTRIBUTESEXTPROC glad_eglCompositorSetContextAttributesEXT = NULL;
PFNEGLCOMPOSITORSETCONTEXTLISTEXTPROC glad_eglCompositorSetContextListEXT = NULL;
PFNEGLCOMPOSITORSETSIZEEXTPROC glad_eglCompositorSetSizeEXT = NULL;
PFNEGLCOMPOSITORSETWINDOWATTRIBUTESEXTPROC glad_eglCompositorSetWindowAttributesEXT = NULL;
PFNEGLCOMPOSITORSETWINDOWLISTEXTPROC glad_eglCompositorSetWindowListEXT = NULL;
PFNEGLCOMPOSITORSWAPPOLICYEXTPROC glad_eglCompositorSwapPolicyEXT = NULL;
PFNEGLCOPYBUFFERSPROC glad_eglCopyBuffers = NULL;
PFNEGLCREATECONTEXTPROC glad_eglCreateContext = NULL;
PFNEGLCREATEIMAGEPROC glad_eglCreateImage = NULL;
PFNEGLCREATEIMAGEKHRPROC glad_eglCreateImageKHR = NULL;
PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC glad_eglCreatePbufferFromClientBuffer = NULL;
PFNEGLCREATEPBUFFERSURFACEPROC glad_eglCreatePbufferSurface = NULL;
PFNEGLCREATEPIXMAPSURFACEPROC glad_eglCreatePixmapSurface = NULL;
PFNEGLCREATEPLATFORMPIXMAPSURFACEPROC glad_eglCreatePlatformPixmapSurface = NULL;
PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC glad_eglCreatePlatformPixmapSurfaceEXT = NULL;
PFNEGLCREATEPLATFORMWINDOWSURFACEPROC glad_eglCreatePlatformWindowSurface = NULL;
PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC glad_eglCreatePlatformWindowSurfaceEXT = NULL;
PFNEGLCREATESTREAMATTRIBKHRPROC glad_eglCreateStreamAttribKHR = NULL;
PFNEGLCREATESTREAMFROMFILEDESCRIPTORKHRPROC glad_eglCreateStreamFromFileDescriptorKHR = NULL;
PFNEGLCREATESTREAMKHRPROC glad_eglCreateStreamKHR = NULL;
PFNEGLCREATESTREAMPRODUCERSURFACEKHRPROC glad_eglCreateStreamProducerSurfaceKHR = NULL;
PFNEGLCREATESYNCPROC glad_eglCreateSync = NULL;
PFNEGLCREATESYNC64KHRPROC glad_eglCreateSync64KHR = NULL;
PFNEGLCREATESYNCKHRPROC glad_eglCreateSyncKHR = NULL;
PFNEGLCREATEWINDOWSURFACEPROC glad_eglCreateWindowSurface = NULL;
PFNEGLDEBUGMESSAGECONTROLKHRPROC glad_eglDebugMessageControlKHR = NULL;
PFNEGLDESTROYCONTEXTPROC glad_eglDestroyContext = NULL;
PFNEGLDESTROYDISPLAYEXTPROC glad_eglDestroyDisplayEXT = NULL;
PFNEGLDESTROYIMAGEPROC glad_eglDestroyImage = NULL;
PFNEGLDESTROYIMAGEKHRPROC glad_eglDestroyImageKHR = NULL;
PFNEGLDESTROYSTREAMKHRPROC glad_eglDestroyStreamKHR = NULL;
PFNEGLDESTROYSURFACEPROC glad_eglDestroySurface = NULL;
PFNEGLDESTROYSYNCPROC glad_eglDestroySync = NULL;
PFNEGLDESTROYSYNCKHRPROC glad_eglDestroySyncKHR = NULL;
PFNEGLGETCONFIGATTRIBPROC glad_eglGetConfigAttrib = NULL;
PFNEGLGETCONFIGSPROC glad_eglGetConfigs = NULL;
PFNEGLGETCURRENTCONTEXTPROC glad_eglGetCurrentContext = NULL;
PFNEGLGETCURRENTDISPLAYPROC glad_eglGetCurrentDisplay = NULL;
PFNEGLGETCURRENTSURFACEPROC glad_eglGetCurrentSurface = NULL;
PFNEGLGETDISPLAYPROC glad_eglGetDisplay = NULL;
PFNEGLGETERRORPROC glad_eglGetError = NULL;
PFNEGLGETMSCRATEANGLEPROC glad_eglGetMscRateANGLE = NULL;
PFNEGLGETOUTPUTLAYERSEXTPROC glad_eglGetOutputLayersEXT = NULL;
PFNEGLGETOUTPUTPORTSEXTPROC glad_eglGetOutputPortsEXT = NULL;
PFNEGLGETPLATFORMDISPLAYPROC glad_eglGetPlatformDisplay = NULL;
PFNEGLGETPLATFORMDISPLAYEXTPROC glad_eglGetPlatformDisplayEXT = NULL;
PFNEGLGETPROCADDRESSPROC glad_eglGetProcAddress = NULL;
PFNEGLGETSTREAMFILEDESCRIPTORKHRPROC glad_eglGetStreamFileDescriptorKHR = NULL;
PFNEGLGETSYNCATTRIBPROC glad_eglGetSyncAttrib = NULL;
PFNEGLGETSYNCATTRIBKHRPROC glad_eglGetSyncAttribKHR = NULL;
PFNEGLINITIALIZEPROC glad_eglInitialize = NULL;
PFNEGLLABELOBJECTKHRPROC glad_eglLabelObjectKHR = NULL;
PFNEGLLOCKSURFACEKHRPROC glad_eglLockSurfaceKHR = NULL;
PFNEGLMAKECURRENTPROC glad_eglMakeCurrent = NULL;
PFNEGLOUTPUTLAYERATTRIBEXTPROC glad_eglOutputLayerAttribEXT = NULL;
PFNEGLOUTPUTPORTATTRIBEXTPROC glad_eglOutputPortAttribEXT = NULL;
PFNEGLQUERYAPIPROC glad_eglQueryAPI = NULL;
PFNEGLQUERYCONTEXTPROC glad_eglQueryContext = NULL;
PFNEGLQUERYDEBUGKHRPROC glad_eglQueryDebugKHR = NULL;
PFNEGLQUERYDEVICEATTRIBEXTPROC glad_eglQueryDeviceAttribEXT = NULL;
PFNEGLQUERYDEVICEBINARYEXTPROC glad_eglQueryDeviceBinaryEXT = NULL;
PFNEGLQUERYDEVICESTRINGEXTPROC glad_eglQueryDeviceStringEXT = NULL;
PFNEGLQUERYDEVICESEXTPROC glad_eglQueryDevicesEXT = NULL;
PFNEGLQUERYDISPLAYATTRIBEXTPROC glad_eglQueryDisplayAttribEXT = NULL;
PFNEGLQUERYDISPLAYATTRIBKHRPROC glad_eglQueryDisplayAttribKHR = NULL;
PFNEGLQUERYDMABUFFORMATSEXTPROC glad_eglQueryDmaBufFormatsEXT = NULL;
PFNEGLQUERYDMABUFMODIFIERSEXTPROC glad_eglQueryDmaBufModifiersEXT = NULL;
PFNEGLQUERYOUTPUTLAYERATTRIBEXTPROC glad_eglQueryOutputLayerAttribEXT = NULL;
PFNEGLQUERYOUTPUTLAYERSTRINGEXTPROC glad_eglQueryOutputLayerStringEXT = NULL;
PFNEGLQUERYOUTPUTPORTATTRIBEXTPROC glad_eglQueryOutputPortAttribEXT = NULL;
PFNEGLQUERYOUTPUTPORTSTRINGEXTPROC glad_eglQueryOutputPortStringEXT = NULL;
PFNEGLQUERYSTREAMATTRIBKHRPROC glad_eglQueryStreamAttribKHR = NULL;
PFNEGLQUERYSTREAMKHRPROC glad_eglQueryStreamKHR = NULL;
PFNEGLQUERYSTREAMTIMEKHRPROC glad_eglQueryStreamTimeKHR = NULL;
PFNEGLQUERYSTREAMU64KHRPROC glad_eglQueryStreamu64KHR = NULL;
PFNEGLQUERYSTRINGPROC glad_eglQueryString = NULL;
PFNEGLQUERYSUPPORTEDCOMPRESSIONRATESEXTPROC glad_eglQuerySupportedCompressionRatesEXT = NULL;
PFNEGLQUERYSURFACEPROC glad_eglQuerySurface = NULL;
PFNEGLQUERYSURFACE64KHRPROC glad_eglQuerySurface64KHR = NULL;
PFNEGLQUERYSURFACEPOINTERANGLEPROC glad_eglQuerySurfacePointerANGLE = NULL;
PFNEGLRELEASETEXIMAGEPROC glad_eglReleaseTexImage = NULL;
PFNEGLRELEASETHREADPROC glad_eglReleaseThread = NULL;
PFNEGLSETDAMAGEREGIONKHRPROC glad_eglSetDamageRegionKHR = NULL;
PFNEGLSETSTREAMATTRIBKHRPROC glad_eglSetStreamAttribKHR = NULL;
PFNEGLSIGNALSYNCKHRPROC glad_eglSignalSyncKHR = NULL;
PFNEGLSTREAMATTRIBKHRPROC glad_eglStreamAttribKHR = NULL;
PFNEGLSTREAMCONSUMERACQUIREATTRIBKHRPROC glad_eglStreamConsumerAcquireAttribKHR = NULL;
PFNEGLSTREAMCONSUMERACQUIREKHRPROC glad_eglStreamConsumerAcquireKHR = NULL;
PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALKHRPROC glad_eglStreamConsumerGLTextureExternalKHR = NULL;
PFNEGLSTREAMCONSUMEROUTPUTEXTPROC glad_eglStreamConsumerOutputEXT = NULL;
PFNEGLSTREAMCONSUMERRELEASEATTRIBKHRPROC glad_eglStreamConsumerReleaseAttribKHR = NULL;
PFNEGLSTREAMCONSUMERRELEASEKHRPROC glad_eglStreamConsumerReleaseKHR = NULL;
PFNEGLSURFACEATTRIBPROC glad_eglSurfaceAttrib = NULL;
PFNEGLSWAPBUFFERSPROC glad_eglSwapBuffers = NULL;
PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC glad_eglSwapBuffersWithDamageEXT = NULL;
PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC glad_eglSwapBuffersWithDamageKHR = NULL;
PFNEGLSWAPINTERVALPROC glad_eglSwapInterval = NULL;
PFNEGLTERMINATEPROC glad_eglTerminate = NULL;
PFNEGLUNLOCKSURFACEKHRPROC glad_eglUnlockSurfaceKHR = NULL;
PFNEGLUNSIGNALSYNCEXTPROC glad_eglUnsignalSyncEXT = NULL;
PFNEGLWAITCLIENTPROC glad_eglWaitClient = NULL;
PFNEGLWAITGLPROC glad_eglWaitGL = NULL;
PFNEGLWAITNATIVEPROC glad_eglWaitNative = NULL;
PFNEGLWAITSYNCPROC glad_eglWaitSync = NULL;
PFNEGLWAITSYNCKHRPROC glad_eglWaitSyncKHR = NULL;


static void glad_egl_load_EGL_VERSION_1_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_0) return;
    glad_eglChooseConfig = (PFNEGLCHOOSECONFIGPROC) load(userptr, "eglChooseConfig");
    glad_eglCopyBuffers = (PFNEGLCOPYBUFFERSPROC) load(userptr, "eglCopyBuffers");
    glad_eglCreateContext = (PFNEGLCREATECONTEXTPROC) load(userptr, "eglCreateContext");
    glad_eglCreatePbufferSurface = (PFNEGLCREATEPBUFFERSURFACEPROC) load(userptr, "eglCreatePbufferSurface");
    glad_eglCreatePixmapSurface = (PFNEGLCREATEPIXMAPSURFACEPROC) load(userptr, "eglCreatePixmapSurface");
    glad_eglCreateWindowSurface = (PFNEGLCREATEWINDOWSURFACEPROC) load(userptr, "eglCreateWindowSurface");
    glad_eglDestroyContext = (PFNEGLDESTROYCONTEXTPROC) load(userptr, "eglDestroyContext");
    glad_eglDestroySurface = (PFNEGLDESTROYSURFACEPROC) load(userptr, "eglDestroySurface");
    glad_eglGetConfigAttrib = (PFNEGLGETCONFIGATTRIBPROC) load(userptr, "eglGetConfigAttrib");
    glad_eglGetConfigs = (PFNEGLGETCONFIGSPROC) load(userptr, "eglGetConfigs");
    glad_eglGetCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC) load(userptr, "eglGetCurrentDisplay");
    glad_eglGetCurrentSurface = (PFNEGLGETCURRENTSURFACEPROC) load(userptr, "eglGetCurrentSurface");
    glad_eglGetDisplay = (PFNEGLGETDISPLAYPROC) load(userptr, "eglGetDisplay");
    glad_eglGetError = (PFNEGLGETERRORPROC) load(userptr, "eglGetError");
    glad_eglGetProcAddress = (PFNEGLGETPROCADDRESSPROC) load(userptr, "eglGetProcAddress");
    glad_eglInitialize = (PFNEGLINITIALIZEPROC) load(userptr, "eglInitialize");
    glad_eglMakeCurrent = (PFNEGLMAKECURRENTPROC) load(userptr, "eglMakeCurrent");
    glad_eglQueryContext = (PFNEGLQUERYCONTEXTPROC) load(userptr, "eglQueryContext");
    glad_eglQueryString = (PFNEGLQUERYSTRINGPROC) load(userptr, "eglQueryString");
    glad_eglQuerySurface = (PFNEGLQUERYSURFACEPROC) load(userptr, "eglQuerySurface");
    glad_eglSwapBuffers = (PFNEGLSWAPBUFFERSPROC) load(userptr, "eglSwapBuffers");
    glad_eglTerminate = (PFNEGLTERMINATEPROC) load(userptr, "eglTerminate");
    glad_eglWaitGL = (PFNEGLWAITGLPROC) load(userptr, "eglWaitGL");
    glad_eglWaitNative = (PFNEGLWAITNATIVEPROC) load(userptr, "eglWaitNative");
}
static void glad_egl_load_EGL_VERSION_1_1( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_1) return;
    glad_eglBindTexImage = (PFNEGLBINDTEXIMAGEPROC) load(userptr, "eglBindTexImage");
    glad_eglReleaseTexImage = (PFNEGLRELEASETEXIMAGEPROC) load(userptr, "eglReleaseTexImage");
    glad_eglSurfaceAttrib = (PFNEGLSURFACEATTRIBPROC) load(userptr, "eglSurfaceAttrib");
    glad_eglSwapInterval = (PFNEGLSWAPINTERVALPROC) load(userptr, "eglSwapInterval");
}
static void glad_egl_load_EGL_VERSION_1_2( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_2) return;
    glad_eglBindAPI = (PFNEGLBINDAPIPROC) load(userptr, "eglBindAPI");
    glad_eglCreatePbufferFromClientBuffer = (PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC) load(userptr, "eglCreatePbufferFromClientBuffer");
    glad_eglQueryAPI = (PFNEGLQUERYAPIPROC) load(userptr, "eglQueryAPI");
    glad_eglReleaseThread = (PFNEGLRELEASETHREADPROC) load(userptr, "eglReleaseThread");
    glad_eglWaitClient = (PFNEGLWAITCLIENTPROC) load(userptr, "eglWaitClient");
}
static void glad_egl_load_EGL_VERSION_1_4( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_4) return;
    glad_eglGetCurrentContext = (PFNEGLGETCURRENTCONTEXTPROC) load(userptr, "eglGetCurrentContext");
}
static void glad_egl_load_EGL_VERSION_1_5( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_VERSION_1_5) return;
    glad_eglClientWaitSync = (PFNEGLCLIENTWAITSYNCPROC) load(userptr, "eglClientWaitSync");
    glad_eglCreateImage = (PFNEGLCREATEIMAGEPROC) load(userptr, "eglCreateImage");
    glad_eglCreatePlatformPixmapSurface = (PFNEGLCREATEPLATFORMPIXMAPSURFACEPROC) load(userptr, "eglCreatePlatformPixmapSurface");
    glad_eglCreatePlatformWindowSurface = (PFNEGLCREATEPLATFORMWINDOWSURFACEPROC) load(userptr, "eglCreatePlatformWindowSurface");
    glad_eglCreateSync = (PFNEGLCREATESYNCPROC) load(userptr, "eglCreateSync");
    glad_eglDestroyImage = (PFNEGLDESTROYIMAGEPROC) load(userptr, "eglDestroyImage");
    glad_eglDestroySync = (PFNEGLDESTROYSYNCPROC) load(userptr, "eglDestroySync");
    glad_eglGetPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYPROC) load(userptr, "eglGetPlatformDisplay");
    glad_eglGetSyncAttrib = (PFNEGLGETSYNCATTRIBPROC) load(userptr, "eglGetSyncAttrib");
    glad_eglWaitSync = (PFNEGLWAITSYNCPROC) load(userptr, "eglWaitSync");
}
static void glad_egl_load_EGL_ANGLE_query_surface_pointer( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_ANGLE_query_surface_pointer) return;
    glad_eglQuerySurfacePointerANGLE = (PFNEGLQUERYSURFACEPOINTERANGLEPROC) load(userptr, "eglQuerySurfacePointerANGLE");
}
static void glad_egl_load_EGL_ANGLE_sync_control_rate( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_ANGLE_sync_control_rate) return;
    glad_eglGetMscRateANGLE = (PFNEGLGETMSCRATEANGLEPROC) load(userptr, "eglGetMscRateANGLE");
}
static void glad_egl_load_EGL_EXT_client_sync( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_client_sync) return;
    glad_eglClientSignalSyncEXT = (PFNEGLCLIENTSIGNALSYNCEXTPROC) load(userptr, "eglClientSignalSyncEXT");
}
static void glad_egl_load_EGL_EXT_compositor( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_compositor) return;
    glad_eglCompositorBindTexWindowEXT = (PFNEGLCOMPOSITORBINDTEXWINDOWEXTPROC) load(userptr, "eglCompositorBindTexWindowEXT");
    glad_eglCompositorSetContextAttributesEXT = (PFNEGLCOMPOSITORSETCONTEXTATTRIBUTESEXTPROC) load(userptr, "eglCompositorSetContextAttributesEXT");
    glad_eglCompositorSetContextListEXT = (PFNEGLCOMPOSITORSETCONTEXTLISTEXTPROC) load(userptr, "eglCompositorSetContextListEXT");
    glad_eglCompositorSetSizeEXT = (PFNEGLCOMPOSITORSETSIZEEXTPROC) load(userptr, "eglCompositorSetSizeEXT");
    glad_eglCompositorSetWindowAttributesEXT = (PFNEGLCOMPOSITORSETWINDOWATTRIBUTESEXTPROC) load(userptr, "eglCompositorSetWindowAttributesEXT");
    glad_eglCompositorSetWindowListEXT = (PFNEGLCOMPOSITORSETWINDOWLISTEXTPROC) load(userptr, "eglCompositorSetWindowListEXT");
    glad_eglCompositorSwapPolicyEXT = (PFNEGLCOMPOSITORSWAPPOLICYEXTPROC) load(userptr, "eglCompositorSwapPolicyEXT");
}
static void glad_egl_load_EGL_EXT_device_base( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_device_base) return;
    glad_eglQueryDeviceAttribEXT = (PFNEGLQUERYDEVICEATTRIBEXTPROC) load(userptr, "eglQueryDeviceAttribEXT");
    glad_eglQueryDeviceStringEXT = (PFNEGLQUERYDEVICESTRINGEXTPROC) load(userptr, "eglQueryDeviceStringEXT");
    glad_eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC) load(userptr, "eglQueryDevicesEXT");
    glad_eglQueryDisplayAttribEXT = (PFNEGLQUERYDISPLAYATTRIBEXTPROC) load(userptr, "eglQueryDisplayAttribEXT");
}
static void glad_egl_load_EGL_EXT_device_enumeration( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_device_enumeration) return;
    glad_eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC) load(userptr, "eglQueryDevicesEXT");
}
static void glad_egl_load_EGL_EXT_device_persistent_id( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_device_persistent_id) return;
    glad_eglQueryDeviceBinaryEXT = (PFNEGLQUERYDEVICEBINARYEXTPROC) load(userptr, "eglQueryDeviceBinaryEXT");
}
static void glad_egl_load_EGL_EXT_device_query( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_device_query) return;
    glad_eglQueryDeviceAttribEXT = (PFNEGLQUERYDEVICEATTRIBEXTPROC) load(userptr, "eglQueryDeviceAttribEXT");
    glad_eglQueryDeviceStringEXT = (PFNEGLQUERYDEVICESTRINGEXTPROC) load(userptr, "eglQueryDeviceStringEXT");
    glad_eglQueryDisplayAttribEXT = (PFNEGLQUERYDISPLAYATTRIBEXTPROC) load(userptr, "eglQueryDisplayAttribEXT");
}
static void glad_egl_load_EGL_EXT_display_alloc( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_display_alloc) return;
    glad_eglDestroyDisplayEXT = (PFNEGLDESTROYDISPLAYEXTPROC) load(userptr, "eglDestroyDisplayEXT");
}
static void glad_egl_load_EGL_EXT_image_dma_buf_import_modifiers( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_image_dma_buf_import_modifiers) return;
    glad_eglQueryDmaBufFormatsEXT = (PFNEGLQUERYDMABUFFORMATSEXTPROC) load(userptr, "eglQueryDmaBufFormatsEXT");
    glad_eglQueryDmaBufModifiersEXT = (PFNEGLQUERYDMABUFMODIFIERSEXTPROC) load(userptr, "eglQueryDmaBufModifiersEXT");
}
static void glad_egl_load_EGL_EXT_output_base( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_output_base) return;
    glad_eglGetOutputLayersEXT = (PFNEGLGETOUTPUTLAYERSEXTPROC) load(userptr, "eglGetOutputLayersEXT");
    glad_eglGetOutputPortsEXT = (PFNEGLGETOUTPUTPORTSEXTPROC) load(userptr, "eglGetOutputPortsEXT");
    glad_eglOutputLayerAttribEXT = (PFNEGLOUTPUTLAYERATTRIBEXTPROC) load(userptr, "eglOutputLayerAttribEXT");
    glad_eglOutputPortAttribEXT = (PFNEGLOUTPUTPORTATTRIBEXTPROC) load(userptr, "eglOutputPortAttribEXT");
    glad_eglQueryOutputLayerAttribEXT = (PFNEGLQUERYOUTPUTLAYERATTRIBEXTPROC) load(userptr, "eglQueryOutputLayerAttribEXT");
    glad_eglQueryOutputLayerStringEXT = (PFNEGLQUERYOUTPUTLAYERSTRINGEXTPROC) load(userptr, "eglQueryOutputLayerStringEXT");
    glad_eglQueryOutputPortAttribEXT = (PFNEGLQUERYOUTPUTPORTATTRIBEXTPROC) load(userptr, "eglQueryOutputPortAttribEXT");
    glad_eglQueryOutputPortStringEXT = (PFNEGLQUERYOUTPUTPORTSTRINGEXTPROC) load(userptr, "eglQueryOutputPortStringEXT");
}
static void glad_egl_load_EGL_EXT_platform_base( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_platform_base) return;
    glad_eglCreatePlatformPixmapSurfaceEXT = (PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC) load(userptr, "eglCreatePlatformPixmapSurfaceEXT");
    glad_eglCreatePlatformWindowSurfaceEXT = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) load(userptr, "eglCreatePlatformWindowSurfaceEXT");
    glad_eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC) load(userptr, "eglGetPlatformDisplayEXT");
}
static void glad_egl_load_EGL_EXT_stream_consumer_egloutput( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_stream_consumer_egloutput) return;
    glad_eglStreamConsumerOutputEXT = (PFNEGLSTREAMCONSUMEROUTPUTEXTPROC) load(userptr, "eglStreamConsumerOutputEXT");
}
static void glad_egl_load_EGL_EXT_surface_compression( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_surface_compression) return;
    glad_eglQuerySupportedCompressionRatesEXT = (PFNEGLQUERYSUPPORTEDCOMPRESSIONRATESEXTPROC) load(userptr, "eglQuerySupportedCompressionRatesEXT");
}
static void glad_egl_load_EGL_EXT_swap_buffers_with_damage( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_swap_buffers_with_damage) return;
    glad_eglSwapBuffersWithDamageEXT = (PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC) load(userptr, "eglSwapBuffersWithDamageEXT");
}
static void glad_egl_load_EGL_EXT_sync_reuse( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_EXT_sync_reuse) return;
    glad_eglUnsignalSyncEXT = (PFNEGLUNSIGNALSYNCEXTPROC) load(userptr, "eglUnsignalSyncEXT");
}
static void glad_egl_load_EGL_KHR_cl_event2( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_cl_event2) return;
    glad_eglCreateSync64KHR = (PFNEGLCREATESYNC64KHRPROC) load(userptr, "eglCreateSync64KHR");
}
static void glad_egl_load_EGL_KHR_debug( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_debug) return;
    glad_eglDebugMessageControlKHR = (PFNEGLDEBUGMESSAGECONTROLKHRPROC) load(userptr, "eglDebugMessageControlKHR");
    glad_eglLabelObjectKHR = (PFNEGLLABELOBJECTKHRPROC) load(userptr, "eglLabelObjectKHR");
    glad_eglQueryDebugKHR = (PFNEGLQUERYDEBUGKHRPROC) load(userptr, "eglQueryDebugKHR");
}
static void glad_egl_load_EGL_KHR_display_reference( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_display_reference) return;
    glad_eglQueryDisplayAttribKHR = (PFNEGLQUERYDISPLAYATTRIBKHRPROC) load(userptr, "eglQueryDisplayAttribKHR");
}
static void glad_egl_load_EGL_KHR_fence_sync( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_fence_sync) return;
    glad_eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC) load(userptr, "eglClientWaitSyncKHR");
    glad_eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC) load(userptr, "eglCreateSyncKHR");
    glad_eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC) load(userptr, "eglDestroySyncKHR");
    glad_eglGetSyncAttribKHR = (PFNEGLGETSYNCATTRIBKHRPROC) load(userptr, "eglGetSyncAttribKHR");
}
static void glad_egl_load_EGL_KHR_image( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_image) return;
    glad_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) load(userptr, "eglCreateImageKHR");
    glad_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) load(userptr, "eglDestroyImageKHR");
}
static void glad_egl_load_EGL_KHR_image_base( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_image_base) return;
    glad_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) load(userptr, "eglCreateImageKHR");
    glad_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) load(userptr, "eglDestroyImageKHR");
}
static void glad_egl_load_EGL_KHR_lock_surface( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_lock_surface) return;
    glad_eglLockSurfaceKHR = (PFNEGLLOCKSURFACEKHRPROC) load(userptr, "eglLockSurfaceKHR");
    glad_eglUnlockSurfaceKHR = (PFNEGLUNLOCKSURFACEKHRPROC) load(userptr, "eglUnlockSurfaceKHR");
}
static void glad_egl_load_EGL_KHR_lock_surface3( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_lock_surface3) return;
    glad_eglLockSurfaceKHR = (PFNEGLLOCKSURFACEKHRPROC) load(userptr, "eglLockSurfaceKHR");
    glad_eglQuerySurface64KHR = (PFNEGLQUERYSURFACE64KHRPROC) load(userptr, "eglQuerySurface64KHR");
    glad_eglUnlockSurfaceKHR = (PFNEGLUNLOCKSURFACEKHRPROC) load(userptr, "eglUnlockSurfaceKHR");
}
static void glad_egl_load_EGL_KHR_partial_update( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_partial_update) return;
    glad_eglSetDamageRegionKHR = (PFNEGLSETDAMAGEREGIONKHRPROC) load(userptr, "eglSetDamageRegionKHR");
}
static void glad_egl_load_EGL_KHR_reusable_sync( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_reusable_sync) return;
    glad_eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC) load(userptr, "eglClientWaitSyncKHR");
    glad_eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC) load(userptr, "eglCreateSyncKHR");
    glad_eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC) load(userptr, "eglDestroySyncKHR");
    glad_eglGetSyncAttribKHR = (PFNEGLGETSYNCATTRIBKHRPROC) load(userptr, "eglGetSyncAttribKHR");
    glad_eglSignalSyncKHR = (PFNEGLSIGNALSYNCKHRPROC) load(userptr, "eglSignalSyncKHR");
}
static void glad_egl_load_EGL_KHR_stream( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_stream) return;
    glad_eglCreateStreamKHR = (PFNEGLCREATESTREAMKHRPROC) load(userptr, "eglCreateStreamKHR");
    glad_eglDestroyStreamKHR = (PFNEGLDESTROYSTREAMKHRPROC) load(userptr, "eglDestroyStreamKHR");
    glad_eglQueryStreamKHR = (PFNEGLQUERYSTREAMKHRPROC) load(userptr, "eglQueryStreamKHR");
    glad_eglQueryStreamu64KHR = (PFNEGLQUERYSTREAMU64KHRPROC) load(userptr, "eglQueryStreamu64KHR");
    glad_eglStreamAttribKHR = (PFNEGLSTREAMATTRIBKHRPROC) load(userptr, "eglStreamAttribKHR");
}
static void glad_egl_load_EGL_KHR_stream_attrib( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_stream_attrib) return;
    glad_eglCreateStreamAttribKHR = (PFNEGLCREATESTREAMATTRIBKHRPROC) load(userptr, "eglCreateStreamAttribKHR");
    glad_eglQueryStreamAttribKHR = (PFNEGLQUERYSTREAMATTRIBKHRPROC) load(userptr, "eglQueryStreamAttribKHR");
    glad_eglSetStreamAttribKHR = (PFNEGLSETSTREAMATTRIBKHRPROC) load(userptr, "eglSetStreamAttribKHR");
    glad_eglStreamConsumerAcquireAttribKHR = (PFNEGLSTREAMCONSUMERACQUIREATTRIBKHRPROC) load(userptr, "eglStreamConsumerAcquireAttribKHR");
    glad_eglStreamConsumerReleaseAttribKHR = (PFNEGLSTREAMCONSUMERRELEASEATTRIBKHRPROC) load(userptr, "eglStreamConsumerReleaseAttribKHR");
}
static void glad_egl_load_EGL_KHR_stream_consumer_gltexture( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_stream_consumer_gltexture) return;
    glad_eglStreamConsumerAcquireKHR = (PFNEGLSTREAMCONSUMERACQUIREKHRPROC) load(userptr, "eglStreamConsumerAcquireKHR");
    glad_eglStreamConsumerGLTextureExternalKHR = (PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALKHRPROC) load(userptr, "eglStreamConsumerGLTextureExternalKHR");
    glad_eglStreamConsumerReleaseKHR = (PFNEGLSTREAMCONSUMERRELEASEKHRPROC) load(userptr, "eglStreamConsumerReleaseKHR");
}
static void glad_egl_load_EGL_KHR_stream_cross_process_fd( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_stream_cross_process_fd) return;
    glad_eglCreateStreamFromFileDescriptorKHR = (PFNEGLCREATESTREAMFROMFILEDESCRIPTORKHRPROC) load(userptr, "eglCreateStreamFromFileDescriptorKHR");
    glad_eglGetStreamFileDescriptorKHR = (PFNEGLGETSTREAMFILEDESCRIPTORKHRPROC) load(userptr, "eglGetStreamFileDescriptorKHR");
}
static void glad_egl_load_EGL_KHR_stream_fifo( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_stream_fifo) return;
    glad_eglQueryStreamTimeKHR = (PFNEGLQUERYSTREAMTIMEKHRPROC) load(userptr, "eglQueryStreamTimeKHR");
}
static void glad_egl_load_EGL_KHR_stream_producer_eglsurface( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_stream_producer_eglsurface) return;
    glad_eglCreateStreamProducerSurfaceKHR = (PFNEGLCREATESTREAMPRODUCERSURFACEKHRPROC) load(userptr, "eglCreateStreamProducerSurfaceKHR");
}
static void glad_egl_load_EGL_KHR_swap_buffers_with_damage( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_swap_buffers_with_damage) return;
    glad_eglSwapBuffersWithDamageKHR = (PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC) load(userptr, "eglSwapBuffersWithDamageKHR");
}
static void glad_egl_load_EGL_KHR_wait_sync( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_EGL_KHR_wait_sync) return;
    glad_eglWaitSyncKHR = (PFNEGLWAITSYNCKHRPROC) load(userptr, "eglWaitSyncKHR");
}



static int glad_egl_get_extensions(EGLDisplay display, const char **extensions) {
    *extensions = eglQueryString(display, EGL_EXTENSIONS);

    return extensions != NULL;
}

static int glad_egl_has_extension(const char *extensions, const char *ext) {
    const char *loc;
    const char *terminator;
    if(extensions == NULL) {
        return 0;
    }
    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL) {
            return 0;
        }
        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0')) {
            return 1;
        }
        extensions = terminator;
    }
}

static GLADapiproc glad_egl_get_proc_from_userptr(void *userptr, const char *name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_egl_find_extensions_egl(EGLDisplay display) {
    const char *extensions;
    if (!glad_egl_get_extensions(display, &extensions)) return 0;

    GLAD_EGL_ANGLE_d3d_share_handle_client_buffer = glad_egl_has_extension(extensions, "EGL_ANGLE_d3d_share_handle_client_buffer");
    GLAD_EGL_ANGLE_device_d3d = glad_egl_has_extension(extensions, "EGL_ANGLE_device_d3d");
    GLAD_EGL_ANGLE_query_surface_pointer = glad_egl_has_extension(extensions, "EGL_ANGLE_query_surface_pointer");
    GLAD_EGL_ANGLE_surface_d3d_texture_2d_share_handle = glad_egl_has_extension(extensions, "EGL_ANGLE_surface_d3d_texture_2d_share_handle");
    GLAD_EGL_ANGLE_sync_control_rate = glad_egl_has_extension(extensions, "EGL_ANGLE_sync_control_rate");
    GLAD_EGL_ANGLE_window_fixed_size = glad_egl_has_extension(extensions, "EGL_ANGLE_window_fixed_size");
    GLAD_EGL_EXT_bind_to_front = glad_egl_has_extension(extensions, "EGL_EXT_bind_to_front");
    GLAD_EGL_EXT_buffer_age = glad_egl_has_extension(extensions, "EGL_EXT_buffer_age");
    GLAD_EGL_EXT_client_extensions = glad_egl_has_extension(extensions, "EGL_EXT_client_extensions");
    GLAD_EGL_EXT_client_sync = glad_egl_has_extension(extensions, "EGL_EXT_client_sync");
    GLAD_EGL_EXT_compositor = glad_egl_has_extension(extensions, "EGL_EXT_compositor");
    GLAD_EGL_EXT_config_select_group = glad_egl_has_extension(extensions, "EGL_EXT_config_select_group");
    GLAD_EGL_EXT_create_context_robustness = glad_egl_has_extension(extensions, "EGL_EXT_create_context_robustness");
    GLAD_EGL_EXT_device_base = glad_egl_has_extension(extensions, "EGL_EXT_device_base");
    GLAD_EGL_EXT_device_drm = glad_egl_has_extension(extensions, "EGL_EXT_device_drm");
    GLAD_EGL_EXT_device_drm_render_node = glad_egl_has_extension(extensions, "EGL_EXT_device_drm_render_node");
    GLAD_EGL_EXT_device_enumeration = glad_egl_has_extension(extensions, "EGL_EXT_device_enumeration");
    GLAD_EGL_EXT_device_openwf = glad_egl_has_extension(extensions, "EGL_EXT_device_openwf");
    GLAD_EGL_EXT_device_persistent_id = glad_egl_has_extension(extensions, "EGL_EXT_device_persistent_id");
    GLAD_EGL_EXT_device_query = glad_egl_has_extension(extensions, "EGL_EXT_device_query");
    GLAD_EGL_EXT_device_query_name = glad_egl_has_extension(extensions, "EGL_EXT_device_query_name");
    GLAD_EGL_EXT_display_alloc = glad_egl_has_extension(extensions, "EGL_EXT_display_alloc");
    GLAD_EGL_EXT_explicit_device = glad_egl_has_extension(extensions, "EGL_EXT_explicit_device");
    GLAD_EGL_EXT_gl_colorspace_bt2020_hlg = glad_egl_has_extension(extensions, "EGL_EXT_gl_colorspace_bt2020_hlg");
    GLAD_EGL_EXT_gl_colorspace_bt2020_linear = glad_egl_has_extension(extensions, "EGL_EXT_gl_colorspace_bt2020_linear");
    GLAD_EGL_EXT_gl_colorspace_bt2020_pq = glad_egl_has_extension(extensions, "EGL_EXT_gl_colorspace_bt2020_pq");
    GLAD_EGL_EXT_gl_colorspace_display_p3 = glad_egl_has_extension(extensions, "EGL_EXT_gl_colorspace_display_p3");
    GLAD_EGL_EXT_gl_colorspace_display_p3_linear = glad_egl_has_extension(extensions, "EGL_EXT_gl_colorspace_display_p3_linear");
    GLAD_EGL_EXT_gl_colorspace_display_p3_passthrough = glad_egl_has_extension(extensions, "EGL_EXT_gl_colorspace_display_p3_passthrough");
    GLAD_EGL_EXT_gl_colorspace_scrgb = glad_egl_has_extension(extensions, "EGL_EXT_gl_colorspace_scrgb");
    GLAD_EGL_EXT_gl_colorspace_scrgb_linear = glad_egl_has_extension(extensions, "EGL_EXT_gl_colorspace_scrgb_linear");
    GLAD_EGL_EXT_image_dma_buf_import = glad_egl_has_extension(extensions, "EGL_EXT_image_dma_buf_import");
    GLAD_EGL_EXT_image_dma_buf_import_modifiers = glad_egl_has_extension(extensions, "EGL_EXT_image_dma_buf_import_modifiers");
    GLAD_EGL_EXT_image_gl_colorspace = glad_egl_has_extension(extensions, "EGL_EXT_image_gl_colorspace");
    GLAD_EGL_EXT_image_implicit_sync_control = glad_egl_has_extension(extensions, "EGL_EXT_image_implicit_sync_control");
    GLAD_EGL_EXT_multiview_window = glad_egl_has_extension(extensions, "EGL_EXT_multiview_window");
    GLAD_EGL_EXT_output_base = glad_egl_has_extension(extensions, "EGL_EXT_output_base");
    GLAD_EGL_EXT_output_drm = glad_egl_has_extension(extensions, "EGL_EXT_output_drm");
    GLAD_EGL_EXT_output_openwf = glad_egl_has_extension(extensions, "EGL_EXT_output_openwf");
    GLAD_EGL_EXT_pixel_format_float = glad_egl_has_extension(extensions, "EGL_EXT_pixel_format_float");
    GLAD_EGL_EXT_platform_base = glad_egl_has_extension(extensions, "EGL_EXT_platform_base");
    GLAD_EGL_EXT_platform_device = glad_egl_has_extension(extensions, "EGL_EXT_platform_device");
    GLAD_EGL_EXT_platform_wayland = glad_egl_has_extension(extensions, "EGL_EXT_platform_wayland");
    GLAD_EGL_EXT_platform_x11 = glad_egl_has_extension(extensions, "EGL_EXT_platform_x11");
    GLAD_EGL_EXT_platform_xcb = glad_egl_has_extension(extensions, "EGL_EXT_platform_xcb");
    GLAD_EGL_EXT_present_opaque = glad_egl_has_extension(extensions, "EGL_EXT_present_opaque");
    GLAD_EGL_EXT_protected_content = glad_egl_has_extension(extensions, "EGL_EXT_protected_content");
    GLAD_EGL_EXT_protected_surface = glad_egl_has_extension(extensions, "EGL_EXT_protected_surface");
    GLAD_EGL_EXT_query_reset_notification_strategy = glad_egl_has_extension(extensions, "EGL_EXT_query_reset_notification_strategy");
    GLAD_EGL_EXT_stream_consumer_egloutput = glad_egl_has_extension(extensions, "EGL_EXT_stream_consumer_egloutput");
    GLAD_EGL_EXT_surface_CTA861_3_metadata = glad_egl_has_extension(extensions, "EGL_EXT_surface_CTA861_3_metadata");
    GLAD_EGL_EXT_surface_SMPTE2086_metadata = glad_egl_has_extension(extensions, "EGL_EXT_surface_SMPTE2086_metadata");
    GLAD_EGL_EXT_surface_compression = glad_egl_has_extension(extensions, "EGL_EXT_surface_compression");
    GLAD_EGL_EXT_swap_buffers_with_damage = glad_egl_has_extension(extensions, "EGL_EXT_swap_buffers_with_damage");
    GLAD_EGL_EXT_sync_reuse = glad_egl_has_extension(extensions, "EGL_EXT_sync_reuse");
    GLAD_EGL_EXT_yuv_surface = glad_egl_has_extension(extensions, "EGL_EXT_yuv_surface");
    GLAD_EGL_IMG_context_priority = glad_egl_has_extension(extensions, "EGL_IMG_context_priority");
    GLAD_EGL_KHR_cl_event = glad_egl_has_extension(extensions, "EGL_KHR_cl_event");
    GLAD_EGL_KHR_cl_event2 = glad_egl_has_extension(extensions, "EGL_KHR_cl_event2");
    GLAD_EGL_KHR_client_get_all_proc_addresses = glad_egl_has_extension(extensions, "EGL_KHR_client_get_all_proc_addresses");
    GLAD_EGL_KHR_config_attribs = glad_egl_has_extension(extensions, "EGL_KHR_config_attribs");
    GLAD_EGL_KHR_context_flush_control = glad_egl_has_extension(extensions, "EGL_KHR_context_flush_control");
    GLAD_EGL_KHR_create_context = glad_egl_has_extension(extensions, "EGL_KHR_create_context");
    GLAD_EGL_KHR_create_context_no_error = glad_egl_has_extension(extensions, "EGL_KHR_create_context_no_error");
    GLAD_EGL_KHR_debug = glad_egl_has_extension(extensions, "EGL_KHR_debug");
    GLAD_EGL_KHR_display_reference = glad_egl_has_extension(extensions, "EGL_KHR_display_reference");
    GLAD_EGL_KHR_fence_sync = glad_egl_has_extension(extensions, "EGL_KHR_fence_sync");
    GLAD_EGL_KHR_get_all_proc_addresses = glad_egl_has_extension(extensions, "EGL_KHR_get_all_proc_addresses");
    GLAD_EGL_KHR_gl_colorspace = glad_egl_has_extension(extensions, "EGL_KHR_gl_colorspace");
    GLAD_EGL_KHR_gl_renderbuffer_image = glad_egl_has_extension(extensions, "EGL_KHR_gl_renderbuffer_image");
    GLAD_EGL_KHR_gl_texture_2D_image = glad_egl_has_extension(extensions, "EGL_KHR_gl_texture_2D_image");
    GLAD_EGL_KHR_gl_texture_3D_image = glad_egl_has_extension(extensions, "EGL_KHR_gl_texture_3D_image");
    GLAD_EGL_KHR_gl_texture_cubemap_image = glad_egl_has_extension(extensions, "EGL_KHR_gl_texture_cubemap_image");
    GLAD_EGL_KHR_image = glad_egl_has_extension(extensions, "EGL_KHR_image");
    GLAD_EGL_KHR_image_base = glad_egl_has_extension(extensions, "EGL_KHR_image_base");
    GLAD_EGL_KHR_image_pixmap = glad_egl_has_extension(extensions, "EGL_KHR_image_pixmap");
    GLAD_EGL_KHR_lock_surface = glad_egl_has_extension(extensions, "EGL_KHR_lock_surface");
    GLAD_EGL_KHR_lock_surface2 = glad_egl_has_extension(extensions, "EGL_KHR_lock_surface2");
    GLAD_EGL_KHR_lock_surface3 = glad_egl_has_extension(extensions, "EGL_KHR_lock_surface3");
    GLAD_EGL_KHR_mutable_render_buffer = glad_egl_has_extension(extensions, "EGL_KHR_mutable_render_buffer");
    GLAD_EGL_KHR_no_config_context = glad_egl_has_extension(extensions, "EGL_KHR_no_config_context");
    GLAD_EGL_KHR_partial_update = glad_egl_has_extension(extensions, "EGL_KHR_partial_update");
    GLAD_EGL_KHR_platform_android = glad_egl_has_extension(extensions, "EGL_KHR_platform_android");
    GLAD_EGL_KHR_platform_gbm = glad_egl_has_extension(extensions, "EGL_KHR_platform_gbm");
    GLAD_EGL_KHR_platform_wayland = glad_egl_has_extension(extensions, "EGL_KHR_platform_wayland");
    GLAD_EGL_KHR_platform_x11 = glad_egl_has_extension(extensions, "EGL_KHR_platform_x11");
    GLAD_EGL_KHR_reusable_sync = glad_egl_has_extension(extensions, "EGL_KHR_reusable_sync");
    GLAD_EGL_KHR_stream = glad_egl_has_extension(extensions, "EGL_KHR_stream");
    GLAD_EGL_KHR_stream_attrib = glad_egl_has_extension(extensions, "EGL_KHR_stream_attrib");
    GLAD_EGL_KHR_stream_consumer_gltexture = glad_egl_has_extension(extensions, "EGL_KHR_stream_consumer_gltexture");
    GLAD_EGL_KHR_stream_cross_process_fd = glad_egl_has_extension(extensions, "EGL_KHR_stream_cross_process_fd");
    GLAD_EGL_KHR_stream_fifo = glad_egl_has_extension(extensions, "EGL_KHR_stream_fifo");
    GLAD_EGL_KHR_stream_producer_aldatalocator = glad_egl_has_extension(extensions, "EGL_KHR_stream_producer_aldatalocator");
    GLAD_EGL_KHR_stream_producer_eglsurface = glad_egl_has_extension(extensions, "EGL_KHR_stream_producer_eglsurface");
    GLAD_EGL_KHR_surfaceless_context = glad_egl_has_extension(extensions, "EGL_KHR_surfaceless_context");
    GLAD_EGL_KHR_swap_buffers_with_damage = glad_egl_has_extension(extensions, "EGL_KHR_swap_buffers_with_damage");
    GLAD_EGL_KHR_vg_parent_image = glad_egl_has_extension(extensions, "EGL_KHR_vg_parent_image");
    GLAD_EGL_KHR_wait_sync = glad_egl_has_extension(extensions, "EGL_KHR_wait_sync");
    GLAD_EGL_NV_context_priority_realtime = glad_egl_has_extension(extensions, "EGL_NV_context_priority_realtime");

    return 1;
}

static int glad_egl_find_core_egl(EGLDisplay display) {
    int major, minor;
    const char *version;

    if (display == NULL) {
        display = EGL_NO_DISPLAY; /* this is usually NULL, better safe than sorry */
    }
    if (display == EGL_NO_DISPLAY) {
        display = eglGetCurrentDisplay();
    }
#ifdef EGL_VERSION_1_4
    if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
#endif
#ifndef EGL_VERSION_1_5
    if (display == EGL_NO_DISPLAY) {
        return 0;
    }
#endif

    version = eglQueryString(display, EGL_VERSION);
    (void) eglGetError();

    if (version == NULL) {
        major = 1;
        minor = 0;
    } else {
        GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);
    }

    GLAD_EGL_VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
    GLAD_EGL_VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
    GLAD_EGL_VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
    GLAD_EGL_VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
    GLAD_EGL_VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
    GLAD_EGL_VERSION_1_5 = (major == 1 && minor >= 5) || major > 1;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadEGLUserPtr(EGLDisplay display, GLADuserptrloadfunc load, void* userptr) {
    int version;
    eglGetDisplay = (PFNEGLGETDISPLAYPROC) load(userptr, "eglGetDisplay");
    eglGetCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC) load(userptr, "eglGetCurrentDisplay");
    eglQueryString = (PFNEGLQUERYSTRINGPROC) load(userptr, "eglQueryString");
    eglGetError = (PFNEGLGETERRORPROC) load(userptr, "eglGetError");
    if (eglGetDisplay == NULL || eglGetCurrentDisplay == NULL || eglQueryString == NULL || eglGetError == NULL) return 0;

    version = glad_egl_find_core_egl(display);
    if (!version) return 0;
    glad_egl_load_EGL_VERSION_1_0(load, userptr);
    glad_egl_load_EGL_VERSION_1_1(load, userptr);
    glad_egl_load_EGL_VERSION_1_2(load, userptr);
    glad_egl_load_EGL_VERSION_1_4(load, userptr);
    glad_egl_load_EGL_VERSION_1_5(load, userptr);

    if (!glad_egl_find_extensions_egl(display)) return 0;
    glad_egl_load_EGL_ANGLE_query_surface_pointer(load, userptr);
    glad_egl_load_EGL_ANGLE_sync_control_rate(load, userptr);
    glad_egl_load_EGL_EXT_client_sync(load, userptr);
    glad_egl_load_EGL_EXT_compositor(load, userptr);
    glad_egl_load_EGL_EXT_device_base(load, userptr);
    glad_egl_load_EGL_EXT_device_enumeration(load, userptr);
    glad_egl_load_EGL_EXT_device_persistent_id(load, userptr);
    glad_egl_load_EGL_EXT_device_query(load, userptr);
    glad_egl_load_EGL_EXT_display_alloc(load, userptr);
    glad_egl_load_EGL_EXT_image_dma_buf_import_modifiers(load, userptr);
    glad_egl_load_EGL_EXT_output_base(load, userptr);
    glad_egl_load_EGL_EXT_platform_base(load, userptr);
    glad_egl_load_EGL_EXT_stream_consumer_egloutput(load, userptr);
    glad_egl_load_EGL_EXT_surface_compression(load, userptr);
    glad_egl_load_EGL_EXT_swap_buffers_with_damage(load, userptr);
    glad_egl_load_EGL_EXT_sync_reuse(load, userptr);
    glad_egl_load_EGL_KHR_cl_event2(load, userptr);
    glad_egl_load_EGL_KHR_debug(load, userptr);
    glad_egl_load_EGL_KHR_display_reference(load, userptr);
    glad_egl_load_EGL_KHR_fence_sync(load, userptr);
    glad_egl_load_EGL_KHR_image(load, userptr);
    glad_egl_load_EGL_KHR_image_base(load, userptr);
    glad_egl_load_EGL_KHR_lock_surface(load, userptr);
    glad_egl_load_EGL_KHR_lock_surface3(load, userptr);
    glad_egl_load_EGL_KHR_partial_update(load, userptr);
    glad_egl_load_EGL_KHR_reusable_sync(load, userptr);
    glad_egl_load_EGL_KHR_stream(load, userptr);
    glad_egl_load_EGL_KHR_stream_attrib(load, userptr);
    glad_egl_load_EGL_KHR_stream_consumer_gltexture(load, userptr);
    glad_egl_load_EGL_KHR_stream_cross_process_fd(load, userptr);
    glad_egl_load_EGL_KHR_stream_fifo(load, userptr);
    glad_egl_load_EGL_KHR_stream_producer_eglsurface(load, userptr);
    glad_egl_load_EGL_KHR_swap_buffers_with_damage(load, userptr);
    glad_egl_load_EGL_KHR_wait_sync(load, userptr);


    return version;
}

int gladLoadEGL(EGLDisplay display, GLADloadfunc load) {
    return gladLoadEGLUserPtr(display, glad_egl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

 

#ifdef GLAD_EGL

#ifndef GLAD_LOADER_LIBRARY_C_
#define GLAD_LOADER_LIBRARY_C_

#include <stddef.h>
#include <stdlib.h>

#if GLAD_PLATFORM_WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif


static void* glad_get_dlopen_handle(const char *lib_names[], int length) {
    void *handle = NULL;
    int i;

    for (i = 0; i < length; ++i) {
#if GLAD_PLATFORM_WIN32
  #if GLAD_PLATFORM_UWP
        size_t buffer_size = (strlen(lib_names[i]) + 1) * sizeof(WCHAR);
        LPWSTR buffer = (LPWSTR) malloc(buffer_size);
        if (buffer != NULL) {
            int ret = MultiByteToWideChar(CP_ACP, 0, lib_names[i], -1, buffer, buffer_size);
            if (ret != 0) {
                handle = (void*) LoadPackagedLibrary(buffer, 0);
            }
            free((void*) buffer);
        }
  #else
        handle = (void*) LoadLibraryA(lib_names[i]);
  #endif
#else
        handle = dlopen(lib_names[i], RTLD_LAZY | RTLD_LOCAL);
#endif
        if (handle != NULL) {
            return handle;
        }
    }

    return NULL;
}

static void glad_close_dlopen_handle(void* handle) {
    if (handle != NULL) {
#if GLAD_PLATFORM_WIN32
        FreeLibrary((HMODULE) handle);
#else
        dlclose(handle);
#endif
    }
}

static GLADapiproc glad_dlsym_handle(void* handle, const char *name) {
    if (handle == NULL) {
        return NULL;
    }

#if GLAD_PLATFORM_WIN32
    return (GLADapiproc) GetProcAddress((HMODULE) handle, name);
#else
    return GLAD_GNUC_EXTENSION (GLADapiproc) dlsym(handle, name);
#endif
}

#endif /* GLAD_LOADER_LIBRARY_C_ */

struct _glad_egl_userptr {
    void *handle;
    PFNEGLGETPROCADDRESSPROC get_proc_address_ptr;
};

static GLADapiproc glad_egl_get_proc(void *vuserptr, const char* name) {
    struct _glad_egl_userptr userptr = *(struct _glad_egl_userptr*) vuserptr;
    GLADapiproc result = NULL;

    result = glad_dlsym_handle(userptr.handle, name);
    if (result == NULL) {
        result = GLAD_GNUC_EXTENSION (GLADapiproc) userptr.get_proc_address_ptr(name);
    }

    return result;
}

static void* _egl_handle = NULL;

static void* glad_egl_dlopen_handle(void) {
#if GLAD_PLATFORM_APPLE
    static const char *NAMES[] = {"libEGL.dylib"};
#elif GLAD_PLATFORM_WIN32
    static const char *NAMES[] = {"libEGL.dll", "EGL.dll"};
#else
    static const char *NAMES[] = {"libEGL.so.1", "libEGL.so"};
#endif

    if (_egl_handle == NULL) {
        _egl_handle = glad_get_dlopen_handle(NAMES, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    return _egl_handle;
}

static struct _glad_egl_userptr glad_egl_build_userptr(void *handle) {
    struct _glad_egl_userptr userptr;
    userptr.handle = handle;
    userptr.get_proc_address_ptr = (PFNEGLGETPROCADDRESSPROC) glad_dlsym_handle(handle, "eglGetProcAddress");
    return userptr;
}

int gladLoaderLoadEGL(EGLDisplay display) {
    int version = 0;
    void *handle = NULL;
    int did_load = 0;
    struct _glad_egl_userptr userptr;

    did_load = _egl_handle == NULL;
    handle = glad_egl_dlopen_handle();
    if (handle != NULL) {
        userptr = glad_egl_build_userptr(handle);

        if (userptr.get_proc_address_ptr != NULL) {
            version = gladLoadEGLUserPtr(display, glad_egl_get_proc, &userptr);
        }

        if (!version && did_load) {
            gladLoaderUnloadEGL();
        }
    }

    return version;
}


void gladLoaderUnloadEGL(void) {
    if (_egl_handle != NULL) {
        glad_close_dlopen_handle(_egl_handle);
        _egl_handle = NULL;
    }
}

#endif /* GLAD_EGL */

#ifdef __cplusplus
}
#endif
