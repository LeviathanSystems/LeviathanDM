#pragma once

#include "ui/reusable-widgets/BaseModal.hpp"
#include "ui/reusable-widgets/VBox.hpp"
#include "ui/reusable-widgets/HBox.hpp"
#include "ui/reusable-widgets/Label.hpp"
#include <vector>
#include <map>
#include <string>

namespace Leviathan {

// Forward declaration
class KeyBindings;

namespace UI {

/**
 * @brief Keybinding help modal that displays all available actions and their keys
 */
class KeybindingHelpModal : public Modal {
public:
    struct KeybindingEntry {
        std::string keys;           // e.g., "Super+Return"
        std::string action_name;    // e.g., "open-terminal"
        std::string description;    // e.g., "Open a new terminal window"
    };
    
    KeybindingHelpModal();
    
    // Populate modal from the global KeyBindings instance
    void PopulateKeybindings();
    
private:
    // Build the widget tree for displaying keybindings
    void BuildWidgetContent();
    
    std::vector<KeybindingEntry> keybindings_;
};

} // namespace UI
} // namespace Leviathan
