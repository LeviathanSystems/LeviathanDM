#include "TagsWidget.hpp"
#include "ui/reusable-widgets/Button.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace Leviathan {
namespace Plugins {

TagsWidget::TagsWidget()
      : event_subscription_id_(-1) {
}

TagsWidget::~TagsWidget() {
    // Unsubscribe from events
    if (event_subscription_id_ >= 0) {
        UI::Plugin::UnsubscribeFromEvent(event_subscription_id_);
    }
}

UI::PluginMetadata TagsWidget::GetMetadata() const {
    return {
        .name = "TagsWidget",
        .version = "1.0.0",
        .author = "LeviathanDM",
        .description = "Displays workspace tags with active and occupied states",
        .api_version = UI::WIDGET_API_VERSION
    };
}

bool TagsWidget::InitializeImpl(const std::map<std::string, std::string>& config) {
    // Parse configuration
    auto it = config.find("font_size");
    if (it != config.end()) {
        font_size_ = std::stoi(it->second);
    }
    
    it = config.find("tag_spacing");
    if (it != config.end()) {
        tag_spacing_ = std::stoi(it->second);
    }
    
    it = config.find("tag_padding_h");
    if (it != config.end()) {
        tag_padding_h_ = std::stoi(it->second);
    }
    
    it = config.find("tag_padding_v");
    if (it != config.end()) {
        tag_padding_v_ = std::stoi(it->second);
    }
    
    it = config.find("border_radius");
    if (it != config.end()) {
        border_radius_ = std::stoi(it->second);
    }
    
    it = config.find("active_bg_color");
    if (it != config.end()) {
        active_bg_color_ = it->second;
    }
    
    it = config.find("active_fg_color");
    if (it != config.end()) {
        active_fg_color_ = it->second;
    }
    
    it = config.find("occupied_bg_color");
    if (it != config.end()) {
        occupied_bg_color_ = it->second;
    }
    
    it = config.find("occupied_fg_color");
    if (it != config.end()) {
        occupied_fg_color_ = it->second;
    }
    
    it = config.find("empty_bg_color");
    if (it != config.end()) {
        empty_bg_color_ = it->second;
    }
    
    it = config.find("empty_fg_color");
    if (it != config.end()) {
        empty_fg_color_ = it->second;
    }
    
    it = config.find("show_icons");
    if (it != config.end()) {
        show_icons_ = (it->second == "true" || it->second == "1");
    }
    
    it = config.find("show_empty_tags");
    if (it != config.end()) {
        show_empty_tags_ = (it->second == "true" || it->second == "1");
    }
    
    // Initial fetch BEFORE subscribing to events
    // This ensures tags_ vector is populated before any events fire
    FetchTagsFromCompositor();
    
    // Build initial button set
    RebuildTagButtons();
    
    // Subscribe to compositor events
    event_subscription_id_ = UI::Plugin::SubscribeToEvent(
        UI::Plugin::EventType::TagSwitched,
        [this](const UI::Plugin::Event& event) {
            OnCompositorEvent(event);
        }
    );
    
    // Also subscribe to client added/removed to update occupied state
    // TEMPORARILY DISABLED TO DEBUG CRASH
    UI::Plugin::SubscribeToEvent(
        UI::Plugin::EventType::ClientAdded,
        [this](const UI::Plugin::Event& event) {
            OnCompositorEvent(event);
        }
    );
    
    
    UI::Plugin::SubscribeToEvent(
        UI::Plugin::EventType::ClientRemoved,
        [this](const UI::Plugin::Event& event) {
            OnCompositorEvent(event);
        }
    );
    
    return true;
}

void TagsWidget::CleanupImpl() {
    // Nothing special to clean up
}

void TagsWidget::UpdateData() {
    // Fetch current tag state from compositor
    FetchTagsFromCompositor();
    RebuildTagButtons();
}

void TagsWidget::OnCompositorEvent(const UI::Plugin::Event& event) {
    // Any tag-related event means we should refresh
    try {
        //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "TagsWidget", "Handling event type {}", static_cast<int>(event.type));
        FetchTagsFromCompositor();
        //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "TagsWidget", "Tags fetched successfully");
        RebuildTagButtons();
        //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "TagsWidget", "Buttons rebuilt successfully");
        RequestRender();
        //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "TagsWidget", "Render requested successfully");
    } catch (const std::exception& e) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "TagsWidget", "OnCompositorEvent: Exception: {}", e.what());
    } catch (...) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "TagsWidget", "OnCompositorEvent: Unknown exception");
    }
}

void TagsWidget::FetchTagsFromCompositor() {
    //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "TagsWidget", "FetchTagsFromCompositor - Start");
    
    auto* compositor = UI::GetCompositorState();
    if (!compositor) {
        // Compositor is not available
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "TagsWidget", "Compositor state not available");
        return;
    }
    
    // No lock needed - tags_ only accessed on main thread
    // Background events trigger fetch, but execution is serialized
    
    //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "TagsWidget", "Clearing {} existing tags", tags_.size());
    tags_.clear();
    //Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "TagsWidget", "Tags cleared");
    
    // Get all tags from compositor
    auto tags = compositor->GetTags();
    auto* active_tag = compositor->GetActiveTag();
    
    int tag_id = 1;
    for (auto* tag : tags) {
        TagInfo info;
        info.id = tag_id++;
        
        // Parse name and icon from tag name using Plugin API
        std::string tag_name = UI::Plugin::GetTagName(tag);
        
        // Simple heuristic: if first character is emoji (>= 0x80 in UTF-8)
        // then split into icon and name
        if (!tag_name.empty() && (static_cast<unsigned char>(tag_name[0]) >= 0x80)) {
            // Has icon - find first space
            size_t space_pos = tag_name.find(' ');
            if (space_pos != std::string::npos) {
                info.icon = tag_name.substr(0, space_pos);
                info.name = tag_name.substr(space_pos + 1);
            } else {
                info.name = tag_name;
            }
        } else {
            info.name = tag_name;
        }
        
        info.is_active = (tag == active_tag);
        info.has_clients = (UI::Plugin::GetTagClientCount(tag) > 0);
        
        tags_.push_back(info);
    }
}

void TagsWidget::RebuildTagButtons() {
    // No lock needed - only called from main thread (event handlers run on main thread)
    
    // Clear existing buttons
    tag_buttons_.clear();
    
    // Create a button for each tag
    for (const auto& tag : tags_) {
        if (!show_empty_tags_ && !tag.has_clients && !tag.is_active) {
            continue;
        }
        
        // Prepare label
        std::string label = tag.name;
        if (show_icons_ && !tag.icon.empty()) {
            label = tag.icon + " " + tag.name;
        }
        
        auto button = std::make_shared<UI::Button>(label);
        
        // Set button properties
        button->SetFontSize(font_size_);
        button->SetPadding(tag_padding_h_, tag_padding_v_);
        button->SetBorderRadius(border_radius_);
        
        // Set colors based on tag state
        std::string bg_color, fg_color, hover_color;
        if (tag.is_active) {
            bg_color = active_bg_color_;
            fg_color = active_fg_color_;
            // Lighter hover color for active tag
            hover_color = LightenColor(active_bg_color_, 0.15);
        } else if (tag.has_clients) {
            bg_color = occupied_bg_color_;
            fg_color = occupied_fg_color_;
            hover_color = LightenColor(occupied_bg_color_, 0.2);
        } else {
            bg_color = empty_bg_color_;
            fg_color = empty_fg_color_;
            hover_color = LightenColor(empty_bg_color_, 0.25);
        }
        
        // Parse and set background color
        if (bg_color.size() >= 7 && bg_color[0] == '#') {
            int r, g, b;
            sscanf(bg_color.c_str(), "#%02x%02x%02x", &r, &g, &b);
            button->SetBackgroundColor(r/255.0, g/255.0, b/255.0);
        }
        
        // Parse and set hover color
        if (hover_color.size() >= 7 && hover_color[0] == '#') {
            int r, g, b;
            sscanf(hover_color.c_str(), "#%02x%02x%02x", &r, &g, &b);
            button->SetHoverColor(r/255.0, g/255.0, b/255.0);
        }
        
        // Parse and set text color
        if (fg_color.size() >= 7 && fg_color[0] == '#') {
            int r, g, b;
            sscanf(fg_color.c_str(), "#%02x%02x%02x", &r, &g, &b);
            button->SetTextColor(r/255.0, g/255.0, b/255.0);
        }
        
        // Set click handler
        int tag_id = tag.id;
        button->SetOnClick([this, tag_id]() {
            OnTagClicked(tag_id);
        });
        
        tag_buttons_.push_back(button);
    }
}

std::string TagsWidget::LightenColor(const std::string& hex_color, double amount) {
    if (hex_color.size() < 7 || hex_color[0] != '#') {
        return hex_color;
    }
    
    int r, g, b;
    sscanf(hex_color.c_str(), "#%02x%02x%02x", &r, &g, &b);
    
    // Lighten by mixing with white
    r = std::min(255, static_cast<int>(r + (255 - r) * amount));
    g = std::min(255, static_cast<int>(g + (255 - g) * amount));
    b = std::min(255, static_cast<int>(b + (255 - b) * amount));
    
    char result[8];
    snprintf(result, sizeof(result), "#%02x%02x%02x", r, g, b);
    return std::string(result);
}

void TagsWidget::OnTagClicked(int tag_id) {
    // Tag IDs start at 1, but SwitchToTag expects 0-based index
    UI::Plugin::SwitchToTag(tag_id - 1);
}

void TagsWidget::CalculateSize(int available_width, int available_height) {
    
    // Calculate total width needed from all buttons
    int total_width = 0;
    int max_height = 0;
    
    for (auto& button : tag_buttons_) {
        if (!button) continue;
        
        // Calculate button size
        button->CalculateSize(available_width, available_height);
        
        total_width += button->GetWidth() + tag_spacing_;
        max_height = std::max(max_height, button->GetHeight());
    }
    
    // Remove last spacing
    if (!tag_buttons_.empty()) {
        total_width -= tag_spacing_;
    }
    
    width_ = std::min(total_width, available_width);
    height_ = max_height;
}

void TagsWidget::Render(cairo_t* cr) {
    if (!IsVisible()) return;
    
    // Translate to widget position
    cairo_save(cr);
    cairo_translate(cr, x_, y_);
    
    int x_offset = 0;
    
    // Render each button
    for (auto& button : tag_buttons_) {
        if (!button) continue;
        
        // Calculate button size
        button->CalculateSize(width_, height_);
        
        // Position button
        cairo_save(cr);
        cairo_translate(cr, x_offset, 0);
        
        // Render the button
        button->Render(cr);
        
        cairo_restore(cr);
        
        // Update offset for next button
        x_offset += button->GetWidth() + tag_spacing_;
    }
    
    cairo_restore(cr);
}

bool TagsWidget::HandleClick(int click_x, int click_y) {
    
    // Check if click is within widget bounds
    if (click_x < GetX() || click_x > GetX() + GetWidth() ||
        click_y < GetY() || click_y > GetY() + GetHeight()) {
        return false;
    }
    
    // Convert click coordinates to widget-local coordinates
    int local_x = click_x - GetX();
    int local_y = click_y - GetY();
    
    // Find which button was clicked
    int x_offset = 0;
    for (auto& button : tag_buttons_) {
        if (!button) continue;
        
        int button_width = button->GetWidth();
        
        // Check if click is within this button
        if (local_x >= x_offset && local_x < x_offset + button_width) {
            button->Click();
            return true;
        }
        
        x_offset += button_width + tag_spacing_;
    }
    
    return false;
}

} // namespace Plugins
} // namespace Leviathan

// Plugin exports - need to be in correct namespace for macros
namespace Leviathan {
namespace UI {
    // Wrapper to create our plugin (required for macro compatibility)
    using TagsWidgetWrapper = Plugins::TagsWidget;
}
}

// Plugin interface
extern "C" {
    Leviathan::UI::WidgetPlugin* CreatePlugin() {
        return new Leviathan::Plugins::TagsWidget();
    }
    
    void DestroyPlugin(Leviathan::UI::WidgetPlugin* plugin) {
        delete plugin;
    }
    
    Leviathan::UI::PluginMetadata GetPluginMetadata() {
        return {
            .name = "TagsWidget",
            .version = "1.0.0",
            .author = "LeviathanDM",
            .description = "Displays workspace tags with active and occupied states",
            .api_version = Leviathan::UI::WIDGET_API_VERSION
        };
    }
}
