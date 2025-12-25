#include "ui/StatusBar.hpp"
#include "ui/ShmBuffer.hpp"
#include "wayland/LayerManager.hpp"
#include "ui/WidgetPluginManager.hpp"
#include "Logger.hpp"
#include <ctime>
#include <cstring>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <drm_fourcc.h>

namespace Leviathan {

StatusBar::StatusBar(const StatusBarConfig& config,
                     Wayland::LayerManager* layer_manager,
                     struct wl_event_loop* event_loop,
                     uint32_t output_width,
                     uint32_t output_height)
    : config_(config),
      layer_manager_(layer_manager),
      event_loop_(event_loop),
      dirty_check_timer_(nullptr),
      scene_rect_(nullptr),
      scene_buffer_(nullptr),
      popover_scene_buffer_(nullptr),
      texture_(nullptr),
      renderer_(nullptr),
      shm_buffer_(nullptr),
      popover_shm_buffer_(nullptr),
      pos_x_(0),
      pos_y_(0),
      cairo_surface_(nullptr),
      cairo_(nullptr),
      popover_cairo_surface_(nullptr),
      popover_cairo_(nullptr),
      buffer_data_(nullptr),
      popover_buffer_data_(nullptr),
      buffer_attached_(false),
      popover_buffer_attached_(false),
      is_rendering_popover_(false),
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
    
    LOG_INFO_FMT("Creating status bar '{}' at position {} with size {}x{}", 
             config_.name, 
             static_cast<int>(config_.position),
             bar_width_, 
             bar_height_);
    
    CreateSceneNodes();
    CreateWidgets();
    
    // Initial render
    //LOG_DEBUG("About to call RenderToBuffer()");
    RenderToBuffer();
    //LOG_DEBUG("RenderToBuffer() completed, about to call UploadToTexture()");
    UploadToTexture();
    //LOG_DEBUG("UploadToTexture() completed");
    
    // Setup dirty check timer to poll widgets for updates
    SetupDirtyCheckTimer();
    
    LOG_INFO_FMT("Status bar '{}' created successfully", config_.name);
}

StatusBar::~StatusBar() {
    // Remove dirty check timer
    if (dirty_check_timer_) {
        wl_event_source_remove(dirty_check_timer_);
        dirty_check_timer_ = nullptr;
    }
    
    if (cairo_) {
        cairo_destroy(cairo_);
    }
    if (cairo_surface_) {
        cairo_surface_destroy(cairo_surface_);
    }
    
    if (popover_cairo_) {
        cairo_destroy(popover_cairo_);
    }
    if (popover_cairo_surface_) {
        cairo_surface_destroy(popover_cairo_surface_);
    }
    
    // Clean up SHM buffer (this will also clean up the wlr_buffer)
    if (shm_buffer_) {
        wlr_buffer_drop(shm_buffer_->GetWlrBuffer());
        // ShmBuffer will be deleted by wlr_buffer_drop calling BufferDestroy
        shm_buffer_ = nullptr;
    }
    
    // Clean up popover SHM buffer
    if (popover_shm_buffer_) {
        wlr_buffer_drop(popover_shm_buffer_->GetWlrBuffer());
        popover_shm_buffer_ = nullptr;
    }
    
    if (texture_) {
        wlr_texture_destroy(texture_);
    }
    // Scene nodes are cleaned up automatically by wlroots
    LOG_DEBUG_FMT("Destroyed status bar '{}'", config_.name);
}

void StatusBar::CreateSceneNodes() {
    // Get the working area layer from the LayerManager
    auto* working_layer = layer_manager_->GetLayer(Wayland::Layer::WorkingArea);
    auto* top_layer = layer_manager_->GetLayer(Wayland::Layer::Top);
    
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
    
    // Create scene buffer node for widget rendering in working area
    scene_buffer_ = wlr_scene_buffer_create(working_layer, nullptr);
    wlr_scene_node_set_position(&scene_buffer_->node, pos_x_, pos_y_);
    wlr_scene_node_raise_to_top(&scene_buffer_->node);
    
    // Create scene buffer node for popover rendering in TOP layer
    popover_scene_buffer_ = wlr_scene_buffer_create(top_layer, nullptr);
    wlr_scene_node_set_position(&popover_scene_buffer_->node, 0, 0);  // Will be positioned dynamically
    wlr_scene_node_set_enabled(&popover_scene_buffer_->node, false);  // Start disabled
    wlr_scene_node_raise_to_top(&popover_scene_buffer_->node);
    
    LOG_DEBUG_FMT("Status bar '{}' scene nodes created at ({}, {})", config_.name, pos_x_, pos_y_);
    LOG_DEBUG("Popover scene node created in Top layer");
}

void StatusBar::CreateWidgets() {
    LOG_INFO_FMT("Creating widgets for status bar '{}'", config_.name);
    
    // Recursive widget creation function
    std::function<std::shared_ptr<UI::Widget>(const WidgetConfig&)> create_widget;
    create_widget = [this, &create_widget](const WidgetConfig& widget_config) -> std::shared_ptr<UI::Widget> {
        // Check for container types (hbox, vbox)
        if (widget_config.type == "hbox") {
            auto hbox = std::make_shared<UI::HBox>();
            
            // Apply spacing
            if (widget_config.properties.count("spacing")) {
                hbox->SetSpacing(std::stoi(widget_config.properties.at("spacing")));
            }
            
            // Apply alignment
            if (widget_config.properties.count("alignment")) {
                std::string align = widget_config.properties.at("alignment");
                if (align == "left" || align == "start") {
                    hbox->SetAlign(UI::Align::Start);
                } else if (align == "center") {
                    hbox->SetAlign(UI::Align::Center);
                } else if (align == "right" || align == "end") {
                    hbox->SetAlign(UI::Align::End);
                } else if (align == "fill") {
                    hbox->SetAlign(UI::Align::Fill);
                } else if (align == "apart") {
                    hbox->SetAlign(UI::Align::Apart);
                }
            }
            
            // Recursively create children
            for (const auto& child_config : widget_config.children) {
                auto child = create_widget(child_config);
                if (child) {
                    hbox->AddChild(child);
                }
            }
            
            return hbox;
        }
        else if (widget_config.type == "vbox") {
            auto vbox = std::make_shared<UI::VBox>();
            
            // Apply spacing
            if (widget_config.properties.count("spacing")) {
                vbox->SetSpacing(std::stoi(widget_config.properties.at("spacing")));
            }
            
            // Apply alignment
            if (widget_config.properties.count("alignment")) {
                std::string align = widget_config.properties.at("alignment");
                if (align == "top" || align == "start") {
                    vbox->SetAlign(UI::Align::Start);
                } else if (align == "center") {
                    vbox->SetAlign(UI::Align::Center);
                } else if (align == "bottom" || align == "end") {
                    vbox->SetAlign(UI::Align::End);
                } else if (align == "fill") {
                    vbox->SetAlign(UI::Align::Fill);
                }
            }
            
            // Recursively create children
            for (const auto& child_config : widget_config.children) {
                auto child = create_widget(child_config);
                if (child) {
                    vbox->AddChild(child);
                }
            }
            
            return vbox;
        }
        // Check for built-in widget types
        else if (widget_config.type == "label") {
            auto label = std::make_shared<UI::Label>();
            
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
            
            return label;
        }
        
        // Not a built-in widget - try loading as plugin
        auto& plugin_mgr = UI::WidgetPluginManager::Instance();
        
        if (plugin_mgr.IsPluginLoaded(widget_config.type)) {
            LOG_DEBUG_FMT("Creating plugin widget: {}", widget_config.type);
            
            // Create plugin instance with config properties
            auto plugin_widget = plugin_mgr.CreatePluginWidget(
                widget_config.type,
                widget_config.properties
            );
            
            if (plugin_widget) {
                // Store the shared_ptr to keep it alive
                plugin_widgets_.push_back(plugin_widget);
                return plugin_widget;
            } else {
                LOG_ERROR_FMT("Failed to create plugin widget: {}", widget_config.type);
            }
        } else {
            LOG_WARN_FMT("Unknown widget type '{}' and no plugin found with that name", 
                     widget_config.type);
        }
        
        return nullptr;
    };
    
    // Check if we have a root widget structure (new style)
    if (!config_.root.type.empty()) {
        LOG_INFO("Using new root widget structure");
        auto root_widget = create_widget(config_.root);
        root_container_ = std::dynamic_pointer_cast<UI::Container>(root_widget);
        
        if (!root_container_) {
            LOG_ERROR("Root widget must be a container (hbox or vbox)");
            return;
        }
    }
    // Fall back to legacy left/center/right sections
    else {
        LOG_INFO("Using legacy left/center/right structure");
        
        // Create the container hierarchy (old style)
        root_container_ = std::make_shared<UI::HBox>();
        root_container_->SetPosition(pos_x_, pos_y_);
        root_container_->SetSpacing(0);  // No spacing between sections
        
        left_container_ = std::make_shared<UI::HBox>();
        left_container_->SetAlign(UI::Align::Start);
        left_container_->SetSpacing(config_.left.spacing);
        
        center_container_ = std::make_shared<UI::HBox>();
        center_container_->SetAlign(UI::Align::Center);
        center_container_->SetSpacing(config_.center.spacing);
        
        right_container_ = std::make_shared<UI::HBox>();
        right_container_->SetAlign(UI::Align::End);
        right_container_->SetSpacing(config_.right.spacing);
        
        // Create and add left section widgets
        for (const auto& widget_config : config_.left.widgets) {
            auto widget = create_widget(widget_config);
            if (widget) {
                left_container_->AddChild(widget);
                LOG_DEBUG_FMT("Added widget '{}' to left section", widget_config.type);
            }
        }
        
        // Create and add center section widgets
        for (const auto& widget_config : config_.center.widgets) {
            auto widget = create_widget(widget_config);
            if (widget) {
                center_container_->AddChild(widget);
                LOG_DEBUG_FMT("Added widget '{}' to center section", widget_config.type);
            }
        }
        
        // Create and add right section widgets
        for (const auto& widget_config : config_.right.widgets) {
            auto widget = create_widget(widget_config);
            if (widget) {
                right_container_->AddChild(widget);
                LOG_DEBUG_FMT("Added widget '{}' to right section", widget_config.type);
            }
        }
        
        // Add the three sections to the root container
        root_container_->AddChild(left_container_);
        root_container_->AddChild(center_container_);
        root_container_->AddChild(right_container_);
        
        LOG_INFO_FMT("Created {} left, {} center, {} right widgets",
                 left_container_->GetChildren().size(), 
                 center_container_->GetChildren().size(), 
                 right_container_->GetChildren().size());
    }
}

void StatusBar::RenderToBuffer() {
    // Create SHM buffer if needed (only once)
    if (!shm_buffer_) {
        shm_buffer_ = ShmBuffer::Create(bar_width_, bar_height_);
        if (!shm_buffer_) {
            LOG_ERROR("Failed to create SHM buffer for status bar");
            return;
        }
        
        buffer_data_ = static_cast<uint32_t*>(shm_buffer_->GetData());
        
        // Create Cairo surface for rendering (reuses the same SHM buffer memory)
        cairo_surface_ = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(buffer_data_),
            CAIRO_FORMAT_ARGB32,
            bar_width_,
            bar_height_,
            shm_buffer_->GetStride()
        );
        
        cairo_ = cairo_create(cairo_surface_);
        
        LOG_INFO_FMT("Created Cairo surface {}x{} with SHM buffer", 
                 bar_width_, bar_height_);
    }
    
    // NOTE: We reuse the same buffer for all renders to avoid memory leaks.
    // The buffer memory is redrawn each frame, and wlr_scene_buffer_set_buffer()
    // properly manages the buffer reference count.
    
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
    
    // Use container-based layout system (Flutter-style)
    // The HBox containers automatically handle all positioning and alignment
    if (root_container_) {
        // Check if we're using legacy mode (left/center/right containers exist)
        if (left_container_ && center_container_ && right_container_) {
            // Legacy mode: manually position the three sections
            // Calculate section widths: left/right get dynamic width, center gets remainder
            int section_width = bar_width_ / 3;  // Each section starts with 1/3
            
            // Configure section sizes for better layout
            left_container_->CalculateSize(section_width, bar_height_);
            right_container_->CalculateSize(section_width, bar_height_);
            
            // Calculate actual widths used
            int left_width = left_container_->GetWidth() + config_.left.padding * 2;
            int right_width = right_container_->GetWidth() + config_.right.padding * 2;
            int center_width = bar_width_ - left_width - right_width;
            
            // Position the sections
            left_container_->SetPosition(config_.left.padding, config_.left.padding);
            center_container_->SetPosition(left_width, config_.center.padding);
            right_container_->SetPosition(left_width + center_width, config_.right.padding);
            
            // Recalculate with proper widths
            center_container_->CalculateSize(center_width, bar_height_);
        }
        
        // Let the root container handle all rendering (works for both legacy and new style)
        root_container_->SetPosition(0, 0);
        root_container_->CalculateSize(bar_width_, bar_height_);
        root_container_->Render(cairo_);
        
        // NOTE: Popovers are now rendered to a separate Top layer buffer
        // See RenderPopoverToTopLayer() method
    }
    
    cairo_surface_flush(cairo_surface_);
    
    // DEBUG: Save to PNG to verify rendering works
    static bool saved_debug = false;
    if (!saved_debug) {
        cairo_surface_write_to_png(cairo_surface_, "/tmp/statusbar_debug.png");
        LOG_INFO("Saved status bar rendering to /tmp/statusbar_debug.png for debugging");
        saved_debug = true;
    }
}

void StatusBar::UploadToTexture() {
    if (!buffer_data_ || !scene_buffer_) {
        LOG_WARN("Cannot upload texture - buffer_data or scene_buffer is null");
        return;
    }
    
    if (!shm_buffer_) {
        LOG_ERROR("No SHM buffer available");
        return;
    }
    
    // Get the wlr_buffer from our custom ShmBuffer
    wlr_buffer* wlr_buf = shm_buffer_->GetWlrBuffer();
    
    // Only attach the buffer on the first render
    // On subsequent renders, we reuse the same buffer by calling set_buffer again
    // This is the correct approach: wlroots expects you to call set_buffer whenever
    // the buffer content changes, but since it's the SAME buffer object, the reference
    // counting works correctly (drops old ref, adds new ref = net zero change)
    if (!buffer_attached_) {
        // Get renderer from output (via LayerManager) on first render
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
        
        LOG_INFO("Attaching SHM buffer to scene_buffer (first time)");
        buffer_attached_ = true;
    }
    
    // Call set_buffer on every render to tell wlroots the content changed
    // Since it's the same buffer object, the reference count stays balanced:
    // - Drops reference to old buffer (if any)
    // - Adds reference to new buffer (same object)
    // Net effect: reference count stays at 1 (held by scene_buffer)
    wlr_scene_buffer_set_buffer(scene_buffer_, wlr_buf);
    
    //LOG_DEBUG_FMT("Buffer set on scene (locks={})", wlr_buf->n_locks);
}

void StatusBar::RenderPopoverToTopLayer() {
    // Guard against recursive calls
    if (is_rendering_popover_) {
        LOG_WARN("Preventing recursive popover rendering");
        return;
    }
    is_rendering_popover_ = true;
    
    // Find any visible popover in the widget tree
    std::shared_ptr<UI::Popover> visible_popover = nullptr;
    int popover_x = 0, popover_y = 0;
    
    // Recursive function to find visible popover
    auto find_popover = [&](const std::shared_ptr<UI::Widget>& widget, auto& self) -> void {
        if (visible_popover) return;  // Already found one
        
        if (widget->HasPopover()) {
            auto popover = widget->GetPopover();
            if (popover && popover->IsVisible()) {
                visible_popover = popover;
                // Get popover position (already in screen coordinates)
                popover->GetPosition(popover_x, popover_y);
                return;
            }
        }
        
        // Recursively check children if this is a container
        if (auto container = std::dynamic_pointer_cast<UI::Container>(widget)) {
            for (const auto& child : container->GetChildren()) {
                self(child, self);
            }
        }
    };
    
    if (root_container_) {
        find_popover(root_container_, find_popover);
    }
    
    // If no visible popover, hide the Top layer buffer
    if (!visible_popover) {
        if (popover_scene_buffer_) {
            wlr_scene_node_set_enabled(&popover_scene_buffer_->node, false);
            // Don't log - this happens frequently in normal operation
        }
        is_rendering_popover_ = false;
        return;
    }
    
    LOG_DEBUG("Found visible popover, rendering it");
    
    // Get popover dimensions
    int popover_width = 0, popover_height = 0;
    visible_popover->GetSize(popover_width, popover_height);
    
    // Validate dimensions
    if (popover_width <= 0 || popover_height <= 0) {
        LOG_ERROR_FMT("Invalid popover dimensions: {}x{}", popover_width, popover_height);
        if (popover_scene_buffer_) {
            wlr_scene_node_set_enabled(&popover_scene_buffer_->node, false);
        }
        is_rendering_popover_ = false;
        return;
    }
    
    // Adjust popover position to keep it on screen (anchor point logic)
    // Check right edge - if popover extends beyond screen, shift it left
    if (popover_x + popover_width > static_cast<int>(output_width_)) {
        popover_x = output_width_ - popover_width;
        LOG_DEBUG_FMT("Adjusted popover left to prevent overflow: x={}", popover_x);
    }
    
    // Check bottom edge - if popover extends beyond screen, shift it up
    if (popover_y + popover_height > static_cast<int>(output_height_)) {
        popover_y = output_height_ - popover_height;
        LOG_DEBUG_FMT("Adjusted popover up to prevent overflow: y={}", popover_y);
    }
    
    // Check left edge - clamp to 0
    if (popover_x < 0) {
        popover_x = 0;
        LOG_DEBUG("Clamped popover to left edge");
    }
    
    // Check top edge - clamp to 0
    if (popover_y < 0) {
        popover_y = 0;
        LOG_DEBUG("Clamped popover to top edge");
    }
    
    LOG_DEBUG_FMT("Rendering popover at ({}, {}) size {}x{}", 
              popover_x, popover_y, popover_width, popover_height);
    
    // Create or recreate SHM buffer if size changed
    bool need_new_buffer = false;
    if (!popover_shm_buffer_) {
        need_new_buffer = true;
    } else {
        // Check if size changed - only recreate if dimensions actually changed
        int current_width = cairo_image_surface_get_width(popover_cairo_surface_);
        int current_height = cairo_image_surface_get_height(popover_cairo_surface_);
        if (current_width != popover_width || current_height != popover_height) {
            LOG_DEBUG_FMT("Popover size changed from {}x{} to {}x{}, recreating buffer",
                     current_width, current_height, popover_width, popover_height);
            need_new_buffer = true;
        }
    }
    
    if (need_new_buffer) {
        // Cleanup old resources
        if (popover_cairo_) {
            cairo_destroy(popover_cairo_);
            popover_cairo_ = nullptr;
        }
        if (popover_cairo_surface_) {
            cairo_surface_destroy(popover_cairo_surface_);
            popover_cairo_surface_ = nullptr;
        }
        if (popover_shm_buffer_) {
            wlr_buffer_drop(popover_shm_buffer_->GetWlrBuffer());
            // ShmBuffer will be deleted by wlr_buffer_drop calling BufferDestroy
            popover_shm_buffer_ = nullptr;
            popover_buffer_data_ = nullptr;
        }
        
        // Create new buffer with popover size
        popover_shm_buffer_ = ShmBuffer::Create(popover_width, popover_height);
        if (!popover_shm_buffer_) {
            LOG_ERROR("Failed to create SHM buffer for popover");
            is_rendering_popover_ = false;
            return;
        }
        
        popover_buffer_data_ = static_cast<uint32_t*>(popover_shm_buffer_->GetData());
        
        // Create Cairo surface for the popover
        popover_cairo_surface_ = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(popover_buffer_data_),
            CAIRO_FORMAT_ARGB32,
            popover_width,
            popover_height,
            popover_shm_buffer_->GetStride()
        );
        
        popover_cairo_ = cairo_create(popover_cairo_surface_);
        
        LOG_INFO_FMT("Created popover Cairo surface {}x{}", popover_width, popover_height);
        popover_buffer_attached_ = false;
    }
    
    // Clear the popover surface (fully transparent)
    cairo_save(popover_cairo_);
    cairo_set_operator(popover_cairo_, CAIRO_OPERATOR_CLEAR);
    cairo_paint(popover_cairo_);
    cairo_restore(popover_cairo_);
    
    // Render the popover at (0, 0) on its own surface
    // The popover Render() method expects screen coordinates, so we need to
    // temporarily adjust its position
    int saved_x, saved_y;
    visible_popover->GetPosition(saved_x, saved_y);
    visible_popover->SetPosition(0, 0);
    visible_popover->Render(popover_cairo_);
    visible_popover->SetPosition(saved_x, saved_y);  // Restore original position
    
    cairo_surface_flush(popover_cairo_surface_);
    
    // Upload to scene buffer
    if (!popover_shm_buffer_ || !popover_scene_buffer_) {
        LOG_WARN("Cannot upload popover - missing buffer or scene node");
        is_rendering_popover_ = false;
        return;
    }
    
    // Get the wlr_buffer
    wlr_buffer* wlr_buf = popover_shm_buffer_->GetWlrBuffer();
    
    // Attach buffer on first render
    if (!popover_buffer_attached_) {
        LOG_INFO("Attaching popover SHM buffer to Top layer scene");
        popover_buffer_attached_ = true;
    }
    
    // Set the buffer content
    wlr_scene_buffer_set_buffer(popover_scene_buffer_, wlr_buf);
    
    // Position the scene node at the popover's screen coordinates
    wlr_scene_node_set_position(&popover_scene_buffer_->node, popover_x, popover_y);
    
    // Raise to top to ensure it appears above windows
    wlr_scene_node_raise_to_top(&popover_scene_buffer_->node);
    
    // Make sure the node is visible
    wlr_scene_node_set_enabled(&popover_scene_buffer_->node, true);
    
    LOG_DEBUG_FMT("Popover rendered and uploaded to Top layer at ({}, {})", popover_x, popover_y);
    
    is_rendering_popover_ = false;
}

void StatusBar::Render() {
    //LOG_DEBUG_FMT("StatusBar::Render() called for '{}'", config_.name);
    RenderToBuffer();
    UploadToTexture();
    // Don't render popovers on every render - only when explicitly needed (on click)
}

void StatusBar::Update() {
    // Update widgets that need periodic updates
    // TODO: Call update on dynamic widgets (clock, etc.)
    RenderToBuffer();
    UploadToTexture();
    // Don't render popovers on every update - only when explicitly needed (on click)
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

void StatusBar::CheckDirtyWidgets() {
    bool needs_render = false;
    
    // Helper lambda to recursively check for dirty widgets
    auto check_dirty = [&needs_render](const std::shared_ptr<UI::Widget>& widget, auto& self) -> void {
        if (widget->IsDirty()) {
            needs_render = true;
            return;
        }
        
        // Recursively check children if this is a container
        if (auto container = std::dynamic_pointer_cast<UI::Container>(widget)) {
            for (const auto& child : container->GetChildren()) {
                self(child, self);
                if (needs_render) return;  // Early exit
            }
        }
    };
    
    // Check all widgets in the root container
    if (root_container_) {
        check_dirty(root_container_, check_dirty);
    }
    
    // If any widget needs re-rendering, trigger a full render
    if (needs_render) {
        //LOG_DEBUG_FMT("Dirty widgets detected, triggering re-render for '{}'", config_.name);
        Render();
        
        // Helper lambda to recursively clear dirty flags
        auto clear_dirty = [](const std::shared_ptr<UI::Widget>& widget, auto& self) -> void {
            widget->ClearDirty();
            
            // Recursively clear children if this is a container
            if (auto container = std::dynamic_pointer_cast<UI::Container>(widget)) {
                for (const auto& child : container->GetChildren()) {
                    self(child, self);
                }
            }
        };
        
        // Clear dirty flags after rendering
        if (root_container_) {
            clear_dirty(root_container_, clear_dirty);
        }
    }
}

void StatusBar::SetupDirtyCheckTimer() {
    if (!event_loop_) {
        LOG_WARN("No event loop available for dirty check timer");
        return;
    }
    
    // Check for dirty widgets every 100ms (10 times per second)
    dirty_check_timer_ = wl_event_loop_add_timer(event_loop_, OnDirtyCheckTimer, this);
    if (!dirty_check_timer_) {
        LOG_ERROR("Failed to create dirty check timer for status bar");
        return;
    }
    
    // Start the timer
    wl_event_source_timer_update(dirty_check_timer_, 100);  // 100ms
    
    LOG_DEBUG("Status bar dirty check timer created (interval: 100ms)");
}

int StatusBar::OnDirtyCheckTimer(void* data) {
    StatusBar* bar = static_cast<StatusBar*>(data);
    
    // Check if any widgets need re-rendering
    bar->CheckDirtyWidgets();
    
    // Re-arm the timer for next check
    wl_event_source_timer_update(bar->dirty_check_timer_, 100);  // 100ms
    
    return 0;  // Return value is ignored
}

bool StatusBar::HandleClick(int x, int y) {
    LOG_DEBUG_FMT("StatusBar::HandleClick at ({}, {})", x, y);
    
    // Check if click is within bar bounds
    if (x < pos_x_ || x > pos_x_ + bar_width_ ||
        y < pos_y_ || y > pos_y_ + bar_height_) {
        return false;
    }
    
    // Helper lambda to recursively handle clicks
    bool needs_render = false;
    auto handle_click_recursive = [&](const std::shared_ptr<UI::Widget>& widget, auto& self) -> bool {
        // First check if any widget has a visible popover
        if (widget->HasPopover()) {
            auto popover = widget->GetPopover();
            if (popover && popover->IsVisible()) {
                if (popover->HandleClick(x, y)) {
                    needs_render = true;
                    return true;
                }
                // Click outside popover - hide it
                popover->Hide();
                needs_render = true;
                return true;  // Consume the click
            }
        }
        
        // Recursively check children first (top-to-bottom order)
        if (auto container = std::dynamic_pointer_cast<UI::Container>(widget)) {
            for (const auto& child : container->GetChildren()) {
                if (self(child, self)) {
                    return true;
                }
            }
        }
        
        // Then check the widget itself
        if (widget->HandleClick(x, y)) {
            needs_render = true;
            return true;
        }
        return false;
    };
    
    // Handle click through the root container
    if (root_container_) {
        bool handled = handle_click_recursive(root_container_, handle_click_recursive);
        if (needs_render) {
            LOG_DEBUG("Widget handled click, triggering render");
            Render();
            RenderPopoverToTopLayer();  // Update popover layer after click
        }
        return handled;
    }

    return false;
}bool StatusBar::HandleHover(int x, int y) {
    // Check if hover is within bar bounds
    if (x < pos_x_ || x > pos_x_ + bar_width_ ||
        y < pos_y_ || y > pos_y_ + bar_height_) {
        return false;
    }
    
    // Helper lambda to recursively handle hover
    auto handle_hover_recursive = [&](const std::shared_ptr<UI::Widget>& widget, auto& self) -> bool {
        // First check if any widget has a visible popover
        if (widget->HasPopover()) {
            auto popover = widget->GetPopover();
            if (popover && popover->IsVisible()) {
                if (popover->HandleHover(x, y)) {
                    Render();  // Re-render to show hover effects
                    RenderPopoverToTopLayer();  // Update popover layer
                    return true;
                }
            }
        }
        
        // Recursively check children first (top-to-bottom order)
        if (auto container = std::dynamic_pointer_cast<UI::Container>(widget)) {
            for (const auto& child : container->GetChildren()) {
                if (self(child, self)) {
                    return true;
                }
            }
        }
        
        // Then check the widget itself
        return widget->HandleHover(x, y);
    };
    
    // Handle hover through the root container
    if (root_container_) {
        return handle_hover_recursive(root_container_, handle_hover_recursive);
    }
    
    return false;
}

} // namespace Leviathan

