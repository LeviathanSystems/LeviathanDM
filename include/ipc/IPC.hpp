#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>

namespace Leviathan {
namespace IPC {

// IPC protocol using simple JSON messages over Unix socket
// Socket path: /run/user/$UID/leviathan-ipc.sock

enum class CommandType {
    GET_TAGS,           // Get all tags and their state
    GET_CLIENTS,        // Get all clients (windows)
    GET_OUTPUTS,        // Get all outputs (monitors)
    GET_ACTIVE_TAG,     // Get currently active tag
    SET_ACTIVE_TAG,     // Switch to a different tag
    GET_LAYOUT,         // Get current layout mode
    GET_VERSION,        // Get compositor version
    GET_PLUGIN_STATS,   // Get plugin memory statistics
    PING,              // Simple ping/pong for testing
    UNKNOWN
};

struct TagInfo {
    std::string name;
    bool visible;
    int client_count;
};

struct ClientInfo {
    std::string title;
    std::string app_id;
    int x, y;
    int width, height;
    bool floating;
    bool fullscreen;
    std::string tag;
};

struct OutputInfo {
    std::string name;
    std::string description;  // Human-readable description (e.g., "Dell U2415 (DP-1)")
    std::string make;         // Manufacturer from EDID
    std::string model;        // Model name from EDID
    std::string serial;       // Serial number from EDID
    int width, height;
    int refresh_mhz;
    int phys_width_mm;        // Physical width in millimeters
    int phys_height_mm;       // Physical height in millimeters
    float scale;
    bool enabled;
};

struct PluginStats {
    std::string name;
    size_t rss_bytes;
    size_t virtual_bytes;
    int instance_count;
};

struct Response {
    bool success;
    std::string error;
    std::map<std::string, std::string> data;
    std::vector<TagInfo> tags;
    std::vector<ClientInfo> clients;
    std::vector<OutputInfo> outputs;
    std::vector<PluginStats> plugin_stats;
};

// IPC Server - runs in compositor
class Server {
public:
    Server();
    ~Server();
    
    bool Initialize();
    void HandleEvents();
    void Cleanup();
    
    std::string GetSocketPath() const;
    
    // Set command processor callback
    using CommandProcessor = std::function<Response(const std::string&)>;
    void SetCommandProcessor(CommandProcessor processor) { command_processor_ = processor; }
    
private:
    int socket_fd;
    std::string socket_path;
    std::vector<int> client_fds;
    CommandProcessor command_processor_;
    
    void AcceptClient();
    void HandleClient(int client_fd);
    Response ProcessCommand(const std::string& command_str);
};

// IPC Client - used by leviathanctl
class Client {
public:
    Client();
    ~Client();
    
    bool Connect();
    std::optional<Response> SendCommand(CommandType type, const std::map<std::string, std::string>& args = {});
    
private:
    int socket_fd;
    std::string socket_path;
};

// Helper functions
std::string CommandTypeToString(CommandType type);
CommandType StringToCommandType(const std::string& str);
std::string SerializeResponse(const Response& response);
std::optional<Response> DeserializeResponse(const std::string& json);

} // namespace IPC
} // namespace Leviathan
