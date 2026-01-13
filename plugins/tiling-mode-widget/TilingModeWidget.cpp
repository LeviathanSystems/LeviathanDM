#include "TilingModeWidget.hpp"
#include "version.h"
#include <cairo.h>
#include <cmath>

namespace Leviathan {
namespace UI {

TilingModeWidget::TilingModeWidget() 
    : PeriodicWidget(),
      current_layout_(LayoutType::MASTER_STACK),
      event_subscription_id_(-1) {
}

TilingModeWidget::~TilingModeWidget() {
}

PluginMetadata TilingModeWidget::GetMetadata() const {
    return {
        .name = PLUGIN_NAME,
        .version = PLUGIN_VERSION,
        .author = "LeviathanDM",
        .description = "Shows current tiling layout mode",
        .api_version = WIDGET_API_VERSION
    };
}

bool TilingModeWidget::InitializeImpl(const std::map<std::string, std::string>& config) {
    // Set update interval
    update_interval_ = 1; // Update every second
    
    // Parse configuration
    if (config.count("font_size")) {
        font_size_ = std::stoi(config.at("font_size"));
    }
    if (config.count("show_icon")) {
        show_icon_ = (config.at("show_icon") == "true");
    }
    if (config.count("show_text")) {
        show_text_ = (config.at("show_text") == "true");
    }
    if (config.count("bg_color")) {
        bg_color_ = config.at("bg_color");
    }
    if (config.count("fg_color")) {
        fg_color_ = config.at("fg_color");
    }
    
    // Subscribe to layout change events
    event_subscription_id_ = UI::Plugin::SubscribeToEvent(
        UI::Plugin::EventType::LayoutChanged,
        [this](const UI::Plugin::Event& event) {
            OnCompositorEvent(event);
        }
    );
    
    // Subscribe to tag switch events (layout is per-tag)
    UI::Plugin::SubscribeToEvent(
        UI::Plugin::EventType::TagSwitched,
        [this](const UI::Plugin::Event& event) {
            OnCompositorEvent(event);
        }
    );
    
    // Fetch initial layout
    FetchLayoutFromCompositor();
    
    return true;
}

void TilingModeWidget::CleanupImpl() {
    if (event_subscription_id_ >= 0) {
        UI::Plugin::UnsubscribeFromEvent(event_subscription_id_);
        event_subscription_id_ = -1;
    }
}

void TilingModeWidget::UpdateData() {
    // Periodically fetch layout in case we missed an event
    FetchLayoutFromCompositor();
}

void TilingModeWidget::OnCompositorEvent(const UI::Plugin::Event& event) {
    FetchLayoutFromCompositor();
    MarkDirty();
}

void TilingModeWidget::FetchLayoutFromCompositor() {
    // Get the screen for this widget's output
    // Plugin::GetWidgetScreen() returns the screen context set by StatusBar
    // This ensures we show the layout for the correct monitor in multi-monitor setups
    auto* screen = Plugin::GetWidgetScreen();
    
    if (!screen) {
        return;
    }
    
    // Get the current tag for THIS screen (not the globally active tag)
    auto* current_tag = Plugin::GetScreenCurrentTag(screen);
    if (!current_tag) {
        return;
    }
    
    // Get layout from current tag
    current_layout_ = Plugin::GetTagLayout(current_tag);
    MarkDirty();
}

std::string TilingModeWidget::GetLayoutName(LayoutType layout) const {
    switch (layout) {
        case LayoutType::MASTER_STACK:
            return "Master";
        case LayoutType::MONOCLE:
            return "Monocle";
        case LayoutType::GRID:
            return "Grid";
        case LayoutType::FLOATING:
            return "Float";
        default:
            return "Unknown";
    }
}

std::string TilingModeWidget::GetLayoutIcon(LayoutType layout) const {
    switch (layout) {
        case LayoutType::MASTER_STACK:
            return "⚏";  // Master-stack icon
        case LayoutType::MONOCLE:
            return "▣";  // Full screen icon
        case LayoutType::GRID:
            return "▦";  // Grid icon
        case LayoutType::FLOATING:
            return "⧉";  // Floating icon
        default:
            return "?";
    }
}

void TilingModeWidget::CalculateSize(int available_width, int available_height) {
    // Calculate text size
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, "sans-serif", 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(temp_cr, font_size_);
    
    std::string display_text;
    if (show_icon_) {
        display_text += GetLayoutIcon(current_layout_);
    }
    if (show_text_) {
        if (show_icon_) {
            display_text += " ";
        }
        display_text += GetLayoutName(current_layout_);
    }
    
    cairo_text_extents_t extents;
    cairo_text_extents(temp_cr, display_text.c_str(), &extents);
    
    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);
    
    int padding = 8;
    width_ = static_cast<int>(extents.width) + padding * 2;
    height_ = font_size_ + padding;
}

void TilingModeWidget::Render(cairo_t* cr) {
    // Save and translate to widget position
    cairo_save(cr);
    cairo_translate(cr, x_, y_);
    
    // Draw background
    double bg_r, bg_g, bg_b, bg_a;
    if (ParseColor(bg_color_, bg_r, bg_g, bg_b, bg_a)) {
        cairo_set_source_rgba(cr, bg_r, bg_g, bg_b, bg_a);
    } else {
        cairo_set_source_rgba(cr, 0.23, 0.25, 0.32, 1.0); // fallback
    }
    cairo_rectangle(cr, 0, 0, width_, height_);
    cairo_fill(cr);
    
    // Draw text
    double fg_r, fg_g, fg_b, fg_a;
    if (ParseColor(fg_color_, fg_r, fg_g, fg_b, fg_a)) {
        cairo_set_source_rgba(cr, fg_r, fg_g, fg_b, fg_a);
    } else {
        cairo_set_source_rgba(cr, 0.93, 0.94, 0.96, 1.0); // fallback
    }
    cairo_select_font_face(cr, "sans-serif", 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size_);
    
    std::string display_text;
    if (show_icon_) {
        display_text += GetLayoutIcon(current_layout_);
    }
    if (show_text_) {
        if (show_icon_) {
            display_text += " ";
        }
        display_text += GetLayoutName(current_layout_);
    }
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, display_text.c_str(), &extents);
    
    int padding = 8;
    int text_x = padding;
    int text_y = (height_ + extents.height) / 2;
    
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, display_text.c_str());
    
    cairo_restore(cr);
}

bool TilingModeWidget::HandleClick(int click_x, int click_y) {
    // Optional: Implement layout cycling on click
    // For now, just indicate click was handled
    return true;
}

bool TilingModeWidget::ParseColor(const std::string& hex, double& r, double& g, double& b, double& a) const {
    a = 1.0; // Default alpha
    
    if (hex.empty() || hex[0] != '#') {
        return false;
    }
    
    std::string color = hex.substr(1);
    
    if (color.length() == 6) {
        // #RRGGBB
        r = std::stoi(color.substr(0, 2), nullptr, 16) / 255.0;
        g = std::stoi(color.substr(2, 2), nullptr, 16) / 255.0;
        b = std::stoi(color.substr(4, 2), nullptr, 16) / 255.0;
    } else if (color.length() == 8) {
        // #RRGGBBAA
        r = std::stoi(color.substr(0, 2), nullptr, 16) / 255.0;
        g = std::stoi(color.substr(2, 2), nullptr, 16) / 255.0;
        b = std::stoi(color.substr(4, 2), nullptr, 16) / 255.0;
        a = std::stoi(color.substr(6, 2), nullptr, 16) / 255.0;
    } else {
        return false;
    }
    
    return true;
}

} // namespace UI
} // namespace Leviathan

// Export plugin
extern "C" {
    EXPORT_PLUGIN_CREATE(TilingModeWidget)
    EXPORT_PLUGIN_DESTROY(TilingModeWidget)
    EXPORT_PLUGIN_METADATA(TilingModeWidget)
}
