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

// Tag configuration
struct TagConfig {
    int id = 0;
    std::string name;
    std::string icon;  // Optional icon/emoji for the tag
};

struct GeneralConfig {
    std::string terminal = "alacritty";
    bool auto_launch_terminal = true;
    int gap_size = 0;
    int workspace_count = 9;  // Deprecated - use tags instead
    std::vector<TagConfig> tags;  // Named tags with icons
    bool focus_follows_mouse = true;
    bool click_to_focus = true;
    bool remove_client_titlebars = true;
    
    // Window styling
    float window_opacity = 1.0f;                      // Window opacity (0.0 - 1.0)
    float window_opacity_inactive = 1.0f;             // Inactive window opacity (0.0 - 1.0)
    int border_radius = 0;                            // Border radius in pixels (rounded corners)
    bool enable_shadows = false;                      // Enable window shadows
    int shadow_size = 10;                             // Shadow size/blur radius in pixels
    std::string shadow_color = "#000000";             // Shadow color
    float shadow_opacity = 0.5f;                      // Shadow opacity (0.0 - 1.0)
};

// Night Light configuration
struct NightLightConfig {
    bool enabled = false;                // Enable/disable night light
    int start_hour = 20;                 // Start time (24-hour format)
    int start_minute = 0;
    int end_hour = 6;                    // End time (24-hour format)
    int end_minute = 0;
    float temperature = 3400.0f;         // Color temperature in Kelvin
    float strength = 0.85f;              // Strength of the effect (0.0 - 1.0)
    int transition_duration = 1800;      // Transition duration in seconds
    bool smooth_transition = true;       // Gradual transition vs instant
};

// Forward declaration for recursive structure
struct WidgetConfig;

// Widget configuration for status bars (supports nested containers)
struct WidgetConfig {
    std::string type;  // "label", "clock", "battery", "hbox", "vbox", etc.
    std::map<std::string, std::string> properties;  // Widget-specific properties
    std::vector<WidgetConfig> children;  // For containers (hbox, vbox) - nested widgets
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
    
    // Root widget (usually an HBox or VBox containing the entire layout)
    WidgetConfig root;
    
    // Legacy layout sections (deprecated - use root instead)
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
    
    // Wallpaper config for this specific monitor
    std::string wallpaper;  // Name of wallpaper config (optional)
    
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
    bool is_default = false;             // Fallback group
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

// Wallpaper type enumeration
enum class WallpaperType {
    StaticImage,       // Static image files (PNG, JPEG, etc.)
    WallpaperEngine    // Wallpaper Engine animated wallpapers
};

// Wallpaper configuration
struct WallpaperConfig {
    std::string name;                    // Name of the wallpaper config (e.g., "casual", "work")
    WallpaperType type;                  // Type of wallpaper (static or wallpaper engine)
    std::vector<std::string> wallpapers; // Path(s) to wallpaper file(s) or folder
    int change_interval_seconds;         // How often to rotate wallpapers (0 = no rotation)
    
    WallpaperConfig() 
        : name("default")
        , type(WallpaperType::StaticImage)
        , change_interval_seconds(0) {}
};

// Wallpapers configuration - collection of wallpaper configs
struct WallpapersConfig {
    std::vector<WallpaperConfig> wallpapers;  // All defined wallpaper configs
    
    // Find a wallpaper config by name
    const WallpaperConfig* FindByName(const std::string& name) const;
};

// Window decoration configuration group
struct WindowDecorationConfig {
    std::string name;                         // Unique identifier (e.g., "default", "floating", "transparent")
    
    // Border settings
    int border_width = 2;
    std::string border_color_focused = "#5E81AC";
    std::string border_color_unfocused = "#3B4252";
    int border_radius = 0;                    // Rounded corners in pixels
    
    // Opacity/transparency
    float opacity = 1.0f;                     // Window opacity (0.0 - 1.0)
    float opacity_inactive = 1.0f;            // Inactive window opacity
    
    // Shadow settings
    bool enable_shadows = false;
    int shadow_size = 10;                     // Shadow blur radius in pixels
    std::string shadow_color = "#000000";
    float shadow_opacity = 0.5f;
    int shadow_offset_x = 0;                  // Shadow horizontal offset
    int shadow_offset_y = 0;                  // Shadow vertical offset
    
    // Dimming
    bool dim_inactive = false;                // Dim inactive windows
    float dim_amount = 0.3f;                  // How much to dim (0.0 - 1.0)
    
    WindowDecorationConfig() = default;
};

// Window decorations configuration - collection of decoration groups
struct WindowDecorationsConfig {
    std::vector<WindowDecorationConfig> decorations;  // All defined decoration groups
    
    // Find decoration group by name (returns nullptr if not found)
    const WindowDecorationConfig* FindByName(const std::string& name) const;
    
    // Get default decoration (first one or built-in defaults)
    const WindowDecorationConfig* GetDefault() const;
};

// Window rule configuration - matches windows and applies decoration
struct WindowRuleConfig {
    std::string name;                         // Rule identifier for logging/debugging
    
    // Match criteria (any can be used, all specified must match)
    std::string app_id;                       // Match app_id (e.g., "firefox", "alacritty")
    std::string title;                        // Match window title (supports wildcards: *)
    std::string class_name;                   // Match window class
    bool match_floating = false;              // Only match floating windows
    bool match_tiled = false;                 // Only match tiled windows
    
    // Actions
    std::string decoration_group;             // Apply this decoration group
    bool force_floating = false;              // Force window to float
    bool force_tiled = false;                 // Force window to tile
    std::optional<int> opacity_override;      // Override opacity (0-100)
    std::optional<int> tag;                   // Move to specific tag
    
    WindowRuleConfig() = default;
    
    // Check if this rule matches a window
    bool Matches(const std::string& window_app_id, 
                 const std::string& window_title,
                 const std::string& window_class,
                 bool is_floating) const;
};

// Window rules configuration - collection of rules
struct WindowRulesConfig {
    std::vector<WindowRuleConfig> rules;      // All defined window rules
    
    // Find first matching rule for a window
    const WindowRuleConfig* FindMatch(const std::string& app_id,
                                      const std::string& title, 
                                      const std::string& class_name,
                                      bool is_floating) const;
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
    NightLightConfig night_light;
    PluginsConfig plugins;
    StatusBarsConfig status_bars;
    MonitorGroupsConfig monitor_groups;
    WallpapersConfig wallpapers;
    WindowDecorationsConfig window_decorations;
    WindowRulesConfig window_rules;
    
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
    void ParseNightLight(const YAML::Node& node);
    void ParsePlugins(const YAML::Node& node);
    void ParseStatusBars(const YAML::Node& node);
    void ParseMonitorGroups(const YAML::Node& node);
    void ParseWallpapers(const YAML::Node& node);
    void ParseWindowDecorations(const YAML::Node& node);
    void ParseWindowRules(const YAML::Node& node);
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
