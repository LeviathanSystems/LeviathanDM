#include "ConfigParser.hpp"
#include "Logger.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace Leviathan {

bool ConfigParser::Load(const std::string& config_path) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        
        if (config["libinput"]) {
            ParseLibInput(config["libinput"]);
        }
        
        if (config["general"]) {
            ParseGeneral(config["general"]);
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
    if (node["border_color"]) {
        general.border_color = node["border_color"].as<std::string>();
    }
}

} // namespace Leviathan
