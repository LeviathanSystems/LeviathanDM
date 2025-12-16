#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "Types.hpp"
#include <string>
#include <unordered_map>

namespace Leviathan {

class Config {
public:
    Config();
    
    bool LoadFromFile(const std::string& filename);
    
    // Getters
    int GetBorderWidth() const { return border_width_; }
    int GetGapSize() const { return gap_size_; }
    unsigned long GetBorderFocused() const { return border_focused_; }
    unsigned long GetBorderUnfocused() const { return border_unfocused_; }
    int GetWorkspaceCount() const { return workspace_count_; }
    bool GetFocusFollowsMouse() const { return focus_follows_mouse_; }
    
private:
    void SetDefaults();
    unsigned long ParseColor(const std::string& color);
    
private:
    int border_width_;
    int gap_size_;
    unsigned long border_focused_;
    unsigned long border_unfocused_;
    int workspace_count_;
    bool focus_follows_mouse_;
};

} // namespace Leviathan

#endif // CONFIG_HPP
