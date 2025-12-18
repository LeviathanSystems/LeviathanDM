#include "ui/WidgetPlugin.hpp"
#include <ctime>
#include <thread>
#include <atomic>
#include <cairo.h>

using namespace Leviathan::UI;

// Example Clock Widget Plugin
class ClockWidget : public WidgetPlugin {
public:
    ClockWidget() 
        : running_(false),
          font_size_(12),
          update_interval_(1) {
        // Default colors (white text)
        text_color_[0] = 1.0;
        text_color_[1] = 1.0;
        text_color_[2] = 1.0;
        text_color_[3] = 1.0;
    }
    
    ~ClockWidget() override {
        Cleanup();
    }
    
    PluginMetadata GetMetadata() const override {
        return PluginMetadata{
            .name = "ClockWidget",
            .version = "1.0.0",
            .author = "LeviathanDM",
            .description = "Displays current time with configurable format",
            .api_version = WIDGET_API_VERSION
        };
    }
    
    bool Initialize(const std::map<std::string, std::string>& config) override {
        // Parse config
        auto format_it = config.find("format");
        if (format_it != config.end()) {
            format_ = format_it->second;
        } else {
            format_ = "%H:%M:%S";  // Default format
        }
        
        auto interval_it = config.find("update_interval");
        if (interval_it != config.end()) {
            update_interval_ = std::stoi(interval_it->second);
        }
        
        auto font_size_it = config.find("font_size");
        if (font_size_it != config.end()) {
            font_size_ = std::stoi(font_size_it->second);
        }
        
        // Start update thread
        running_ = true;
        update_thread_ = std::thread(&ClockWidget::UpdateLoop, this);
        
        return true;
    }
    
    void Update() override {
        // Get current time
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        char buffer[128];
        strftime(buffer, sizeof(buffer), format_.c_str(), timeinfo);
        
        // Update time string (thread-safe)
        std::lock_guard<std::mutex> lock(mutex_);
        if (time_str_ != buffer) {
            time_str_ = buffer;
            dirty_ = true;
        }
    }
    
    void CalculateSize(int available_width, int available_height) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Create temporary surface to measure text
        cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* temp_cr = cairo_create(temp_surface);
        
        cairo_select_font_face(temp_cr, "monospace",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(temp_cr, font_size_);
        
        cairo_text_extents_t extents;
        cairo_text_extents(temp_cr, time_str_.c_str(), &extents);
        
        width_ = static_cast<int>(extents.width) + 8;
        height_ = static_cast<int>(extents.height) + 8;
        
        width_ = std::min(width_, available_width);
        height_ = std::min(height_, available_height);
        
        cairo_destroy(temp_cr);
        cairo_surface_destroy(temp_surface);
    }
    
    void Render(cairo_t* cr) override {
        if (!IsVisible()) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        cairo_save(cr);
        
        // Draw text
        cairo_select_font_face(cr, "monospace",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, font_size_);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, time_str_.c_str(), &extents);
        
        // Center text
        double text_x = x_ + 4;
        double text_y = y_ + (height_ / 2.0) + (extents.height / 2.0);
        
        cairo_set_source_rgba(cr, text_color_[0], text_color_[1], 
                             text_color_[2], text_color_[3]);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, time_str_.c_str());
        
        cairo_restore(cr);
    }
    
    void Cleanup() override {
        if (running_) {
            running_ = false;
            if (update_thread_.joinable()) {
                update_thread_.join();
            }
        }
    }

private:
    void UpdateLoop() {
        while (running_) {
            Update();
            std::this_thread::sleep_for(std::chrono::seconds(update_interval_));
        }
    }
    
    std::string time_str_;
    std::string format_;
    int font_size_;
    int update_interval_;
    double text_color_[4];
    
    std::atomic<bool> running_;
    std::thread update_thread_;
};

// Export plugin functions
extern "C" {
    EXPORT_PLUGIN_CREATE(ClockWidget)
    EXPORT_PLUGIN_DESTROY(ClockWidget)
    EXPORT_PLUGIN_METADATA(ClockWidget)
}
