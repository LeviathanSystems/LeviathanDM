#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Leviathan {

Config::Config() {
    SetDefaults();
}

void Config::SetDefaults() {
    border_width_ = 2;
    gap_size_ = 0;  // No gaps by default - windows tile edge-to-edge
    border_focused_ = 0x5e81ac;    // Nord blue
    border_unfocused_ = 0x3b4252;  // Nord dark gray
    workspace_count_ = 9;
    focus_follows_mouse_ = true;
}

bool Config::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open config file: " << filename << "\n";
        std::cerr << "Using default configuration\n";
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string key, value;
        
        if (!(iss >> key >> value)) {
            continue;
        }
        
        // Parse configuration options
        if (key == "border_width") {
            border_width_ = std::stoi(value);
        } else if (key == "gap_size") {
            gap_size_ = std::stoi(value);
        } else if (key == "border_focused") {
            border_focused_ = ParseColor(value);
        } else if (key == "border_unfocused") {
            border_unfocused_ = ParseColor(value);
        } else if (key == "workspace_count") {
            workspace_count_ = std::stoi(value);
        } else if (key == "focus_follows_mouse") {
            focus_follows_mouse_ = (value == "true" || value == "1");
        }
    }
    
    file.close();
    return true;
}

unsigned long Config::ParseColor(const std::string& color) {
    // Expecting format: #RRGGBB or 0xRRGGBB
    std::string hex = color;
    
    if (hex[0] == '#') {
        hex = "0x" + hex.substr(1);
    }
    
    return std::stoul(hex, nullptr, 16);
}

} // namespace Leviathan
