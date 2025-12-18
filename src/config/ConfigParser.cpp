#include "config/ConfigParser.hpp"
#include "Logger.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace Leviathan {

ConfigParser& ConfigParser::Instance() {
    static ConfigParser instance;
    return instance;
}

bool ConfigParser::Load(const std::string& config_path) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        
        if (config["libinput"]) {
            ParseLibInput(config["libinput"]);
        }
        
        if (config["general"]) {
            ParseGeneral(config["general"]);
        }
        
        if (config["plugins"]) {
            ParsePlugins(config["plugins"]);
        }
        
        LOG_INFO("Loaded configuration from: {}", config_path);
        return true;
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Failed to parse config file {}: {}", config_path, e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading config file {}: {}", config_path, e.what());
        return false;
    }
}

bool ConfigParser::LoadWithIncludes(const std::string& main_config) {
    loaded_files_.clear();
    
    try {
        std::filesystem::path config_path(main_config);
        if (!std::filesystem::exists(config_path)) {
            LOG_WARN("Config file not found: {}, using defaults", main_config);
            return false;
        }
        
        YAML::Node config = YAML::LoadFile(main_config);
        loaded_files_.push_back(std::filesystem::canonical(config_path).string());
        
        // Process includes first
        if (config["include"]) {
            ProcessIncludes(config["include"], config_path.parent_path().string());
        }
        
        // Then load main config (overrides included configs)
        if (config["libinput"]) {
            ParseLibInput(config["libinput"]);
        }
        
        if (config["general"]) {
            ParseGeneral(config["general"]);
        }
        
        if (config["plugins"]) {
            ParsePlugins(config["plugins"]);
        }
        
        LOG_INFO("Loaded configuration with includes from: {}", main_config);
        return true;
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Failed to parse config: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading config: {}", e.what());
        return false;
    }
}

void ConfigParser::ProcessIncludes(const YAML::Node& include_node, const std::string& base_path) {
    try {
        if (include_node.IsScalar()) {
            // Single include file
            std::filesystem::path include_path = std::filesystem::path(base_path) / include_node.as<std::string>();
            
            if (!std::filesystem::exists(include_path)) {
                LOG_WARN("Include file not found: {}", include_path.string());
                return;
            }
            
            std::string canonical_path = std::filesystem::canonical(include_path).string();
            
            // Check for circular includes
            if (std::find(loaded_files_.begin(), loaded_files_.end(), canonical_path) != loaded_files_.end()) {
                LOG_WARN("Circular include detected, skipping: {}", canonical_path);
                return;
            }
            
            loaded_files_.push_back(canonical_path);
            Load(canonical_path);
            
        } else if (include_node.IsSequence()) {
            // Multiple include files
            for (const auto& item : include_node) {
                ProcessIncludes(item, base_path);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing includes: {}", e.what());
    }
}

void ConfigParser::ParseLibInput(const YAML::Node& node) {
    // Mouse configuration
    if (node["mouse"]) {
        const auto& mouse = node["mouse"];
        if (mouse["speed"]) {
            double speed = mouse["speed"].as<double>();
            // Clamp to valid range
            libinput.mouse.speed = std::clamp(speed, -1.0, 1.0);
            LOG_DEBUG("Mouse speed: {}", libinput.mouse.speed);
        }
        if (mouse["natural_scroll"]) {
            libinput.mouse.natural_scroll = mouse["natural_scroll"].as<bool>();
        }
        if (mouse["accel_profile"]) {
            libinput.mouse.accel_profile = mouse["accel_profile"].as<std::string>();
        }
    }
    
    // Touchpad configuration
    if (node["touchpad"]) {
        const auto& touchpad = node["touchpad"];
        if (touchpad["speed"]) {
            double speed = touchpad["speed"].as<double>();
            libinput.touchpad.speed = std::clamp(speed, -1.0, 1.0);
        }
        if (touchpad["natural_scroll"]) {
            libinput.touchpad.natural_scroll = touchpad["natural_scroll"].as<bool>();
        }
        if (touchpad["tap_to_click"]) {
            libinput.touchpad.tap_to_click = touchpad["tap_to_click"].as<bool>();
        }
        if (touchpad["tap_and_drag"]) {
            libinput.touchpad.tap_and_drag = touchpad["tap_and_drag"].as<bool>();
        }
        if (touchpad["accel_profile"]) {
            libinput.touchpad.accel_profile = touchpad["accel_profile"].as<std::string>();
        }
    }
    
    // Keyboard configuration
    if (node["keyboard"]) {
        const auto& keyboard = node["keyboard"];
        if (keyboard["repeat_rate"]) {
            libinput.keyboard.repeat_rate = keyboard["repeat_rate"].as<int>();
        }
        if (keyboard["repeat_delay"]) {
            libinput.keyboard.repeat_delay = keyboard["repeat_delay"].as<int>();
        }
    }
}

void ConfigParser::ParseGeneral(const YAML::Node& node) {
    if (node["terminal"]) {
        general.terminal = node["terminal"].as<std::string>();
        LOG_DEBUG("Terminal: {}", general.terminal);
    }
    if (node["auto_launch_terminal"]) {
        general.auto_launch_terminal = node["auto_launch_terminal"].as<bool>();
    }
    if (node["border_width"]) {
        general.border_width = node["border_width"].as<int>();
    }
    if (node["border_color_focused"]) {
        general.border_color_focused = node["border_color_focused"].as<std::string>();
    }
    if (node["border_color_unfocused"]) {
        general.border_color_unfocused = node["border_color_unfocused"].as<std::string>();
    }
    // Legacy support for old config
    if (node["border_color"]) {
        general.border_color_focused = node["border_color"].as<std::string>();
    }
    if (node["gap_size"]) {
        general.gap_size = node["gap_size"].as<int>();
    }
    if (node["workspace_count"]) {
        general.workspace_count = node["workspace_count"].as<int>();
    }
    if (node["focus_follows_mouse"]) {
        general.focus_follows_mouse = node["focus_follows_mouse"].as<bool>();
    }
    if (node["click_to_focus"]) {
        general.click_to_focus = node["click_to_focus"].as<bool>();
    }
    if (node["remove_client_titlebars"]) {
        general.remove_client_titlebars = node["remove_client_titlebars"].as<bool>();
    }
}

void ConfigParser::ParsePlugins(const YAML::Node& node) {
    // Set default plugin paths if none configured
    if (!node["plugin_paths"] || 
        (node["plugin_paths"].IsSequence() && node["plugin_paths"].size() == 0)) {
        
        // Default search paths (in priority order)
        plugins.plugin_paths.clear();
        
        // 1. User config directory
        const char* home = getenv("HOME");
        const char* xdg_config = getenv("XDG_CONFIG_HOME");
        if (xdg_config) {
            plugins.plugin_paths.push_back(std::string(xdg_config) + "/leviathan/plugins");
        } else if (home) {
            plugins.plugin_paths.push_back(std::string(home) + "/.config/leviathan/plugins");
        }
        
        // 2. System-wide local plugins
        plugins.plugin_paths.push_back("/usr/local/lib/leviathan/plugins");
        
        // 3. Distribution plugins
        plugins.plugin_paths.push_back("/usr/lib/leviathan/plugins");
        
        LOG_DEBUG("Using default plugin paths");
    } else {
        // User specified custom paths
        plugins.plugin_paths.clear();
        if (node["plugin_paths"].IsSequence()) {
            for (const auto& path : node["plugin_paths"]) {
                std::string path_str = path.as<std::string>();
                
                // Expand ~ to home directory
                if (!path_str.empty() && path_str[0] == '~') {
                    const char* home = getenv("HOME");
                    if (home) {
                        path_str = std::string(home) + path_str.substr(1);
                    }
                }
                
                plugins.plugin_paths.push_back(path_str);
                LOG_DEBUG("Plugin path: {}", path_str);
            }
        } else if (node["plugin_paths"].IsScalar()) {
            std::string path_str = node["plugin_paths"].as<std::string>();
            
            // Expand ~ to home directory
            if (!path_str.empty() && path_str[0] == '~') {
                const char* home = getenv("HOME");
                if (home) {
                    path_str = std::string(home) + path_str.substr(1);
                }
            }
            
            plugins.plugin_paths.push_back(path_str);
            LOG_DEBUG("Plugin path: {}", path_str);
        }
    }
    
    // Log all plugin paths
    for (const auto& path : plugins.plugin_paths) {
        LOG_INFO("Plugin search path: {}", path);
    }
    
    // Parse plugins list
    if (node["list"]) {
        plugins.plugins.clear();
        if (node["list"].IsSequence()) {
            for (const auto& plugin_node : node["list"]) {
                PluginConfig plugin_config;
                
                // Plugin name is required
                if (plugin_node["name"]) {
                    plugin_config.name = plugin_node["name"].as<std::string>();
                    LOG_DEBUG("Plugin: {}", plugin_config.name);
                } else {
                    LOG_WARN("Plugin entry missing 'name', skipping");
                    continue;
                }
                
                // Parse plugin-specific config
                if (plugin_node["config"]) {
                    const YAML::Node& config_node = plugin_node["config"];
                    if (config_node.IsMap()) {
                        for (const auto& kv : config_node) {
                            std::string key = kv.first.as<std::string>();
                            std::string value = kv.second.as<std::string>();
                            plugin_config.config[key] = value;
                            LOG_DEBUG("  {}: {}", key, value);
                        }
                    }
                }
                
                plugins.plugins.push_back(plugin_config);
            }
        }
    }
}

void ConfigParser::HexToRGBA(const std::string& hex, float rgba[4]) {
    std::string color = hex;
    
    // Remove leading # if present
    if (!color.empty() && color[0] == '#') {
        color = color.substr(1);
    }
    
    // Default to opaque black if parsing fails
    rgba[0] = rgba[1] = rgba[2] = 0.0f;
    rgba[3] = 1.0f;
    
    try {
        if (color.length() == 6) {
            // RGB format: RRGGBB
            int r = std::stoi(color.substr(0, 2), nullptr, 16);
            int g = std::stoi(color.substr(2, 2), nullptr, 16);
            int b = std::stoi(color.substr(4, 2), nullptr, 16);
            
            rgba[0] = r / 255.0f;
            rgba[1] = g / 255.0f;
            rgba[2] = b / 255.0f;
            rgba[3] = 1.0f;
        } else if (color.length() == 8) {
            // RGBA format: RRGGBBAA
            int r = std::stoi(color.substr(0, 2), nullptr, 16);
            int g = std::stoi(color.substr(2, 2), nullptr, 16);
            int b = std::stoi(color.substr(4, 2), nullptr, 16);
            int a = std::stoi(color.substr(6, 2), nullptr, 16);
            
            rgba[0] = r / 255.0f;
            rgba[1] = g / 255.0f;
            rgba[2] = b / 255.0f;
            rgba[3] = a / 255.0f;
        } else {
            LOG_WARN("Invalid hex color format: {}, using black", hex);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse hex color '{}': {}", hex, e.what());
    }
}

} // namespace Leviathan
