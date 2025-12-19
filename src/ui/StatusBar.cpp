#include "ui/StatusBar.hpp"
#include "wayland/LayerManager.hpp"
#include "ui/WidgetPluginManager.hpp"
#include "Logger.hpp"
#include <ctime>
#include <cstring>
#include <wlr/types/wlr_scene.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <drm_fourcc.h>

namespace Leviathan {

StatusBar::StatusBar(const StatusBarConfig& config,
                     Wayland::LayerManager* layer_manager,
                     uint32_t output_width,
                     uint32_t output_height)
    : config_(config),
      layer_manager_(layer_manager),
      scene_rect_(nullptr),
      scene_buffer_(nullptr),
      texture_(nullptr),
      renderer_(nullptr),
      pos_x_(0),
      pos_y_(0),
      cairo_surface_(nullptr),
      cairo_(nullptr),
      buffer_data_(nullptr),
      output_width_(output_width),
      output_height_(output_height),
      bar_width_(0),
      bar_height_(0) {
    
    // Calculate bar dimensions based on position
    if (config_.position == StatusBarConfig::Position::Top || 
        config_.position == StatusBarConfig::Position::Bottom) {
        bar_width_ = output_width_;
        bar_height_ = config_.height;
    } else {  // Left or Right
        bar_width_ = config_.width;
        bar_height_ = output_height_;
    }
    
    LOG_INFO("Creating status bar '{}' at position {} with size {}x{}", 
             config_.name, 
             static_cast<int>(config_.position),
             bar_width_, 
             bar_height_);
    
    CreateSceneNodes();
    CreateWidgets();
    
    // Initial render
    RenderToBuffer();
    UploadToTexture();
    
    LOG_INFO("Status bar '{}' created successfully", config_.name);
}

StatusBar::~StatusBar() {
    if (cairo_) {
        cairo_destroy(cairo_);
    }
    if (cairo_surface_) {
        cairo_surface_destroy(cairo_surface_);
    }
    if (buffer_data_) {
        delete[] buffer_data_;
    }
    if (texture_) {
        wlr_texture_destroy(texture_);
    }
    // Scene nodes are cleaned up automatically by wlroots
    LOG_DEBUG("Destroyed status bar '{}'", config_.name);
}

void StatusBar::CreateSceneNodes() {
    // Get the working area layer from the LayerManager
    auto* working_layer = layer_manager_->GetLayer(Wayland::Layer::WorkingArea);
    
    // Parse background color from config
    // Format: "#RRGGBB"
    float bg_r = 0.18f, bg_g = 0.2f, bg_b = 0.25f;  // Default Nord color
    if (config_.background_color.size() == 7 && config_.background_color[0] == '#') {
        int r, g, b;
        sscanf(config_.background_color.c_str(), "#%02x%02x%02x", &r, &g, &b);
        bg_r = r / 255.0f;
        bg_g = g / 255.0f;
        bg_b = b / 255.0f;
    }
    
    float bg_color[4] = { bg_r, bg_g, bg_b, 0.95f };  // 95% opacity
    
    // Create background rectangle
    scene_rect_ = wlr_scene_rect_create(working_layer, bar_width_, bar_height_, bg_color);
    
    // Position based on config
    switch (config_.position) {
        case StatusBarConfig::Position::Top:
            pos_x_ = 0;
            pos_y_ = 0;
            break;
        case StatusBarConfig::Position::Bottom:
            pos_x_ = 0;
            pos_y_ = output_height_ - bar_height_;
            break;
        case StatusBarConfig::Position::Left:
            pos_x_ = 0;
            pos_y_ = 0;
            break;
        case StatusBarConfig::Position::Right:
            pos_x_ = output_width_ - bar_width_;
            pos_y_ = 0;
            break;
    }
    
    wlr_scene_node_set_position(&scene_rect_->node, pos_x_, pos_y_);
    wlr_scene_node_raise_to_top(&scene_rect_->node);
    
    // Create scene buffer node for widget rendering
    scene_buffer_ = wlr_scene_buffer_create(working_layer, nullptr);
    wlr_scene_node_set_position(&scene_buffer_->node, pos_x_, pos_y_);
    wlr_scene_node_raise_to_top(&scene_buffer_->node);
    
    LOG_DEBUG("Status bar '{}' scene nodes created at ({}, {})", config_.name, pos_x_, pos_y_);
}

void StatusBar::CreateWidgets() {
    LOG_INFO("Creating widgets for status bar '{}'", config_.name);
    
    // Helper lambda to create widget from config
    auto create_widget = [this](const WidgetConfig& widget_config) -> UI::Widget* {
        // Check for built-in widget types first
        if (widget_config.type == "label") {
            auto label = std::make_unique<UI::Label>();
            
            // Apply properties
            if (widget_config.properties.count("text")) {
                label->SetText(widget_config.properties.at("text"));
            }
            if (widget_config.properties.count("font-size")) {
                label->SetFontSize(std::stoi(widget_config.properties.at("font-size")));
            }
            if (widget_config.properties.count("color")) {
                // Parse color "#RRGGBB"
                std::string color = widget_config.properties.at("color");
                if (color.size() == 7 && color[0] == '#') {
                    int r, g, b;
                    sscanf(color.c_str(), "#%02x%02x%02x", &r, &g, &b);
                    label->SetTextColor(r/255.0, g/255.0, b/255.0);
                }
            }
            
            auto* ptr = label.get();
            owned_widgets_.push_back(std::move(label));
            return ptr;
        }
        else if (widget_config.type == "hbox") {
            auto hbox = std::make_unique<UI::HBox>();
            
            // Apply spacing if specified
            if (widget_config.properties.count("spacing")) {
                hbox->SetSpacing(std::stoi(widget_config.properties.at("spacing")));
            }
            
            auto* ptr = hbox.get();
            owned_widgets_.push_back(std::move(hbox));
            return ptr;
        }
        else if (widget_config.type == "vbox") {
            auto vbox = std::make_unique<UI::VBox>();
            
            if (widget_config.properties.count("spacing")) {
                vbox->SetSpacing(std::stoi(widget_config.properties.at("spacing")));
            }
            
            auto* ptr = vbox.get();
            owned_widgets_.push_back(std::move(vbox));
            return ptr;
        }
        
        // Not a built-in widget - try loading as plugin
        auto& plugin_mgr = UI::PluginManager();
        
        if (plugin_mgr.IsPluginLoaded(widget_config.type)) {
            LOG_DEBUG("Creating plugin widget: {}", widget_config.type);
            
            // Create plugin instance with config properties
            auto plugin_widget = plugin_mgr.CreatePluginWidget(
                widget_config.type,
                widget_config.properties
            );
            
            if (plugin_widget) {
                // Store the shared_ptr to keep it alive
                auto* ptr = plugin_widget.get();
                plugin_widgets_.push_back(plugin_widget);
                return ptr;
            } else {
                LOG_ERROR("Failed to create plugin widget: {}", widget_config.type);
            }
        } else {
            LOG_WARN("Unknown widget type '{}' and no plugin found with that name", 
                     widget_config.type);
        }
        
        return nullptr;
    };
    
    // Create left section widgets
    for (const auto& widget_config : config_.left.widgets) {
        auto* widget = create_widget(widget_config);
        if (widget) {
            left_widgets_.push_back(widget);
            LOG_DEBUG("Added widget '{}' to left section", widget_config.type);
        }
    }
    
    // Create center section widgets
    for (const auto& widget_config : config_.center.widgets) {
        auto* widget = create_widget(widget_config);
        if (widget) {
            center_widgets_.push_back(widget);
            LOG_DEBUG("Added widget '{}' to center section", widget_config.type);
        }
    }
    
    // Create right section widgets
    for (const auto& widget_config : config_.right.widgets) {
        auto* widget = create_widget(widget_config);
        if (widget) {
            right_widgets_.push_back(widget);
            LOG_DEBUG("Added widget '{}' to right section", widget_config.type);
        }
    }
    
    LOG_INFO("Created {} left, {} center, {} right widgets",
             left_widgets_.size(), center_widgets_.size(), right_widgets_.size());
}

void StatusBar::RenderToBuffer() {
    // Allocate buffer if needed
    if (!buffer_data_) {
        buffer_data_ = new uint32_t[bar_width_ * bar_height_];
        
        // Create Cairo surface for rendering
        cairo_surface_ = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(buffer_data_),
            CAIRO_FORMAT_ARGB32,
            bar_width_,
            bar_height_,
            bar_width_ * 4  // stride in bytes
        );
        
        cairo_ = cairo_create(cairo_surface_);
    }
    
    // Clear surface
    cairo_save(cairo_);
    cairo_set_operator(cairo_, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cairo_);
    cairo_restore(cairo_);
    
    // Set font from config
    cairo_select_font_face(cairo_, config_.font_family.c_str(),
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo_, config_.font_size);
    
    // Parse foreground color
    double fg_r = 0.85, fg_g = 0.87, fg_b = 0.91;  // Default Nord color
    if (config_.foreground_color.size() == 7 && config_.foreground_color[0] == '#') {
        int r, g, b;
        sscanf(config_.foreground_color.c_str(), "#%02x%02x%02x", &r, &g, &b);
        fg_r = r / 255.0;
        fg_g = g / 255.0;
        fg_b = b / 255.0;
    }
    cairo_set_source_rgb(cairo_, fg_r, fg_g, fg_b);
    
    // Calculate layout sections
    int section_width = bar_width_ / 3;  // Simplified: equal thirds
    int current_x = 0;
    
    // Render left section
    if (!left_widgets_.empty()) {
        cairo_save(cairo_);
        cairo_translate(cairo_, current_x + config_.left.padding, config_.left.padding);
        
        int available_width = section_width - (config_.left.padding * 2);
        int available_height = bar_height_ - (config_.left.padding * 2);
        
        for (auto& widget : left_widgets_) {
            widget->CalculateSize(available_width, available_height);
            widget->Render(cairo_);
            
            // Move to next widget position (horizontal layout for now)
            cairo_translate(cairo_, widget->GetWidth() + config_.left.spacing, 0);
            available_width -= widget->GetWidth() + config_.left.spacing;
        }
        
        cairo_restore(cairo_);
    }
    
    // Render center section
    current_x = section_width;
    if (!center_widgets_.empty()) {
        cairo_save(cairo_);
        
        // Calculate total width of center widgets to center them
        int total_center_width = 0;
        for (auto& widget : center_widgets_) {
            widget->CalculateSize(section_width, bar_height_);
            total_center_width += widget->GetWidth() + config_.center.spacing;
        }
        if (!center_widgets_.empty()) {
            total_center_width -= config_.center.spacing;  // Remove last spacing
        }
        
        // Center the widgets
        int center_x = current_x + (section_width - total_center_width) / 2;
        cairo_translate(cairo_, center_x + config_.center.padding, config_.center.padding);
        
        for (auto& widget : center_widgets_) {
            widget->Render(cairo_);
            cairo_translate(cairo_, widget->GetWidth() + config_.center.spacing, 0);
        }
        
        cairo_restore(cairo_);
    }
    
    // Render right section (right-aligned)
    current_x = section_width * 2;
    if (!right_widgets_.empty()) {
        cairo_save(cairo_);
        
        // Calculate total width to right-align
        int total_right_width = 0;
        for (auto& widget : right_widgets_) {
            widget->CalculateSize(section_width, bar_height_);
            total_right_width += widget->GetWidth() + config_.right.spacing;
        }
        if (!right_widgets_.empty()) {
            total_right_width -= config_.right.spacing;  // Remove last spacing
        }
        
        // Right-align
        int right_x = bar_width_ - total_right_width - config_.right.padding;
        cairo_translate(cairo_, right_x, config_.right.padding);
        
        for (auto& widget : right_widgets_) {
            widget->Render(cairo_);
            cairo_translate(cairo_, widget->GetWidth() + config_.right.spacing, 0);
        }
        
        cairo_restore(cairo_);
    }
    
    cairo_surface_flush(cairo_surface_);
    
    // DEBUG: Save to PNG to verify rendering works
    static bool saved_debug = false;
    if (!saved_debug) {
        cairo_surface_write_to_png(cairo_surface_, "/tmp/statusbar_debug.png");
        LOG_INFO("Saved status bar rendering to /tmp/statusbar_debug.png for debugging");
        saved_debug = true;
    }
    
    LOG_DEBUG("Rendered status bar '{}' with {} total widgets to Cairo buffer", 
              config_.name,
              left_widgets_.size() + center_widgets_.size() + right_widgets_.size());
}

void StatusBar::UploadToTexture() {
    if (!buffer_data_ || !scene_buffer_) {
        LOG_WARN("Cannot upload texture - buffer_data or scene_buffer is null");
        return;
    }
    
    // Get renderer from output (via LayerManager)
    if (!renderer_) {
        struct wlr_output* output = layer_manager_->GetOutput();
        if (output && output->renderer) {
            renderer_ = output->renderer;
            LOG_DEBUG("Got renderer from output");
        } else {
            LOG_ERROR("Cannot get renderer from output");
            return;
        }
    }
    
    // Create texture from pixel data (for future use or debugging)
    if (texture_) {
        wlr_texture_destroy(texture_);
        texture_ = nullptr;
    }
    
    texture_ = wlr_texture_from_pixels(renderer_,
                                       DRM_FORMAT_ARGB8888,
                                       bar_width_ * 4,  // stride in bytes
                                       bar_width_,
                                       bar_height_,
                                       buffer_data_);
    
    if (!texture_) {
        LOG_ERROR("Failed to create texture from Cairo buffer");
        return;
    }
    
    LOG_INFO("Created/updated texture {}x{} for status bar '{}'", 
             bar_width_, bar_height_, config_.name);
    
    // HACK: Render using scene_rect pixels as proof of concept
    // This is inefficient but proves the rendering works
    // Sample every 2nd pixel to balance quality and performance
    int sample_rate = 2;
    int pixels_rendered = 0;
    int max_pixels = 5000;  // Limit to prevent too many scene nodes
    
    for (int y = 0; y < bar_height_ && pixels_rendered < max_pixels; y += sample_rate) {
        for (int x = 0; x < bar_width_ && pixels_rendered < max_pixels; x += sample_rate) {
            uint32_t pixel = buffer_data_[y * bar_width_ + x];
            
            // Extract ARGB components (little-endian ARGB8888)
            uint8_t a = (pixel >> 24) & 0xFF;
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            
            // Skip fully transparent pixels
            if (a < 10) continue;
            
            // Create a small rect for this pixel
            float color[4] = { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
            auto* pixel_rect = wlr_scene_rect_create(
                layer_manager_->GetLayer(Wayland::Layer::WorkingArea),
                sample_rate, sample_rate, color
            );
            
            wlr_scene_node_set_position(&pixel_rect->node, pos_x_ + x, pos_y_ + y);
            wlr_scene_node_raise_to_top(&pixel_rect->node);
            
            pixels_rendered++;
        }
    }
    
    LOG_INFO("Rendered {} pixel rects for status bar (sample rate: {}, max: {})", 
             pixels_rendered, sample_rate, max_pixels);
    LOG_WARN("Using inefficient scene_rect pixel rendering - this is a PROOF OF CONCEPT");
}

void StatusBar::Render() {
    RenderToBuffer();
    UploadToTexture();
}

void StatusBar::Update() {
    // Update widgets that need periodic updates
    // TODO: Call update on dynamic widgets (clock, etc.)
    RenderToBuffer();
    UploadToTexture();
}

int StatusBar::GetReservedSize() const {
    if (config_.position == StatusBarConfig::Position::Top || 
        config_.position == StatusBarConfig::Position::Bottom) {
        return config_.height;
    } else {
        return config_.width;
    }
}

int StatusBar::GetHeight() const {
    return bar_height_;
}

int StatusBar::GetWidth() const {
    return bar_width_;
}

} // namespace Leviathan
