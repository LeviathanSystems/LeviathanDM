#ifndef CORE_SCREEN_HPP
#define CORE_SCREEN_HPP

#include "Tag.hpp"
#include <string>

extern "C" {
#include <wlr/types/wlr_output.h>
}

namespace Leviathan {
namespace Core {

/**
 * Screen represents a physical monitor
 * - Wraps wlr_output
 * - Has position, resolution, scale
 * - Can display one tag at a time
 * - Provides monitor description from EDID
 */
class Screen {
public:
    Screen(struct wlr_output* output);
    ~Screen();
    
    // Output properties
    const std::string& GetName() const { return name_; }
    const std::string& GetDescription() const { return description_; }
    const std::string& GetMake() const { return make_; }
    const std::string& GetModel() const { return model_; }
    const std::string& GetSerial() const { return serial_; }
    int GetX() const { return x_; }
    int GetY() const { return y_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    float GetScale() const { return scale_; }
    
    // Position on global layout
    void SetPosition(int x, int y);
    void SetScale(float scale);
    
    // Tag displayed on this screen
    void ShowTag(Tag* tag);
    Tag* GetCurrentTag() const { return current_tag_; }
    
    // Wlroots access
    struct wlr_output* GetWlrOutput() const { return wlr_output_; }
    
private:
    void ParseOutputInfo();
    
    struct wlr_output* wlr_output_;
    std::string name_;
    std::string description_;  // Full description: "Make Model (connector)"
    std::string make_;          // Manufacturer
    std::string model_;         // Model name
    std::string serial_;        // Serial number
    int x_, y_;
    int width_, height_;
    float scale_;
    Tag* current_tag_;
};

} // namespace Core
} // namespace Leviathan

#endif // CORE_SCREEN_HPP
