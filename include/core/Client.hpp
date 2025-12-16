#ifndef CORE_CLIENT_HPP
#define CORE_CLIENT_HPP

#include "wayland/View.hpp"
#include <string>

namespace Leviathan {
namespace Core {

/**
 * Client represents a window/application
 * - Wraps a Wayland View
 * - Belongs to one or more tags
 * - Has properties like title, floating state, etc.
 */
class Client {
public:
    Client(Wayland::View* view);
    ~Client();
    
    // Client properties
    const std::string& GetTitle() const { return title_; }
    const std::string& GetAppId() const { return app_id_; }
    
    // State
    bool IsFloating() const { return is_floating_; }
    void SetFloating(bool floating);
    
    bool IsFullscreen() const { return is_fullscreen_; }
    void SetFullscreen(bool fullscreen);
    
    bool IsMapped() const;
    bool IsFocused() const { return is_focused_; }
    void SetFocused(bool focused) { is_focused_ = focused; }
    
    bool IsVisible() const { return is_visible_; }
    void SetVisible(bool visible);
    
    // Geometry
    int GetX() const;
    int GetY() const;
    int GetWidth() const;
    int GetHeight() const;
    
    void SetPosition(int x, int y);
    void SetSize(int width, int height);
    void SetGeometry(int x, int y, int width, int height);
    
    // Wayland view access
    Wayland::View* GetView() const { return view_; }
    
    // Actions
    void Close();
    void Focus();
    void Raise();
    
private:
    void UpdateTitle();
    void UpdateAppId();
    
private:
    Wayland::View* view_;
    std::string title_;
    std::string app_id_;
    bool is_floating_;
    bool is_fullscreen_;
    bool is_focused_;
    bool is_visible_;
};

} // namespace Core
} // namespace Leviathan

#endif // CORE_CLIENT_HPP
