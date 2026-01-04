#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <cairo.h>

namespace Leviathan {
namespace UI {

/**
 * @brief Represents a single tab in a TabBar
 */
struct Tab {
    std::string id;           // Unique identifier
    std::string label;        // Display text
    std::string icon;         // Optional icon path/name
    bool enabled = true;      // Whether the tab can be clicked
    void* user_data = nullptr; // Optional user data
    
    Tab(const std::string& id, const std::string& label, const std::string& icon = "")
        : id(id), label(label), icon(icon) {}
};

/**
 * @brief Configuration for TabBar appearance
 */
struct TabBarConfig {
    // Layout
    int height = 30;
    int tab_min_width = 80;
    int tab_max_width = 200;
    int tab_padding = 15;
    int icon_size = 16;
    int icon_spacing = 8;
    
    // Colors - Background
    struct {
        double r = 0.15, g = 0.15, b = 0.15, a = 1.0;
    } background_color;
    
    // Colors - Active tab
    struct {
        double r = 0.3, g = 0.5, b = 0.7, a = 1.0;
    } active_tab_color;
    
    // Colors - Inactive tab
    struct {
        double r = 0.2, g = 0.2, b = 0.2, a = 1.0;
    } inactive_tab_color;
    
    // Colors - Hover tab
    struct {
        double r = 0.25, g = 0.25, b = 0.25, a = 1.0;
    } hover_tab_color;
    
    // Colors - Text
    struct {
        double r = 1.0, g = 1.0, b = 1.0, a = 1.0;
    } text_color;
    
    struct {
        double r = 0.7, g = 0.7, b = 0.7, a = 1.0;
    } inactive_text_color;
    
    // Separator
    struct {
        double r = 0.3, g = 0.3, b = 0.3, a = 1.0;
    } separator_color;
    int separator_width = 1;
    bool show_separators = false;
    
    // Border
    struct {
        double r = 0.4, g = 0.4, b = 0.4, a = 1.0;
    } border_color;
    int border_width = 0;
    
    // Fonts
    std::string font_family = "Sans";
    int font_size = 11;
    bool bold_active = true;
    
    // Behavior
    bool equal_width_tabs = false;  // All tabs same width
    bool show_icons = true;
    bool center_text = true;
};

/**
 * @brief Reusable TabBar component
 * 
 * A horizontal tab bar that can be used in any UI context.
 * Supports keyboard navigation (arrow keys) and mouse interaction.
 */
class TabBar {
public:
    using TabChangeCallback = std::function<void(const std::string& tab_id, int index)>;
    
    explicit TabBar(const TabBarConfig& config = TabBarConfig());
    ~TabBar() = default;
    
    // Tab management
    void AddTab(const Tab& tab);
    void AddTab(const std::string& id, const std::string& label, const std::string& icon = "");
    void RemoveTab(const std::string& id);
    void ClearTabs();
    
    // Get/Set active tab
    void SetActiveTab(const std::string& id);
    void SetActiveTab(int index);
    std::string GetActiveTabId() const;
    int GetActiveTabIndex() const { return active_index_; }
    
    // Tab queries
    int GetTabCount() const { return tabs_.size(); }
    bool HasTab(const std::string& id) const;
    const Tab* GetTab(const std::string& id) const;
    const std::vector<Tab>& GetTabs() const { return tabs_; }
    
    // Enable/disable tabs
    void SetTabEnabled(const std::string& id, bool enabled);
    
    // Callbacks
    void SetOnTabChange(TabChangeCallback callback) { on_tab_change_ = callback; }
    
    // Navigation
    void SelectNext();
    void SelectPrevious();
    void SelectFirst();
    void SelectLast();
    
    // Input handling
    bool HandleClick(int x, int y);  // Returns true if a tab was clicked
    bool HandleHover(int x, int y);  // Returns true if hovering over a tab
    void HandleKeyPress(uint32_t key);  // Handle arrow keys
    
    // Rendering
    void Render(cairo_t* cr, int x, int y, int width);
    
    // Get dimensions
    int GetHeight() const { return config_.height; }
    int GetPreferredWidth() const;  // Minimum width needed for all tabs
    
    // Get tab bounds (for hit testing)
    struct TabBounds {
        int x, y, width, height;
        std::string id;
    };
    std::vector<TabBounds> GetTabBounds(int container_x, int container_y, int container_width) const;
    
    // Config
    const TabBarConfig& GetConfig() const { return config_; }
    void SetConfig(const TabBarConfig& config) { config_ = config; }

private:
    void CalculateTabWidths(int available_width, std::vector<int>& widths) const;
    int GetTabTextWidth(const Tab& tab, cairo_t* cr) const;
    int FindTabAtPosition(int x, int y, int container_x, int container_y, int container_width) const;
    void NotifyTabChange();
    
    TabBarConfig config_;
    std::vector<Tab> tabs_;
    int active_index_;
    int hover_index_;
    TabChangeCallback on_tab_change_;
};

} // namespace UI
} // namespace Leviathan
