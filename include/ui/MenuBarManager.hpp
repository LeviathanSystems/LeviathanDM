#pragma once

#include "ui/MenuBar.hpp"
#include "wayland/WaylandTypes.hpp"
#include <memory>
#include <map>
#include <string>

namespace Leviathan {

// Forward declarations
namespace Wayland {
    class LayerManager;
}

namespace UI {

/**
 * @brief Global MenuBar Manager
 * 
 * Manages menubars across all outputs. Creates one menubar per output
 * and handles showing/hiding them in the top layer of each LayerManager.
 */
class MenuBarManager {
public:
    // Singleton access
    static MenuBarManager& Instance();
    
    // Initialize with default configuration
    void Initialize(struct wl_event_loop* event_loop);
    
    // Register a menubar for a specific output
    void RegisterMenuBar(struct wlr_output* output, 
                        Wayland::LayerManager* layer_manager,
                        uint32_t output_width, 
                        uint32_t output_height);
    
    // Unregister menubar for an output
    void UnregisterMenuBar(struct wlr_output* output);
    
    // Show menubar on the focused/active output
    void ShowOnOutput(struct wlr_output* output);
    
    // Hide menubar on a specific output
    void HideOnOutput(struct wlr_output* output);
    
    // Toggle menubar visibility on an output
    void ToggleOnOutput(struct wlr_output* output);
    
    // Hide all menubars
    void HideAll();
    
    // Get menubar for an output
    MenuBar* GetMenuBarForOutput(struct wlr_output* output);
    
    // Check if any menubar is visible
    bool IsAnyMenuBarVisible() const;
    
    // Handle input events (forward to visible menubar)
    bool HandleKeyPress(uint32_t key, uint32_t modifiers);
    bool HandleTextInput(const std::string& text);
    bool HandleBackspace();
    bool HandleEnter();
    bool HandleEscape();
    bool HandleArrowUp();
    bool HandleArrowDown();
    bool HandleClick(int x, int y, struct wlr_output* output);
    bool HandleHover(int x, int y, struct wlr_output* output);
    
    // Configuration
    void SetConfig(const MenuBarConfig& config);
    const MenuBarConfig& GetConfig() const { return config_; }
    
    // Provider management (shared across all menubars)
    void AddProvider(std::shared_ptr<IMenuItemProvider> provider);
    void RemoveProvider(const std::string& provider_name);
    void ClearProviders();
    void RefreshAllItems();
    
    // Cleanup
    void Shutdown();
    
private:
    MenuBarManager();
    ~MenuBarManager();
    
    // Prevent copying
    MenuBarManager(const MenuBarManager&) = delete;
    MenuBarManager& operator=(const MenuBarManager&) = delete;
    
    // Get the currently visible menubar (if any)
    MenuBar* GetVisibleMenuBar();
    
    struct MenuBarData {
        std::unique_ptr<MenuBar> menubar;
        Wayland::LayerManager* layer_manager;
        struct wlr_output* output;
        uint32_t width;
        uint32_t height;
    };
    
    MenuBarConfig config_;
    struct wl_event_loop* event_loop_;
    std::map<struct wlr_output*, MenuBarData> menubars_;
    std::vector<std::shared_ptr<IMenuItemProvider>> providers_;
    bool initialized_;
};

} // namespace UI
} // namespace Leviathan
