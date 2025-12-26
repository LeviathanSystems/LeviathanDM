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
        
        if (config["status-bars"]) {
            ParseStatusBars(config["status-bars"]);
        }
        
        if (config["monitor-groups"]) {
            ParseMonitorGroups(config["monitor-groups"]);
        }
        
        if (config["wallpapers"]) {
            LOG_DEBUG("Found wallpapers section in config");
            ParseWallpapers(config["wallpapers"]);
        } else {
            LOG_WARN("No wallpapers section found in config");
        }
        
        LOG_INFO_FMT("Loaded configuration from: {}", config_path);
        return true;
    } catch (const YAML::Exception& e) {
        LOG_ERROR_FMT("Failed to parse config file {}: {}", config_path, e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("Error loading config file {}: {}", config_path, e.what());
        return false;
    }
}

bool ConfigParser::LoadWithIncludes(const std::string& main_config) {
    loaded_files_.clear();
    
    try {
        std::filesystem::path config_path(main_config);
        if (!std::filesystem::exists(config_path)) {
            LOG_WARN_FMT("Config file not found: {}, using defaults", main_config);
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
        
        if (config["status-bars"]) {
            ParseStatusBars(config["status-bars"]);
        }
        
        if (config["monitor-groups"]) {
            ParseMonitorGroups(config["monitor-groups"]);
        }
        
        if (config["wallpapers"]) {
            LOG_DEBUG("Found wallpapers section in config (LoadWithIncludes)");
            ParseWallpapers(config["wallpapers"]);
        } else {
            LOG_WARN("No wallpapers section found in config (LoadWithIncludes)");
        }
        
        LOG_INFO_FMT("Loaded configuration with includes from: {}", main_config);
        return true;
    } catch (const YAML::Exception& e) {
        LOG_ERROR_FMT("Failed to parse config: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("Error loading config: {}", e.what());
        return false;
    }
}

void ConfigParser::ProcessIncludes(const YAML::Node& include_node, const std::string& base_path) {
    try {
        if (include_node.IsScalar()) {
            // Single include file
            std::filesystem::path include_path = std::filesystem::path(base_path) / include_node.as<std::string>();
            
            if (!std::filesystem::exists(include_path)) {
                LOG_WARN_FMT("Include file not found: {}", include_path.string());
                return;
            }
            
            std::string canonical_path = std::filesystem::canonical(include_path).string();
            
            // Check for circular includes
            if (std::find(loaded_files_.begin(), loaded_files_.end(), canonical_path) != loaded_files_.end()) {
                LOG_WARN_FMT("Circular include detected, skipping: {}", canonical_path);
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
        LOG_ERROR_FMT("Error processing includes: {}", e.what());
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
            LOG_DEBUG_FMT("Mouse speed: {}", libinput.mouse.speed);
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
        LOG_DEBUG_FMT("Terminal: {}", general.terminal);
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
    
    // Parse tags (new format)
    if (node["tags"]) {
        general.tags.clear();
        for (const auto& tag_node : node["tags"]) {
            TagConfig tag;
            
            if (tag_node["id"]) {
                tag.id = tag_node["id"].as<int>();
            }
            if (tag_node["name"]) {
                tag.name = tag_node["name"].as<std::string>();
            }
            if (tag_node["icon"]) {
                tag.icon = tag_node["icon"].as<std::string>();
            }
            
            // Only add if name is not empty
            if (!tag.name.empty()) {
                general.tags.push_back(tag);
                LOG_DEBUG_FMT("Tag {}: {} {}", tag.id, tag.icon.empty() ? "" : tag.icon, tag.name);
            }
        }
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
                LOG_DEBUG_FMT("Plugin path: {}", path_str);
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
            LOG_DEBUG_FMT("Plugin path: {}", path_str);
        }
    }
    
    // Log all plugin paths
    for (const auto& path : plugins.plugin_paths) {
        LOG_INFO_FMT("Plugin search path: {}", path);
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
                    LOG_DEBUG_FMT("Plugin: {}", plugin_config.name);
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
                            LOG_DEBUG_FMT("  {}: {}", key, value);
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
            LOG_WARN_FMT("Invalid hex color format: {}, using black", hex);
        }
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("Failed to parse hex color '{}': {}", hex, e.what());
    }
}

// Recursive widget parsing function
static WidgetConfig ParseWidget(const YAML::Node& widget_node) {
    WidgetConfig widget;
    
    if (!widget_node["type"]) {
        LOG_WARN("Widget missing 'type', skipping");
        return widget;
    }
    
    widget.type = widget_node["type"].as<std::string>();
    
    // Parse properties
    if (widget_node["properties"] && widget_node["properties"].IsMap()) {
        for (const auto& prop : widget_node["properties"]) {
            std::string key = prop.first.as<std::string>();
            widget.properties[key] = prop.second.as<std::string>();
        }
    }
    
    // Also support properties at root level (backward compatibility)
    for (const auto& prop : widget_node) {
        std::string key = prop.first.as<std::string>();
        if (key != "type" && key != "properties" && key != "children") {
            widget.properties[key] = prop.second.as<std::string>();
        }
    }
    
    // Recursively parse children (for containers like hbox, vbox)
    if (widget_node["children"] && widget_node["children"].IsSequence()) {
        for (const auto& child_node : widget_node["children"]) {
            widget.children.push_back(ParseWidget(child_node));
        }
    }
    
    return widget;
}

void ConfigParser::ParseStatusBars(const YAML::Node& node) {
    if (!node.IsSequence()) {
        LOG_WARN("status-bars should be a sequence");
        return;
    }
    
    status_bars.bars.clear();
    
    for (const auto& bar_node : node) {
        StatusBarConfig bar;
        
        // Parse name (required)
        if (bar_node["name"]) {
            bar.name = bar_node["name"].as<std::string>();
        } else {
            LOG_WARN("Status bar missing 'name', skipping");
            continue;
        }
        
        // Parse position
        if (bar_node["position"]) {
            std::string pos = bar_node["position"].as<std::string>();
            if (pos == "top") {
                bar.position = StatusBarConfig::Position::Top;
            } else if (pos == "bottom") {
                bar.position = StatusBarConfig::Position::Bottom;
            } else if (pos == "left") {
                bar.position = StatusBarConfig::Position::Left;
            } else if (pos == "right") {
                bar.position = StatusBarConfig::Position::Right;
            } else {
                LOG_WARN_FMT("Unknown status bar position '{}', using top", pos);
            }
        }
        
        // Parse dimensions
        if (bar_node["height"]) {
            bar.height = bar_node["height"].as<int>();
        }
        if (bar_node["width"]) {
            bar.width = bar_node["width"].as<int>();
        }
        
        // Parse appearance
        if (bar_node["background_color"]) {
            bar.background_color = bar_node["background_color"].as<std::string>();
        }
        if (bar_node["foreground_color"]) {
            bar.foreground_color = bar_node["foreground_color"].as<std::string>();
        }
        if (bar_node["font_size"]) {
            bar.font_size = bar_node["font_size"].as<int>();
        }
        if (bar_node["font_family"]) {
            bar.font_family = bar_node["font_family"].as<std::string>();
        }
        
        // Parse widget containers
        auto parse_container = [](const YAML::Node& container_node, ContainerConfig& container) {
            if (!container_node.IsDefined()) {
                return;
            }
            
            if (container_node["spacing"]) {
                container.spacing = container_node["spacing"].as<int>();
            }
            if (container_node["alignment"]) {
                container.alignment = container_node["alignment"].as<std::string>();
            }
            if (container_node["padding"]) {
                container.padding = container_node["padding"].as<int>();
            }
            
            // Parse widgets
            if (container_node["widgets"] && container_node["widgets"].IsSequence()) {
                for (const auto& widget_node : container_node["widgets"]) {
                    WidgetConfig widget;
                    
                    if (widget_node["type"]) {
                        widget.type = widget_node["type"].as<std::string>();
                    } else {
                        LOG_WARN("Widget missing 'type', skipping");
                        continue;
                    }
                    
                    // Parse all other properties as string key-value pairs
                    for (const auto& prop : widget_node) {
                        std::string key = prop.first.as<std::string>();
                        if (key != "type") {
                            widget.properties[key] = prop.second.as<std::string>();
                        }
                    }
                    
                    container.widgets.push_back(widget);
                }
            }
        };
        
        parse_container(bar_node["left"], bar.left);
        parse_container(bar_node["center"], bar.center);
        parse_container(bar_node["right"], bar.right);
        
        // Parse new root widget structure (preferred)
        if (bar_node["root"]) {
            bar.root = ParseWidget(bar_node["root"]);
        }
        
        status_bars.bars.push_back(bar);
        LOG_DEBUG_FMT("Loaded status bar config: '{}'", bar.name);
    }
    
    LOG_INFO_FMT("Loaded {} status bar configuration(s)", status_bars.bars.size());
}

void ConfigParser::ParseMonitorGroups(const YAML::Node& node) {
    if (!node.IsSequence()) {
        LOG_WARN("monitor-groups should be a sequence");
        return;
    }
    
    monitor_groups.groups.clear();
    bool has_default = false;
    
    for (const auto& group_node : node) {
        MonitorGroup group;
        
        if (group_node["name"]) {
            group.name = group_node["name"].as<std::string>();
        } else {
            LOG_WARN("Monitor group missing 'name', skipping");
            continue;
        }
        
        // Check if this is the default group
        if (group_node["default"]) {
            group.is_default = group_node["default"].as<bool>();
            if (group.is_default) {
                if (has_default) {
                    LOG_WARN("Multiple default monitor groups defined, using first one");
                    group.is_default = false;
                } else {
                    has_default = true;
                }
            }
        }
        
        // Special case: "Default" name is always default
        if (group.name == "Default") {
            if (has_default && !group.is_default) {
                LOG_WARN("Group named 'Default' found but another default already set");
            } else {
                group.is_default = true;
                has_default = true;
            }
        }
        
        // Parse monitors in this group
        if (group_node["monitors"] && group_node["monitors"].IsSequence()) {
            for (const auto& mon_node : group_node["monitors"]) {
                MonitorConfig mon;
                
                // Get identifier (required)
                if (mon_node["display"]) {
                    mon.identifier = mon_node["display"].as<std::string>();
                } else if (mon_node["id"]) {
                    mon.identifier = mon_node["id"].as<std::string>();
                } else {
                    LOG_WARN("Monitor config missing 'display' or 'id', skipping");
                    continue;
                }
                
                // Parse status bars assigned to this monitor
                if (mon_node["status-bars"] && mon_node["status-bars"].IsSequence()) {
                    for (const auto& bar_name : mon_node["status-bars"]) {
                        mon.status_bars.push_back(bar_name.as<std::string>());
                    }
                } else if (mon_node["status-bar"]) {
                    // Allow single status bar as string
                    mon.status_bars.push_back(mon_node["status-bar"].as<std::string>());
                }
                
                // Parse wallpaper reference for this specific monitor
                if (mon_node["wallpaper"]) {
                    mon.wallpaper = mon_node["wallpaper"].as<std::string>();
                }
                
                // Parse position (e.g., "1920x0" or "0x1080")
                if (mon_node["pos"] || mon_node["position"]) {
                    std::string pos_str = mon_node["pos"] ? 
                        mon_node["pos"].as<std::string>() : 
                        mon_node["position"].as<std::string>();
                    
                    size_t x_pos = pos_str.find('x');
                    if (x_pos != std::string::npos) {
                        try {
                            int x = std::stoi(pos_str.substr(0, x_pos));
                            int y = std::stoi(pos_str.substr(x_pos + 1));
                            mon.position = {x, y};
                        } catch (const std::exception& e) {
                            LOG_WARN_FMT("Invalid position format '{}': {}", pos_str, e.what());
                        }
                    }
                }
                
                // Parse mode/size (e.g., "1920x1080" or "3840x2160@60")
                if (mon_node["mode"] || mon_node["size"]) {
                    mon.mode = mon_node["mode"] ? 
                        mon_node["mode"].as<std::string>() : 
                        mon_node["size"].as<std::string>();
                }
                
                // Parse scale
                if (mon_node["scale"]) {
                    mon.scale = mon_node["scale"].as<float>();
                }
                
                // Parse transform/rotation
                if (mon_node["transform"] || mon_node["rotation"]) {
                    mon.transform = mon_node["transform"] ? 
                        mon_node["transform"].as<int>() : 
                        mon_node["rotation"].as<int>();
                }
                
                group.monitors.push_back(mon);
            }
        }
        
        monitor_groups.groups.push_back(group);
        LOG_INFO_FMT("Loaded monitor group '{}' with {} monitors{}", 
                 group.name, group.monitors.size(), 
                 group.is_default ? " (default)" : "");
    }
    
    // Ensure we have a default group
    if (!has_default && !monitor_groups.groups.empty()) {
        LOG_WARN("No default monitor group specified, using first group as default");
        monitor_groups.groups[0].is_default = true;
    }
}

const MonitorGroup* MonitorGroupsConfig::GetDefaultGroup() const {
    for (const auto& group : groups) {
        if (group.is_default) {
            return &group;
        }
    }
    return groups.empty() ? nullptr : &groups[0];
}

const MonitorGroup* MonitorGroupsConfig::FindMatchingGroup(
    const std::vector<std::string>& connected_outputs) const {
    
    // Helper to check if an identifier matches an output
    auto matches = [](const std::string& identifier, const std::string& output_name, 
                      const std::string& output_desc, const std::string& output_make_model) {
        // Direct name match (e.g., "eDP-1", "HDMI-A-1")
        if (identifier == output_name) {
            return true;
        }
        
        // Description match (e.g., "d:Dell Inc. U2720Q")
        if (identifier.size() > 2 && identifier.substr(0, 2) == "d:") {
            std::string search = identifier.substr(2);
            return output_desc.find(search) != std::string::npos;
        }
        
        // Make/model match (e.g., "m:Dell Inc./U2720Q")
        if (identifier.size() > 2 && identifier.substr(0, 2) == "m:") {
            std::string search = identifier.substr(2);
            return output_make_model.find(search) != std::string::npos;
        }
        
        return false;
    };
    
    // Try to find a group where ALL monitors match connected outputs
    for (const auto& group : groups) {
        if (group.is_default) {
            continue;  // Skip default group, try specific ones first
        }
        
        if (group.monitors.empty()) {
            continue;
        }
        
        // Check if all monitors in this group can be matched
        bool all_matched = true;
        for (const auto& mon : group.monitors) {
            bool found = false;
            for (const auto& output : connected_outputs) {
                // TODO: Get actual description and make/model from wlroots
                // For now, just do name matching
                if (matches(mon.identifier, output, "", "")) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                all_matched = false;
                break;
            }
        }
        
        if (all_matched) {
            return &group;
        }
    }
    
    // No specific group matched, return default
    return GetDefaultGroup();
}

void ConfigParser::ParseWallpapers(const YAML::Node& node) {
    LOG_DEBUG("Parsing wallpapers section");
    
    if (!node.IsSequence()) {
        LOG_WARN("wallpapers should be a sequence");
        return;
    }
    
    wallpapers.wallpapers.clear();
    
    for (const auto& wp_node : node) {
        WallpaperConfig wp;
        
        if (wp_node["name"]) {
            wp.name = wp_node["name"].as<std::string>();
        } else {
            LOG_WARN("Wallpaper config missing 'name', skipping");
            continue;
        }
        
        // Parse wallpaper paths - can be a single string or sequence
        if (wp_node["wallpaper"]) {
            const auto& path_node = wp_node["wallpaper"];
            if (path_node.IsScalar()) {
                // Single wallpaper path
                wp.wallpapers.push_back(path_node.as<std::string>());
            } else if (path_node.IsSequence()) {
                // Multiple wallpaper paths
                for (const auto& path : path_node) {
                    wp.wallpapers.push_back(path.as<std::string>());
                }
            }
        }
        
        // Check if any paths are folders and expand them
        std::vector<std::string> expanded_wallpapers;
        for (const auto& path : wp.wallpapers) {
            std::filesystem::path fs_path(path);
            
            // Expand ~ to home directory
            if (path.size() > 0 && path[0] == '~') {
                const char* home = std::getenv("HOME");
                if (home) {
                    fs_path = std::string(home) + path.substr(1);
                }
            }
            
            if (std::filesystem::is_directory(fs_path)) {
                // It's a folder, scan for image files
                LOG_INFO_FMT("Scanning wallpaper folder: {}", fs_path.string());
                try {
                    for (const auto& entry : std::filesystem::directory_iterator(fs_path)) {
                        if (entry.is_regular_file()) {
                            auto ext = entry.path().extension().string();
                            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                            // Support common image formats
                            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || 
                                ext == ".bmp" || ext == ".webp") {
                                expanded_wallpapers.push_back(entry.path().string());
                            }
                        }
                    }
                } catch (const std::filesystem::filesystem_error& e) {
                    LOG_ERROR_FMT("Failed to read wallpaper folder {}: {}", fs_path.string(), e.what());
                }
            } else if (std::filesystem::exists(fs_path)) {
                // It's a file
                expanded_wallpapers.push_back(fs_path.string());
            } else {
                LOG_WARN_FMT("Wallpaper path does not exist: {}", path);
            }
        }
        
        wp.wallpapers = expanded_wallpapers;
        
        // Parse change interval
        if (wp_node["change_every_seconds"]) {
            wp.change_interval_seconds = wp_node["change_every_seconds"].as<int>();
        } else if (wp_node["change-every-seconds"]) {
            wp.change_interval_seconds = wp_node["change-every-seconds"].as<int>();
        } else if (wp_node["change_interval"]) {
            wp.change_interval_seconds = wp_node["change_interval"].as<int>();
        }
        
        LOG_INFO_FMT("Loaded wallpaper config '{}' with {} wallpaper(s), change interval: {}s", 
                     wp.name, wp.wallpapers.size(), wp.change_interval_seconds);
        
        wallpapers.wallpapers.push_back(wp);
    }
}

const WallpaperConfig* WallpapersConfig::FindByName(const std::string& name) const {
    for (const auto& wp : wallpapers) {
        if (wp.name == name) {
            return &wp;
        }
    }
    return nullptr;
}

const StatusBarConfig* StatusBarsConfig::FindByName(const std::string& name) const {
    for (const auto& bar : bars) {
        if (bar.name == name) {
            return &bar;
        }
    }
    return nullptr;
}

} // namespace Leviathan
