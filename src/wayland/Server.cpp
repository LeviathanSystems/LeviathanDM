#include "wayland/Server.hpp"
#include "wayland/Output.hpp"
#include "wayland/View.hpp"
#include "wayland/Input.hpp"
#include "wayland/LayerSurface.hpp"
#include "ui/StatusBar.hpp"
#include "ui/ModalManager.hpp"
#include "ui/KeybindingHelpModal.hpp"
#include "ui/WidgetPluginManager.hpp"
#include "ui/NotificationDaemon.hpp"
#include "ui/MenuBarManager.hpp"
#include "ui/MenuItemProviders.hpp"
#include "config/ConfigParser.hpp"
#include "Logger.hpp"
#include "wayland/WaylandTypes.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <unistd.h>  // For fork(), execlp(), setenv()
#include <sys/types.h>  // For pid_t

namespace Leviathan {
namespace Wayland {

// C wrapper functions for callbacks
static void handle_new_output(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_output);
    server->OnNewOutput(static_cast<struct wlr_output*>(data));
}

static void handle_new_xdg_surface(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_xdg_surface);
    server->OnNewXdgSurface(static_cast<struct wlr_xdg_surface*>(data));
}

static void handle_new_xdg_toplevel(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_xdg_toplevel);
    server->OnNewXdgToplevel(static_cast<struct wlr_xdg_toplevel*>(data));
}

static void handle_new_xdg_decoration(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_xdg_decoration);
    server->OnNewXdgDecoration(static_cast<struct wlr_xdg_toplevel_decoration_v1*>(data));
}

static void handle_new_layer_surface(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_layer_surface);
    LayerSurfaceManager::HandleNewLayerSurface(listener, data);
}

static void handle_new_input(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_input);
    server->OnNewInput(static_cast<struct wlr_input_device*>(data));
}

static void handle_session_active(struct wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, session_active);
    struct wlr_session* session = static_cast<struct wlr_session*>(data);
    server->OnSessionActive(session->active);
}

static void handle_cursor_motion(struct wl_listener* listener, void* data) {
    InputManager::HandleCursorMotion(listener, data);
}

static void handle_cursor_motion_absolute(struct wl_listener* listener, void* data) {
    InputManager::HandleCursorMotionAbsolute(listener, data);
}

static void handle_cursor_button(struct wl_listener* listener, void* data) {
    InputManager::HandleCursorButton(listener, data);
}

static void handle_cursor_axis(struct wl_listener* listener, void* data) {
    InputManager::HandleCursorAxis(listener, data);
}

static void handle_cursor_frame(struct wl_listener* listener, void* data) {
    InputManager::HandleCursorFrame(listener, data);
}

Server::Server()
    : wl_display(nullptr)
    , wl_event_loop(nullptr)
    , backend(nullptr)
    , session(nullptr)
    , renderer(nullptr)
    , allocator(nullptr)
    , compositor(nullptr)
    , subcompositor(nullptr)
    , data_device_manager(nullptr)
    , primary_selection_mgr(nullptr)
    , data_control_mgr(nullptr)
    , scene(nullptr)
    , scene_layout(nullptr)
    , output_layout(nullptr)
    , xdg_shell(nullptr)
    , cursor(nullptr)
    , cursor_mgr(nullptr)
    , seat(nullptr)
    , focused_view_(nullptr)
    , should_shutdown_(false) {
    
    wl_list_init(&outputs);
    wl_list_init(&keyboards);
    wl_list_init(&pointers);
    
    // Border colors will be loaded from config in Initialize()
    // Set defaults here in case config loading fails
    border_focused_[0] = 0.36f;   // R (Nord blue #5E81AC)
    border_focused_[1] = 0.50f;   // G
    border_focused_[2] = 0.67f;   // B
    border_focused_[3] = 1.0f;    // A
    
    border_unfocused_[0] = 0.23f; // R (Nord dark gray #3B4252)
    border_unfocused_[1] = 0.26f; // G
    border_unfocused_[2] = 0.32f; // B
    border_unfocused_[3] = 1.0f;  // A
    
    // Register modal types
    UI::ModalManager::RegisterModalType("keybindingshelp", []() -> std::unique_ptr<UI::Modal> {
        return std::make_unique<UI::KeybindingHelpModal>();
    });
}

Server::~Server() {
    LOG_INFO("Server destructor - cleaning up resources");
    
    // Shutdown MenuBar manager
    LOG_INFO("Shutting down MenuBar manager...");
    UI::MenuBarManager::Instance().Shutdown();
    
    // Shutdown notification daemon
    if (notification_daemon_) {
        LOG_INFO("Shutting down notification daemon...");
        notification_daemon_->Shutdown();
        notification_daemon_.reset();
    }
    
    // Clean up remaining views (in case they weren't destroyed by Wayland)
    // Note: Normally Wayland destroy callbacks handle this, but we clean up for safety
    LOG_INFO_FMT("Cleaning up {} remaining views...", views.size());
    for (auto* view : views) {
        delete view;
    }
    views.clear();
    
    // Clean up remaining clients
    LOG_INFO_FMT("Cleaning up {} remaining clients...", clients_.size());
    for (auto* client : clients_) {
        delete client;
    }
    clients_.clear();
    
    // core_seat_ is a unique_ptr, it will automatically clean up (and delete tags)
    // Outputs are cleaned up by wlroots destroy callbacks
    
    LOG_INFO("Destroying Wayland display...");
    if (wl_display) {
        wl_display_destroy(wl_display);
    }
    
    LOG_INFO("Server cleanup complete");
}

Server* Server::Create() {
    Server* server = new Server();
    if (!server->Initialize()) {
        delete server;
        return nullptr;
    }
    return server;
}

bool Server::Initialize() {
    wlr_log_init(WLR_DEBUG, nullptr);
    
    // Create Wayland display
    wl_display = wl_display_create();
    if (!wl_display) {
        std::cerr << "Failed to create Wayland display\n";
        return false;
    }
    
    wl_event_loop = wl_display_get_event_loop(wl_display);
    
    // Create backend and get session if available
    backend = wlr_backend_autocreate(wl_event_loop, &session);
    if (!backend) {
        std::cerr << "Failed to create backend\n";
        return false;
    }
    
    // Create renderer
    renderer = wlr_renderer_autocreate(backend);
    if (!renderer) {
        std::cerr << "Failed to create renderer\n";
        return false;
    }
    
    wlr_renderer_init_wl_display(renderer, wl_display);
    
    // Create allocator
    allocator = wlr_allocator_autocreate(backend, renderer);
    if (!allocator) {
        std::cerr << "Failed to create allocator\n";
        return false;
    }
    
    // Create compositor
    compositor = wlr_compositor_create(wl_display, 5, renderer);
    subcompositor = wlr_subcompositor_create(wl_display);
    data_device_manager = wlr_data_device_manager_create(wl_display);
    primary_selection_mgr = wlr_primary_selection_v1_device_manager_create(wl_display);
    data_control_mgr = wlr_data_control_manager_v1_create(wl_display);
    
    LOG_INFO("Clipboard and primary selection support enabled");
    
    // Create output layout
    output_layout = wlr_output_layout_create(wl_display);
    
    // Create scene
    scene = wlr_scene_create();
    scene_layout = wlr_scene_attach_output_layout(scene, output_layout);
    
    // Note: LayerManager is now created per-output in OnNewOutput()
    // Window layer will be from the output's LayerManager
    window_layer = nullptr;  // Will be set per-output
    
    // Setup output listener
    new_output.notify = handle_new_output;
    wl_signal_add(&backend->events.new_output, &new_output);
    
    // Create XDG shell
    xdg_shell = wlr_xdg_shell_create(wl_display, 3);
    
    // Create XDG decoration manager (for server-side decorations)
    xdg_decoration_mgr = wlr_xdg_decoration_manager_v1_create(wl_display);
    new_xdg_decoration.notify = handle_new_xdg_decoration;
    wl_signal_add(&xdg_decoration_mgr->events.new_toplevel_decoration, &new_xdg_decoration);
    
    // Create Layer Shell (for bars, notifications, overlays)
    layer_shell = wlr_layer_shell_v1_create(wl_display, 4);
    wl_list_init(&layer_surfaces);
    new_layer_surface.notify = handle_new_layer_surface;
    wl_signal_add(&layer_shell->events.new_surface, &new_layer_surface);
    LOG_INFO("Layer shell v1 initialized");
    
    // NOTE: Don't listen to new_surface - tinywl doesn't do this
    // Only listen to new_toplevel and new_popup
    // new_xdg_surface.notify = handle_new_xdg_surface;
    // wl_signal_add(&xdg_shell->events.new_surface, &new_xdg_surface);
    
    // Listen for new toplevels (this fires when client actually creates a toplevel window)
    new_xdg_toplevel.notify = handle_new_xdg_toplevel;
    wl_signal_add(&xdg_shell->events.new_toplevel, &new_xdg_toplevel);
    
    // Create cursor for input tracking
    cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(cursor, output_layout);
    
    // Create cursor manager with a default theme
    cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    wlr_xcursor_manager_load(cursor_mgr, 1);
    
    // Setup cursor event listeners
    cursor_motion.notify = handle_cursor_motion;
    wl_signal_add(&cursor->events.motion, &cursor_motion);
    
    cursor_motion_absolute.notify = handle_cursor_motion_absolute;
    wl_signal_add(&cursor->events.motion_absolute, &cursor_motion_absolute);
    
    cursor_button.notify = handle_cursor_button;
    wl_signal_add(&cursor->events.button, &cursor_button);
    
    cursor_axis.notify = handle_cursor_axis;
    wl_signal_add(&cursor->events.axis, &cursor_axis);
    
    cursor_frame.notify = handle_cursor_frame;
    wl_signal_add(&cursor->events.frame, &cursor_frame);
    
    // Create seat
    seat = wlr_seat_create(wl_display, "seat0");
    
    // Setup input listener
    new_input.notify = handle_new_input;
    wl_signal_add(&backend->events.new_input, &new_input);
    
    // Setup session listener for VT switching (if running in TTY)
    if (session) {
        LOG_INFO("Session detected - VT switching will be available");
        session_active.notify = handle_session_active;
        wl_signal_add(&session->events.active, &session_active);
    } else {
        LOG_INFO("No session (probably nested) - VT switching unavailable");
    }
    
    // Border colors are now handled by window decoration system
    
    layout_engine_ = std::make_unique<TilingLayout>();
    keybindings_ = std::make_unique<KeyBindings>(this);
    KeyBindings::SetInstance(keybindings_.get());
    
    // Create core seat
    core_seat_ = std::make_unique<Core::Seat>();
    
    // NOTE: Tags are now created per-output by LayerManager in HandleNewOutput()
    // Each screen will have its own independent set of tags (AwesomeWM model)
    
    // Initialize IPC server
    ipc_server_ = std::make_unique<IPC::Server>();
    if (!ipc_server_->Initialize()) {
        LOG_WARN("Failed to initialize IPC server - leviathanctl will not work");
    } else {
        // Set command processor to delegate to this server
        ipc_server_->SetCommandProcessor([this](const std::string& cmd) {
            return this->ProcessIPCCommand(cmd);
        });
    }
    
    // Register this server as the global compositor state for widgets
    UI::SetCompositorState(this);
    
    // Initialize notification daemon
    notification_daemon_ = std::make_unique<UI::NotificationDaemon>(this);
    if (!notification_daemon_->Initialize()) {
        LOG_WARN("Failed to initialize notification daemon");
        notification_daemon_.reset();
    }
    
    // Initialize MenuBar Manager
    UI::MenuBarManager::Instance().Initialize(wl_event_loop);
    LOG_INFO("MenuBarManager initialized");
    
    // Add Desktop Application provider to MenuBar
    auto desktop_app_provider = std::make_shared<UI::DesktopApplicationProvider>();
    UI::MenuBarManager::Instance().AddProvider(desktop_app_provider);
    LOG_INFO("Desktop Application provider added to MenuBar");
    
    std::cout << "Compositor initialized successfully\n";
    return true;
}

void Server::Run() {
    // Add socket for clients to connect
    const char* socket = wl_display_add_socket_auto(wl_display);
    if (!socket) {
        LOG_ERROR("Failed to add socket");
        return;
    }
    
    // Save socket name for child processes
    wayland_socket_name_ = socket;
    
    // Start backend
    if (!wlr_backend_start(backend)) {
        LOG_ERROR("Failed to start backend");
        return;
    }
    
    // If this is a wayland backend, create an output (window) manually
    if (wlr_backend_is_wl(backend)) {
        struct wlr_output* wl_output = wlr_wl_output_create(backend);
        if (wl_output) {
            wlr_wl_output_set_title(wl_output, "LeviathanDM");
            LOG_INFO("Created Wayland output window");
        }
    }
    
    // Note: cursor theme loading is deferred to OnNewOutput() when renderer is ready
    
    setenv("WAYLAND_DISPLAY", socket, true);
    LOG_INFO_FMT("Running compositor on WAYLAND_DISPLAY={}", socket);
    
    // Launch default terminal
    LaunchDefaultTerminal();
    
    // Run event loop with IPC handling
    while (wl_display_get_destroy_listener(wl_display, nullptr) == nullptr) {
        // Check for IPC-initiated shutdown
        if (should_shutdown_) {
            LOG_INFO("IPC shutdown requested - initiating graceful shutdown");
            Shutdown();
            break;
        }
        
        // Handle IPC events
        if (ipc_server_) {
            ipc_server_->HandleEvents();
        }
        
        // Update notification daemon (process expired notifications)
        if (notification_daemon_) {
            notification_daemon_->Update();
        }
        
        // Process Wayland events
        wl_display_flush_clients(wl_display);
        wl_event_loop_dispatch(wl_event_loop, 1);  // 1ms timeout
    }
}

void Server::Shutdown() {
    LOG_INFO("========================================");
    LOG_INFO("Starting graceful compositor shutdown...");
    LOG_INFO("========================================");
    
    // Step 1: Hide all MenuBars
    LOG_INFO("Step 1: Hiding menu bars...");
    UI::MenuBarManager::Instance().Shutdown();
    
    // Step 2: Close all client windows gracefully
    LOG_INFO_FMT("Step 2: Closing {} client windows...", clients_.size());
    for (auto* client : clients_) {
        if (client && client->GetView()) {
            LOG_INFO_FMT("  - Closing client: {}", client->GetAppId());
            CloseView(client->GetView());
        }
    }
    
    // Step 3: Shutdown notification daemon
    if (notification_daemon_) {
        LOG_INFO("Step 3: Shutting down notification daemon...");
        notification_daemon_->Shutdown();
    }
    
    // Step 4: Shutdown IPC server
    if (ipc_server_) {
        LOG_INFO("Step 4: Shutting down IPC server...");
        ipc_server_.reset();
    }
    
    // Step 5: Clean up status bars via LayerManagers
    LOG_INFO("Step 5: Cleaning up status bars...");
    Output* output;
    wl_list_for_each(output, &outputs, link) {
        if (output->layer_manager) {
            output->layer_manager->ClearAllStatusBars();
            LOG_INFO_FMT("  - Cleared status bars on output '{}'", output->wlr_output->name);
        }
    }
    
    // Step 6: Stop the Wayland display event loop
    LOG_INFO("Step 6: Stopping Wayland display...");
    wl_display_terminate(wl_display);
    
    LOG_INFO("========================================");
    LOG_INFO("Graceful shutdown sequence complete");
    LOG_INFO("========================================");
    
    // Exit the process cleanly
    exit(0);
}

void Server::OnNewOutput(struct wlr_output* wlr_output) {
    LOG_INFO_FMT("New output: {}", wlr_output->name);
    
    // CRITICAL: Initialize output rendering with allocator and renderer
    // This must be done before creating the scene output or committing
    if (!wlr_output_init_render(wlr_output, allocator, renderer)) {
        LOG_ERROR_FMT("Failed to initialize rendering for output '{}'", wlr_output->name);
        return;
    }
    
    // Configure and enable output using state API
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_state_set_mode(&state, mode);
    }
    
    // Commit the output configuration
    if (!wlr_output_commit_state(wlr_output, &state)) {
        LOG_ERROR_FMT("Failed to commit output '{}'", wlr_output->name);
        wlr_output_state_finish(&state);
        return;
    }
    wlr_output_state_finish(&state);
    
    // Create output wrapper
    Output* output = new Output(wlr_output, this);
    
    // CRITICAL: Add output to the server's outputs list
    wl_list_insert(&outputs, &output->link);
    LOG_DEBUG("Added Output to outputs list");
    
    // Add to layout
    struct wlr_output_layout_output* layout_output = wlr_output_layout_add_auto(output_layout, wlr_output);
    
    // Create scene output - this is the proper wlroots 0.19 way
    output->scene_output = wlr_scene_output_create(scene, wlr_output);
    if (!output->scene_output) {
        LOG_ERROR_FMT("Failed to create scene output for '{}'", wlr_output->name);
        delete output;
        return;
    }
    
    LOG_INFO_FMT("Created scene output for '{}' at {:p}", wlr_output->name, 
             static_cast<void*>(output->scene_output));
    
    // Create Core::Screen object for this output
    Core::Screen* screen = new Core::Screen(wlr_output);
    output->core_screen = screen;  // Store reference in Output for cleanup
    
    if (core_seat_) {
        core_seat_->AddScreen(screen);
        LOG_INFO_FMT("Added screen '{}' to core seat", screen->GetName());
    }
    
    // Create per-output LayerManager
    output->layer_manager = new LayerManager(scene, wlr_output, wl_event_loop);
    LOG_INFO_FMT("Created LayerManager for output '{}'", wlr_output->name);
    
    // Set server pointer so LayerManager can access global state (like keybindings)
    output->layer_manager->SetServer(this);
    
    // Initialize tags for this output/screen
    auto& config = Config();
    if (!config.general.tags.empty()) {
        output->layer_manager->InitializeTags(config.general.tags);
    } else {
        // Create default numbered tags
        std::vector<TagConfig> default_tags;
        for (int i = 0; i < config.general.workspace_count; ++i) {
            TagConfig tag;
            tag.id = i;
            tag.name = std::to_string(i + 1);
            default_tags.push_back(tag);
        }
        output->layer_manager->InitializeTags(default_tags);
    }
    
    // CRITICAL: Connect the output to the scene layout so the scene knows where to render
    wlr_scene_output_layout_add_output(scene_layout, layout_output, output->scene_output);
    LOG_INFO_FMT("Connected output '{}' to scene layout", wlr_output->name);
    
    // Register frame listener
    output->frame.notify = OutputManager::HandleFrame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);
    
    LOG_INFO_FMT("Registered frame listener for output '{}'", wlr_output->name);
    
    // Register destroy listener
    output->destroy.notify = OutputManager::HandleDestroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);
    
    LOG_INFO_FMT("Output '{}' fully configured and enabled", wlr_output->name);
    
    // Apply monitor group configuration
    ApplyMonitorGroupConfiguration();
    
    // Set cursor image for this output
    if (cursor_mgr) {
        wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");
    }
}

void Server::ApplyMonitorGroupConfiguration() {
    auto& config = Config();
    
    // Build list of currently connected output names and their EDID info
    std::vector<std::string> connected_names;
    std::map<std::string, Output*> output_map;
    
    Output* iter;
    wl_list_for_each(iter, &outputs, link) {
        if (!iter->wlr_output) continue;
        
        std::string name = iter->wlr_output->name;
        connected_names.push_back(name);
        output_map[name] = iter;
    }
    
    if (connected_names.empty()) {
        LOG_WARN("No outputs connected, skipping monitor group configuration");
        return;
    }
    
    // Find matching monitor group
    const MonitorGroup* group = config.monitor_groups.FindMatchingGroup(connected_names);
    
    if (!group) {
        LOG_WARN("No matching monitor group found, using auto-configuration");
        return;
    }
    
    LOG_INFO_FMT("Applying monitor group: '{}'", group->name);
    
    // Apply configuration for each monitor in the group
    for (const auto& mon_config : group->monitors) {
        // Find the matching output
        Output* output = nullptr;
        
        for (auto& [name, out] : output_map) {
            // For now, just match by name
            // TODO: Also match by EDID description and make/model
            if (mon_config.identifier == name) {
                output = out;
                break;
            }
            
            // Check description prefix match
            if (mon_config.identifier.size() > 2 && mon_config.identifier.substr(0, 2) == "d:") {
                if (out->core_screen) {
                    std::string search = mon_config.identifier.substr(2);
                    std::string desc = out->core_screen->GetDescription();
                    if (desc.find(search) != std::string::npos) {
                        output = out;
                        break;
                    }
                }
            }
            
            // Check make/model match
            if (mon_config.identifier.size() > 2 && mon_config.identifier.substr(0, 2) == "m:") {
                if (out->core_screen) {
                    std::string search = mon_config.identifier.substr(2);
                    std::string make_model = out->core_screen->GetMake() + "/" + out->core_screen->GetModel();
                    if (make_model.find(search) != std::string::npos) {
                        output = out;
                        break;
                    }
                }
            }
        }
        
        if (!output) {
            LOG_WARN_FMT("Monitor '{}' from group '{}' not found in connected outputs", 
                     mon_config.identifier, group->name);
            continue;
        }
        
        LOG_INFO_FMT("Configuring output '{}' from monitor group", output->wlr_output->name);
        
        // Create output state for applying configuration
        struct wlr_output_state state;
        wlr_output_state_init(&state);
        
        // Apply mode if specified
        if (mon_config.mode.has_value()) {
            const std::string& mode_str = mon_config.mode.value();
            
            // Parse mode string (e.g., "1920x1080" or "1920x1080@60")
            int width = 0, height = 0, refresh = 0;
            size_t x_pos = mode_str.find('x');
            size_t at_pos = mode_str.find('@');
            
            if (x_pos != std::string::npos) {
                try {
                    width = std::stoi(mode_str.substr(0, x_pos));
                    
                    if (at_pos != std::string::npos) {
                        height = std::stoi(mode_str.substr(x_pos + 1, at_pos - x_pos - 1));
                        refresh = std::stoi(mode_str.substr(at_pos + 1));
                    } else {
                        height = std::stoi(mode_str.substr(x_pos + 1));
                    }
                    
                    // Find and set the mode
                    struct wlr_output_mode* mode = nullptr;
                    struct wlr_output_mode* best_mode = nullptr;
                    
                    wl_list_for_each(mode, &output->wlr_output->modes, link) {
                        if (mode->width == width && mode->height == height) {
                            if (refresh > 0) {
                                // Match exact refresh rate (wlroots uses millihertz)
                                if (mode->refresh == refresh * 1000) {
                                    best_mode = mode;
                                    break;
                                }
                            } else {
                                // No refresh specified, pick highest
                                if (!best_mode || mode->refresh > best_mode->refresh) {
                                    best_mode = mode;
                                }
                            }
                        }
                    }
                    
                    if (best_mode) {
                        wlr_output_state_set_mode(&state, best_mode);
                        LOG_INFO_FMT("Set mode {}x{}@{}Hz for '{}'", 
                                 width, height, best_mode->refresh / 1000, 
                                 output->wlr_output->name);
                    } else {
                        LOG_WARN_FMT("Mode {} not available for '{}'", mode_str, output->wlr_output->name);
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR_FMT("Failed to parse mode '{}': {}", mode_str, e.what());
                }
            }
        }
        
        // Apply scale if specified
        if (mon_config.scale.has_value()) {
            wlr_output_state_set_scale(&state, mon_config.scale.value());
            LOG_INFO_FMT("Set scale {} for '{}'", mon_config.scale.value(), output->wlr_output->name);
        }
        
        // Apply transform if specified
        if (mon_config.transform.has_value()) {
            enum wl_output_transform transform;
            switch (mon_config.transform.value()) {
                case 90:  transform = WL_OUTPUT_TRANSFORM_90; break;
                case 180: transform = WL_OUTPUT_TRANSFORM_180; break;
                case 270: transform = WL_OUTPUT_TRANSFORM_270; break;
                default:  transform = WL_OUTPUT_TRANSFORM_NORMAL; break;
            }
            wlr_output_state_set_transform(&state, transform);
            LOG_INFO_FMT("Set transform {} for '{}'", mon_config.transform.value(), output->wlr_output->name);
        }
        
        // Commit the output configuration
        if (!wlr_output_commit_state(output->wlr_output, &state)) {
            LOG_ERROR_FMT("Failed to commit configuration for '{}'", output->wlr_output->name);
        }
        
        // Cleanup output state
        wlr_output_state_finish(&state);
        
        // Apply position if specified
        if (mon_config.position.has_value()) {
            auto [x, y] = mon_config.position.value();
            wlr_output_layout_add(output_layout, output->wlr_output, x, y);
            LOG_INFO_FMT("Set position {}x{} for '{}'", x, y, output->wlr_output->name);
        }
        
        // Create status bars for this monitor (handled by LayerManager)
        output->layer_manager->CreateStatusBars(
            mon_config.status_bars,
            config.status_bars,
            output->wlr_output->width,
            output->wlr_output->height
        );
        
        // Register menubar for this output
        UI::MenuBarManager::Instance().RegisterMenuBar(
            output->wlr_output,
            output->layer_manager,
            output->wlr_output->width,
            output->wlr_output->height
        );
        
        // Pass monitor config to LayerManager (triggers wallpaper initialization if configured)
        output->layer_manager->SetMonitorConfig(mon_config);
    }
    
    LOG_INFO_FMT("Monitor group '{}' configuration applied", group->name);
}

void Server::OnNewXdgSurface(struct wlr_xdg_surface* xdg_surface) {
    LOG_DEBUG_FMT("OnNewXdgSurface called, role={}, toplevel={}", 
              static_cast<int>(xdg_surface->role),
              static_cast<void*>(xdg_surface->toplevel));
    
    // Check if this is a toplevel by verifying the toplevel pointer exists
    // The role might still be NONE at this point, but toplevel will be set
    if (xdg_surface->toplevel == nullptr) {
        LOG_DEBUG("Not a toplevel (toplevel pointer is NULL), checking if popup...");
        if (xdg_surface->popup == nullptr) {
            LOG_WARN("xdg_surface has neither toplevel nor popup - invalid state");
        } else {
            LOG_DEBUG("This is a popup, ignoring");
        }
        return;
    }
    
    LOG_INFO("Creating view for XDG toplevel (toplevel pointer exists)!");
    
    // Create view
    View* view = new View(xdg_surface->toplevel, this);
    views.push_back(view);
    
    // Find first output's WorkingArea layer
    struct wlr_scene_tree* parent_layer = &scene->tree;  // Fallback to root
    if (!wl_list_empty(&outputs)) {
        Output* first_output = wl_container_of(outputs.next, first_output, link);
        if (first_output && first_output->layer_manager) {
            parent_layer = first_output->layer_manager->GetLayer(Layer::WorkingArea);
        }
    }
    
    // Add to scene graph working area layer (where windows go)
    view->scene_tree = wlr_scene_xdg_surface_create(
        parent_layer, xdg_surface);
    
    LOG_DEBUG("Created scene tree for toplevel view");
    
    // Create client wrapper
    auto* client = new Core::Client(view);
    clients_.push_back(client);
    
    // Add to core seat
    core_seat_->AddClient(client);
    
    // Add to the focused screen's active tag
    auto* focused_screen = core_seat_->GetFocusedScreen();
    if (focused_screen) {
        Output* iter;
        wl_list_for_each(iter, &outputs, link) {
            if (iter->core_screen == focused_screen && iter->layer_manager) {
                auto* tag = iter->layer_manager->GetCurrentTag();
                if (tag) {
                    tag->AddClient(client);
                    LOG_DEBUG("Added client to current tag");
                }
                break;
            }
        }
    }
    
    LOG_INFO("View fully initialized");
}

void Server::OnNewXdgToplevel(struct wlr_xdg_toplevel* toplevel) {
    LOG_INFO("OnNewXdgToplevel called - this is the proper signal!");
    LOG_DEBUG_FMT("Toplevel pointer: {}, base surface: {}", 
              static_cast<void*>(toplevel),
              static_cast<void*>(toplevel->base));
    
    // Create view
    View* view = new View(toplevel, this);
    views.push_back(view);
    
    // Find first output's WorkingArea layer
    struct wlr_scene_tree* parent_layer = &scene->tree;  // Fallback to root
    if (!wl_list_empty(&outputs)) {
        Output* first_output = wl_container_of(outputs.next, first_output, link);
        if (first_output && first_output->layer_manager) {
            parent_layer = first_output->layer_manager->GetLayer(Layer::WorkingArea);
            LOG_DEBUG("Adding window to WorkingArea layer");
        }
    }
    
    // Add to scene graph working area layer (where windows go)
    // wlr_scene_xdg_surface_create automatically handles configure events
    view->scene_tree = wlr_scene_xdg_surface_create(
        parent_layer, toplevel->base);
    
    // CRITICAL: Link the xdg_surface to the scene_tree
    // This is required for wlroots scene helpers to work properly
    view->scene_tree->node.data = view;
    toplevel->base->data = view->scene_tree;
    
    LOG_DEBUG("Created scene tree for toplevel view");
    LOG_DEBUG_FMT("Surface initialized: {}", toplevel->base->initialized);
    
    // NOTE: wlr_scene_xdg_surface_create() handles configure automatically
    // Do NOT manually call configure functions here
    
    // Create client wrapper
    auto* client = new Core::Client(view);
    clients_.push_back(client);
    
    // Add to core seat
    core_seat_->AddClient(client);
    
    // Add to the focused screen's active tag
    auto* focused_screen = core_seat_->GetFocusedScreen();
    if (focused_screen) {
        Output* iter;
        wl_list_for_each(iter, &outputs, link) {
            if (iter->core_screen == focused_screen && iter->layer_manager) {
                auto* tag = iter->layer_manager->GetCurrentTag();
                if (tag) {
                    tag->AddClient(client);
                    LOG_DEBUG("Added client to current tag");
                }
                break;
            }
        }
    }
    
    LOG_INFO("Toplevel view fully initialized and added to active tag");
}

void Server::OnNewInput(struct wlr_input_device* device) {
    LOG_INFO_FMT("New input device: {}", device->name);
    InputManager::HandleNewInput(this, device);
}

void Server::LaunchDefaultTerminal() {
    pid_t pid = fork();
    
    if (pid < 0) {
        LOG_ERROR("Failed to fork process for terminal");
        return;
    }
    
    if (pid == 0) {
        // Child process
        // Set environment variable so terminal knows which display to use
        setenv("WAYLAND_DISPLAY", wayland_socket_name_.c_str(), 1);
        // Execute terminal - try kitty, then fallback
        execlp("kitty", "kitty", nullptr);

        // If we get here, exec failed
        _exit(EXIT_FAILURE);
    }
    
    // Parent process
    LOG_INFO_FMT("Launched alacritty terminal (PID: {})", pid);
}

void Server::OnSessionActive(bool active) {
    if (active) {
        LOG_INFO("Session became active - compositor regained VT");
        // When we switch back to this VT, outputs should automatically resume
    } else {
        LOG_INFO("Session became inactive - VT switched away");
        // When VT switches away, wlroots automatically pauses output
    }
}

void Server::FocusView(View* view) {
    if (!view || !view->mapped) {
        return;
    }
    
    // Update border colors - unfocus previous view
    if (focused_view_ && focused_view_ != view) {
        focused_view_->UpdateBorderColor(border_unfocused_);
    }
    
    focused_view_ = view;
    
    // Update border color for newly focused view
    view->UpdateBorderColor(border_focused_);
    
    // Raise view in scene graph
    wlr_scene_node_raise_to_top(&view->scene_tree->node);
    
    // Set keyboard focus
    if (view->surface) {
        // Get the keyboard from the seat (if one exists)
        struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
        if (keyboard) {
            // Set this keyboard as the active one for the seat
            wlr_seat_set_keyboard(seat, keyboard);
        }
        
        // Notify the surface it has keyboard focus
        wlr_seat_keyboard_notify_enter(seat, view->surface,
            keyboard ? keyboard->keycodes : nullptr,
            keyboard ? keyboard->num_keycodes : 0,
            keyboard ? &keyboard->modifiers : nullptr);
    }
}

void Server::CloseView(View* view) {
    if (!view) {
        view = focused_view_;
    }
    
    if (!view) {
        return;
    }
    
    wlr_xdg_toplevel_send_close(view->xdg_toplevel);
}

void Server::RemoveView(View* view) {
    if (!view) {
        return;
    }
    
    LOG_DEBUG_FMT("RemoveView: Cleaning up view {}", static_cast<void*>(view));
    
    // Remove from views list
    auto it = std::find(views.begin(), views.end(), view);
    if (it != views.end()) {
        views.erase(it);
        LOG_DEBUG_FMT("Removed view from server's view list, {} views remaining", views.size());
    }
    
    // Clear focus if this view was focused
    if (focused_view_ == view) {
        focused_view_ = nullptr;
        LOG_DEBUG("Cleared focused_view since it was the destroyed view");
    }
    
    // Find and remove the associated Client
    Core::Client* client_to_remove = nullptr;
    for (auto* client : clients_) {
        if (client->GetView() == view) {
            client_to_remove = client;
            break;
        }
    }
    
    if (client_to_remove) {
        LOG_DEBUG("Found client for view, removing from seat and deleting");
        
        // Remove from tag first
        auto* focused_screen = core_seat_->GetFocusedScreen();
        if (focused_screen) {
            Output* iter;
            wl_list_for_each(iter, &outputs, link) {
                if (iter->core_screen == focused_screen && iter->layer_manager) {
                    // Try to find the client in any tag
                    for (auto* tag : iter->layer_manager->GetTags()) {
                        const auto& tag_clients = tag->GetClients();
                        if (std::find(tag_clients.begin(), tag_clients.end(), client_to_remove) != tag_clients.end()) {
                            tag->RemoveClient(client_to_remove);
                            LOG_DEBUG("Removed client from tag");
                            break;
                        }
                    }
                    break;
                }
            }
        }
        
        core_seat_->RemoveClient(client_to_remove);
        
        // Remove from clients_ vector
        auto client_it = std::find(clients_.begin(), clients_.end(), client_to_remove);
        if (client_it != clients_.end()) {
            clients_.erase(client_it);
        }
        
        // Delete the client to prevent memory leak
        delete client_to_remove;
    }
}

void Server::FocusNext() {
    core_seat_->FocusNextClient();
    auto* client = core_seat_->GetFocusedClient();
    if (client) {
        FocusView(client->GetView());
    }
}

void Server::FocusPrev() {
    core_seat_->FocusPrevClient();
    auto* client = core_seat_->GetFocusedClient();
    if (client) {
        FocusView(client->GetView());
    }
}

void Server::SwapWithNext() {
    // TODO: Update to work with per-screen tags from LayerManager
    LOG_WARN("SwapWithNext temporarily disabled during per-screen tag refactoring");
}

void Server::SwapWithPrev() {
    // TODO: Update to work with per-screen tags from LayerManager
    LOG_WARN("SwapWithPrev temporarily disabled during per-screen tag refactoring");
}

View* Server::FindView(struct wlr_surface* surface) {
    for (auto* view : views) {
        if (view->surface == surface) {
            return view;
        }
    }
    return nullptr;
}

IPC::Response Server::ProcessIPCCommand(const std::string& command_json) {
    IPC::Response response;
    response.success = false;
    
    try {
        nlohmann::json j = nlohmann::json::parse(command_json);
        
        if (!j.contains("command")) {
            response.error = "Missing 'command' field";
            return response;
        }
        
        std::string cmd = j["command"];
        IPC::CommandType type = IPC::StringToCommandType(cmd);
        
        switch (type) {
            case IPC::CommandType::PING:
                response.success = true;
                response.data["pong"] = "pong";
                break;
                
            case IPC::CommandType::GET_VERSION:
                response.success = true;
                response.data["version"] = "0.1.0";
                response.data["compositor"] = "LeviathanDM";
                break;
                
            case IPC::CommandType::GET_TAGS: {
                response.success = true;
                auto tags = GetTags();  // Use Server's method which gets from focused screen
                auto active_tag = GetActiveTag();
                
                for (const auto* tag : tags) {
                    IPC::TagInfo info;
                    info.name = tag->GetName();
                    info.visible = (tag == active_tag);
                    info.client_count = tag->GetClients().size();
                    response.tags.push_back(info);
                }
                break;
            }
            
            case IPC::CommandType::GET_ACTIVE_TAG: {
                response.success = true;
                auto active_tag = GetActiveTag();  // Use Server's method
                if (active_tag) {
                    response.data["tag"] = active_tag->GetName();
                    response.data["client_count"] = std::to_string(active_tag->GetClients().size());
                } else {
                    response.data["tag"] = "none";
                    response.data["client_count"] = "0";
                }
                break;
            }
            
            case IPC::CommandType::GET_CLIENTS: {
                response.success = true;
                
                // Iterate through all tags to find which tag each client belongs to
                auto tags = GetTags();  // Use Server's method
                for (const auto* tag : tags) {
                    for (const auto* client : tag->GetClients()) {
                        IPC::ClientInfo info;
                        info.title = client->GetTitle();
                        info.app_id = client->GetAppId();
                        
                        // Get geometry from view
                        auto* view = client->GetView();
                        if (view && view->scene_tree) {
                            info.x = view->scene_tree->node.x;
                            info.y = view->scene_tree->node.y;
                            
                            if (view->xdg_toplevel) {
                                info.width = view->xdg_toplevel->base->current.geometry.width;
                                info.height = view->xdg_toplevel->base->current.geometry.height;
                            } else {
                                info.width = 0;
                                info.height = 0;
                            }
                        } else {
                            info.x = info.y = info.width = info.height = 0;
                        }
                        
                        info.floating = client->IsFloating();
                        info.fullscreen = client->IsFullscreen();
                        info.tag = tag->GetName();
                        
                        response.clients.push_back(info);
                    }
                }
                break;
            }
            
            case IPC::CommandType::GET_OUTPUTS: {
                response.success = true;
                
                // Get Screen objects which have parsed EDID info
                auto screens = GetScreens();
                for (const auto* screen : screens) {
                    IPC::OutputInfo info;
                    info.name = screen->GetName();
                    info.description = screen->GetDescription();
                    info.make = screen->GetMake();
                    info.model = screen->GetModel();
                    info.serial = screen->GetSerial();
                    info.width = screen->GetWidth();
                    info.height = screen->GetHeight();
                    info.scale = screen->GetScale();
                    
                    // Get physical dimensions and refresh from wlr_output
                    auto* wlr_output = screen->GetWlrOutput();
                    if (wlr_output) {
                        info.refresh_mhz = wlr_output->refresh;
                        info.phys_width_mm = wlr_output->phys_width;
                        info.phys_height_mm = wlr_output->phys_height;
                        info.enabled = wlr_output->enabled;
                    } else {
                        info.refresh_mhz = 0;
                        info.phys_width_mm = 0;
                        info.phys_height_mm = 0;
                        info.enabled = false;
                    }
                    
                    response.outputs.push_back(info);
                }
                break;
            }
            
            case IPC::CommandType::GET_LAYOUT: {
                response.success = true;
                // For now, just report that we have a tiling layout
                // TODO: Add layout mode tracking to TilingLayout class
                response.data["layout"] = "tile";
                response.data["compositor"] = "LeviathanDM";
                break;
            }
            
            case IPC::CommandType::GET_PLUGIN_STATS: {
                response.success = true;
                
                auto& plugin_manager = UI::WidgetPluginManager::Instance();
                auto loaded_plugins = plugin_manager.GetLoadedPlugins();
                LOG_DEBUG_FMT("IPC: GET_PLUGIN_STATS - {} plugins loaded", loaded_plugins.size());
                
                auto all_stats = plugin_manager.GetAllPluginMemoryStats();
                LOG_DEBUG_FMT("IPC: Retrieved {} plugin stats", all_stats.size());
                
                for (const auto& [name, stats] : all_stats) {
                    IPC::PluginStats info;
                    info.name = name;
                    info.rss_bytes = stats.rss_bytes;
                    info.virtual_bytes = stats.virtual_bytes;
                    info.instance_count = stats.instance_count;
                    LOG_DEBUG_FMT("IPC: Plugin '{}' - RSS: {} bytes, Instances: {}", 
                             name, stats.rss_bytes, stats.instance_count);
                    response.plugin_stats.push_back(info);
                }
                LOG_DEBUG_FMT("IPC: Returning {} plugin stats in response", response.plugin_stats.size());
                break;
            }
            
            case IPC::CommandType::SET_ACTIVE_TAG: {
                if (!j.contains("args") || !j["args"].contains("tag")) {
                    response.error = "Missing 'tag' argument";
                    break;
                }
                
                std::string tag_name = j["args"]["tag"];
                auto tags = GetTags();  // Use Server's method
                
                bool found = false;
                for (size_t i = 0; i < tags.size(); i++) {
                    if (tags[i]->GetName() == tag_name) {
                        SwitchToTag(i);
                        response.success = true;
                        response.data["switched_to"] = tag_name;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    response.error = "Tag '" + tag_name + "' not found";
                }
                break;
            }
            
            case IPC::CommandType::EXECUTE_ACTION: {
                if (!j.contains("args") || !j["args"].contains("action")) {
                    response.error = "Missing 'action' argument";
                    break;
                }
                
                std::string action_name = j["args"]["action"];
                LOG_INFO_FMT("IPC: Executing action '{}'", action_name);
                
                // Execute the action using the ActionRegistry from keybindings
                if (keybindings_ && keybindings_->GetActionRegistry()) {
                    if (keybindings_->GetActionRegistry()->HasAction(action_name)) {
                        keybindings_->GetActionRegistry()->ExecuteAction(action_name);
                        response.success = true;
                        response.data["action"] = action_name;
                        response.data["status"] = "executed";
                    } else {
                        response.error = "Action '" + action_name + "' not found";
                        LOG_WARN_FMT("IPC: Unknown action '{}'", action_name);
                    }
                } else {
                    response.error = "ActionRegistry not initialized";
                    LOG_ERROR("IPC: ActionRegistry is null");
                }
                break;
            }
            
            case IPC::CommandType::SHUTDOWN: {
                // Security check: Only allow shutdown from same UID as compositor
                uid_t compositor_uid = getuid();
                int client_uid = ipc_server_->GetCurrentClientUid();
                
                if (client_uid < 0) {
                    response.error = "Failed to verify client credentials";
                    LOG_WARN("IPC shutdown attempt with invalid client UID");
                    break;
                }
                
                if (static_cast<uid_t>(client_uid) != compositor_uid) {
                    response.error = "Permission denied: shutdown requires matching UID";
                    LOG_WARN_FMT("IPC shutdown denied: client UID {} != compositor UID {}", 
                                client_uid, compositor_uid);
                    break;
                }
                
                // Authorization successful
                LOG_INFO_FMT("IPC shutdown authorized for UID {}", client_uid);
                response.success = true;
                response.data["message"] = "Initiating graceful shutdown";
                
                // Note: We'll send the response before shutting down
                // The shutdown will be triggered after this response is sent
                // by setting a flag that the event loop will check
                should_shutdown_ = true;
                break;
            }
            
            default:
                response.error = "Command not implemented: " + cmd;
                break;
        }
        
    } catch (const nlohmann::json::exception& e) {
        response.error = std::string("JSON parse error: ") + e.what();
    }
    
    return response;
}

void Server::OnNewXdgDecoration(struct wlr_xdg_toplevel_decoration_v1* decoration) {
    LOG_INFO_FMT("New XDG decoration request for toplevel={}", 
             static_cast<void*>(decoration->toplevel));
    
    // Find the view for this toplevel
    View* view = nullptr;
    for (auto* v : views) {
        if (v->xdg_toplevel == decoration->toplevel) {
            view = v;
            break;
        }
    }
    
    if (view) {
        view->decoration = decoration;
        LOG_INFO_FMT("Associated decoration with view={}, will set mode after surface init", 
                 static_cast<void*>(view));
    } else {
        LOG_WARN("Could not find view for decoration");
    }
}

// CompositorState interface implementation
std::vector<Core::Screen*> Server::GetScreens() const {
    if (!core_seat_) {
        return {};
    }
    return core_seat_->GetScreens();
}

Output* Server::FindOutput(struct wlr_output* wlr_output) {
    Output* iter;
    wl_list_for_each(iter, &outputs, link) {
        if (iter->wlr_output == wlr_output) {
            return iter;
        }
    }
    return nullptr;
}

bool Server::CheckStatusBarHover(int x, int y) {
    Output* output = nullptr;
    wl_list_for_each(output, &outputs, link) {
        if (output->layer_manager) {
            const auto& status_bars = output->layer_manager->GetStatusBars();
            for (auto* bar : status_bars) {
                if (bar->HandleHover(x, y)) {
                    return true;  // Hover handled by a status bar
                }
            }
        }
    }
    return false;  // Hover not over any status bar
}

bool Server::CheckStatusBarClick(int x, int y) {
    Output* output = nullptr;
    wl_list_for_each(output, &outputs, link) {
        if (output->layer_manager) {
            const auto& status_bars = output->layer_manager->GetStatusBars();
            for (auto* bar : status_bars) {
                if (bar->HandleClick(x, y)) {
                    return true;  // Click handled by a status bar
                }
            }
        }
    }
    return false;  // Click not on any status bar
}

bool Server::CheckModalScroll(int x, int y, double delta_x, double delta_y) {
    Output* output = nullptr;
    wl_list_for_each(output, &outputs, link) {
        if (output->layer_manager) {
            if (output->layer_manager->HasVisibleModal()) {
                if (output->layer_manager->HandleModalScroll(x, y, delta_x, delta_y)) {
                    return true;  // Scroll handled by a modal
                }
            }
        }
    }
    return false;  // Scroll not on any modal
}

Core::Screen* Server::GetFocusedScreen() const {
    if (!core_seat_) {
        return nullptr;
    }
    return core_seat_->GetFocusedScreen();
}

std::vector<Core::Tag*> Server::GetTags() const {
    // Get tags from the focused screen's LayerManager
    if (!core_seat_) {
        return {};
    }
    
    auto* focused_screen = core_seat_->GetFocusedScreen();
    if (!focused_screen) {
        return {};
    }
    
    // Find the output for this screen and get its LayerManager
    Output* iter;
    wl_list_for_each(iter, &outputs, link) {
        if (iter->core_screen == focused_screen && iter->layer_manager) {
            return iter->layer_manager->GetTags();
        }
    }
    
    return {};
}

Core::Tag* Server::GetActiveTag() const {
    // Get active tag from the focused screen's LayerManager
    if (!core_seat_) {
        return nullptr;
    }
    
    auto* focused_screen = core_seat_->GetFocusedScreen();
    if (!focused_screen) {
        return nullptr;
    }
    
    // Find the output for this screen and get its LayerManager
    Output* iter;
    wl_list_for_each(iter, &outputs, link) {
        if (iter->core_screen == focused_screen && iter->layer_manager) {
            return iter->layer_manager->GetCurrentTag();
        }
    }
    
    return nullptr;
}

void Server::SwitchToTag(int tag_index) {
    if (!core_seat_) {
        LOG_WARN("Cannot switch tag: core_seat_ is null");
        return;
    }
    
    LOG_DEBUG_FMT("Plugin requested tag switch to index {}", tag_index);
    
    // Switch tag on the focused screen's LayerManager
    auto* focused_screen = core_seat_->GetFocusedScreen();
    if (!focused_screen) {
        LOG_WARN("Cannot switch tag: no focused screen");
        return;
    }
    
    // Find the output for this screen and switch tag
    Output* iter;
    wl_list_for_each(iter, &outputs, link) {
        if (iter->core_screen == focused_screen && iter->layer_manager) {
            iter->layer_manager->SwitchToTag(tag_index);
            return;
        }
    }
    
    LOG_WARN("Cannot switch tag: LayerManager not found for focused screen");
}

std::vector<Core::Client*> Server::GetAllClients() const {
    return clients_;
}

std::vector<Core::Client*> Server::GetClientsOnTag(Core::Tag* tag) const {
    if (!tag) {
        return {};
    }
    return tag->GetClients();
}

std::vector<Core::Client*> Server::GetClientsOnScreen(Core::Screen* screen) const {
    if (!screen) {
        return {};
    }
    
    Core::Tag* tag = screen->GetCurrentTag();
    if (!tag) {
        return {};
    }
    
    return tag->GetClients();
}

Core::Client* Server::GetFocusedClient() const {
    if (!core_seat_) {
        return nullptr;
    }
    return core_seat_->GetFocusedClient();
}

UI::MenuBarManager* Server::GetMenuBarManager() {
    return &UI::MenuBarManager::Instance();
}

LayerManager* Server::GetLayerManagerForScreen(Core::Screen* screen) const {
    if (!screen) {
        return nullptr;
    }
    
    Output* iter;
    wl_list_for_each(iter, &outputs, link) {
        if (iter->core_screen == screen) {
            return iter->layer_manager;
        }
    }
    
    return nullptr;
}

} // namespace Wayland
} // namespace Leviathan
