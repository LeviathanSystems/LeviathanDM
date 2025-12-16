#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
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
    std::string border_color = "#ff0000";
};

struct ConfigParser {
    LibInputConfig libinput;
    GeneralConfig general;
    
    // Load configuration from file
    bool Load(const std::string& config_path);
    
    // Load with includes support
    bool LoadWithIncludes(const std::string& main_config);
    
private:
    void ParseLibInput(const YAML::Node& node);
    void ParseGeneral(const YAML::Node& node);
    void ProcessIncludes(const YAML::Node& node, const std::string& base_path);
    
    std::vector<std::string> loaded_files_;  // Track loaded files to prevent circular includes
};

} // namespace Leviathan

#endif // CONFIG_PARSER_HPP
