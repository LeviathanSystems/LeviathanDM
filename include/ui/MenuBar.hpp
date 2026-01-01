#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <cairo.h>
#include "wayland/WaylandTypes.hpp"
#include "ui/ShmBuffer.hpp"
#include "ui/reusable-widgets/TextField.hpp"
#include "ui/reusable-widgets/VBox.hpp"
#include "ui/reusable-widgets/ScrollView.hpp"
#include "ui/IconLoader.hpp"

namespace Leviathan {

// Forward declarations
namespace Wayland {
    class LayerManager;
}

namespace UI {

/**
 * @brief Abstract menu item that can represent anything in the menubar
 * 
 * This could be an application, a command, a bookmark, or any custom item
 */
class MenuItem {
public:
    virtual ~MenuItem() = default;
    
    // Get display name (shown in the menu)
    virtual std::string GetDisplayName() const = 0;
    
    // Get search keywords for filtering
    virtual std::vector<std::string> GetSearchKeywords() const = 0;
    
    // Get icon path (optional, return empty string if no icon)
    virtual std::string GetIconPath() const { return ""; }
    
    // Get description/subtitle (optional)
    virtual std::string GetDescription() const { return ""; }
    
    // Execute/activate this item
    virtual void Execute() = 0;
    
    // Get priority for sorting (higher = appears first, default = 0)
    virtual int GetPriority() const { return 0; }
    
    // Check if item matches search query (can override for custom matching)
    virtual bool Matches(const std::string& query) const;
};

/**
 * @brief Provider interface for loading menu items
 * 
 * Implement this to create custom item sources (apps, commands, bookmarks, etc.)
 */
class IMenuItemProvider {
public:
    virtual ~IMenuItemProvider() = default;
    
    // Get the name of this provider (for debugging/logging)
    virtual std::string GetName() const = 0;
    
    // Load items (called during menubar initialization or refresh)
    virtual std::vector<std::shared_ptr<MenuItem>> LoadItems() = 0;
    
    // Check if provider supports live updates
    virtual bool SupportsLiveUpdates() const { return false; }
    
    // Refresh items (for providers that support live updates)
    virtual void Refresh() {}
};

/**
 * @brief Configuration for MenuBar appearance and behavior
 */
struct MenuBarConfig {
    // Visual properties
    int height = 40;
    int item_height = 35;
    int max_visible_items = 8;
    int padding = 10;
    
    // Colors (RGBA)
    struct {
        double r = 0.1, g = 0.1, b = 0.1, a = 0.95;
    } background_color;
    
    struct {
        double r = 0.2, g = 0.4, b = 0.6, a = 1.0;
    } selected_color;
    
    struct {
        double r = 1.0, g = 1.0, b = 1.0, a = 1.0;
    } text_color;
    
    struct {
        double r = 0.7, g = 0.7, b = 0.7, a = 1.0;
    } description_color;
    
    // Fonts
    std::string font_family = "Sans";
    int font_size = 12;
    int description_font_size = 9;
    
    // Behavior
    bool fuzzy_matching = true;
    bool case_sensitive = false;
    int min_chars_for_search = 0;
};

/**
 * @brief Abstract MenuBar - Application launcher and custom item menu
 * 
 * This menubar can display applications, commands, bookmarks, or any custom items.
 * It appears on-demand as an overlay at the top of the screen.
 */
class MenuBar {
public:
    MenuBar(const MenuBarConfig& config,
            Wayland::LayerManager* layer_manager,
            struct wl_event_loop* event_loop,
            uint32_t output_width,
            uint32_t output_height);
    ~MenuBar();
    
    // Show/hide the menubar
    void Show();
    void Hide();
    void Toggle();
    bool IsVisible() const { return is_visible_; }
    
    // Add/remove item providers
    void AddProvider(std::shared_ptr<IMenuItemProvider> provider);
    void RemoveProvider(const std::string& provider_name);
    void ClearProviders();
    
    // Refresh all items from providers
    void RefreshItems();
    
    // Handle input
    void HandleKeyPress(uint32_t key, uint32_t modifiers);
    void HandleTextInput(const std::string& text);
    void HandleBackspace();
    void HandleEnter();
    void HandleEscape();
    void HandleArrowUp();
    void HandleArrowDown();
    
    // Mouse input
    bool HandleClick(int x, int y);
    bool HandleHover(int x, int y);
    
    // Render
    void Render();
    
    // Get bounds for input handling
    void GetBounds(int& x, int& y, int& width, int& height) const {
        x = pos_x_; y = pos_y_;
        width = bar_width_; height = bar_height_;
    }

private:
    void CreateSceneNodes();
    void UpdateFilteredItems();
    void RenderToBuffer();
    void UploadToTexture();
    void ExecuteSelectedItem();
    void EnsureSelectionVisible();
    
    // Fuzzy/simple matching helpers
    bool FuzzyMatch(const std::string& text, const std::string& query) const;
    bool SimpleMatch(const std::string& text, const std::string& query) const;
    
    MenuBarConfig config_;
    Wayland::LayerManager* layer_manager_;
    struct wl_event_loop* event_loop_;
    
    // Scene graph nodes
    struct wlr_scene_rect* scene_rect_;      // Background
    struct wlr_scene_buffer* scene_buffer_;  // Content
    struct wlr_texture* texture_;
    struct wlr_renderer* renderer_;
    ShmBuffer* shm_buffer_;
    bool buffer_attached_;
    
    // Position and dimensions
    int pos_x_, pos_y_;
    int bar_width_, bar_height_;
    uint32_t output_width_, output_height_;
    
    // Cairo rendering
    cairo_surface_t* cairo_surface_;
    cairo_t* cairo_;
    uint32_t* buffer_data_;
    
    // Menu state
    bool is_visible_;
    std::string search_query_;
    int selected_index_;
    int scroll_offset_;
    
    // UI Components
    std::shared_ptr<TextField> search_field_;
    std::unique_ptr<IconLoader> icon_loader_;
    
    // Item management
    std::vector<std::shared_ptr<IMenuItemProvider>> providers_;
    std::vector<std::shared_ptr<MenuItem>> all_items_;
    std::vector<std::shared_ptr<MenuItem>> filtered_items_;
};

} // namespace UI
} // namespace Leviathan
