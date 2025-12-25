#ifndef SERVER_HPP
#define SERVER_HPP

#include "Types.hpp"
#include "layout/TilingLayout.hpp"
#include "config/ConfigParser.hpp"
#include "KeyBindings.hpp"
#include "ipc/IPC.hpp"
#include "core/Seat.hpp"
#include "core/Client.hpp"
#include "wayland/LayerManager.hpp"
#include "wayland/WaylandTypes.hpp"
#include "ui/CompositorState.hpp"

// Layer shell needs special handling for 'namespace' keyword
#define namespace namespace_
extern "C" {
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_data_control_v1.h>
}
#undef namespace

#include <memory>
#include <vector>

namespace Leviathan {
namespace Wayland {

class Server : public UI::CompositorState {
public:
    static Server* Create();
    ~Server();
    
    void Run();
    
    // View operations
    void FocusView(View* view);
    void CloseView(View* view);
    void RemoveView(View* view);  // Called by View destructor to clean up
    
    // Layout operations
    void TileViews();
    void SetLayout(LayoutType layout);
    void IncreaseMasterCount();
    void DecreaseMasterCount();
    void IncreaseMasterRatio();
    void DecreaseMasterRatio();
    
    // Tag operations (replacing workspace)
    void SwitchToTag(int index);
    void MoveClientToTag(int index);
    
    // Navigation
    void FocusNext();
    void FocusPrev();
    void SwapWithNext();
    void SwapWithPrev();
    
    // Getters
    struct wl_display* GetDisplay() { return wl_display; }
    struct wlr_scene* GetScene() { return scene; }
    struct wlr_seat* GetSeat() { return seat; }
    struct wlr_output_layout* GetOutputLayout() { return output_layout; }
    Core::Seat* GetCoreSeat() { return core_seat_.get(); }
    const std::vector<View*>& GetViews() const { return views; }
    const std::vector<Core::Client*>& GetClients() const { return clients_; }
    TilingLayout* GetLayoutEngine() { return layout_engine_.get(); }
    KeyBindings* GetKeyBindings() { return keybindings_.get(); }
    
    // Find Output struct by wlr_output
    Output* FindOutput(struct wlr_output* wlr_output);
    
    // Check if cursor is over a status bar and handle hover
    // Returns true if hover was handled by a status bar
    bool CheckStatusBarHover(int x, int y);
    
    // Check if click is on a status bar and handle it
    // Returns true if click was handled by a status bar
    bool CheckStatusBarClick(int x, int y);
    
    // CompositorState interface implementation
    std::vector<Core::Screen*> GetScreens() const override;
    Core::Screen* GetFocusedScreen() const override;
    std::vector<Core::Tag*> GetTags() const override;
    Core::Tag* GetActiveTag() const override;
    std::vector<Core::Client*> GetAllClients() const override;
    std::vector<Core::Client*> GetClientsOnTag(Core::Tag* tag) const override;
    std::vector<Core::Client*> GetClientsOnScreen(Core::Screen* screen) const override;
    Core::Client* GetFocusedClient() const override;
    
    // IPC command processor
    IPC::Response ProcessIPCCommand(const std::string& command_json);
    
    // Public for Input layer access
    struct wl_list keyboards;
    struct wl_list pointers;
    struct wlr_cursor* cursor;
    struct wlr_seat* seat;
    
    // Public for layer surface access
    struct wl_list layer_surfaces; // LayerSurface::link
    
    // Public for C callback access
    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_listener new_xdg_toplevel;
    struct wl_listener new_xdg_decoration;
    struct wl_listener new_layer_surface;
    struct wl_listener new_input;
    struct wl_listener session_active;  // For VT switching
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    
    // Callbacks (must be public for C callbacks)
    void OnNewOutput(struct wlr_output* output);
    void OnNewXdgSurface(struct wlr_xdg_surface* xdg_surface);
    void OnNewXdgToplevel(struct wlr_xdg_toplevel* toplevel);
    void OnNewXdgDecoration(struct wlr_xdg_toplevel_decoration_v1* decoration);
    void OnNewInput(struct wlr_input_device* device);
    void OnSessionActive(bool active);
    
    // Auto-launch application
    void LaunchDefaultTerminal();
    
    // Monitor group configuration
    void ApplyMonitorGroupConfiguration();
    
private:
    Server();
    bool Initialize();
    
    View* FindView(struct wlr_surface* surface);
    void UpdateViewList();
    
private:
    // Wayland/wlroots core
    struct wl_display* wl_display;
    struct wl_event_loop* wl_event_loop;
    struct wlr_backend* backend;
    struct wlr_session* session;  // Session for VT switching (may be NULL if nested)
    struct wlr_renderer* renderer;
    struct wlr_allocator* allocator;
    struct wlr_compositor* compositor;
    struct wlr_subcompositor* subcompositor;
    struct wlr_data_device_manager* data_device_manager;
    struct wlr_primary_selection_v1_device_manager* primary_selection_mgr;
    struct wlr_data_control_manager_v1* data_control_mgr;
    
    // Scene graph
    struct wlr_scene* scene;
    struct wlr_scene_output_layout* scene_layout;
    struct wlr_scene_tree* window_layer;  // Layer for windows (above background)
    
    // Output management
    struct wlr_output_layout* output_layout;
    struct wl_list outputs; // Output::link (each has its own LayerManager)
    
    // XDG shell
    struct wlr_xdg_shell* xdg_shell;
    struct wlr_xdg_decoration_manager_v1* xdg_decoration_mgr;
    std::vector<View*> views;
    
    // Layer shell
    struct wlr_layer_shell_v1* layer_shell;
    
    // Input
    struct wlr_xcursor_manager* cursor_mgr;
    struct wl_listener request_cursor;
    struct wl_listener request_set_selection;
    
    // Core architecture
    std::unique_ptr<Core::Seat> core_seat_;
    std::vector<Core::Client*> clients_;  // Owned clients
    View* focused_view_;  // Current wayland-level focus
    
    std::unique_ptr<TilingLayout> layout_engine_;
    std::unique_ptr<KeyBindings> keybindings_;
    
    // Wayland socket name (for child processes)
    std::string wayland_socket_name_;
    
    // IPC server
    std::unique_ptr<IPC::Server> ipc_server_;
    
    // Colors (RGBA format for wlroots)
    float border_focused_[4];
    float border_unfocused_[4];
};

} // namespace Wayland
} // namespace Leviathan

#endif // SERVER_HPP
