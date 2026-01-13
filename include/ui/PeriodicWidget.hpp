#pragma once

#include "WidgetPlugin.hpp"
#include <thread>
#include <atomic>
#include <chrono>

namespace Leviathan {
namespace UI {

/**
 * @brief Base class for widgets that update periodically in a background thread
 * 
 * This class handles all the boilerplate for:
 * - Background thread management (start/stop/lifecycle)
 * - Periodic update loop with configurable interval
 * - Automatic cleanup on destruction
 * - Common config parsing (update_interval, font_size, colors)
 * 
 * Plugins just need to implement:
 * - UpdateData() - Called periodically on background thread to update widget data
 * - CalculateSize() - Measure widget size (helper methods provided)
 * - Render() - Draw the widget (mutex already locked)
 */
class PeriodicWidget : public WidgetPlugin {
public:
    PeriodicWidget() 
        : running_(false),
          update_interval_(1),
          font_size_(12),
          font_family_("monospace") {
        // Default colors (white text)
        text_color_[0] = 1.0;
        text_color_[1] = 1.0;
        text_color_[2] = 1.0;
        text_color_[3] = 1.0;
    }
    
    ~PeriodicWidget() override {
        StopUpdateThread();
    }
    
    /**
     * @brief Initialize the widget with configuration
     * 
     * Automatically parses common config options:
     * - update_interval: How often to update (seconds)
     * - font_size: Font size for text rendering
     * - font_family: Font family name (default: "monospace")
     * - text_color: Text color in #RRGGBB format
     * 
     * Calls InitializeImpl() for plugin-specific initialization.
     */
    bool Initialize(const std::map<std::string, std::string>& config) override {
        // Parse common config options
        auto interval_it = config.find("update_interval");
        if (interval_it != config.end()) {
            update_interval_ = std::stoi(interval_it->second);
            if (update_interval_ < 1) update_interval_ = 1;
        }
        
        auto font_size_it = config.find("font_size");
        if (font_size_it != config.end()) {
            font_size_ = std::stoi(font_size_it->second);
        }
        
        auto font_family_it = config.find("font_family");
        if (font_family_it != config.end()) {
            font_family_ = font_family_it->second;
        }
        
        auto text_color_it = config.find("text_color");
        if (text_color_it != config.end()) {
            ParseColor(text_color_it->second, text_color_);
        }
        
        // Let plugin do its own initialization
        if (!InitializeImpl(config)) {
            return false;
        }
        
        // Start background update thread
        StartUpdateThread();
        
        return true;
    }
    
    /**
     * @brief Final cleanup - called by Cleanup() override
     */
    void Cleanup() override {
        StopUpdateThread();
        CleanupImpl();
    }
    
    /**
     * @brief Called by base class on background thread - delegates to UpdateData()
     */
    void Update() override final {
        UpdateData();
    }

protected:
    // ===== Plugin must implement these =====
    
    /**
     * @brief Plugin-specific initialization
     * Common config is already parsed. Parse plugin-specific config here.
     */
    virtual bool InitializeImpl(const std::map<std::string, std::string>& config) = 0;
    
    /**
     * @brief Update widget data on background thread
     * Called periodically every `update_interval_` seconds.
     * Set dirty_=true if data changed to trigger re-render.
     * mutex_ is automatically locked by the caller.
     */
    virtual void UpdateData() = 0;
    
    /**
     * @brief Plugin-specific cleanup (optional)
     */
    virtual void CleanupImpl() {}
    
    // ===== Helper methods for plugins =====
    
    /**
     * @brief Measure text size with current font settings
     * Creates a temporary Cairo surface to measure text.
     * Thread-safe - can be called from CalculateSize().
     */
    void MeasureText(const std::string& text, int& width, int& height, int padding = 8) {
        cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* temp_cr = cairo_create(temp_surface);
        
        cairo_select_font_face(temp_cr, font_family_.c_str(),
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(temp_cr, font_size_);
        
        cairo_text_extents_t extents;
        cairo_text_extents(temp_cr, text.c_str(), &extents);
        
        width = static_cast<int>(extents.width) + padding;
        height = static_cast<int>(extents.height) + padding;
        
        cairo_destroy(temp_cr);
        cairo_surface_destroy(temp_surface);
    }
    
    /**
     * @brief Draw centered text with current font settings
     * Helper for Render() implementations.
     * Assumes cairo context is already set up and mutex is locked.
     */
    void DrawText(cairo_t* cr, const std::string& text, 
                  double center_x, double center_y) {
        cairo_select_font_face(cr, font_family_.c_str(),
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, font_size_);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, text.c_str(), &extents);
        
        double text_x = center_x - (extents.width / 2.0) - extents.x_bearing;
        double text_y = center_y - (extents.height / 2.0) - extents.y_bearing;
        
        cairo_set_source_rgba(cr, text_color_[0], text_color_[1], 
                             text_color_[2], text_color_[3]);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, text.c_str());
    }
    
    /**
     * @brief Parse color from #RRGGBB format
     */
    bool ParseColor(const std::string& color_str, double color[4]) {
        if (color_str.size() == 7 && color_str[0] == '#') {
            int r, g, b;
            if (sscanf(color_str.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3) {
                color[0] = r / 255.0;
                color[1] = g / 255.0;
                color[2] = b / 255.0;
                color[3] = 1.0;  // Full opacity
                return true;
            }
        }
        return false;
    }
    
    // ===== Common member variables =====
    int update_interval_;       // Update interval in seconds
    int font_size_;             // Font size for text
    std::string font_family_;   // Font family name
    double text_color_[4];      // Text color (RGBA)

private:
    void StartUpdateThread() {
        if (running_) return;  // Already running
        
        running_ = true;
        update_thread_ = std::thread(&PeriodicWidget::UpdateLoop, this);
    }
    
    void StopUpdateThread() {
        if (!running_) return;  // Not running
        
        running_ = false;
        if (update_thread_.joinable()) {
            update_thread_.join();
        }
    }
    
    void UpdateLoop() {
        // Small delay before first update to let initialization complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        while (running_) {
            // Lock mutex and call UpdateData()
            {
                UpdateData();
            }
            
            // Sleep for configured interval
            std::this_thread::sleep_for(std::chrono::seconds(update_interval_));
        }
    }
    
    std::atomic<bool> running_;
    std::thread update_thread_;
};

} // namespace UI
} // namespace Leviathan
