#pragma once

#include "ui/PeriodicWidget.hpp"
#include "ui/PluginAPI.hpp"
#include <string>
#include <vector>
#include <map>

namespace Leviathan {
namespace UI {

struct NetworkInterface {
    std::string name;           // e.g., "eth0", "wlan0"
    std::string ip_address;     // IPv4 address
    bool is_up;                 // Interface is up
    bool is_wireless;           // Is WiFi interface
    int signal_strength;        // WiFi signal strength (0-100), -1 if not applicable
    std::string ssid;           // WiFi SSID (empty if not wireless)
};

class NetworkWidget : public UI::PeriodicWidget {
public:
    NetworkWidget();
    ~NetworkWidget() override;
    
    // WidgetPlugin interface
    UI::PluginMetadata GetMetadata() const override;
    
    // Widget interface
    void CalculateSize(int available_width, int available_height) override;
    void Render(cairo_t* cr) override;
    bool HandleClick(int click_x, int click_y) override;
    
protected:
    // PeriodicWidget interface
    bool InitializeImpl(const std::map<std::string, std::string>& config) override;
    void UpdateData() override;
    void CleanupImpl() override;
    
private:
    void ScanNetworkInterfaces();
    void UpdatePrimaryInterface();
    std::string GetSignalIcon(int signal_strength) const;
    std::string GetInterfaceIcon(const NetworkInterface& iface) const;
    bool ParseColor(const std::string& hex, double& r, double& g, double& b, double& a) const;
    
    void ShowPopover();
    void HidePopover();
    void RenderPopoverContent(cairo_t* cr);
    
    std::vector<NetworkInterface> interfaces_;
    NetworkInterface primary_interface_;
    bool popover_visible_;
    
    // Configuration
    int font_size_ = 12;
    bool show_icon_ = true;
    bool show_ip_ = true;
    std::string bg_color_ = "#3B4252";      // Nord dark gray
    std::string fg_color_ = "#ECEFF4";      // Nord snow storm
    std::string popover_bg_color_ = "#2E3440";  // Nord darkest
    std::string popover_fg_color_ = "#ECEFF4";
};

} // namespace UI
} // namespace Leviathan
