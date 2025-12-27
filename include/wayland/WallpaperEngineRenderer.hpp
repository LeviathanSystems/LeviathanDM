#ifndef WALLPAPER_ENGINE_RENDERER_HPP
#define WALLPAPER_ENGINE_RENDERER_HPP

#include <string>
#include <memory>

// Forward declarations for wlroots
struct wlr_scene_tree;
struct wlr_scene_node;
struct wl_event_loop;
struct wlr_renderer;
struct wlr_buffer;

// Forward declarations for EGL
typedef void *EGLDisplay;
typedef void *EGLContext;
typedef void *EGLConfig;
typedef void *EGLSurface;
typedef unsigned int EGLenum;
typedef unsigned int GLuint;

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

/**
 * WallpaperEngineRenderer - Manages Wallpaper Engine wallpapers
 * 
 * This class wraps the linux-wallpaperengine library to render
 * animated wallpapers from Steam's Wallpaper Engine.
 * 
 * It creates an offscreen EGL context, renders the WallpaperEngine
 * wallpaper to a texture, and imports that texture into wlroots.
 */
class WallpaperEngineRenderer {
public:
    /**
     * Constructor
     * @param background_layer The scene tree layer to render wallpapers to
     * @param event_loop The Wayland event loop
     * @param renderer The wlroots renderer for texture import
     * @param width Width of the wallpaper area
     * @param height Height of the wallpaper area
     */
    WallpaperEngineRenderer(struct wlr_scene_tree* background_layer, 
                           struct wl_event_loop* event_loop,
                           struct wlr_renderer* renderer,
                           int width, int height);
    
    /**
     * Destructor
     */
    ~WallpaperEngineRenderer();
    
    /**
     * Load a Wallpaper Engine project
     * @param project_path Path to the Wallpaper Engine project directory (containing project.json)
     * @return true if loaded successfully
     */
    bool LoadWallpaper(const std::string& project_path);
    
    /**
     * Unload the current wallpaper
     */
    void UnloadWallpaper();
    
    /**
     * Update/render a frame
     * Called from the frame timer
     */
    void RenderFrame();
    
    /**
     * Pause/unpause rendering
     */
    void SetPaused(bool paused);
    
    /**
     * Check if a wallpaper is currently loaded
     */
    bool IsLoaded() const { return is_loaded_; }
    
    /**
     * Get the current project path
     */
    const std::string& GetCurrentPath() const { return current_path_; }

private:
    // Offscreen EGL rendering context
    struct EGLContext {
        EGLDisplay display = nullptr;
        EGLContext context = nullptr;
        EGLSurface surface = nullptr;
        EGLConfig config = nullptr;
        GLuint framebuffer = 0;
        GLuint texture = 0;
    };
    
    struct wlr_scene_tree* background_layer_;
    struct wl_event_loop* event_loop_;
    struct wlr_renderer* wlr_renderer_;
    struct wlr_scene_node* wallpaper_node_ = nullptr;
    struct wlr_buffer* wallpaper_buffer_ = nullptr;
    
    int width_;
    int height_;
    bool is_loaded_ = false;
    bool is_paused_ = false;
    std::string current_path_;
    
    // WallpaperEngine application context and objects
    std::unique_ptr<WallpaperEngine::Application::ApplicationContext> we_context_;
    std::unique_ptr<WallpaperEngine::Application::WallpaperApplication> we_app_;
    WallpaperEngine::Render::CWallpaper* we_wallpaper_ = nullptr;
    
    // Offscreen EGL rendering
    EGLContext egl_context_;
    
    // Frame timer
    struct wl_event_source* frame_timer_ = nullptr;
    static int FrameCallback(void* data);
    
    // Initialize offscreen EGL context
    bool InitializeEGL();
    
    // Create framebuffer and texture for offscreen rendering
    bool CreateRenderTarget();
    
    // Import OpenGL texture into wlroots buffer
    bool ImportTextureToWlroots();
    
    // Cleanup EGL resources
    void CleanupEGL();
};

} // namespace Wayland
} // namespace Leviathan

#endif // WALLPAPER_ENGINE_RENDERER_HPP
