#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace Leviathan {

namespace Wayland {
    class Server;
}

// Action types supported by the compositor
enum class ActionType {
    // Window management
    CLOSE_WINDOW,
    FOCUS_NEXT,
    FOCUS_PREV,
    SWAP_NEXT,
    SWAP_PREV,
    TOGGLE_FLOATING,
    TOGGLE_FULLSCREEN,
    
    // Tag/Workspace switching
    SWITCH_TO_TAG,  // Requires tag_index parameter
    MOVE_TO_TAG,    // Requires tag_index parameter
    
    // Applications
    SPAWN,          // Requires command parameter
    
    // Compositor control
    TOGGLE_MENUBAR,
    RELOAD_CONFIG,
    SHUTDOWN,
    SHOW_HELP,      // Show keybinding help modal
    
    // Custom action (for future extensibility)
    CUSTOM
};

// Action parameter for commands that need extra data
struct ActionParam {
    std::string key;
    std::string value;
};

// Action definition - can be bound to keys or called from UI
struct Action {
    std::string name;           // Friendly name like "open-terminal"
    std::string description;    // Description for UI/documentation
    std::string category;       // Category for grouping in help (e.g., "Window Management")
    ActionType type;
    std::vector<ActionParam> params;  // Parameters for the action
    
    // Optional: Direct function for custom actions
    std::function<void()> custom_function;
};

// Action registry - maps action names to implementations
class ActionRegistry {
public:
    explicit ActionRegistry(Wayland::Server* server);
    
    // Register a new action
    void RegisterAction(const Action& action);
    
    // Execute an action by name
    bool ExecuteAction(const std::string& action_name);
    
    // Get all registered actions (for UI/documentation)
    const std::map<std::string, Action>& GetAllActions() const { return actions_; }
    
    // Check if action exists
    bool HasAction(const std::string& action_name) const;
    
    // Get action by name
    const Action* GetAction(const std::string& action_name) const;
    
private:
    void RegisterBuiltinActions();
    void ExecuteBuiltinAction(const Action& action);
    
private:
    Wayland::Server* server_;
    std::map<std::string, Action> actions_;
};

} // namespace Leviathan
