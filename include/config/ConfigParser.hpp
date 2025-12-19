#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <yaml-cpp/yaml.h>

namespace Leviathan {

struct LibInputConfig {
    struct MouseConfig {
        double speed = 0.5;  // -1.0 to 1.0
        bool natural_scroll = false;
        std::string accel_profile = "adaptive";  // "adaptive" or "flat"
    } mouse;
    
    struct TouchpadConfig {
        double speed = 0.0;
        bool natural_scroll = true;
        bool tap_to_click = true;
        bool tap_and_drag = true;
        std::string accel_profile = "adaptive";
    } touchpad;
    
    struct KeyboardConfig {
        int repeat_rate = 25;  // characters per second
        int repeat_delay = 600;  // milliseconds
    } keyboard;
};

struct GeneralConfig {
    std::string terminal = "alacritty";
    bool auto_launch_terminal = true;
    int border_width = 2;
    std::string border_color_focused = "#5E81AC";     // Nord blue
    std::string border_color_unfocused = "#3B4252";   // Nord dark gray
    int gap_size = 0;
    int workspace_count = 9;
    bool focus_follows_mouse = true;
    bool click_to_focus = true;
    bool remove_client_titlebars = true;
};

// Widget configuration for status bars
struct WidgetConfig {
    std::string type;  // "label", "clock", "battery", "workspaces", etc.
    std::map<std::string, std::string> properties;  // Widget-specific properties
};

// Container configuration (HBox, VBox)
struct ContainerConfig {
    std::string type;  // "hbox" or "vbox"
    std::vector<WidgetConfig> widgets;
    
    // Container properties
    int spacing = 5;
    std::string alignment = "center";  // "left", "center", "right" for HBox; "top", "center", "bottom" for VBox
    int padding = 0;
};

// Status bar configuration
struct StatusBarConfig {
    std::string name;  // Unique identifier (e.g., "main-bar", "laptop-bar", "monitor-left-bar")
    
    // Position on screen
    enum class Position {
        Top,
        Bottom,
        Left,
        Right
    };
    Position position = Position::Top;
    
    // Size
    int height = 30;  // For top/bottom bars (pixels)
    int width = 30;   // For left/right bars (pixels)
    
    // Appearance
    std::string background_color = "#2E3440";  // Nord polar night
    std::string foreground_color = "#D8DEE9";  // Nord snow storm
    int font_size = 12;
    std::string font_family = "monospace";
    
    // Layout sections
    ContainerConfig left;    // Left section (for horizontal bars) or top (for vertical bars)
    ContainerConfig center;  // Center section
    ContainerConfig right;   // Right section (for horizontal bars) or bottom (for vertical bars)
};

// Monitor configuration within a group
struct MonitorConfig {
    // Identifier can be:
    // - Output name: "eDP-1", "HDMI-A-1", etc.
    // - Description prefix: "d:Dell Inc. U2720Q" (matches EDID description)
    // - Make/Model: "m:Dell Inc./U2720Q"
    std::string identifier;
    
    // Status bars assigned to this monitor
    // Can have multiple bars at different positions
    std::vector<std::string> status_bars;  // Names of status bars to show
    
    // Optional position (e.g., "0x0", "1920x0")
    // If not set, wlroots auto-arranges
    std::optional<std::pair<int, int>> position;  // x, y
    
    // Optional size/mode (e.g., "1920x1080", "3840x2160@60")
    // If not set, uses preferred mode
    std::optional<std::string> mode;
    
    // Optional scale (e.g., 1.0, 1.5, 2.0)
    std::optional<float> scale;
    
    // Optional transform (0, 90, 180, 270)
    std::optional<int> transform;
};

// Monitor group - a complete multi-monitor layout
struct MonitorGroup {
    std::string name;
    std::vector<MonitorConfig> monitors;
    bool is_default = false;  // Fallback group
};

// Monitor groups configuration
struct MonitorGroupsConfig {
    std::vector<MonitorGroup> groups;
    
    // Find the matching group for currently connected outputs
    const MonitorGroup* FindMatchingGroup(const std::vector<std::string>& connected_outputs) const;
    
    // Get the default/fallback group
    const MonitorGroup* GetDefaultGroup() const;
};

// Plugin configuration
struct PluginConfig {
    std::string name;                                    // Plugin name (e.g., "ClockWidget")
    std::map<std::string, std::string> config;          // Plugin-specific config key-value pairs
};

// Plugins configuration
struct PluginsConfig {
    std::vector<std::string> plugin_paths;              // Directories to search for plugins
    std::vector<PluginConfig> plugins;                  // List of plugins to load with their configs
};

// Status bars configuration
struct StatusBarsConfig {
    std::vector<StatusBarConfig> bars;  // All defined status bars
    
    // Find a status bar by name
    const StatusBarConfig* FindByName(const std::string& name) const;
};

struct ConfigParser {
    LibInputConfig libinput;
    GeneralConfig general;
    PluginsConfig plugins;
    StatusBarsConfig status_bars;
    MonitorGroupsConfig monitor_groups;
    
    // Load configuration from file
    bool Load(const std::string& config_path);
    
    // Load with includes support
    bool LoadWithIncludes(const std::string& main_config);
    
    // Utility: Convert hex color string to RGBA floats (0.0-1.0)
    static void HexToRGBA(const std::string& hex, float rgba[4]);
    
    // Global instance access
    static ConfigParser& Instance();
    
private:
    void ParseLibInput(const YAML::Node& node);
    void ParseGeneral(const YAML::Node& node);
    void ParsePlugins(const YAML::Node& node);
    void ParseStatusBars(const YAML::Node& node);
    void ParseMonitorGroups(const YAML::Node& node);
    void ProcessIncludes(const YAML::Node& node, const std::string& base_path);
    
    std::vector<std::string> loaded_files_;  // Track loaded files to prevent circular includes
    
    // Private constructor for singleton
    ConfigParser() = default;
};

// Global convenience accessor
inline ConfigParser& Config() {
    return ConfigParser::Instance();
}

} // namespace Leviathan

#endif // CONFIG_PARSER_HPP
