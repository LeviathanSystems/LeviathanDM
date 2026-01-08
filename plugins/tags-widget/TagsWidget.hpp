#pragma once

#include "ui/PeriodicWidget.hpp"
#include "ui/PluginAPI.hpp"
#include "ui/reusable-widgets/Button.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Leviathan {
namespace Plugins {

struct TagInfo {
    int id;
    std::string name;
    std::string icon;
    bool is_active;    // Is this the currently active/focused tag
    bool has_clients;  // Does this tag have any windows
};

class TagsWidget : public UI::PeriodicWidget {
public:
    TagsWidget();
    ~TagsWidget() override;
    
    // WidgetPlugin interface
    UI::PluginMetadata GetMetadata() const override;
    
    // Widget interface
    void CalculateSize(int available_width, int available_height) override;
    void Render(cairo_t* cr) override;
    bool HandleClick(int click_x, int click_y) override;
    
protected:
    // PeriodicWidget interface
    bool InitializeImpl(const std::map<std::string, std::string>& config) override;
    void UpdateData() override;
    void CleanupImpl() override;
    
private:
    void FetchTagsFromCompositor();
    void RebuildTagButtons();
    void OnCompositorEvent(const UI::Plugin::Event& event);
    void OnTagClicked(int tag_id);
    std::string LightenColor(const std::string& hex_color, double amount);
    
    std::vector<TagInfo> tags_;
    std::vector<std::shared_ptr<UI::Button>> tag_buttons_;
    int event_subscription_id_;  // For unsubscribing from events
    
    // Configuration
    int font_size_ = 12;
    int tag_spacing_ = 5;          // Space between tags
    int tag_padding_h_ = 8;        // Horizontal padding inside tag box
    int tag_padding_v_ = 4;        // Vertical padding inside tag box
    int border_radius_ = 6;        // Border radius for tags
    std::string active_bg_color_ = "#5E81AC";      // Nord bluecag
    std::string active_fg_color_ = "#ECEFF4";      // Nord snow storm
    std::string occupied_bg_color_ = "#3B4252";    // Nord dark gray
    std::string occupied_fg_color_ = "#D8DEE9";    // Nord lighter gray
    std::string empty_bg_color_ = "#2E3440";       // Nord darkest
    std::string empty_fg_color_ = "#4C566A";       // Nord muted gray
    bool show_icons_ = true;
    bool show_empty_tags_ = true;  // Show tags with no windows
};

} // namespace Plugins
} // namespace Leviathan
