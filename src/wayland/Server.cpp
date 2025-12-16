#include "wayland/Server.hpp"
#include "wayland/Output.hpp"
#include "wayland/View.hpp"
#include "wayland/Input.hpp"
#include "Logger.hpp"
#include <nlohmann/json.hpp>

extern "C" {
#include <wlr/backend.h>
#include <wlr/backend/wayland.h>
#include <wlr/backend/session.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/backend/interface.h>
#include <wlr/util/log.h>
}

#include <iostream>
#include <algorithm>
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
    , scene(nullptr)
    , scene_layout(nullptr)
    , output_layout(nullptr)
    , xdg_shell(nullptr)
    , cursor(nullptr)
    , cursor_mgr(nullptr)
    , seat(nullptr)
    , focused_view_(nullptr) {
    
    wl_list_init(&outputs);
    wl_list_init(&keyboards);
    wl_list_init(&pointers);
    
    // Default colors (Nord theme)
    border_focused_[0] = 0.36f;   // R
    border_focused_[1] = 0.50f;   // G
    border_focused_[2] = 0.67f;   // B
    border_focused_[3] = 1.0f;    // A
    
    border_unfocused_[0] = 0.23f;
    border_unfocused_[1] = 0.26f;
    border_unfocused_[2] = 0.32f;
    border_unfocused_[3] = 1.0f;
}

Server::~Server() {
    if (wl_display) {
        wl_display_destroy(wl_display);
    }
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
    
    // Create output layout
    output_layout = wlr_output_layout_create(wl_display);
    
    // Create scene
    scene = wlr_scene_create();
    scene_layout = wlr_scene_attach_output_layout(scene, output_layout);
    
    // Initialize layer manager
    layer_manager_ = std::make_unique<LayerManager>(scene);
    
    // Get window layer from layer manager
    window_layer = layer_manager_->GetLayer(Layer::WorkingArea);
    LOG_INFO("Using layer manager's working area for windows");
    
    // Setup output listener
    new_output.notify = handle_new_output;
    wl_signal_add(&backend->events.new_output, &new_output);
    
    // Create XDG shell
    xdg_shell = wlr_xdg_shell_create(wl_display, 3);
    
    // Create XDG decoration manager (for server-side decorations)
    xdg_decoration_mgr = wlr_xdg_decoration_manager_v1_create(wl_display);
    new_xdg_decoration.notify = handle_new_xdg_decoration;
    wl_signal_add(&xdg_decoration_mgr->events.new_toplevel_decoration, &new_xdg_decoration);
    
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
    
    // Initialize components
    config_ = std::make_unique<Config>();
    config_parser_ = std::make_unique<ConfigParser>();
    
    // Load configuration from standard locations
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
        if (config_parser_->LoadWithIncludes(path)) {
            config_loaded = true;
            break;
        }
    }
    
    if (!config_loaded) {
        LOG_INFO("No configuration file found, using defaults");
    }
    
    layout_engine_ = std::make_unique<TilingLayout>();
    keybindings_ = std::make_unique<KeyBindings>(this);
    
    // Create core seat
    core_seat_ = std::make_unique<Core::Seat>();
    
    // Create default tags
    int tag_count = config_->GetWorkspaceCount();
    for (int i = 0; i < tag_count; ++i) {
        auto* tag = new Core::Tag(std::to_string(i + 1));
        core_seat_->AddTag(tag);
    }
    
    // Activate first tag
    if (tag_count > 0) {
        core_seat_->SwitchToTag(0);
    }
    
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
    LOG_INFO("Running compositor on WAYLAND_DISPLAY={}", socket);
    
    // Launch default terminal
    LaunchDefaultTerminal();
    
    // Run event loop with IPC handling
    while (wl_display_get_destroy_listener(wl_display, nullptr) == nullptr) {
        // Handle IPC events
        if (ipc_server_) {
            ipc_server_->HandleEvents();
        }
        
        // Process Wayland events
        wl_display_flush_clients(wl_display);
        wl_event_loop_dispatch(wl_event_loop, 1);  // 1ms timeout
    }
}

void Server::OnNewOutput(struct wlr_output* wlr_output) {
    LOG_INFO("New output: {}", wlr_output->name);
    
    // CRITICAL: Initialize output rendering with allocator and renderer
    // This must be done before creating the scene output or committing
    if (!wlr_output_init_render(wlr_output, allocator, renderer)) {
        LOG_ERROR("Failed to initialize rendering for output '{}'", wlr_output->name);
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
        LOG_ERROR("Failed to commit output '{}'", wlr_output->name);
        wlr_output_state_finish(&state);
        return;
    }
    wlr_output_state_finish(&state);
    
    // Create output wrapper
    Output* output = new Output(wlr_output);
    
    // Add to layout
    struct wlr_output_layout_output* layout_output = wlr_output_layout_add_auto(output_layout, wlr_output);
    
    // Create scene output - this is the proper wlroots 0.19 way
    output->scene_output = wlr_scene_output_create(scene, wlr_output);
    if (!output->scene_output) {
        LOG_ERROR("Failed to create scene output for '{}'", wlr_output->name);
        delete output;
        return;
    }
    
    LOG_INFO("Created scene output for '{}' at {:p}", wlr_output->name, 
             static_cast<void*>(output->scene_output));
    
    // CRITICAL: Connect the output to the scene layout so the scene knows where to render
    wlr_scene_output_layout_add_output(scene_layout, layout_output, output->scene_output);
    LOG_INFO("Connected output '{}' to scene layout", wlr_output->name);
    
    // Register frame listener
    output->frame.notify = OutputManager::HandleFrame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);
    
    LOG_INFO("Registered frame listener for output '{}'", wlr_output->name);
    
    // Register destroy listener
    output->destroy.notify = OutputManager::HandleDestroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);
    
    LOG_INFO("Output '{}' fully configured and enabled", wlr_output->name);
    
    // Set cursor image for this output
    if (cursor_mgr) {
        wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");
    }
    
    TileViews();
}

void Server::OnNewXdgSurface(struct wlr_xdg_surface* xdg_surface) {
    LOG_DEBUG("OnNewXdgSurface called, role={}, toplevel={}", 
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
    
    // Add to scene graph window layer (above background)
    view->scene_tree = wlr_scene_xdg_surface_create(
        window_layer, xdg_surface);
    
    LOG_DEBUG("Created scene tree for toplevel view");
    
    // Create client wrapper
    auto* client = new Core::Client(view);
    clients_.push_back(client);
    
    // Add to core seat (will add to active tag)
    core_seat_->AddClient(client);
    
    LOG_INFO("View fully initialized");
}

void Server::OnNewXdgToplevel(struct wlr_xdg_toplevel* toplevel) {
    LOG_INFO("OnNewXdgToplevel called - this is the proper signal!");
    LOG_DEBUG("Toplevel pointer: {}, base surface: {}", 
              static_cast<void*>(toplevel),
              static_cast<void*>(toplevel->base));
    
    // Create view
    View* view = new View(toplevel, this);
    views.push_back(view);
    
    // Add to scene graph
    // wlr_scene_xdg_surface_create automatically handles configure events
    view->scene_tree = wlr_scene_xdg_surface_create(
        &scene->tree, toplevel->base);
    
    // CRITICAL: Link the xdg_surface to the scene_tree
    // This is required for wlroots scene helpers to work properly
    view->scene_tree->node.data = view;
    toplevel->base->data = view->scene_tree;
    
    LOG_DEBUG("Created scene tree for toplevel view");
    LOG_DEBUG("Surface initialized: {}", toplevel->base->initialized);
    
    // NOTE: wlr_scene_xdg_surface_create() handles configure automatically
    // Do NOT manually call configure functions here
    
    // Create client wrapper
    auto* client = new Core::Client(view);
    clients_.push_back(client);
    
    // Add to core seat (will add to active tag)
    core_seat_->AddClient(client);
    
    LOG_INFO("Toplevel view fully initialized and added to active tag");
}

void Server::OnNewInput(struct wlr_input_device* device) {
    LOG_INFO("New input device: {}", device->name);
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
    LOG_INFO("Launched alacritty terminal (PID: {})", pid);
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
    
    focused_view_ = view;
    
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

void Server::TileViews() {
    LOG_DEBUG("TileViews() called");
    
    auto* tag = core_seat_->GetActiveTag();
    if (!tag) {
        LOG_INFO("TileViews: No active tag");
        return;
    }
    
    auto clients = tag->GetClients();
    LOG_DEBUG("TileViews: Active tag has {} clients", clients.size());
    
    // Filter mapped, non-floating views
    std::vector<View*> tiled_views;
    for (auto* client : clients) {
        auto* view = client->GetView();
        LOG_DEBUG("  Client view: mapped={}, floating={}, fullscreen={}", 
                  view->mapped, view->is_floating, view->is_fullscreen);
        if (view->mapped && !view->is_floating && !view->is_fullscreen) {
            tiled_views.push_back(view);
        }
    }
    
    LOG_DEBUG("TileViews: {} tiled views after filtering", tiled_views.size());
    
    if (tiled_views.empty()) {
        LOG_DEBUG("TileViews: No tiled views, returning");
        return;
    }
    
    // Get output dimensions from first output in layout
    int screen_width = 1920;
    int screen_height = 1080;
    
    struct wlr_output_layout_output* layout_output = 
        wl_container_of(output_layout->outputs.next, layout_output, link);
    
    if (layout_output && layout_output->output) {
        screen_width = layout_output->output->width;
        screen_height = layout_output->output->height;
    }
    
    int gap = config_->GetGapSize();
    int master_count = tag->GetMasterCount();
    float master_ratio = tag->GetMasterRatio();
    
    switch (tag->GetLayout()) {
        case LayoutType::MASTER_STACK:
            layout_engine_->ApplyMasterStack(tiled_views, 
                                            master_count,
                                            master_ratio,
                                            screen_width, screen_height, gap);
            break;
        case LayoutType::MONOCLE:
            layout_engine_->ApplyMonocle(tiled_views, screen_width, screen_height);
            break;
        case LayoutType::GRID:
            layout_engine_->ApplyGrid(tiled_views, screen_width, screen_height, gap);
            break;
        default:
            break;
    }
}

void Server::SetLayout(LayoutType layout) {
    auto* tag = core_seat_->GetActiveTag();
    if (tag) {
        tag->SetLayout(layout);
        TileViews();
    }
}

void Server::IncreaseMasterCount() {
    auto* tag = core_seat_->GetActiveTag();
    if (tag) {
        tag->SetMasterCount(tag->GetMasterCount() + 1);
        TileViews();
    }
}

void Server::DecreaseMasterCount() {
    auto* tag = core_seat_->GetActiveTag();
    if (tag && tag->GetMasterCount() > 0) {
        tag->SetMasterCount(tag->GetMasterCount() - 1);
        TileViews();
    }
}

void Server::IncreaseMasterRatio() {
    auto* tag = core_seat_->GetActiveTag();
    if (tag) {
        float ratio = std::min(0.95f, tag->GetMasterRatio() + 0.05f);
        tag->SetMasterRatio(ratio);
        TileViews();
    }
}

void Server::DecreaseMasterRatio() {
    auto* tag = core_seat_->GetActiveTag();
    if (tag) {
        float ratio = std::max(0.05f, tag->GetMasterRatio() - 0.05f);
        tag->SetMasterRatio(ratio);
        TileViews();
    }
}

void Server::SwitchToTag(int index) {
    core_seat_->SwitchToTag(index);
    TileViews();
}

void Server::MoveClientToTag(int index) {
    auto* focused_client = core_seat_->GetFocusedClient();
    if (!focused_client) {
        return;
    }
    
    core_seat_->MoveClientToTag(focused_client, index);
    focused_view_ = nullptr;
    TileViews();
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
    auto* tag = core_seat_->GetActiveTag();
    if (!tag) return;
    
    auto clients = tag->GetClients();
    if (clients.size() < 2) return;
    
    auto* focused = core_seat_->GetFocusedClient();
    if (!focused) return;
    
    auto it = std::find(clients.begin(), clients.end(), focused);
    if (it != clients.end()) {
        auto next_it = it + 1;
        if (next_it == clients.end()) {
            next_it = clients.begin();
        }
        std::iter_swap(it, next_it);
        TileViews();
    }
}

void Server::SwapWithPrev() {
    auto* tag = core_seat_->GetActiveTag();
    if (!tag) return;
    
    auto clients = tag->GetClients();
    if (clients.size() < 2) return;
    
    auto* focused = core_seat_->GetFocusedClient();
    if (!focused) return;
    
    auto it = std::find(clients.begin(), clients.end(), focused);
    if (it != clients.end()) {
        auto prev_it = it;
        if (it == clients.begin()) {
            prev_it = clients.end() - 1;
        } else {
            prev_it = it - 1;
        }
        std::iter_swap(it, prev_it);
        TileViews();
    }
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
                auto tags = core_seat_->GetTags();
                auto active_tag = core_seat_->GetActiveTag();
                
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
                auto active_tag = core_seat_->GetActiveTag();
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
                auto tags = core_seat_->GetTags();
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
                
                // Use output_layout to enumerate outputs
                struct wlr_output_layout_output* layout_output;
                wl_list_for_each(layout_output, &output_layout->outputs, link) {
                    struct wlr_output* output = layout_output->output;
                    IPC::OutputInfo info;
                    info.name = output->name;
                    info.width = output->width;
                    info.height = output->height;
                    info.refresh_mhz = output->refresh;
                    info.enabled = output->enabled;
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
            
            case IPC::CommandType::SET_ACTIVE_TAG: {
                if (!j.contains("args") || !j["args"].contains("tag")) {
                    response.error = "Missing 'tag' argument";
                    break;
                }
                
                std::string tag_name = j["args"]["tag"];
                auto tags = core_seat_->GetTags();
                
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
    LOG_INFO("New XDG decoration request for toplevel={}", 
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
        LOG_INFO("Associated decoration with view={}, will set mode after surface init", 
                 static_cast<void*>(view));
    } else {
        LOG_WARN("Could not find view for decoration");
    }
}

} // namespace Wayland
} // namespace Leviathan
