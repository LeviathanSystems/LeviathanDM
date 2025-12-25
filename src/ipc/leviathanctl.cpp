#include "ipc/IPC.hpp"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cmath>

using namespace Leviathan::IPC;

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " <command> [args]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  ping                    - Test connection to compositor\n";
    std::cout << "  version                 - Get compositor version\n";
    std::cout << "  get-tags                - List all tags and their state\n";
    std::cout << "  get-clients             - List all clients (windows)\n";
    std::cout << "  get-outputs             - List all outputs (monitors)\n";
    std::cout << "  get-active-tag          - Get currently active tag\n";
    std::cout << "  set-active-tag <name>   - Switch to specified tag\n";
    std::cout << "  get-layout              - Get current layout mode\n";
    std::cout << "  get-plugin-stats        - Show memory usage per plugin\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << prog << " ping\n";
    std::cout << "  " << prog << " get-clients\n";
    std::cout << "  " << prog << " set-active-tag 2\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "--help" || command == "-h") {
        print_usage(argv[0]);
        return 0;
    }
    
    Client client;
    if (!client.Connect()) {
        std::cerr << "Error: Could not connect to LeviathanDM compositor\n";
        std::cerr << "Make sure the compositor is running.\n";
        return 1;
    }
    
    CommandType cmd_type = CommandType::UNKNOWN;
    std::map<std::string, std::string> args;
    
    if (command == "ping") {
        cmd_type = CommandType::PING;
    } else if (command == "version") {
        cmd_type = CommandType::GET_VERSION;
    } else if (command == "get-tags") {
        cmd_type = CommandType::GET_TAGS;
    } else if (command == "get-clients") {
        cmd_type = CommandType::GET_CLIENTS;
    } else if (command == "get-outputs") {
        cmd_type = CommandType::GET_OUTPUTS;
    } else if (command == "get-active-tag") {
        cmd_type = CommandType::GET_ACTIVE_TAG;
    } else if (command == "set-active-tag") {
        if (argc < 3) {
            std::cerr << "Error: set-active-tag requires a tag name\n";
            return 1;
        }
        cmd_type = CommandType::SET_ACTIVE_TAG;
        args["tag"] = argv[2];
    } else if (command == "get-layout") {
        cmd_type = CommandType::GET_LAYOUT;
    } else if (command == "get-plugin-stats") {
        cmd_type = CommandType::GET_PLUGIN_STATS;
    } else {
        std::cerr << "Error: Unknown command '" << command << "'\n";
        print_usage(argv[0]);
        return 1;
    }
    
    auto response = client.SendCommand(cmd_type, args);
    if (!response) {
        std::cerr << "Error: Failed to get response from compositor\n";
        return 1;
    }
    
    if (!response->success) {
        std::cerr << "Error: " << response->error << "\n";
        return 1;
    }
    
    // Pretty-print specific command outputs
    if (command == "get-outputs" && !response->outputs.empty()) {
        std::cout << "Outputs:\n";
        for (const auto& output : response->outputs) {
            std::cout << "\n";
            std::cout << "  Name:        " << output.name << "\n";
            
            if (!output.description.empty()) {
                std::cout << "  Description: " << output.description << "\n";
            }
            if (!output.make.empty()) {
                std::cout << "  Make:        " << output.make << "\n";
            }
            if (!output.model.empty()) {
                std::cout << "  Model:       " << output.model << "\n";
            }
            if (!output.serial.empty()) {
                std::cout << "  Serial:      " << output.serial << "\n";
            }
            
            std::cout << "  Resolution:  " << output.width << "x" << output.height;
            if (output.refresh_mhz > 0) {
                std::cout << " @ " << std::fixed << std::setprecision(2) 
                         << (output.refresh_mhz / 1000.0) << " Hz";
            }
            std::cout << "\n";
            
            std::cout << "  Scale:       " << std::fixed << std::setprecision(2) << output.scale << "x\n";
            
            if (output.phys_width_mm > 0 && output.phys_height_mm > 0) {
                std::cout << "  Physical:    " << output.phys_width_mm << "mm x " 
                         << output.phys_height_mm << "mm";
                // Calculate diagonal in inches
                float diag_mm = std::sqrt(output.phys_width_mm * output.phys_width_mm + 
                                         output.phys_height_mm * output.phys_height_mm);
                float diag_inches = diag_mm / 25.4;
                std::cout << " (" << std::fixed << std::setprecision(1) << diag_inches << "\")\n";
            }
            
            std::cout << "  Enabled:     " << (output.enabled ? "yes" : "no") << "\n";
        }
    } else if (command == "get-plugin-stats" && !response->plugin_stats.empty()) {
        std::cout << "Plugin Memory Statistics:\n\n";
        
        size_t total_rss = 0;
        size_t total_virtual = 0;
        int total_instances = 0;
        
        for (const auto& stats : response->plugin_stats) {
            std::cout << "  Plugin:     " << stats.name << "\n";
            std::cout << "  Instances:  " << stats.instance_count << "\n";
            
            // Format memory in human-readable units
            auto format_bytes = [](size_t bytes) -> std::string {
                if (bytes < 1024) return std::to_string(bytes) + " B";
                if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
                return std::to_string(bytes / (1024 * 1024)) + " MB";
            };
            
            std::cout << "  RSS:        " << format_bytes(stats.rss_bytes) << "\n";
            std::cout << "  Virtual:    " << format_bytes(stats.virtual_bytes) << "\n";
            std::cout << "\n";
            
            total_rss += stats.rss_bytes;
            total_virtual += stats.virtual_bytes;
            total_instances += stats.instance_count;
        }
        
        if (response->plugin_stats.size() > 1) {
            std::cout << "Total:\n";
            std::cout << "  Plugins:    " << response->plugin_stats.size() << "\n";
            std::cout << "  Instances:  " << total_instances << "\n";
            
            auto format_bytes = [](size_t bytes) -> std::string {
                if (bytes < 1024) return std::to_string(bytes) + " B";
                if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
                return std::to_string(bytes / (1024 * 1024)) + " MB";
            };
            
            std::cout << "  Total RSS:  " << format_bytes(total_rss) << "\n";
            std::cout << "  Total Virt: " << format_bytes(total_virtual) << "\n";
        }
    } else if (response->data.count("raw")) {
        std::cout << response->data["raw"];
    } else {
        std::cout << "Success\n";
    }
    
    return 0;
}
