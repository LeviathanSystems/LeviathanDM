#include "ui/KeybindingHelpModal.hpp"
#include "ui/reusable-widgets/Label.hpp"
#include "ui/reusable-widgets/ScrollView.hpp"
#include "KeyBindings.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <xkbcommon/xkbcommon.h>

namespace Leviathan {
namespace UI {

KeybindingHelpModal::KeybindingHelpModal()
    : Modal()
{
    SetTitle("Keybindings Help");
    SetSize(900, 700);
    
    // Auto-populate from global KeyBindings instance when created
    PopulateKeybindings();
    
    // Build the widget content after populating
    BuildWidgetContent();
}

void KeybindingHelpModal::BuildWidgetContent() {
    // Create main vertical container for all keybindings
    auto main_vbox = std::make_shared<VBox>();
    main_vbox->SetSpacing(5);
    
    // Create header row
    auto header_hbox = std::make_shared<HBox>();
    header_hbox->SetSpacing(10);
    
    auto key_header = std::make_shared<Label>("Key");
    key_header->SetFontSize(14);
    key_header->SetSize(200, 25);
    
    auto action_header = std::make_shared<Label>("Action");
    action_header->SetFontSize(14);
    action_header->SetSize(250, 25);
    
    auto desc_header = std::make_shared<Label>("Description");
    desc_header->SetFontSize(14);
    desc_header->SetSize(400, 25);
    
    header_hbox->AddChild(key_header);
    header_hbox->AddChild(action_header);
    header_hbox->AddChild(desc_header);
    
    main_vbox->AddChild(header_hbox);
    
    // Add keybinding rows
    for (const auto& binding : keybindings_) {
        // Check if this is a category header (empty keys field)
        if (binding.keys.empty() && !binding.action_name.empty()) {
            // Category header
            auto category_label = std::make_shared<Label>(binding.action_name);
            category_label->SetFontSize(16);
            category_label->SetTextColor(1.0, 1.0, 1.0, 1.0);
            category_label->SetSize(850, 30);
            main_vbox->AddChild(category_label);
            continue;
        }
        
        // Regular keybinding row
        auto row_hbox = std::make_shared<HBox>();
        row_hbox->SetSpacing(10);
        
        // Key combination (light blue)
        auto key_label = std::make_shared<Label>(binding.keys);
        key_label->SetFontSize(12);
        key_label->SetTextColor(0.7, 0.9, 1.0, 1.0);
        key_label->SetSize(200, 20);
        
        // Action name (light yellow)
        auto action_label = std::make_shared<Label>(binding.action_name);
        action_label->SetFontSize(12);
        action_label->SetTextColor(1.0, 1.0, 0.7, 1.0);
        action_label->SetSize(250, 20);
        
        // Description (white)
        auto desc_label = std::make_shared<Label>(binding.description);
        desc_label->SetFontSize(12);
        desc_label->SetTextColor(0.9, 0.9, 0.9, 0.8);
        desc_label->SetSize(400, 20);
        
        row_hbox->AddChild(key_label);
        row_hbox->AddChild(action_label);
        row_hbox->AddChild(desc_label);
        
        main_vbox->AddChild(row_hbox);
    }
    
    // Wrap the VBox in a ScrollView
    auto scroll_view = std::make_shared<ScrollView>();
    scroll_view->SetChild(main_vbox);
    scroll_view->SetScrollbarColor(0.6, 0.6, 0.6, 0.7);
    scroll_view->ShowScrollbar(true);
    
    // Set as modal content
    SetContent(scroll_view);
}

void KeybindingHelpModal::PopulateKeybindings() {
    auto* keybindings = KeyBindings::Instance();
    if (!keybindings) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "KeyBindings instance not available");
        return;
    }
    
    keybindings_.clear();
    
    auto* registry = keybindings->GetActionRegistry();
    if (!registry) {
        return;
    }
    
    // Get all keybindings
    const auto& bindings = keybindings->GetBindings();
    
    // Build map of action_name -> keys
    std::map<std::string, std::vector<std::string>> action_to_keys;
    for (const auto& binding : bindings) {
        std::string key_combo;
        
        // Convert modifiers to string
        if (binding.modifiers & (1 << 6)) key_combo += "Super+";  // MOD_LOGO
        if (binding.modifiers & (1 << 2)) key_combo += "Ctrl+";   // MOD_CTRL
        if (binding.modifiers & (1 << 3)) key_combo += "Alt+";    // MOD_ALT
        if (binding.modifiers & (1 << 0)) key_combo += "Shift+";  // MOD_SHIFT
        
        // Convert keysym to string
        char name[64];
        xkb_keysym_get_name(binding.keysym, name, sizeof(name));
        std::string key_name(name);
        
        // Clean up key name
        if (key_name.find("XKB_KEY_") == 0) {
            key_name = key_name.substr(8);
        }
        if (key_name == "Return") key_name = "Enter";
        else if (key_name == "Escape") key_name = "ESC";
        
        key_combo += key_name;
        action_to_keys[binding.action_name].push_back(key_combo);
    }
    
    // Get all actions and organize by category
    const auto& actions = registry->GetAllActions();
    std::map<std::string, std::vector<KeybindingEntry>> categories;
    
    for (const auto& [action_name, action] : actions) {
        std::string category = action.category.empty() ? "Other" : action.category;
        
        // Find keys bound to this action
        std::string keys = "";
        if (action_to_keys.find(action_name) != action_to_keys.end()) {
            const auto& key_list = action_to_keys[action_name];
            for (size_t i = 0; i < key_list.size(); i++) {
                if (i > 0) keys += ", ";
                keys += key_list[i];
            }
        } else {
            keys = "(not bound)";
        }
        
        categories[category].push_back({keys, action_name, action.description});
    }
    
    // Add categories to modal in logical order
    std::vector<std::string> category_order = {
        "Applications", "Window Management", "Focus & Layout",
        "Tags/Workspaces", "UI", "System", "Other"
    };
    
    for (const auto& category_name : category_order) {
        if (categories.find(category_name) != categories.end()) {
            // Add category header
            keybindings_.push_back({"", category_name, ""});
            
            auto& entries = categories[category_name];
            std::sort(entries.begin(), entries.end(),
                     [](const auto& a, const auto& b) {
                         return a.action_name < b.action_name;
                     });
            
            for (const auto& entry : entries) {
                keybindings_.push_back(entry);
            }
        }
    }
}

} // namespace UI
} // namespace Leviathan
