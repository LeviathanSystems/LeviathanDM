#include "ipc/IPC.hpp"
#include "Logger.hpp"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <algorithm>

using json = nlohmann::json;

namespace Leviathan {
namespace IPC {

std::string CommandTypeToString(CommandType type) {
    switch (type) {
        case CommandType::GET_TAGS: return "get_tags";
        case CommandType::GET_CLIENTS: return "get_clients";
        case CommandType::GET_OUTPUTS: return "get_outputs";
        case CommandType::GET_ACTIVE_TAG: return "get_active_tag";
        case CommandType::SET_ACTIVE_TAG: return "set_active_tag";
        case CommandType::GET_LAYOUT: return "get_layout";
        case CommandType::GET_VERSION: return "get_version";
        case CommandType::GET_PLUGIN_STATS: return "get_plugin_stats";
        case CommandType::PING: return "ping";
        default: return "unknown";
    }
}

CommandType StringToCommandType(const std::string& str) {
    if (str == "get_tags") return CommandType::GET_TAGS;
    if (str == "get_clients") return CommandType::GET_CLIENTS;
    if (str == "get_outputs") return CommandType::GET_OUTPUTS;
    if (str == "get_active_tag") return CommandType::GET_ACTIVE_TAG;
    if (str == "set_active_tag") return CommandType::SET_ACTIVE_TAG;
    if (str == "get_layout") return CommandType::GET_LAYOUT;
    if (str == "get_version") return CommandType::GET_VERSION;
    if (str == "get_plugin_stats") return CommandType::GET_PLUGIN_STATS;
    if (str == "ping") return CommandType::PING;
    return CommandType::UNKNOWN;
}

std::string SerializeResponse(const Response& response) {
    json j;
    j["success"] = response.success;
    j["error"] = response.error;
    
    if (!response.data.empty()) {
        j["data"] = response.data;
    }
    
    if (!response.tags.empty()) {
        json tags_arr = json::array();
        for (const auto& tag : response.tags) {
            tags_arr.push_back({
                {"name", tag.name},
                {"visible", tag.visible},
                {"client_count", tag.client_count}
            });
        }
        j["tags"] = tags_arr;
    }
    
    if (!response.clients.empty()) {
        json clients_arr = json::array();
        for (const auto& client : response.clients) {
            clients_arr.push_back({
                {"title", client.title},
                {"app_id", client.app_id},
                {"x", client.x},
                {"y", client.y},
                {"width", client.width},
                {"height", client.height},
                {"floating", client.floating},
                {"fullscreen", client.fullscreen},
                {"tag", client.tag}
            });
        }
        j["clients"] = clients_arr;
    }
    
    if (!response.outputs.empty()) {
        json outputs_arr = json::array();
        for (const auto& output : response.outputs) {
            json output_obj = {
                {"name", output.name},
                {"width", output.width},
                {"height", output.height},
                {"refresh_mhz", output.refresh_mhz},
                {"enabled", output.enabled},
                {"scale", output.scale}
            };
            
            // Add optional fields if they exist
            if (!output.description.empty()) {
                output_obj["description"] = output.description;
            }
            if (!output.make.empty()) {
                output_obj["make"] = output.make;
            }
            if (!output.model.empty()) {
                output_obj["model"] = output.model;
            }
            if (!output.serial.empty()) {
                output_obj["serial"] = output.serial;
            }
            if (output.phys_width_mm > 0) {
                output_obj["phys_width_mm"] = output.phys_width_mm;
            }
            if (output.phys_height_mm > 0) {
                output_obj["phys_height_mm"] = output.phys_height_mm;
            }
            
            outputs_arr.push_back(output_obj);
        }
        j["outputs"] = outputs_arr;
    }
    
    if (!response.plugin_stats.empty()) {
        json stats_arr = json::array();
        for (const auto& stats : response.plugin_stats) {
            stats_arr.push_back({
                {"name", stats.name},
                {"rss_bytes", stats.rss_bytes},
                {"virtual_bytes", stats.virtual_bytes},
                {"instance_count", stats.instance_count}
            });
        }
        j["plugin_stats"] = stats_arr;
    }
    
    return j.dump() + "\n";
}

// IPC Server implementation
Server::Server() : socket_fd(-1) {
    const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        socket_path = std::string(runtime_dir) + "/leviathan-ipc.sock";
    } else {
        socket_path = "/tmp/leviathan-ipc.sock";
    }
}

Server::~Server() {
    Cleanup();
}

bool Server::Initialize() {
    // Remove existing socket file
    unlink(socket_path.c_str());
    
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        LOG_ERROR_FMT("Failed to create IPC socket: {}", strerror(errno));
        return false;
    }
    
    // Set non-blocking
    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
    
    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR_FMT("Failed to bind IPC socket: {}", strerror(errno));
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    if (listen(socket_fd, 5) < 0) {
        LOG_ERROR_FMT("Failed to listen on IPC socket: {}", strerror(errno));
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    LOG_INFO_FMT("IPC server listening on {}", socket_path);
    return true;
}

std::string Server::GetSocketPath() const {
    return socket_path;
}

void Server::AcceptClient() {
    int client_fd = accept(socket_fd, nullptr, nullptr);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR_FMT("Failed to accept IPC client: {}", strerror(errno));
        }
        return;
    }
    
    // Set non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    client_fds.push_back(client_fd);
    LOG_DEBUG_FMT("IPC client connected (fd={})", client_fd);
}

void Server::HandleClient(int client_fd) {
    char buffer[4096];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (n <= 0) {
        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_DEBUG_FMT("IPC client disconnected (fd={})", client_fd);
            close(client_fd);
            client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
        }
        return;
    }
    
    buffer[n] = '\0';
    std::string command(buffer);
    
    // Process command and send response
    Response response = ProcessCommand(command);
    std::string response_str = SerializeResponse(response);
    
    write(client_fd, response_str.c_str(), response_str.length());
    
    // Close connection after response
    close(client_fd);
    client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
}

void Server::HandleEvents() {
    if (socket_fd < 0) return;
    
    // Accept new clients
    AcceptClient();
    
    // Handle existing clients
    for (int client_fd : client_fds) {
        HandleClient(client_fd);
    }
}

Response Server::ProcessCommand(const std::string& command_str) {
    // If we have a command processor callback, use it
    if (command_processor_) {
        return command_processor_(command_str);
    }
    
    // Fallback: handle basic commands
    Response response;
    response.success = false;
    
    try {
        json j = json::parse(command_str);
        
        if (!j.contains("command")) {
            response.error = "Missing 'command' field";
            return response;
        }
        
        std::string cmd = j["command"];
        CommandType type = StringToCommandType(cmd);
        
        if (type == CommandType::PING) {
            response.success = true;
            response.data["pong"] = "pong";
            return response;
        }
        
        if (type == CommandType::GET_VERSION) {
            response.success = true;
            response.data["version"] = "0.1.0";
            response.data["compositor"] = "LeviathanDM";
            return response;
        }
        
        response.error = "Command not implemented: " + cmd;
        return response;
        
    } catch (const json::exception& e) {
        response.error = std::string("JSON parse error: ") + e.what();
        return response;
    }
}

void Server::Cleanup() {
    for (int fd : client_fds) {
        close(fd);
    }
    client_fds.clear();
    
    if (socket_fd >= 0) {
        close(socket_fd);
        unlink(socket_path.c_str());
        socket_fd = -1;
    }
}

// IPC Client implementation
Client::Client() : socket_fd(-1) {
    const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        socket_path = std::string(runtime_dir) + "/leviathan-ipc.sock";
    } else {
        socket_path = "/tmp/leviathan-ipc.sock";
    }
}

Client::~Client() {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

bool Client::Connect() {
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return false;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
    
    if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    return true;
}

std::optional<Response> Client::SendCommand(CommandType type, const std::map<std::string, std::string>& args) {
    if (socket_fd < 0 && !Connect()) {
        return std::nullopt;
    }
    
    // Build command JSON
    json j;
    j["command"] = CommandTypeToString(type);
    
    if (!args.empty()) {
        j["args"] = args;
    }
    
    std::string command = j.dump() + "\n";
    
    // Send command
    if (write(socket_fd, command.c_str(), command.length()) < 0) {
        return std::nullopt;
    }
    
    // Read response
    char buffer[8192];
    ssize_t n = read(socket_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        return std::nullopt;
    }
    
    buffer[n] = '\0';
    
    // Parse JSON response
    try {
        json resp = json::parse(buffer);
        
        Response response;
        response.success = resp.value("success", false);
        response.error = resp.value("error", "");
        
        if (resp.contains("data")) {
            response.data = resp["data"].get<std::map<std::string, std::string>>();
        }
        
        // Parse tags array
        if (resp.contains("tags")) {
            for (const auto& tag_json : resp["tags"]) {
                TagInfo tag;
                tag.name = tag_json.value("name", "");
                tag.visible = tag_json.value("visible", false);
                tag.client_count = tag_json.value("client_count", 0);
                response.tags.push_back(tag);
            }
        }
        
        // Parse clients array
        if (resp.contains("clients")) {
            for (const auto& client_json : resp["clients"]) {
                ClientInfo client;
                client.title = client_json.value("title", "");
                client.app_id = client_json.value("app_id", "");
                client.x = client_json.value("x", 0);
                client.y = client_json.value("y", 0);
                client.width = client_json.value("width", 0);
                client.height = client_json.value("height", 0);
                client.floating = client_json.value("floating", false);
                client.fullscreen = client_json.value("fullscreen", false);
                client.tag = client_json.value("tag", "");
                response.clients.push_back(client);
            }
        }
        
        // Parse outputs array
        if (resp.contains("outputs")) {
            for (const auto& output_json : resp["outputs"]) {
                OutputInfo output;
                output.name = output_json.value("name", "");
                output.description = output_json.value("description", "");
                output.make = output_json.value("make", "");
                output.model = output_json.value("model", "");
                output.serial = output_json.value("serial", "");
                output.width = output_json.value("width", 0);
                output.height = output_json.value("height", 0);
                output.refresh_mhz = output_json.value("refresh_mhz", 0);
                output.phys_width_mm = output_json.value("phys_width_mm", 0);
                output.phys_height_mm = output_json.value("phys_height_mm", 0);
                output.scale = output_json.value("scale", 1.0f);
                output.enabled = output_json.value("enabled", false);
                response.outputs.push_back(output);
            }
        }
        
        // Parse plugin_stats array
        if (resp.contains("plugin_stats")) {
            for (const auto& stats_json : resp["plugin_stats"]) {
                PluginStats stats;
                stats.name = stats_json.value("name", "");
                stats.rss_bytes = stats_json.value("rss_bytes", 0);
                stats.virtual_bytes = stats_json.value("virtual_bytes", 0);
                stats.instance_count = stats_json.value("instance_count", 0);
                response.plugin_stats.push_back(stats);
            }
        }
        
        // Store raw response for debugging
        response.data["raw"] = std::string(buffer);
        
        return response;
    } catch (const json::exception& e) {
        return std::nullopt;
    }
}

} // namespace IPC
} // namespace Leviathan
