#include "ui/menubar/MenuBar.hpp"
#include "wayland/LayerManager.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cctype>

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
    , active_provider_index_(0)
{
    // Calculate dimensions
    bar_width_ = output_width_;
    bar_height_ = config_.height + (config_.item_height * config_.max_visible_items);
    
    // Create IconLoader
    icon_loader_ = std::make_unique<IconLoader>();
    
    // Create TextField for search
    search_field_ = std::make_shared<TextField>("Type to search...", TextField::Variant::Standard);
    search_field_->SetMinWidth(bar_width_ - 2 * config_.padding);
    search_field_->SetFontSize(config_.font_size);
    search_field_->SetFontFamily(config_.font_family);
    search_field_->SetTextColor(config_.text_color.r, config_.text_color.g, 
                               config_.text_color.b, config_.text_color.a);
    search_field_->SetBackgroundColor(0.15, 0.15, 0.15, 1.0);
    search_field_->SetPadding(5);
    
    // Set up callbacks
    search_field_->SetOnTextChanged([this](const std::string& text) {
        search_query_ = text;
        UpdateFilteredItems();
        Render();
    });
    
    search_field_->SetOnSubmit([this](const std::string&) {
        ExecuteSelectedItem();
    });
    
    // Create TabBar
    TabBarConfig tab_config;
    tab_config.height = config_.tab_height;
    tab_config.tab_padding = config_.tab_padding;
    tab_config.background_color.r = config_.tab_background_color.r;
    tab_config.background_color.g = config_.tab_background_color.g;
    tab_config.background_color.b = config_.tab_background_color.b;
    tab_config.background_color.a = config_.tab_background_color.a;
    tab_config.active_tab_color.r = config_.tab_active_color.r;
    tab_config.active_tab_color.g = config_.tab_active_color.g;
    tab_config.active_tab_color.b = config_.tab_active_color.b;
    tab_config.active_tab_color.a = config_.tab_active_color.a;
    tab_config.text_color.r = config_.tab_text_color.r;
    tab_config.text_color.g = config_.tab_text_color.g;
    tab_config.text_color.b = config_.tab_text_color.b;
    tab_config.text_color.a = config_.tab_text_color.a;
    tab_config.font_family = config_.font_family;
    tab_config.font_size = config_.tab_font_size;
    tab_config.equal_width_tabs = true;
    
    tab_bar_ = std::make_unique<TabBar>(tab_config);
    
    // Set up tab change callback
    tab_bar_->SetOnTabChange([this](const std::string& tab_id, int index) {
        if (index >= 0 && index < static_cast<int>(providers_.size())) {
            active_provider_index_ = index;
            RefreshItems();        // Load items from new active provider
            UpdateFilteredItems();  // Apply search filter
            Render();              // Update display
        }
    });
    
    // Create buffer
    shm_buffer_ = ShmBuffer::Create(bar_width_, bar_height_);
    if (!shm_buffer_) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to create ShmBuffer for menubar");
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
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBar created: {}x{}", bar_width_, bar_height_);
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
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBar destroyed");
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
    search_field_->SetText("");  // Clear text field
    search_field_->Focus();      // Focus the text field
    selected_index_ = 0;
    scroll_offset_ = 0;
    
    // Refresh items
    RefreshItems();
    UpdateFilteredItems();
    
    // Enable scene nodes
    wlr_scene_node_set_enabled(&scene_rect_->node, true);
    wlr_scene_node_set_enabled(&scene_buffer_->node, true);
    
    Render();
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBar shown");
}

void MenuBar::Hide() {
    if (!is_visible_) return;
    
    is_visible_ = false;
    search_field_->Blur();  // Blur the text field
    
    // Disable scene nodes
    wlr_scene_node_set_enabled(&scene_rect_->node, false);
    wlr_scene_node_set_enabled(&scene_buffer_->node, false);
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBar hidden");
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
    
    // Add tab for this provider
    std::string tab_id = std::to_string(providers_.size() - 1);
    Tab tab(tab_id, provider->GetTabName(), provider->GetTabIcon());
    tab_bar_->AddTab(tab);
    
    // Set first tab as active
    if (providers_.size() == 1) {
        tab_bar_->SetActiveTab(tab_id);
        active_provider_index_ = 0;
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Added menu item provider: {}", provider->GetName());
}

void MenuBar::RemoveProvider(const std::string& provider_name) {
    // Find provider index
    int removed_index = -1;
    for (size_t i = 0; i < providers_.size(); ++i) {
        if (providers_[i]->GetName() == provider_name) {
            removed_index = static_cast<int>(i);
            break;
        }
    }
    
    // Remove provider
    auto it = std::remove_if(providers_.begin(), providers_.end(),
        [&provider_name](const auto& p) { return p->GetName() == provider_name; });
    providers_.erase(it, providers_.end());
    
    // Remove tab
    if (removed_index >= 0) {
        std::string tab_id = std::to_string(removed_index);
        tab_bar_->RemoveTab(tab_id);
        
        // Adjust active provider index if needed
        if (active_provider_index_ == removed_index) {
            active_provider_index_ = 0;
            if (!providers_.empty()) {
                tab_bar_->SetActiveTab("0");
            }
        } else if (active_provider_index_ > removed_index) {
            active_provider_index_--;
        }
    }
}

void MenuBar::ClearProviders() {
    providers_.clear();
    all_items_.clear();
    filtered_items_.clear();
    tab_bar_->ClearTabs();
    active_provider_index_ = 0;
}

void MenuBar::RefreshItems() {
    all_items_.clear();
    
    // If we have multiple providers, only load from the active one
    // If we have one or no providers, load all
    if (providers_.size() > 1 && active_provider_index_ >= 0 && 
        active_provider_index_ < static_cast<int>(providers_.size())) {
        auto& provider = providers_[active_provider_index_];
        try {
            auto items = provider->LoadItems();
            all_items_.insert(all_items_.end(), items.begin(), items.end());
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Loaded {} items from active provider '{}'", items.size(), provider->GetName());
        } catch (const std::exception& e) {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to load items from provider '{}': {}", provider->GetName(), e.what());
        }
    } else {
        // Load from all providers
        for (auto& provider : providers_) {
            try {
                auto items = provider->LoadItems();
                all_items_.insert(all_items_.end(), items.begin(), items.end());
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Loaded {} items from provider '{}'", items.size(), provider->GetName());
            } catch (const std::exception& e) {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to load items from provider '{}': {}", provider->GetName(), e.what());
            }
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
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Loaded {} total menu items from {} provider(s)", all_items_.size(), 
                 providers_.size() > 1 ? 1 : providers_.size());
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
    if (!is_visible_) return;
    
    // Delegate text input to TextField
    search_field_->HandleTextInput(text);
    // Text change callback will automatically update search_query_ and render
}

void MenuBar::HandleBackspace() {
    if (!is_visible_) return;
    
    // Delegate backspace to TextField via HandleKeyPress
    search_field_->HandleKeyPress(65288, 0);  // 65288 is Backspace key code
    // Text change callback will automatically update search_query_ and render
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
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Executing menu item: {}", item->GetDisplayName());
        
        try {
            item->Execute();
            Hide();
        } catch (const std::exception& e) {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to execute menu item '{}': {}", item->GetDisplayName(), e.what());
        }
    }
}

bool MenuBar::HandleClick(int x, int y) {
    if (!is_visible_) return false;
    
    // Check if click is in TextField input area
    int input_y_start = config_.padding;
    int input_y_end = input_y_start + config_.height;
    
    if (y >= input_y_start && y <= input_y_end) {
        // Delegate click to TextField for cursor positioning
        search_field_->HandleClick(x - config_.padding, y - input_y_start);
        Render();  // Re-render to update cursor position
        return true;
    }
    
    // Check if click is on tabs (if multiple providers)
    if (providers_.size() > 1 && config_.show_tabs) {
        int tab_y_start = bar_height_ - config_.tab_height;
        if (y >= tab_y_start) {
            if (tab_bar_->HandleClick(x, y - tab_y_start)) {
                // Tab change callback will handle provider switching
                return true;
            }
        }
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
    
    // Check if hovering over tabs (if multiple providers)
    if (providers_.size() > 1 && config_.show_tabs) {
        int tab_y_start = bar_height_ - config_.tab_height;
        if (y >= tab_y_start) {
            tab_bar_->HandleHover(x, y - tab_y_start);
            Render();
            return true;
        }
    }
    
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
    
    // Render TextField input field
    int input_y = config_.padding;
    search_field_->SetPosition(config_.padding, input_y);
    search_field_->CalculateSize(bar_width_ - 2 * config_.padding, config_.height - config_.padding);
    search_field_->Render(cairo_);
    
    // Render menu items
    int item_y = config_.height;
    const int icon_size = 24;  // Icon size in pixels
    const int icon_padding = 8;  // Space between icon and text
    const int text_x_offset = config_.padding + icon_size + icon_padding;
    
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
        
        // Draw icon if available
        std::string icon_path = item->GetIconPath();
        if (!icon_path.empty() && icon_loader_) {
            cairo_surface_t* icon_surface = icon_loader_->LoadIcon(icon_path, icon_size);
            if (icon_surface) {
                int icon_y = item_y + (config_.item_height - icon_size) / 2;
                cairo_set_source_surface(cairo_, icon_surface, config_.padding, icon_y);
                cairo_paint(cairo_);
            }
        }
        
        // Item name
        cairo_set_source_rgba(cairo_,
            config_.text_color.r,
            config_.text_color.g,
            config_.text_color.b,
            config_.text_color.a);
        cairo_select_font_face(cairo_, config_.font_family.c_str(),
                              CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cairo_, config_.font_size);
        cairo_move_to(cairo_, text_x_offset, item_y + 18);
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
            cairo_move_to(cairo_, text_x_offset, item_y + 30);
            cairo_show_text(cairo_, desc.c_str());
        }
        
        item_y += config_.item_height;
    }
    
    // Render TabBar if there are multiple providers
    if (providers_.size() > 1 && config_.show_tabs) {
        int tab_y = bar_height_ - config_.tab_height;
        tab_bar_->Render(cairo_, 0, tab_y, bar_width_);
    }
    
    cairo_surface_flush(cairo_surface_);
}

void MenuBar::UploadToTexture() {
    // Get renderer from output on first upload (lazy initialization)
    if (!renderer_) {
        struct wlr_output* output = layer_manager_->GetOutput();
        if (output && output->renderer) {
            renderer_ = output->renderer;
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Got renderer from output for menubar");
        } else {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Cannot get renderer from output for menubar");
            return;
        }
    }
    
    // Get wlr_buffer from ShmBuffer
    struct wlr_buffer* wlr_buf = shm_buffer_->GetWlrBuffer();
    if (!wlr_buf) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to get wlr_buffer from ShmBuffer for menubar");
        return;
    }
    
    // Set buffer on scene (using same approach as StatusBar)
    if (!buffer_attached_) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Attaching menubar SHM buffer to scene_buffer (first time)");
        buffer_attached_ = true;
    }
    
    // Call set_buffer to tell wlroots the content changed
    wlr_scene_buffer_set_buffer(scene_buffer_, wlr_buf);
}

void MenuBar::HandleKeyPress(uint32_t key, uint32_t modifiers) {
    if (!is_visible_) return;
    
    // Debug: Log key presses
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "MenuBar received key: 0x{:x} (modifiers: {})", key, modifiers);
    
    // Handle special keys that TextField doesn't handle
    if (key == XKB_KEY_Escape) {
        HandleEscape();
        return;
    } else if (key == XKB_KEY_Up) {
        HandleArrowUp();
        return;
    } else if (key == XKB_KEY_Down) {
        HandleArrowDown();
        return;
    } else if (key == XKB_KEY_Tab || key == XKB_KEY_ISO_Left_Tab) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Tab key pressed! Providers: {}", providers_.size());
        // Switch to next provider tab if multiple providers
        if (providers_.size() > 1) {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Switching to next tab...");
            tab_bar_->SelectNext();
            // Tab change callback will update active_provider_index_ and refresh
        }
        return;
    }
    
    // Pass all other keys to the TextField
    if (search_field_->HandleKeyPress(key, modifiers)) {
        // TextField handled the key
        // search_query_ is automatically updated via the OnTextChanged callback
        return;
    }
}

} // namespace UI
} // namespace Leviathan
