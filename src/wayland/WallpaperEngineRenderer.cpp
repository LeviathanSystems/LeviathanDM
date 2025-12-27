#include "wayland/WallpaperEngineRenderer.hpp"
#include "wayland/WaylandTypes.hpp"
#include "Logger.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glew.h>
#include <cstring>
#include <filesystem>

// Note: WallpaperEngine includes are commented out until linux-wallpaperengine is built as a library
// #include "WallpaperEngine/Application/ApplicationContext.h"
// #include "WallpaperEngine/Application/WallpaperApplication.h"
// #include "WallpaperEngine/Render/CWallpaper.h"
// #include "WallpaperEngine/FileSystem/FileSystem.h"

// Forward declare WallpaperEngine types for now
namespace WallpaperEngine {
namespace Application {
    class ApplicationContext;
    class WallpaperApplication;
}
namespace Render {
    class CWallpaper;
}
}

namespace Leviathan {
namespace Wayland {

WallpaperEngineRenderer::WallpaperEngineRenderer(struct wlr_scene_tree* background_layer,
                                                 struct wl_event_loop* event_loop,
                                                 struct wlr_renderer* renderer,
                                                 int width, int height)
    : background_layer_(background_layer)
    , event_loop_(event_loop)
    , wlr_renderer_(renderer)
    , width_(width)
    , height_(height) {
    LOG_DEBUG_FMT("Created WallpaperEngineRenderer with dimensions: {}x{}", width, height);
    
    // Initialize EGL for offscreen rendering
    if (!InitializeEGL()) {
        LOG_ERROR("Failed to initialize EGL context for WallpaperEngine");
        return;
    }
    
    LOG_INFO("WallpaperEngine renderer initialized successfully");
}

WallpaperEngineRenderer::~WallpaperEngineRenderer() {
    UnloadWallpaper();
    CleanupEGL();
    LOG_DEBUG("Destroyed WallpaperEngineRenderer");
}

bool WallpaperEngineRenderer::LoadWallpaper(const std::string& project_path) {
    LOG_INFO_FMT("Loading Wallpaper Engine project: {}", project_path);
    
    // Verify project path exists
    if (!std::filesystem::exists(project_path)) {
        LOG_ERROR_FMT("WallpaperEngine project path does not exist: {}", project_path);
        return false;
    }
    
    // Check for project.json
    std::filesystem::path project_json = std::filesystem::path(project_path) / "project.json";
    if (!std::filesystem::exists(project_json)) {
        LOG_ERROR_FMT("No project.json found in: {}", project_path);
        return false;
    }
    
    try {
        // TODO: Uncomment when linux-wallpaperengine is built as a library
        /*
        // Create WallpaperEngine application context
        // Note: This is a simplified version. Full implementation would need proper CLI args
        std::vector<std::string> args = {
            "wallpaperengine",
            "--silent",
            "--no-fullscreen-pause",
            "--screen-root", "default",
            "--background", project_path
        };
        
        we_context_ = std::make_unique<WallpaperEngine::Application::ApplicationContext>(args);
        
        if (!we_context_) {
            LOG_ERROR("Failed to create WallpaperEngine application context");
            return false;
        }
        
        // Create WallpaperEngine application
        we_app_ = std::make_unique<WallpaperEngine::Application::WallpaperApplication>(*we_context_);
        
        if (!we_app_) {
            LOG_ERROR("Failed to create WallpaperEngine application");
            we_context_.reset();
            return false;
        }
        
        // Initialize the application (loads the wallpaper)
        // Note: We need to adapt this to not create actual windows
        // The actual implementation would need custom video driver integration
        
        LOG_INFO("WallpaperEngine application initialized");
        */
        
        // For now, just log that we would load it
        LOG_WARN("WallpaperEngine loading is stubbed - linux-wallpaperengine needs to be built as a library first");
        LOG_INFO_FMT("Would load WallpaperEngine project from: {}", project_path);
        
        // Create render target for offscreen rendering
        if (!CreateRenderTarget()) {
            LOG_ERROR("Failed to create render target");
            we_app_.reset();
            we_context_.reset();
            return false;
        }
        
        // Start frame rendering timer (60 FPS)
        frame_timer_ = wl_event_loop_add_timer(event_loop_, FrameCallback, this);
        if (frame_timer_) {
            wl_event_source_timer_update(frame_timer_, 16); // ~60 FPS
        }
        
        current_path_ = project_path;
        is_loaded_ = true;
        
        LOG_INFO_FMT("Successfully loaded WallpaperEngine wallpaper: {}", project_path);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("Exception loading WallpaperEngine wallpaper: {}", e.what());
        we_app_.reset();
        we_context_.reset();
        return false;
    }
}

void WallpaperEngineRenderer::UnloadWallpaper() {
    if (!is_loaded_) {
        return;
    }
    
    LOG_DEBUG("Unloading Wallpaper Engine wallpaper");
    
    // Remove frame timer
    if (frame_timer_) {
        wl_event_source_remove(frame_timer_);
        frame_timer_ = nullptr;
    }
    
    // Destroy scene node
    if (wallpaper_node_) {
        wlr_scene_node_destroy(wallpaper_node_);
        wallpaper_node_ = nullptr;
    }
    
    // Release wlroots buffer
    if (wallpaper_buffer_) {
        wlr_buffer_drop(wallpaper_buffer_);
        wallpaper_buffer_ = nullptr;
    }
    
    // Clean up WallpaperEngine application
    we_wallpaper_ = nullptr;
    we_app_.reset();
    we_context_.reset();
    
    // Clean up render target
    if (egl_context_.framebuffer) {
        glDeleteFramebuffers(1, &egl_context_.framebuffer);
        egl_context_.framebuffer = 0;
    }
    if (egl_context_.texture) {
        glDeleteTextures(1, &egl_context_.texture);
        egl_context_.texture = 0;
    }
    
    is_loaded_ = false;
    current_path_.clear();
}

void WallpaperEngineRenderer::RenderFrame() {
    if (!is_loaded_ || is_paused_ || !we_app_) {
        return;
    }
    
    // Make our EGL context current
    if (!eglMakeCurrent(egl_context_.display, egl_context_.surface, 
                        egl_context_.surface, egl_context_.context)) {
        LOG_ERROR("Failed to make EGL context current");
        return;
    }
    
    // Bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, egl_context_.framebuffer);
    
    // Clear the framebuffer
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render the WallpaperEngine wallpaper
    // Note: This is a simplified version - full implementation would need proper viewport handling
    try {
        // Update and render the wallpaper
        // The actual rendering would be done by WallpaperEngine's render system
        // we_app_->update(viewport);
        
        // For now, we render a placeholder or call the wallpaper's render method if available
        if (we_wallpaper_) {
            // we_wallpaper_->render(...);
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("Exception during WallpaperEngine render: {}", e.what());
    }
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Import the rendered texture into wlroots
    ImportTextureToWlroots();
    
    // Clear EGL context
    eglMakeCurrent(egl_context_.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void WallpaperEngineRenderer::SetPaused(bool paused) {
    if (is_paused_ == paused) {
        return;
    }
    
    is_paused_ = paused;
    LOG_DEBUG_FMT("WallpaperEngine renderer {}", paused ? "paused" : "resumed");
    
    // Pause/unpause WallpaperEngine rendering
    if (we_wallpaper_) {
        we_wallpaper_->setPause(paused);
    }
}

int WallpaperEngineRenderer::FrameCallback(void* data) {
    WallpaperEngineRenderer* renderer = static_cast<WallpaperEngineRenderer*>(data);
    renderer->RenderFrame();
    
    // Continue rendering at ~60 FPS
    return 16; // milliseconds
}

bool WallpaperEngineRenderer::InitializeEGL() {
    LOG_DEBUG("Initializing EGL context for WallpaperEngine offscreen rendering");
    
    // Get EGL display
    egl_context_.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_context_.display == EGL_NO_DISPLAY) {
        LOG_ERROR("Failed to get EGL display");
        return false;
    }
    
    // Initialize EGL
    EGLint major, minor;
    if (!eglInitialize(egl_context_.display, &major, &minor)) {
        LOG_ERROR("Failed to initialize EGL");
        return false;
    }
    
    LOG_DEBUG_FMT("EGL version: {}.{}", major, minor);
    
    // Choose EGL config
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    
    EGLint num_configs;
    if (!eglChooseConfig(egl_context_.display, config_attribs, &egl_context_.config, 1, &num_configs) || num_configs == 0) {
        LOG_ERROR("Failed to choose EGL config");
        eglTerminate(egl_context_.display);
        return false;
    }
    
    // Bind OpenGL API
    if (!eglBindAPI(EGL_OPENGL_API)) {
        LOG_ERROR("Failed to bind OpenGL API");
        eglTerminate(egl_context_.display);
        return false;
    }
    
    // Create EGL context
    EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    
    egl_context_.context = eglCreateContext(egl_context_.display, egl_context_.config, EGL_NO_CONTEXT, context_attribs);
    if (egl_context_.context == EGL_NO_CONTEXT) {
        LOG_ERROR("Failed to create EGL context");
        eglTerminate(egl_context_.display);
        return false;
    }
    
    // Create pbuffer surface for offscreen rendering
    EGLint pbuffer_attribs[] = {
        EGL_WIDTH, width_,
        EGL_HEIGHT, height_,
        EGL_NONE
    };
    
    egl_context_.surface = eglCreatePbufferSurface(egl_context_.display, egl_context_.config, pbuffer_attribs);
    if (egl_context_.surface == EGL_NO_SURFACE) {
        LOG_ERROR("Failed to create EGL pbuffer surface");
        eglDestroyContext(egl_context_.display, egl_context_.context);
        eglTerminate(egl_context_.display);
        return false;
    }
    
    // Make context current to initialize GLEW
    if (!eglMakeCurrent(egl_context_.display, egl_context_.surface, egl_context_.surface, egl_context_.context)) {
        LOG_ERROR("Failed to make EGL context current");
        eglDestroySurface(egl_context_.display, egl_context_.surface);
        eglDestroyContext(egl_context_.display, egl_context_.context);
        eglTerminate(egl_context_.display);
        return false;
    }
    
    // Initialize GLEW
    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK) {
        LOG_ERROR_FMT("Failed to initialize GLEW: {}", reinterpret_cast<const char*>(glewGetErrorString(glew_err)));
        eglMakeCurrent(egl_context_.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(egl_context_.display, egl_context_.surface);
        eglDestroyContext(egl_context_.display, egl_context_.context);
        eglTerminate(egl_context_.display);
        return false;
    }
    
    LOG_INFO_FMT("OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOG_INFO_FMT("GLSL version: {}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    
    // Clear context
    eglMakeCurrent(egl_context_.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    
    LOG_INFO("EGL context initialized successfully");
    return true;
}

bool WallpaperEngineRenderer::CreateRenderTarget() {
    LOG_DEBUG_FMT("Creating render target: {}x{}", width_, height_);
    
    // Make context current
    if (!eglMakeCurrent(egl_context_.display, egl_context_.surface, egl_context_.surface, egl_context_.context)) {
        LOG_ERROR("Failed to make EGL context current");
        return false;
    }
    
    // Create texture
    glGenTextures(1, &egl_context_.texture);
    glBindTexture(GL_TEXTURE_2D, egl_context_.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create framebuffer
    glGenFramebuffers(1, &egl_context_.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, egl_context_.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, egl_context_.texture, 0);
    
    // Check framebuffer status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR_FMT("Framebuffer incomplete: 0x{:x}", status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        eglMakeCurrent(egl_context_.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    eglMakeCurrent(egl_context_.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    
    LOG_INFO("Render target created successfully");
    return true;
}

bool WallpaperEngineRenderer::ImportTextureToWlroots() {
    // TODO: Import OpenGL texture into wlroots buffer
    // This requires wlroots' dmabuf import functionality
    // For now, this is a placeholder
    
    // The full implementation would:
    // 1. Export the OpenGL texture as a DMA-BUF
    // 2. Import the DMA-BUF into wlroots as a wlr_buffer
    // 3. Create/update the scene buffer node with this buffer
    
    // This is complex and requires:
    // - EGL_EXT_image_dma_buf_import extension
    // - wlr_dmabuf_v1_buffer_from_params()
    // - Proper synchronization between GL and wlroots
    
    LOG_WARN("Texture import to wlroots not yet fully implemented");
    LOG_WARN("This requires DMA-BUF export/import which is complex");
    
    return false;
}

void WallpaperEngineRenderer::CleanupEGL() {
    LOG_DEBUG("Cleaning up EGL resources");
    
    if (egl_context_.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_context_.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (egl_context_.surface != EGL_NO_SURFACE) {
            eglDestroySurface(egl_context_.display, egl_context_.surface);
        }
        
        if (egl_context_.context != EGL_NO_CONTEXT) {
            eglDestroyContext(egl_context_.display, egl_context_.context);
        }
        
        eglTerminate(egl_context_.display);
    }
    
    egl_context_.display = EGL_NO_DISPLAY;
    egl_context_.context = EGL_NO_CONTEXT;
    egl_context_.surface = EGL_NO_SURFACE;
}

} // namespace Wayland
} // namespace Leviathan
