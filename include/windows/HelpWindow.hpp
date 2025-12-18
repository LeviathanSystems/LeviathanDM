#ifndef HELP_WINDOW_HPP
#define HELP_WINDOW_HPP

#include "wayland/WaylandTypes.hpp"
#include <string>
#include <vector>

namespace Leviathan {

// Forward declarations
class KeyBindings;

namespace Wayland {
    class Server;
}

namespace Windows {

struct KeybindingInfo {
    std::string keys;        // e.g., "Super+Return"
    std::string description; // e.g., "Launch terminal"
};

class HelpWindow {
public:
    HelpWindow(Wayland::Server* server);
    ~HelpWindow();
    
    // Toggle visibility
    void Toggle();
    void Show();
    void Hide();
    bool IsVisible() const { return visible_; }
    
    // Update keybinding list
    void UpdateKeybindings(const std::vector<KeybindingInfo>& bindings);
    
private:
    void CreateSurface();
    void DestroySurface();
    void RenderContent();
    
    // Generate help text from keybindings
    std::string GenerateHelpText() const;
    
private:
    Wayland::Server* server_;
    bool visible_;
    
    // Wayland surface for the help window
    struct wlr_scene_tree* scene_tree_;
    struct wlr_scene_rect* background_;
    struct wlr_scene_buffer* text_buffer_;
    
    // Keybinding information
    std::vector<KeybindingInfo> keybindings_;
    
    // Window dimensions
    int width_;
    int height_;
    int x_;
    int y_;
};

} // namespace Windows
} // namespace Leviathan

#endif // HELP_WINDOW_HPP
