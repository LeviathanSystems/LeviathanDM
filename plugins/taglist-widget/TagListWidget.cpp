#include "ui/PluginAPI.hpp"
#include "Logger.hpp"
#include <cairo.h>
#include <sstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>

namespace Leviathan {
namespace UI {

// Tag/workspace list widget that shows active tags and app counts
// Uses PluginAPI.hpp which provides compositor state access without wlroots

class TagListWidget : public WidgetPlugin {
public:
    TagListWidget() : running_(false), font_size_(12.0) {}
    
    ~TagListWidget() override {
        Cleanup();
    }
    
    PluginMetadata GetMetadata() const override {
        return PluginMetadata{
            "TagListWidget",
            "1.0.0",
            "LeviathanDM",
            "Displays tags and running applications",
            WIDGET_API_VERSION
        };
    }
    
    bool Initialize(const std::map<std::string, std::string>& config) override {
        // Parse configuration
        auto it = config.find("font_size");
        if (it != config.end()) {
            try {
                font_size_ = std::stod(it->second);
            } catch (...) {
                LOG_WARN("TagListWidget: Invalid font_size, using default");
            }
        }
        
        it = config.find("update_interval");
        if (it != config.end()) {
            try {
                update_interval_ = std::stoi(it->second);
            } catch (...) {
                LOG_WARN("TagListWidget: Invalid update_interval, using default");
            }
        }
        
        // Start background update thread
        running_ = true;
        update_thread_ = std::thread(&TagListWidget::UpdateLoop, this);
        
        LOG_INFO("TagListWidget initialized: font_size={}, update_interval={}s",
                 font_size_, update_interval_);
        
        return true;
    }
    
    void Update() override {
        // Query compositor state
        auto* comp_state = GetCompositorState();
        if (!comp_state) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Get all tags
        auto tags = comp_state->GetTags();
        auto active_tag = comp_state->GetActiveTag();
        
        // Build tag list string
        std::ostringstream oss;
        for (size_t i = 0; i < tags.size(); ++i) {
            auto* tag = tags[i];
            if (!tag) continue;
            
            // Highlight active tag with brackets
            bool is_active = (tag == active_tag);
            if (is_active) {
                oss << "[";
            }
            
            // Use Plugin API helper functions (no need for full Tag class!)
            oss << Leviathan::UI::Plugin::GetTagName(tag);
            
            // Show number of applications on this tag
            int client_count = Leviathan::UI::Plugin::GetTagClientCount(tag);
            if (client_count > 0) {
                oss << ":" << client_count;
            }
            
            if (is_active) {
                oss << "]";
            }
            
            // Space between tags
            if (i < tags.size() - 1) {
                oss << " ";
            }
        }
        
        std::string new_text = oss.str();
        if (new_text != text_) {
            text_ = new_text;
            MarkDirty();
        }
    }
    
    void Cleanup() override {
        if (running_) {
            running_ = false;
            if (update_thread_.joinable()) {
                update_thread_.join();
            }
        }
    }
    
    void CalculateSize(int available_width, int available_height) override {
        (void)available_width;  // Unused
        (void)available_height; // Unused
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Create temporary surface for text measurement
        cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* cr = cairo_create(temp_surface);
        
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, font_size_);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, text_.c_str(), &extents);
        
        width_ = extents.width + 8;  // Add padding
        height_ = font_size_ + 4;
        
        cairo_destroy(cr);
        cairo_surface_destroy(temp_surface);
    }
    
    void Render(cairo_t* cr) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (text_.empty()) {
            return;
        }
        
        // Draw background
        cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.9);
        cairo_rectangle(cr, x_, y_, width_, height_);
        cairo_fill(cr);
        
        // Draw text
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, font_size_);
        
        cairo_move_to(cr, x_ + 4, y_ + font_size_);
        cairo_show_text(cr, text_.c_str());
    }
    
private:
    void UpdateLoop() {
        while (running_) {
            Update();
            std::this_thread::sleep_for(std::chrono::seconds(update_interval_));
        }
    }
    
    std::atomic<bool> running_;
    std::thread update_thread_;
    std::string text_;
    double font_size_;
    int update_interval_ = 1;
};

} // namespace UI
} // namespace Leviathan

// Export plugin functions
extern "C" {
    EXPORT_PLUGIN_CREATE(TagListWidget)
    EXPORT_PLUGIN_DESTROY(TagListWidget)
    EXPORT_PLUGIN_METADATA(TagListWidget)
}
