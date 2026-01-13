#include "ui/menubar/MenuBarManager.hpp"
#include "wayland/LayerManager.hpp"
#include "Logger.hpp"

namespace Leviathan {
namespace UI {

// Singleton instance
MenuBarManager& MenuBarManager::Instance() {
    static MenuBarManager instance;
    return instance;
}

MenuBarManager::MenuBarManager()
    : event_loop_(nullptr)
    , initialized_(false)
{
    // Default configuration
    config_.height = 40;
    config_.item_height = 35;
    config_.max_visible_items = 8;
    config_.padding = 10;
    config_.fuzzy_matching = true;
    config_.case_sensitive = false;
    config_.min_chars_for_search = 0;
    
    // Default colors
    config_.background_color = {0.1, 0.1, 0.1, 0.95};
    config_.selected_color = {0.2, 0.4, 0.6, 1.0};
    config_.text_color = {1.0, 1.0, 1.0, 1.0};
    config_.description_color = {0.7, 0.7, 0.7, 1.0};
    
    // Default fonts
    config_.font_family = "Sans";
    config_.font_size = 12;
    config_.description_font_size = 9;
}

MenuBarManager::~MenuBarManager() {
    Shutdown();
}

void MenuBarManager::Initialize(struct wl_event_loop* event_loop) {
    if (initialized_) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "MenuBarManager already initialized");
        return;
    }
    
    event_loop_ = event_loop;
    initialized_ = true;
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBarManager initialized");
}

void MenuBarManager::RegisterMenuBar(struct wlr_output* output,
                                     Wayland::LayerManager* layer_manager,
                                     uint32_t output_width,
                                     uint32_t output_height)
{
    if (!initialized_) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "MenuBarManager not initialized");
        return;
    }
    
    if (!output || !layer_manager) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Invalid output or layer_manager for menubar registration");
        return;
    }
    
    // Check if menubar already exists for this output
    if (menubars_.find(output) != menubars_.end()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "MenuBar already registered for output");
        return;
    }
    
    // Create menubar
    auto menubar = std::make_unique<MenuBar>(
        config_,
        layer_manager,
        event_loop_,
        output_width,
        output_height
    );
    
    // Add all registered providers
    for (auto& provider : providers_) {
        menubar->AddProvider(provider);
    }
    
    // Store menubar data
    MenuBarData data;
    data.menubar = std::move(menubar);
    data.layer_manager = layer_manager;
    data.output = output;
    data.width = output_width;
    data.height = output_height;
    
    menubars_[output] = std::move(data);
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBar registered for output: {}x{}", output_width, output_height);
}

void MenuBarManager::UnregisterMenuBar(struct wlr_output* output) {
    auto it = menubars_.find(output);
    if (it == menubars_.end()) {
        return;
    }
    
    menubars_.erase(it);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBar unregistered for output");
}

void MenuBarManager::ShowOnOutput(struct wlr_output* output) {
    auto it = menubars_.find(output);
    if (it == menubars_.end()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "No menubar found for output");
        return;
    }
    
    // Hide all other menubars first
    HideAll();
    
    // Show this menubar
    it->second.menubar->Show();
}

void MenuBarManager::HideOnOutput(struct wlr_output* output) {
    auto it = menubars_.find(output);
    if (it == menubars_.end()) {
        return;
    }
    
    it->second.menubar->Hide();
}

void MenuBarManager::ToggleOnOutput(struct wlr_output* output) {
    auto it = menubars_.find(output);
    if (it == menubars_.end()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "No menubar found for output");
        return;
    }
    
    if (it->second.menubar->IsVisible()) {
        it->second.menubar->Hide();
    } else {
        // Hide all other menubars
        HideAll();
        // Show this one
        it->second.menubar->Show();
    }
}

void MenuBarManager::HideAll() {
    for (auto& [output, data] : menubars_) {
        if (data.menubar->IsVisible()) {
            data.menubar->Hide();
        }
    }
}

MenuBar* MenuBarManager::GetMenuBarForOutput(struct wlr_output* output) {
    auto it = menubars_.find(output);
    if (it == menubars_.end()) {
        return nullptr;
    }
    return it->second.menubar.get();
}

bool MenuBarManager::IsAnyMenuBarVisible() const {
    for (const auto& [output, data] : menubars_) {
        if (data.menubar->IsVisible()) {
            return true;
        }
    }
    return false;
}

MenuBar* MenuBarManager::GetVisibleMenuBar() {
    for (auto& [output, data] : menubars_) {
        if (data.menubar->IsVisible()) {
            return data.menubar.get();
        }
    }
    return nullptr;
}

bool MenuBarManager::HandleKeyPress(uint32_t key, uint32_t modifiers) {
    auto* menubar = GetVisibleMenuBar();
    if (!menubar) return false;
    
    menubar->HandleKeyPress(key, modifiers);
    return true;
}

bool MenuBarManager::HandleTextInput(const std::string& text) {
    auto* menubar = GetVisibleMenuBar();
    if (!menubar) return false;
    
    menubar->HandleTextInput(text);
    return true;
}

bool MenuBarManager::HandleBackspace() {
    auto* menubar = GetVisibleMenuBar();
    if (!menubar) return false;
    
    menubar->HandleBackspace();
    return true;
}

bool MenuBarManager::HandleEnter() {
    auto* menubar = GetVisibleMenuBar();
    if (!menubar) return false;
    
    menubar->HandleEnter();
    return true;
}

bool MenuBarManager::HandleEscape() {
    auto* menubar = GetVisibleMenuBar();
    if (!menubar) return false;
    
    menubar->HandleEscape();
    return true;
}

bool MenuBarManager::HandleArrowUp() {
    auto* menubar = GetVisibleMenuBar();
    if (!menubar) return false;
    
    menubar->HandleArrowUp();
    return true;
}

bool MenuBarManager::HandleArrowDown() {
    auto* menubar = GetVisibleMenuBar();
    if (!menubar) return false;
    
    menubar->HandleArrowDown();
    return true;
}

bool MenuBarManager::HandleClick(int x, int y, struct wlr_output* output) {
    auto* menubar = GetMenuBarForOutput(output);
    if (!menubar || !menubar->IsVisible()) return false;
    
    return menubar->HandleClick(x, y);
}

bool MenuBarManager::HandleHover(int x, int y, struct wlr_output* output) {
    auto* menubar = GetMenuBarForOutput(output);
    if (!menubar || !menubar->IsVisible()) return false;
    
    return menubar->HandleHover(x, y);
}

void MenuBarManager::SetConfig(const MenuBarConfig& config) {
    config_ = config;
    
    // Update all existing menubars with new config
    // Note: This would require recreating them or adding a SetConfig method to MenuBar
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBar configuration updated (requires menubar recreation)");
}

void MenuBarManager::AddProvider(std::shared_ptr<IMenuItemProvider> provider) {
    providers_.push_back(provider);
    
    // Add provider to all existing menubars
    for (auto& [output, data] : menubars_) {
        data.menubar->AddProvider(provider);
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Added menu item provider: {}", provider->GetName());
}

void MenuBarManager::RemoveProvider(const std::string& provider_name) {
    auto it = std::remove_if(providers_.begin(), providers_.end(),
        [&provider_name](const auto& p) { return p->GetName() == provider_name; });
    providers_.erase(it, providers_.end());
    
    // Remove from all existing menubars
    for (auto& [output, data] : menubars_) {
        data.menubar->RemoveProvider(provider_name);
    }
}

void MenuBarManager::ClearProviders() {
    providers_.clear();
    
    // Clear from all existing menubars
    for (auto& [output, data] : menubars_) {
        data.menubar->ClearProviders();
    }
}

void MenuBarManager::RefreshAllItems() {
    // Refresh all menubars
    for (auto& [output, data] : menubars_) {
        data.menubar->RefreshItems();
    }
}

void MenuBarManager::Shutdown() {
    menubars_.clear();
    providers_.clear();
    initialized_ = false;
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "MenuBarManager shutdown");
}

} // namespace UI
} // namespace Leviathan
