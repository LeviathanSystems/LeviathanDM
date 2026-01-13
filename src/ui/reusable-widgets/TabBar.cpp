#include "ui/reusable-widgets/TabBar.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cmath>

namespace Leviathan {
namespace UI {

TabBar::TabBar(const TabBarConfig& config)
    : config_(config)
    , active_index_(-1)
    , hover_index_(-1)
    , on_tab_change_(nullptr) {
}

void TabBar::AddTab(const Tab& tab) {
    // Check for duplicate IDs
    if (HasTab(tab.id)) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "Tab with ID '{}' already exists", tab.id);
        return;
    }
    
    tabs_.push_back(tab);
    
    // If this is the first tab, make it active
    if (tabs_.size() == 1) {
        active_index_ = 0;
        NotifyTabChange();
    }
}

void TabBar::AddTab(const std::string& id, const std::string& label, const std::string& icon) {
    AddTab(Tab(id, label, icon));
}

void TabBar::RemoveTab(const std::string& id) {
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
                           [&id](const Tab& t) { return t.id == id; });
    
    if (it == tabs_.end()) {
        return;
    }
    
    int index = std::distance(tabs_.begin(), it);
    tabs_.erase(it);
    
    // Adjust active index if needed
    if (active_index_ == index) {
        // Active tab was removed, select next or previous
        if (active_index_ >= static_cast<int>(tabs_.size())) {
            active_index_ = tabs_.size() - 1;
        }
        NotifyTabChange();
    } else if (active_index_ > index) {
        // Active tab is after removed tab, adjust index
        active_index_--;
    }
    
    // Reset hover if hovering over removed tab
    if (hover_index_ == index) {
        hover_index_ = -1;
    } else if (hover_index_ > index) {
        hover_index_--;
    }
}

void TabBar::ClearTabs() {
    tabs_.clear();
    active_index_ = -1;
    hover_index_ = -1;
}

void TabBar::SetActiveTab(const std::string& id) {
    for (size_t i = 0; i < tabs_.size(); i++) {
        if (tabs_[i].id == id && tabs_[i].enabled) {
            if (active_index_ != static_cast<int>(i)) {
                active_index_ = i;
                NotifyTabChange();
            }
            return;
        }
    }
}

void TabBar::SetActiveTab(int index) {
    if (index >= 0 && index < static_cast<int>(tabs_.size()) && tabs_[index].enabled) {
        if (active_index_ != index) {
            active_index_ = index;
            NotifyTabChange();
        }
    }
}

std::string TabBar::GetActiveTabId() const {
    if (active_index_ >= 0 && active_index_ < static_cast<int>(tabs_.size())) {
        return tabs_[active_index_].id;
    }
    return "";
}

bool TabBar::HasTab(const std::string& id) const {
    return std::any_of(tabs_.begin(), tabs_.end(),
                      [&id](const Tab& t) { return t.id == id; });
}

const Tab* TabBar::GetTab(const std::string& id) const {
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
                           [&id](const Tab& t) { return t.id == id; });
    return it != tabs_.end() ? &(*it) : nullptr;
}

void TabBar::SetTabEnabled(const std::string& id, bool enabled) {
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
                           [&id](const Tab& t) { return t.id == id; });
    
    if (it != tabs_.end()) {
        it->enabled = enabled;
        
        // If we disabled the active tab, switch to next enabled tab
        if (!enabled && active_index_ == std::distance(tabs_.begin(), it)) {
            SelectNext();
            if (!tabs_[active_index_].enabled) {
                SelectFirst();  // Fallback to first enabled tab
            }
        }
    }
}

void TabBar::SelectNext() {
    if (tabs_.empty()) return;
    
    int start = active_index_;
    int next = (active_index_ + 1) % tabs_.size();
    
    // Find next enabled tab
    while (next != start) {
        if (tabs_[next].enabled) {
            SetActiveTab(next);
            return;
        }
        next = (next + 1) % tabs_.size();
    }
}

void TabBar::SelectPrevious() {
    if (tabs_.empty()) return;
    
    int start = active_index_;
    int prev = (active_index_ - 1 + tabs_.size()) % tabs_.size();
    
    // Find previous enabled tab
    while (prev != start) {
        if (tabs_[prev].enabled) {
            SetActiveTab(prev);
            return;
        }
        prev = (prev - 1 + tabs_.size()) % tabs_.size();
    }
}

void TabBar::SelectFirst() {
    for (size_t i = 0; i < tabs_.size(); i++) {
        if (tabs_[i].enabled) {
            SetActiveTab(i);
            return;
        }
    }
}

void TabBar::SelectLast() {
    for (int i = tabs_.size() - 1; i >= 0; i--) {
        if (tabs_[i].enabled) {
            SetActiveTab(i);
            return;
        }
    }
}

bool TabBar::HandleClick(int x, int y) {
    // This is called with x,y relative to the tab bar
    // We need the container position to calculate tab bounds
    // For now, assume x,y are already relative to tab bar position
    return false;  // Implemented in overload below
}

bool TabBar::HandleHover(int x, int y) {
    return false;  // Implemented in overload below
}

void TabBar::HandleKeyPress(uint32_t key) {
    // Handle arrow keys (Linux keycodes)
    switch (key) {
        case 105:  // Left arrow
        case 113:  // Left arrow (alternative)
            SelectPrevious();
            break;
        case 106:  // Right arrow
        case 114:  // Right arrow (alternative)
            SelectNext();
            break;
        case 110:  // Home
            SelectFirst();
            break;
        case 115:  // End
            SelectLast();
            break;
    }
}

void TabBar::Render(cairo_t* cr, int x, int y, int width) {
    if (tabs_.empty()) return;
    
    // Draw background
    cairo_set_source_rgba(cr,
                         config_.background_color.r,
                         config_.background_color.g,
                         config_.background_color.b,
                         config_.background_color.a);
    cairo_rectangle(cr, x, y, width, config_.height);
    cairo_fill(cr);
    
    // Calculate tab widths
    std::vector<int> tab_widths;
    CalculateTabWidths(width, tab_widths);
    
    // Draw tabs
    int current_x = x;
    for (size_t i = 0; i < tabs_.size(); i++) {
        const Tab& tab = tabs_[i];
        int tab_width = tab_widths[i];
        
        // Determine tab color
        double bg_r, bg_g, bg_b, bg_a;
        double text_r, text_g, text_b, text_a;
        
        if (static_cast<int>(i) == active_index_) {
            bg_r = config_.active_tab_color.r;
            bg_g = config_.active_tab_color.g;
            bg_b = config_.active_tab_color.b;
            bg_a = config_.active_tab_color.a;
            text_r = config_.text_color.r;
            text_g = config_.text_color.g;
            text_b = config_.text_color.b;
            text_a = config_.text_color.a;
        } else if (static_cast<int>(i) == hover_index_) {
            bg_r = config_.hover_tab_color.r;
            bg_g = config_.hover_tab_color.g;
            bg_b = config_.hover_tab_color.b;
            bg_a = config_.hover_tab_color.a;
            text_r = config_.text_color.r;
            text_g = config_.text_color.g;
            text_b = config_.text_color.b;
            text_a = config_.text_color.a;
        } else {
            bg_r = config_.inactive_tab_color.r;
            bg_g = config_.inactive_tab_color.g;
            bg_b = config_.inactive_tab_color.b;
            bg_a = config_.inactive_tab_color.a;
            text_r = config_.inactive_text_color.r;
            text_g = config_.inactive_text_color.g;
            text_b = config_.inactive_text_color.b;
            text_a = config_.inactive_text_color.a;
        }
        
        if (!tab.enabled) {
            text_a *= 0.5;  // Dim disabled tabs
        }
        
        // Draw tab background
        cairo_set_source_rgba(cr, bg_r, bg_g, bg_b, bg_a);
        cairo_rectangle(cr, current_x, y, tab_width, config_.height);
        cairo_fill(cr);
        
        // Draw separator if needed
        if (config_.show_separators && i < tabs_.size() - 1) {
            cairo_set_source_rgba(cr,
                                 config_.separator_color.r,
                                 config_.separator_color.g,
                                 config_.separator_color.b,
                                 config_.separator_color.a);
            cairo_rectangle(cr, current_x + tab_width, y, config_.separator_width, config_.height);
            cairo_fill(cr);
        }
        
        // Draw tab text
        cairo_select_font_face(cr, config_.font_family.c_str(),
                              CAIRO_FONT_SLANT_NORMAL,
                              (config_.bold_active && static_cast<int>(i) == active_index_) 
                                  ? CAIRO_FONT_WEIGHT_BOLD 
                                  : CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, config_.font_size);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, tab.label.c_str(), &extents);
        
        int text_x, text_y;
        if (config_.center_text) {
            text_x = current_x + (tab_width - extents.width) / 2;
        } else {
            text_x = current_x + config_.tab_padding;
        }
        text_y = y + (config_.height + extents.height) / 2;
        
        cairo_set_source_rgba(cr, text_r, text_g, text_b, text_a);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, tab.label.c_str());
        
        current_x += tab_width;
        if (config_.show_separators && i < tabs_.size() - 1) {
            current_x += config_.separator_width;
        }
    }
    
    // Draw border if needed
    if (config_.border_width > 0) {
        cairo_set_source_rgba(cr,
                             config_.border_color.r,
                             config_.border_color.g,
                             config_.border_color.b,
                             config_.border_color.a);
        cairo_set_line_width(cr, config_.border_width);
        cairo_rectangle(cr, x, y, width, config_.height);
        cairo_stroke(cr);
    }
}

int TabBar::GetPreferredWidth() const {
    if (tabs_.empty()) return 0;
    
    // Create a temporary cairo surface to measure text
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, config_.font_family.c_str(),
                          CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(temp_cr, config_.font_size);
    
    int total_width = 0;
    for (const auto& tab : tabs_) {
        int tab_width = GetTabTextWidth(tab, temp_cr);
        total_width += std::min(config_.tab_max_width, std::max(config_.tab_min_width, tab_width));
    }
    
    if (config_.show_separators) {
        total_width += (tabs_.size() - 1) * config_.separator_width;
    }
    
    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);
    
    return total_width;
}

std::vector<TabBar::TabBounds> TabBar::GetTabBounds(int container_x, int container_y, int container_width) const {
    std::vector<TabBounds> bounds;
    if (tabs_.empty()) return bounds;
    
    std::vector<int> tab_widths;
    CalculateTabWidths(container_width, tab_widths);
    
    int current_x = container_x;
    for (size_t i = 0; i < tabs_.size(); i++) {
        TabBounds b;
        b.x = current_x;
        b.y = container_y;
        b.width = tab_widths[i];
        b.height = config_.height;
        b.id = tabs_[i].id;
        bounds.push_back(b);
        
        current_x += tab_widths[i];
        if (config_.show_separators && i < tabs_.size() - 1) {
            current_x += config_.separator_width;
        }
    }
    
    return bounds;
}

void TabBar::CalculateTabWidths(int available_width, std::vector<int>& widths) const {
    widths.clear();
    if (tabs_.empty()) return;
    
    if (config_.equal_width_tabs) {
        // All tabs get equal width
        int separator_space = config_.show_separators ? (tabs_.size() - 1) * config_.separator_width : 0;
        int tab_width = (available_width - separator_space) / tabs_.size();
        widths.resize(tabs_.size(), tab_width);
    } else {
        // Tabs sized based on content, within min/max bounds
        cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* temp_cr = cairo_create(temp_surface);
        
        cairo_select_font_face(temp_cr, config_.font_family.c_str(),
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(temp_cr, config_.font_size);
        
        int total_width = 0;
        for (const auto& tab : tabs_) {
            int tab_width = GetTabTextWidth(tab, temp_cr);
            tab_width = std::min(config_.tab_max_width, std::max(config_.tab_min_width, tab_width));
            widths.push_back(tab_width);
            total_width += tab_width;
        }
        
        cairo_destroy(temp_cr);
        cairo_surface_destroy(temp_surface);
        
        // If tabs don't fill available width, distribute extra space
        int separator_space = config_.show_separators ? (tabs_.size() - 1) * config_.separator_width : 0;
        if (total_width + separator_space < available_width) {
            int extra = available_width - total_width - separator_space;
            int per_tab = extra / tabs_.size();
            int remainder = extra % tabs_.size();
            
            for (size_t i = 0; i < widths.size(); i++) {
                widths[i] += per_tab;
                if (static_cast<int>(i) < remainder) {
                    widths[i]++;
                }
            }
        }
    }
}

int TabBar::GetTabTextWidth(const Tab& tab, cairo_t* cr) const {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, tab.label.c_str(), &extents);
    return extents.width + 2 * config_.tab_padding;
}

int TabBar::FindTabAtPosition(int x, int y, int container_x, int container_y, int container_width) const {
    if (y < container_y || y > container_y + config_.height) {
        return -1;
    }
    
    auto bounds = GetTabBounds(container_x, container_y, container_width);
    for (size_t i = 0; i < bounds.size(); i++) {
        if (x >= bounds[i].x && x < bounds[i].x + bounds[i].width) {
            return i;
        }
    }
    
    return -1;
}

void TabBar::NotifyTabChange() {
    if (on_tab_change_ && active_index_ >= 0 && active_index_ < static_cast<int>(tabs_.size())) {
        on_tab_change_(tabs_[active_index_].id, active_index_);
    }
}

} // namespace UI
} // namespace Leviathan
