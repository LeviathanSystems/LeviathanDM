#ifndef SERVER_HPP
#define SERVER_HPP

#include "Types.hpp"
#include "TilingLayout.hpp"
#include "Config.hpp"
#include "KeyBindings.hpp"
#include "IPC.hpp"
#include "core/Seat.hpp"
#include "core/Client.hpp"

extern "C" {
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_seat.h>
}

#include <memory>
#include <vector>

namespace Leviathan {
namespace Wayland {

class Server {
public:
    static Server* Create();
    ~Server();
    
    void Run();
    
    // View operations
    void FocusView(View* view);
    void CloseView(View* view);
    
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
    
    // IPC command processor
    IPC::Response ProcessIPCCommand(const std::string& command_json);
    
    // Public for Input layer access
    struct wl_list keyboards;
    struct wl_list pointers;
    struct wlr_cursor* cursor;
    
    // Public for C callback access
    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_listener new_xdg_toplevel;
    struct wl_listener new_input;
    
    // Callbacks (must be public for C callbacks)
    void OnNewOutput(struct wlr_output* output);
    void OnNewXdgSurface(struct wlr_xdg_surface* xdg_surface);
    void OnNewXdgToplevel(struct wlr_xdg_toplevel* toplevel);
    void OnNewInput(struct wlr_input_device* device);
    
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
    struct wlr_renderer* renderer;
    struct wlr_allocator* allocator;
    struct wlr_compositor* compositor;
    struct wlr_subcompositor* subcompositor;
    struct wlr_data_device_manager* data_device_manager;
    
    // Scene graph
    struct wlr_scene* scene;
    struct wlr_scene_output_layout* scene_layout;
    struct wlr_scene_tree* window_layer;  // Layer for windows (above background)
    
    // Output management
    struct wlr_output_layout* output_layout;
    struct wl_list outputs; // Output::link
    
    // XDG shell
    struct wlr_xdg_shell* xdg_shell;
    std::vector<View*> views;
    
    // Input
    struct wlr_xcursor_manager* cursor_mgr;
    struct wlr_seat* seat;
    struct wl_listener request_cursor;
    struct wl_listener request_set_selection;
    
    // Core architecture
    std::unique_ptr<Core::Seat> core_seat_;
    std::vector<Core::Client*> clients_;  // Owned clients
    View* focused_view_;  // Current wayland-level focus
    
    std::unique_ptr<TilingLayout> layout_engine_;
    std::unique_ptr<Config> config_;
    std::unique_ptr<KeyBindings> keybindings_;
    
    // IPC server
    std::unique_ptr<IPC::Server> ipc_server_;
    
    // Colors (RGBA format for wlroots)
    float border_focused_[4];
    float border_unfocused_[4];
};

} // namespace Wayland
} // namespace Leviathan

#endif // SERVER_HPP
