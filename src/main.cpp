#include "wayland/Server.hpp"
#include "config/ConfigParser.hpp"
#include "ui/WidgetPluginManager.hpp"
#include "ui/CompositorState.hpp"
#include "Logger.hpp"
#include "version.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

void print_version() {
    std::cout << LEVIATHAN_NAME << " v" << LEVIATHAN_VERSION << std::endl;
    std::cout << LEVIATHAN_DESCRIPTION << std::endl;
    std::cout << std::endl;
    std::cout << "Version: " << LEVIATHAN_VERSION_MAJOR << "." 
              << LEVIATHAN_VERSION_MINOR << "." 
              << LEVIATHAN_VERSION_PATCH << std::endl;
    std::cout << "Built with wlroots 0.19.2" << std::endl;
}

void print_help(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help     Show this help message" << std::endl;
    std::cout << "  -v, --version  Show version information" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            print_help(argv[0]);
            return 1;
        }
    }
    
    // Initialize logger with multi-sink architecture
    // Creates both console sink (with colors) and file sink
    Leviathan::SimpleLogger::Instance().Init(
        "leviathan.log",              // Log file path
        Leviathan::LogLevel::DEBUG,   // Minimum log level
        true                          // Enable console colors
    );
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "{} v{}", LEVIATHAN_NAME, LEVIATHAN_VERSION);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "{}", LEVIATHAN_DESCRIPTION);
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // Load configuration from standard locations
    auto& config = Leviathan::Config();
    
    const char* home = getenv("HOME");
    const char* xdg_config = getenv("XDG_CONFIG_HOME");
    std::string config_dir = xdg_config ? std::string(xdg_config) : 
                             (home ? std::string(home) + "/.config" : "");
    
    std::vector<std::string> config_paths;
    if (!config_dir.empty()) {
        config_paths.push_back(config_dir + "/leviathan/leviathan.yaml");
    }
    config_paths.push_back("/etc/leviathan/leviathan.yaml");
    
    bool config_loaded = false;
    for (const auto& path : config_paths) {
        if (config.LoadWithIncludes(path)) {
            config_loaded = true;
            break;
        }
    }
    
    if (!config_loaded) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "No configuration file found, using defaults");
    }
    
    // Load widget plugins
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Loading Widget Plugins");
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    auto& plugin_manager = Leviathan::UI::PluginManager();
    
    // Discover plugins from configured paths
    if (!config.plugins.plugin_paths.empty()) {
        for (const auto& path : config.plugins.plugin_paths) {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Searching for plugins in: {}", path);
            plugin_manager.DiscoverPlugins(path);
        }
    } else {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "No plugin paths configured, plugins will not be loaded");
    }
    
    // Log loaded plugins
    auto loaded_plugins = plugin_manager.GetLoadedPlugins();
    if (!loaded_plugins.empty()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Loaded {} plugin(s):", loaded_plugins.size());
        for (const auto& plugin_name : loaded_plugins) {
            auto metadata = plugin_manager.GetPluginMetadata(plugin_name);
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "  ✓ {} v{} by {}", 
                     metadata.name, metadata.version, metadata.author);
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "    {}", metadata.description);
        }
    } else {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "No plugins loaded");
    }
    
    // Create plugin instances from config
    if (!config.plugins.plugins.empty()) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Creating {} plugin instance(s) from config...", 
                 config.plugins.plugins.size());
        
        for (const auto& plugin_cfg : config.plugins.plugins) {
            Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Initializing: {}", plugin_cfg.name);
            
            // Log config
            if (!plugin_cfg.config.empty()) {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "  Configuration:");
                for (const auto& [key, value] : plugin_cfg.config) {
                    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "    {}: {}", key, value);
                }
            }
            
            // Create instance
            auto widget = plugin_manager.CreatePluginWidget(
                plugin_cfg.name, 
                plugin_cfg.config
            );
            
            if (widget) {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "  ✓ Successfully initialized {}", plugin_cfg.name);
            } else {
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "  ✗ Failed to initialize {}", plugin_cfg.name);
            }
        }
    } else {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "No plugin instances configured");
    }
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Starting compositor...");
    
    auto server = Leviathan::Wayland::Server::Create();
    if (!server) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Failed to initialize compositor");
        return EXIT_FAILURE;
    }
    
    // Make compositor state available to widgets
    Leviathan::UI::SetCompositorState(server);
    
    server->Run();
    
    // Clear compositor state
    Leviathan::UI::SetCompositorState(nullptr);
    
    delete server;
    
    // Cleanup plugins
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Unloading plugins...");
    plugin_manager.UnloadAll();
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Plugins unloaded");
    
    // Shutdown logger (flushes all sinks and stops worker thread)
    Leviathan::SimpleLogger::Instance().Shutdown();
    
    return EXIT_SUCCESS;
}
