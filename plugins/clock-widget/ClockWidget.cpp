#include "ui/PeriodicWidget.hpp"
#include "version.h"
#include <ctime>

namespace Leviathan {
namespace UI {

// Example Clock Widget Plugin - simplified with PeriodicWidget base class
class ClockWidget : public PeriodicWidget {
public:
    ClockWidget() : time_str_("--:--:--"), format_("%H:%M:%S") {}
    
    PluginMetadata GetMetadata() const override {
        return PluginMetadata{
            .name = PLUGIN_NAME,
            .version = PLUGIN_VERSION,
            .author = "LeviathanDM",
            .description = "Displays current time with configurable format",
            .api_version = WIDGET_API_VERSION
        };
    }

protected:
    // Plugin-specific initialization - just parse the time format
    bool InitializeImpl(const std::map<std::string, std::string>& config) override {
        auto format_it = config.find("format");
        if (format_it != config.end()) {
            format_ = format_it->second;
        }
        return true;
    }
    
    // Called periodically by PeriodicWidget base class
    // Mutex is already locked by the base class
    void UpdateData() override {
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        char buffer[128];
        strftime(buffer, sizeof(buffer), format_.c_str(), timeinfo);
        
        if (time_str_ != buffer) {
            time_str_ = buffer;
            MarkNeedsPaint();  // Flutter-style dirty tracking
        }
    }
    
    void CalculateSize(int available_width, int available_height) override {
        
        // Use helper method from PeriodicWidget base class
        int text_width, text_height;
        MeasureText(time_str_, text_width, text_height, 8);
        
        width_ = std::min(text_width, available_width);
        height_ = std::min(text_height, available_height);
    }
    
    void Render(cairo_t* cr) override {
        if (!IsVisible()) return;
        
        // No lock needed - main thread only reads cached time_str_
        
        cairo_save(cr);
        
        // Use helper method from PeriodicWidget base class
        double center_x = x_ + width_ / 2.0;
        double center_y = y_ + height_ / 2.0;
        DrawText(cr, time_str_, center_x, center_y);
        
        cairo_restore(cr);
    }

private:
    std::string time_str_;
    std::string format_;
};

} // namespace UI
} // namespace Leviathan

// Export plugin functions
extern "C" {
    EXPORT_PLUGIN_CREATE(ClockWidget)
    EXPORT_PLUGIN_DESTROY(ClockWidget)
    EXPORT_PLUGIN_METADATA(ClockWidget)
}
