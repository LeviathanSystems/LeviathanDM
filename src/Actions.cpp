#include "Actions.hpp"
#include "wayland/Server.hpp"
#include "Logger.hpp"
#include "ui/menubar/MenuBarManager.hpp"
#include <unistd.h>
#include <sys/wait.h>

namespace Leviathan {

ActionRegistry::ActionRegistry(Wayland::Server* server)
    : server_(server) {
    RegisterBuiltinActions();
}

void ActionRegistry::RegisterBuiltinActions() {
    // Window management actions
    RegisterAction({
        .name = "close-window",
        .description = "Close the focused window",
        .category = "Window Management",
        .type = ActionType::CLOSE_WINDOW,
        .params = {}
    });
    
    RegisterAction({
        .name = "focus-next",
        .description = "Focus the next window",
        .category = "Focus & Layout",
        .type = ActionType::FOCUS_NEXT,
        .params = {}
    });
    
    RegisterAction({
        .name = "focus-prev",
        .description = "Focus the previous window",
        .category = "Focus & Layout",
        .type = ActionType::FOCUS_PREV,
        .params = {}
    });
    
    RegisterAction({
        .name = "swap-next",
        .description = "Swap focused window with next",
        .category = "Focus & Layout",
        .type = ActionType::SWAP_NEXT,
        .params = {}
    });
    
    RegisterAction({
        .name = "swap-prev",
        .description = "Swap focused window with previous",
        .category = "Focus & Layout",
        .type = ActionType::SWAP_PREV,
        .params = {}
    });
    
    RegisterAction({
        .name = "toggle-floating",
        .description = "Toggle floating mode for focused window",
        .category = "Window Management",
        .type = ActionType::TOGGLE_FLOATING,
        .params = {}
    });
    
    RegisterAction({
        .name = "toggle-fullscreen",
        .description = "Toggle fullscreen for focused window",
        .category = "Window Management",
        .type = ActionType::TOGGLE_FULLSCREEN,
        .params = {}
    });
    
    // Compositor control
    RegisterAction({
        .name = "toggle-menubar",
        .description = "Toggle the application menubar",
        .category = "UI",
        .type = ActionType::TOGGLE_MENUBAR,
        .params = {}
    });
    
    RegisterAction({
        .name = "reload-config",
        .description = "Reload configuration file",
        .category = "System",
        .type = ActionType::RELOAD_CONFIG,
        .params = {}
    });
    
    RegisterAction({
        .name = "shutdown",
        .description = "Gracefully shutdown the compositor",
        .category = "System",
        .type = ActionType::SHUTDOWN,
        .params = {}
    });
    
    RegisterAction({
        .name = "show-help",
        .description = "Show keybinding help window",
        .category = "UI",
        .type = ActionType::SHOW_HELP,
        .params = {}
    });
    
    LOG_INFO_FMT("Registered {} built-in actions", actions_.size());
}

void ActionRegistry::RegisterAction(const Action& action) {
    actions_[action.name] = action;
    LOG_DEBUG_FMT("Registered action: {} - {}", action.name, action.description);
}

bool ActionRegistry::ExecuteAction(const std::string& action_name) {
    auto it = actions_.find(action_name);
    if (it == actions_.end()) {
        LOG_WARN_FMT("Action not found: {}", action_name);
        return false;
    }
    
    const Action& action = it->second;
    LOG_DEBUG_FMT("Executing action: {}", action_name);
    
    // If it's a custom action with a function, call it
    if (action.custom_function) {
        action.custom_function();
        return true;
    }
    
    // Otherwise execute built-in action
    ExecuteBuiltinAction(action);
    return true;
}

void ActionRegistry::ExecuteBuiltinAction(const Action& action) {
    switch (action.type) {
        case ActionType::CLOSE_WINDOW: {
            auto* view = server_->GetFocusedView();
            if (view) {
                server_->CloseView(view);
            }
            break;
        }
        
        case ActionType::FOCUS_NEXT:
            server_->FocusNext();
            break;
        
        case ActionType::FOCUS_PREV:
            server_->FocusPrev();
            break;
        
        case ActionType::SWAP_NEXT:
            server_->SwapWithNext();
            break;
        
        case ActionType::SWAP_PREV:
            server_->SwapWithPrev();
            break;
        
        case ActionType::TOGGLE_FLOATING: {
            auto* client = server_->GetFocusedClient();
            if (client) {
                client->SetFloating(!client->IsFloating());
                // TODO: Trigger re-layout when layout API is updated
            }
            break;
        }
        
        case ActionType::TOGGLE_FULLSCREEN: {
            auto* client = server_->GetFocusedClient();
            if (client) {
                client->SetFullscreen(!client->IsFullscreen());
            }
            break;
        }
        
        case ActionType::SWITCH_TO_TAG: {
            // Find tag_index parameter
            int tag_index = -1;
            for (const auto& param : action.params) {
                if (param.key == "tag_index") {
                    tag_index = std::stoi(param.value);
                    break;
                }
            }
            if (tag_index >= 0) {
                server_->SwitchToTag(tag_index);
            }
            break;
        }
        
        case ActionType::MOVE_TO_TAG: {
            // Find tag_index parameter
            int tag_index = -1;
            for (const auto& param : action.params) {
                if (param.key == "tag_index") {
                    tag_index = std::stoi(param.value);
                    break;
                }
            }
            if (tag_index >= 0) {
                // TODO: Implement move to tag when tag API is updated
                LOG_WARN_FMT("Move to tag not yet implemented (requested tag: {})", tag_index);
            }
            break;
        }
        
        case ActionType::SPAWN: {
            // Find command parameter
            std::string command;
            for (const auto& param : action.params) {
                if (param.key == "command") {
                    command = param.value;
                    break;
                }
            }
            
            if (!command.empty()) {
                pid_t pid = fork();
                if (pid == 0) {
                    // Child process
                    setsid();
                    execl("/bin/sh", "/bin/sh", "-c", command.c_str(), nullptr);
                    _exit(1);
                } else if (pid > 0) {
                    LOG_INFO_FMT("Spawned command: {} (PID: {})", command, pid);
                } else {
                    LOG_ERROR_FMT("Failed to fork for command: {}", command);
                }
            }
            break;
        }
        
        case ActionType::TOGGLE_MENUBAR: {
            // Get focused screen and toggle menubar on its output
            auto* screen = server_->GetFocusedScreen();
            if (screen) {
                auto* layer_mgr = server_->GetLayerManagerForScreen(screen);
                if (layer_mgr) {
                    auto* output = layer_mgr->GetOutput();
                    if (output) {
                        UI::MenuBarManager::Instance().ToggleOnOutput(output);
                    }
                }
            }
            break;
        }
        
        case ActionType::RELOAD_CONFIG: {
            LOG_INFO("Reloading configuration...");
            // TODO: Implement config reload
            LOG_WARN("Config reload not yet implemented");
            break;
        }
        
        case ActionType::SHUTDOWN: {
            server_->Shutdown();
            break;
        }
        
        case ActionType::SHOW_HELP: {
            // Show keybinding help on focused screen
            auto* screen = server_->GetFocusedScreen();
            if (screen) {
                auto* layer_mgr = server_->GetLayerManagerForScreen(screen);
                if (layer_mgr) {
                    layer_mgr->ToggleModal("keybindingshelp");
                }
            }
            break;
        }
        
        case ActionType::CUSTOM:
            LOG_WARN("Custom action type without function handler");
            break;
    }
}

bool ActionRegistry::HasAction(const std::string& action_name) const {
    return actions_.find(action_name) != actions_.end();
}

const Action* ActionRegistry::GetAction(const std::string& action_name) const {
    auto it = actions_.find(action_name);
    if (it != actions_.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace Leviathan
