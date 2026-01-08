#pragma once

#include "ui/PeriodicWidget.hpp"
#include "ui/PluginAPI.hpp"
#include "Types.hpp"
#include <string>
#include <map>

namespace Leviathan {
namespace UI {

class TilingModeWidget : public PeriodicWidget {
public:
    TilingModeWidget();
    ~TilingModeWidget() override;
    
    PluginMetadata GetMetadata() const override;
    
protected:
    bool InitializeImpl(const std::map<std::string, std::string>& config) override;
    void CleanupImpl() override;
    void UpdateData() override;
    void CalculateSize(int available_width, int available_height) override;
    void Render(cairo_t* cr) override;
    bool HandleClick(int click_x, int click_y) override;

private:
    // Event handling
    void OnCompositorEvent(const Plugin::Event& event);
    void FetchLayoutFromCompositor();
    
    // Layout info helpers
    std::string GetLayoutName(LayoutType layout) const;
    std::string GetLayoutIcon(LayoutType layout) const;
    
    // Color parsing helper
    bool ParseColor(const std::string& hex, double& r, double& g, double& b, double& a) const;
    
    // State
    LayoutType current_layout_;
    int event_subscription_id_;
    
    // Configuration
    int font_size_ = 14;
    bool show_icon_ = true;
    bool show_text_ = true;
    std::string bg_color_ = "#3B4252";      // Nord polar night
    std::string fg_color_ = "#ECEFF4";      // Nord snow storm
};

} // namespace UI
} // namespace Leviathan