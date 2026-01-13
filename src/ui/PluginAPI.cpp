#include "ui/PluginAPI.hpp"
#include "core/Tag.hpp"
#include "core/Client.hpp"
#include "core/Screen.hpp"
#include "core/Events.hpp"
#include "ipc/IPC.hpp"
#include "Logger.hpp"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <thread>
#include <atomic>

namespace Leviathan {
namespace UI {
namespace Plugin {

// IPC-based event subscription tracking
struct IPCEventSubscription {
    int id;
    EventType type;
    EventListener listener;
    int socket_fd = -1;
    std::thread poll_thread;
    std::atomic<bool> running{false};
};

static std::map<int, IPCEventSubscription> ipc_subscriptions;
static std::mutex ipc_subscription_mutex;
static int next_subscription_id = 1;

// Connect to IPC server and subscribe to events
static int ConnectToIPCServer() {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Plugin: Failed to create IPC socket");
        return -1;
    }
    
    // Get socket path
    const char* runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir) {
        runtime_dir = "/tmp";
    }
    
    std::string socket_path = std::string(runtime_dir) + "/leviathan-ipc.sock";
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
    
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Plugin: Failed to connect to IPC server: {}", strerror(errno));
        close(sock_fd);
        return -1;
    }
    
    // Set non-blocking
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Send subscribe_events command
    nlohmann::json cmd;
    cmd["command"] = "subscribe_events";
    std::string cmd_str = cmd.dump();
    
    ssize_t sent = write(sock_fd, cmd_str.c_str(), cmd_str.length());
    if (sent < 0) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Plugin: Failed to send subscribe command");
        close(sock_fd);
        return -1;
    }
    
    // Don't wait for confirmation - the polling thread will handle any responses
    // This prevents blocking the main thread during widget initialization
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Plugin: Sent IPC subscribe command, socket fd={}", sock_fd);
    
    return sock_fd;
}

// Event subscription
int SubscribeToEvent(EventType type, EventListener listener) {
    // Convert plugin EventType to Core::EventType
    Core::EventType core_type;
    switch (type) {
        case EventType::TagSwitched:
            core_type = Core::EventType::TagSwitched;
            break;
        case EventType::TagVisibilityChanged:
            core_type = Core::EventType::TagVisibilityChanged;
            break;
        case EventType::ClientAdded:
            core_type = Core::EventType::ClientAdded;
            break;
        case EventType::ClientRemoved:
            core_type = Core::EventType::ClientRemoved;
            break;
        case EventType::ClientTagChanged:
            core_type = Core::EventType::ClientTagChanged;
            break;
        case EventType::ClientFocused:
            core_type = Core::EventType::ClientFocused;
            break;
        case EventType::ScreenAdded:
            core_type = Core::EventType::ScreenAdded;
            break;
        case EventType::ScreenRemoved:
            core_type = Core::EventType::ScreenRemoved;
            break;
        case EventType::LayoutChanged:
            core_type = Core::EventType::LayoutChanged;
            break;
        default:
            return -1;
    }
    
    // Create IPC subscription
    std::lock_guard<std::mutex> lock(ipc_subscription_mutex);
    
    int sub_id = next_subscription_id++;
    IPCEventSubscription& sub = ipc_subscriptions[sub_id];
    sub.id = sub_id;
    sub.type = type;
    sub.listener = listener;
    
    // Connect to IPC server
    sub.socket_fd = ConnectToIPCServer();
    if (sub.socket_fd < 0) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Plugin: Failed to create IPC subscription");
        ipc_subscriptions.erase(sub_id);
        return -1;
    }
    
    // Start polling thread to read events
    sub.running = true;
    sub.poll_thread = std::thread([sub_id, type, listener, socket_fd = sub.socket_fd]() {
        auto& sub_ref = ipc_subscriptions[sub_id];
        char buffer[4096];
        std::string incomplete_msg;
        
        while (sub_ref.running) {
            ssize_t n = read(socket_fd, buffer, sizeof(buffer) - 1);
            
            if (n > 0) {
                buffer[n] = '\0';
                incomplete_msg += buffer;
                
                // Process complete JSON messages (newline-delimited)
                size_t pos;
                while ((pos = incomplete_msg.find('\n')) != std::string::npos) {
                    std::string msg = incomplete_msg.substr(0, pos);
                    incomplete_msg = incomplete_msg.substr(pos + 1);
                    
                    try {
                        auto j = nlohmann::json::parse(msg);
                        
                        // Ignore subscription confirmation messages
                        if (j.contains("success") && j["success"] == true) {
                            Leviathan::Log::WriteToLog(Leviathan::LogLevel::DEBUG, "Plugin: IPC subscription confirmed");
                            continue;
                        }
                        
                        if (j.contains("type") && j["type"] == "event" && j.contains("event_type")) {
                            std::string event_type_str = j["event_type"];
                            
                            // Check if this event matches our subscription
                            bool matches = false;
                            if (type == EventType::TagSwitched && event_type_str == "tag_switched") matches = true;
                            if (type == EventType::ClientAdded && event_type_str == "client_added") matches = true;
                            if (type == EventType::ClientRemoved && event_type_str == "client_removed") matches = true;
                            if (type == EventType::LayoutChanged && event_type_str == "tiling_mode_changed") matches = true;
                            
                            if (matches) {
                                // Create a simple event and call listener
                                Event evt;
                                evt.type = type;
                                listener(evt);
                            }
                        }
                    } catch (const std::exception& e) {
                        Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Plugin: Failed to parse IPC event: {}", e.what());
                    }
                }
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                // Socket error
                Leviathan::Log::WriteToLog(Leviathan::LogLevel::ERROR, "Plugin: IPC socket error: {}", strerror(errno));
                break;
            }
            
            // Sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Plugin: Subscribed to events via IPC (id={})", sub_id);
    return sub_id;
}

void UnsubscribeFromEvent(int subscription_id) {
    std::lock_guard<std::mutex> lock(ipc_subscription_mutex);
    
    auto it = ipc_subscriptions.find(subscription_id);
    if (it != ipc_subscriptions.end()) {
        IPCEventSubscription& sub = it->second;
        
        // Stop polling thread
        sub.running = false;
        if (sub.poll_thread.joinable()) {
            sub.poll_thread.join();
        }
        
        // Close socket
        if (sub.socket_fd >= 0) {
            close(sub.socket_fd);
        }
        
        ipc_subscriptions.erase(it);
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::INFO, "Plugin: Unsubscribed from IPC events (id={})", subscription_id);
    }
}

// Tag queries
std::string GetTagName(Core::Tag* tag) {
    if (!tag) return "";
    return tag->GetName();
}

bool IsTagVisible(Core::Tag* tag) {
    if (!tag) return false;
    return tag->IsVisible();
}

int GetTagClientCount(Core::Tag* tag) {
    if (!tag) return 0;
    return static_cast<int>(tag->GetClients().size());
}

std::vector<Core::Client*> GetTagClients(Core::Tag* tag) {
    if (!tag) return {};
    return tag->GetClients();
}

LayoutType GetTagLayout(Core::Tag* tag) {
    if (!tag) return LayoutType::MASTER_STACK;
    return tag->GetLayout();
}

// Client queries
std::string GetClientTitle(Core::Client* client) {
    if (!client) return "";
    return client->GetTitle();
}

std::string GetClientAppId(Core::Client* client) {
    if (!client) return "";
    return client->GetAppId();
}

bool IsClientFloating(Core::Client* client) {
    if (!client) return false;
    return client->IsFloating();
}

bool IsClientFullscreen(Core::Client* client) {
    if (!client) return false;
    return client->IsFullscreen();
}

// Tag actions
void SwitchToTag(int tag_index) {
    auto* compositor = UI::GetCompositorState();
    if (!compositor) {
        Leviathan::Log::WriteToLog(Leviathan::LogLevel::WARN, "PluginAPI::SwitchToTag: GetCompositorState() returned nullptr");
        return;
    }
    
    compositor->SwitchToTag(tag_index);
}

// Screen queries
std::string GetScreenName(Core::Screen* screen) {
    if (!screen) return "";
    return screen->GetName();
}

int GetScreenWidth(Core::Screen* screen) {
    if (!screen) return 0;
    return screen->GetWidth();
}

int GetScreenHeight(Core::Screen* screen) {
    if (!screen) return 0;
    return screen->GetHeight();
}

Core::Tag* GetScreenCurrentTag(Core::Screen* screen) {
    if (!screen) return nullptr;
    return screen->GetCurrentTag();
}

// Thread-local storage for current render screen context
// Internal use only - StatusBar sets this before rendering widgets
namespace {
    thread_local Core::Screen* current_render_screen_ = nullptr;
}

// Internal function used by StatusBar
void SetCurrentRenderScreen(Core::Screen* screen) {
    current_render_screen_ = screen;
}

Core::Screen* GetWidgetScreen() {
    // Return the thread-local screen context if set
    // Otherwise fallback to focused screen
    if (current_render_screen_) {
        return current_render_screen_;
    }
    
    auto* state = GetCompositorState();
    if (state) {
        return state->GetFocusedScreen();
    }
    
    return nullptr;
}

} // namespace Plugin
} // namespace UI
} // namespace Leviathan
