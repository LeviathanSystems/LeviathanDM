#include "ui/MenuBar.hpp"
#include "wayland/LayerManager.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cctype>

extern "C" {
#include <wlr/types/wlr_scene.h>
#include <wlr/render/wlr_renderer.h>
}

namespace Leviathan {
namespace UI {

// MenuItem base implementation
bool MenuItem::Matches(const std::string& query) const {
    if (query.empty()) return true;
    
    // Check display name
    std::string name_lower = GetDisplayName();
    std::string query_lower = query;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    if (name_lower.find(query_lower) != std::string::npos) {
        return true;
    }
    
    // Check keywords
    for (const auto& keyword : GetSearchKeywords()) {
        std::string kw_lower = keyword;
        std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
        if (kw_lower.find(query_lower) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// MenuBar implementation
MenuBar::MenuBar(const MenuBarConfig& config,
                 Wayland::LayerManager* layer_manager,
                 struct wl_event_loop* event_loop,
                 uint32_t output_width,
                 uint32_t output_height)
    : config_(config)
    , layer_manager_(layer_manager)
    , event_loop_(event_loop)
    , scene_rect_(nullptr)
    , scene_buffer_(nullptr)
    , texture_(nullptr)
    , renderer_(nullptr)
    , shm_buffer_(nullptr)
    , buffer_attached_(false)
    , pos_x_(0)
    , pos_y_(0)
    , output_width_(output_width)
    , output_height_(output_height)
    , cairo_surface_(nullptr)
    , cairo_(nullptr)
    , buffer_data_(nullptr)
    , is_visible_(false)
    , selected_index_(0)
    , scroll_offset_(0)
{
    // Calculate dimensions
    bar_width_ = output_width_;
    bar_height_ = config_.height + (config_.item_height * config_.max_visible_items);
    
    // Create buffer
    shm_buffer_ = ShmBuffer::Create(bar_width_, bar_height_);
    if (!shm_buffer_) {
        LOG_ERROR("Failed to create ShmBuffer for menubar");
        return;
    }
    buffer_data_ = reinterpret_cast<uint32_t*>(shm_buffer_->GetData());
    
    // Create cairo surface
    cairo_surface_ = cairo_image_surface_create_for_data(
        reinterpret_cast<unsigned char*>(buffer_data_),
        CAIRO_FORMAT_ARGB32,
        bar_width_,
        bar_height_,
        cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bar_width_)
    );
    cairo_ = cairo_create(cairo_surface_);
    
    CreateSceneNodes();
    
    LOG_INFO_FMT("MenuBar created: {}x{}", bar_width_, bar_height_);
}

MenuBar::~MenuBar() {
    if (cairo_) cairo_destroy(cairo_);
    if (cairo_surface_) cairo_surface_destroy(cairo_surface_);
    if (texture_) wlr_texture_destroy(texture_);
    
    // Clean up SHM buffer (drop reference)
    if (shm_buffer_) {
        wlr_buffer_drop(shm_buffer_->GetWlrBuffer());
        shm_buffer_ = nullptr;
    }
    
    LOG_INFO("MenuBar destroyed");
}

void MenuBar::CreateSceneNodes() {
    // Get the top layer from the layer manager
    auto* top_layer = layer_manager_->GetTopLayer();
    
    // Create background rectangle (hidden initially)
    float bg_color[4] = {
        static_cast<float>(config_.background_color.r),
        static_cast<float>(config_.background_color.g),
        static_cast<float>(config_.background_color.b),
        static_cast<float>(config_.background_color.a)
    };
    scene_rect_ = wlr_scene_rect_create(top_layer, bar_width_, bar_height_, bg_color);
    wlr_scene_node_set_position(&scene_rect_->node, pos_x_, pos_y_);
    wlr_scene_node_set_enabled(&scene_rect_->node, false);
    
    // Create scene buffer for content (hidden initially)
    scene_buffer_ = wlr_scene_buffer_create(top_layer, nullptr);
    wlr_scene_node_set_position(&scene_buffer_->node, pos_x_, pos_y_);
    wlr_scene_node_set_enabled(&scene_buffer_->node, false);
    
    // Renderer will be retrieved on first render (lazy initialization)
    renderer_ = nullptr;
}

void MenuBar::Show() {
    if (is_visible_) return;
    
    is_visible_ = true;
    search_query_.clear();
    selected_index_ = 0;
    scroll_offset_ = 0;
    
    // Refresh items
    RefreshItems();
    UpdateFilteredItems();
    
    // Enable scene nodes
    wlr_scene_node_set_enabled(&scene_rect_->node, true);
    wlr_scene_node_set_enabled(&scene_buffer_->node, true);
    
    Render();
    
    LOG_INFO("MenuBar shown");
}

void MenuBar::Hide() {
    if (!is_visible_) return;
    
    is_visible_ = false;
    
    // Disable scene nodes
    wlr_scene_node_set_enabled(&scene_rect_->node, false);
    wlr_scene_node_set_enabled(&scene_buffer_->node, false);
    
    LOG_INFO("MenuBar hidden");
}

void MenuBar::Toggle() {
    if (is_visible_) {
        Hide();
    } else {
        Show();
    }
}

void MenuBar::AddProvider(std::shared_ptr<IMenuItemProvider> provider) {
    providers_.push_back(provider);
    LOG_INFO_FMT("Added menu item provider: {}", provider->GetName());
}

void MenuBar::RemoveProvider(const std::string& provider_name) {
    auto it = std::remove_if(providers_.begin(), providers_.end(),
        [&provider_name](const auto& p) { return p->GetName() == provider_name; });
    providers_.erase(it, providers_.end());
}

void MenuBar::ClearProviders() {
    providers_.clear();
    all_items_.clear();
    filtered_items_.clear();
}

void MenuBar::RefreshItems() {
    all_items_.clear();
    
    for (auto& provider : providers_) {
        try {
            auto items = provider->LoadItems();
            all_items_.insert(all_items_.end(), items.begin(), items.end());
            LOG_DEBUG_FMT("Loaded {} items from provider '{}'", items.size(), provider->GetName());
        } catch (const std::exception& e) {
            LOG_ERROR_FMT("Failed to load items from provider '{}': {}", provider->GetName(), e.what());
        }
    }
    
    // Sort by priority (higher first), then alphabetically
    std::sort(all_items_.begin(), all_items_.end(),
        [](const auto& a, const auto& b) {
            if (a->GetPriority() != b->GetPriority()) {
                return a->GetPriority() > b->GetPriority();
            }
            return a->GetDisplayName() < b->GetDisplayName();
        });
    
    LOG_INFO_FMT("Loaded {} total menu items from {} providers", all_items_.size(), providers_.size());
}

void MenuBar::UpdateFilteredItems() {
    filtered_items_.clear();
    
    if (search_query_.size() < static_cast<size_t>(config_.min_chars_for_search)) {
        filtered_items_ = all_items_;
    } else {
        for (const auto& item : all_items_) {
            if (config_.fuzzy_matching) {
                if (FuzzyMatch(item->GetDisplayName(), search_query_)) {
                    filtered_items_.push_back(item);
                }
            } else {
                if (item->Matches(search_query_)) {
                    filtered_items_.push_back(item);
                }
            }
        }
    }
    
    // Reset selection
    selected_index_ = 0;
    scroll_offset_ = 0;
}

bool MenuBar::FuzzyMatch(const std::string& text, const std::string& query) const {
    if (query.empty()) return true;
    
    std::string text_cmp = text;
    std::string query_cmp = query;
    
    if (!config_.case_sensitive) {
        std::transform(text_cmp.begin(), text_cmp.end(), text_cmp.begin(), ::tolower);
        std::transform(query_cmp.begin(), query_cmp.end(), query_cmp.begin(), ::tolower);
    }
    
    size_t text_idx = 0;
    for (char qc : query_cmp) {
        text_idx = text_cmp.find(qc, text_idx);
        if (text_idx == std::string::npos) {
            return false;
        }
        text_idx++;
    }
    
    return true;
}

bool MenuBar::SimpleMatch(const std::string& text, const std::string& query) const {
    if (query.empty()) return true;
    
    std::string text_cmp = text;
    std::string query_cmp = query;
    
    if (!config_.case_sensitive) {
        std::transform(text_cmp.begin(), text_cmp.end(), text_cmp.begin(), ::tolower);
        std::transform(query_cmp.begin(), query_cmp.end(), query_cmp.begin(), ::tolower);
    }
    
    return text_cmp.find(query_cmp) != std::string::npos;
}

void MenuBar::HandleTextInput(const std::string& text) {
    search_query_ += text;
    UpdateFilteredItems();
    Render();
}

void MenuBar::HandleBackspace() {
    if (!search_query_.empty()) {
        search_query_.pop_back();
        UpdateFilteredItems();
        Render();
    }
}

void MenuBar::HandleEnter() {
    ExecuteSelectedItem();
}

void MenuBar::HandleEscape() {
    Hide();
}

void MenuBar::HandleArrowUp() {
    if (selected_index_ > 0) {
        selected_index_--;
        EnsureSelectionVisible();
        Render();
    }
}

void MenuBar::HandleArrowDown() {
    if (selected_index_ < static_cast<int>(filtered_items_.size()) - 1) {
        selected_index_++;
        EnsureSelectionVisible();
        Render();
    }
}

void MenuBar::EnsureSelectionVisible() {
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + config_.max_visible_items) {
        scroll_offset_ = selected_index_ - config_.max_visible_items + 1;
    }
}

void MenuBar::ExecuteSelectedItem() {
    if (selected_index_ >= 0 && selected_index_ < static_cast<int>(filtered_items_.size())) {
        auto& item = filtered_items_[selected_index_];
        LOG_INFO_FMT("Executing menu item: {}", item->GetDisplayName());
        
        try {
            item->Execute();
            Hide();
        } catch (const std::exception& e) {
            LOG_ERROR_FMT("Failed to execute menu item '{}': {}", item->GetDisplayName(), e.what());
        }
    }
}

bool MenuBar::HandleClick(int x, int y) {
    if (!is_visible_) return false;
    
    // Check if click is in input area
    int input_y_start = config_.padding;
    int input_y_end = input_y_start + config_.height;
    
    if (y >= input_y_start && y <= input_y_end) {
        // Click in input area - do nothing, just capture
        return true;
    }
    
    // Check if click is on a menu item
    int items_y_start = config_.height;
    for (int i = 0; i < config_.max_visible_items && (scroll_offset_ + i) < static_cast<int>(filtered_items_.size()); i++) {
        int item_y = items_y_start + (i * config_.item_height);
        if (y >= item_y && y < item_y + config_.item_height) {
            selected_index_ = scroll_offset_ + i;
            ExecuteSelectedItem();
            return true;
        }
    }
    
    return true;
}

bool MenuBar::HandleHover(int x, int y) {
    if (!is_visible_) return false;
    
    // Update selection based on hover
    int items_y_start = config_.height;
    for (int i = 0; i < config_.max_visible_items && (scroll_offset_ + i) < static_cast<int>(filtered_items_.size()); i++) {
        int item_y = items_y_start + (i * config_.item_height);
        if (y >= item_y && y < item_y + config_.item_height) {
            if (selected_index_ != scroll_offset_ + i) {
                selected_index_ = scroll_offset_ + i;
                Render();
            }
            return true;
        }
    }
    
    return true;
}

void MenuBar::Render() {
    if (!is_visible_) return;
    
    RenderToBuffer();
    UploadToTexture();
}

void MenuBar::RenderToBuffer() {
    // Clear with background color
    cairo_set_source_rgba(cairo_,
        config_.background_color.r,
        config_.background_color.g,
        config_.background_color.b,
        config_.background_color.a);
    cairo_paint(cairo_);
    
    // Render input field
    int input_y = config_.padding;
    
    // Input field background (slightly lighter)
    cairo_set_source_rgba(cairo_, 0.15, 0.15, 0.15, 1.0);
    cairo_rectangle(cairo_, config_.padding, input_y,
                    bar_width_ - 2 * config_.padding, config_.height - config_.padding);
    cairo_fill(cairo_);
    
    // Input text
    cairo_set_source_rgba(cairo_,
        config_.text_color.r,
        config_.text_color.g,
        config_.text_color.b,
        config_.text_color.a);
    cairo_select_font_face(cairo_, config_.font_family.c_str(),
                          CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo_, config_.font_size);
    
    std::string display_text = search_query_.empty() ? "Type to search..." : search_query_;
    cairo_move_to(cairo_, config_.padding + 5, input_y + (config_.height / 2) + 5);
    cairo_show_text(cairo_, display_text.c_str());
    
    // Render menu items
    int item_y = config_.height;
    for (int i = 0; i < config_.max_visible_items && (scroll_offset_ + i) < static_cast<int>(filtered_items_.size()); i++) {
        int item_idx = scroll_offset_ + i;
        auto& item = filtered_items_[item_idx];
        
        // Highlight selected item
        if (item_idx == selected_index_) {
            cairo_set_source_rgba(cairo_,
                config_.selected_color.r,
                config_.selected_color.g,
                config_.selected_color.b,
                config_.selected_color.a);
            cairo_rectangle(cairo_, 0, item_y, bar_width_, config_.item_height);
            cairo_fill(cairo_);
        }
        
        // Item name
        cairo_set_source_rgba(cairo_,
            config_.text_color.r,
            config_.text_color.g,
            config_.text_color.b,
            config_.text_color.a);
        cairo_set_font_size(cairo_, config_.font_size);
        cairo_move_to(cairo_, config_.padding + 5, item_y + 18);
        cairo_show_text(cairo_, item->GetDisplayName().c_str());
        
        // Item description (if available)
        std::string desc = item->GetDescription();
        if (!desc.empty()) {
            cairo_set_source_rgba(cairo_,
                config_.description_color.r,
                config_.description_color.g,
                config_.description_color.b,
                config_.description_color.a);
            cairo_set_font_size(cairo_, config_.description_font_size);
            cairo_move_to(cairo_, config_.padding + 5, item_y + 30);
            cairo_show_text(cairo_, desc.c_str());
        }
        
        item_y += config_.item_height;
    }
    
    cairo_surface_flush(cairo_surface_);
}

void MenuBar::UploadToTexture() {
    // Get renderer from output on first upload (lazy initialization)
    if (!renderer_) {
        struct wlr_output* output = layer_manager_->GetOutput();
        if (output && output->renderer) {
            renderer_ = output->renderer;
            LOG_DEBUG("Got renderer from output for menubar");
        } else {
            LOG_ERROR("Cannot get renderer from output for menubar");
            return;
        }
    }
    
    // Get wlr_buffer from ShmBuffer
    struct wlr_buffer* wlr_buf = shm_buffer_->GetWlrBuffer();
    if (!wlr_buf) {
        LOG_ERROR("Failed to get wlr_buffer from ShmBuffer for menubar");
        return;
    }
    
    // Set buffer on scene (using same approach as StatusBar)
    if (!buffer_attached_) {
        LOG_DEBUG("Attaching menubar SHM buffer to scene_buffer (first time)");
        buffer_attached_ = true;
    }
    
    // Call set_buffer to tell wlroots the content changed
    wlr_scene_buffer_set_buffer(scene_buffer_, wlr_buf);
}

void MenuBar::HandleKeyPress(uint32_t key, uint32_t modifiers) {
    // This is a placeholder - actual key handling should be done
    // through HandleEnter, HandleEscape, etc.
}

} // namespace UI
} // namespace Leviathan
