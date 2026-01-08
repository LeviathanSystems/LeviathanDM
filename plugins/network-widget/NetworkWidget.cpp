#include "NetworkWidget.hpp"
#include "version.h"
#include <cairo.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>

namespace Leviathan {
namespace UI {

NetworkWidget::NetworkWidget() 
    : PeriodicWidget(),
      popover_visible_(false) {
}

NetworkWidget::~NetworkWidget() {
}

PluginMetadata NetworkWidget::GetMetadata() const {
    return {
        .name = PLUGIN_NAME,
        .version = PLUGIN_VERSION,
        .author = "LeviathanDM",
        .description = "Shows network interface information with popover details",
        .api_version = WIDGET_API_VERSION
    };
}

bool NetworkWidget::InitializeImpl(const std::map<std::string, std::string>& config) {
    // Set update interval
    update_interval_ = 5; // Update every 5 seconds
    
    // Parse configuration
    if (config.count("font_size")) {
        font_size_ = std::stoi(config.at("font_size"));
    }
    if (config.count("show_icon")) {
        show_icon_ = (config.at("show_icon") == "true");
    }
    if (config.count("show_ip")) {
        show_ip_ = (config.at("show_ip") == "true");
    }
    if (config.count("bg_color")) {
        bg_color_ = config.at("bg_color");
    }
    if (config.count("fg_color")) {
        fg_color_ = config.at("fg_color");
    }
    if (config.count("popover_bg_color")) {
        popover_bg_color_ = config.at("popover_bg_color");
    }
    if (config.count("popover_fg_color")) {
        popover_fg_color_ = config.at("popover_fg_color");
    }
    
    // Initial network scan
    ScanNetworkInterfaces();
    UpdatePrimaryInterface();
    
    return true;
}

void NetworkWidget::CleanupImpl() {
    // Nothing to cleanup
}

void NetworkWidget::UpdateData() {
    ScanNetworkInterfaces();
    UpdatePrimaryInterface();
}

void NetworkWidget::ScanNetworkInterfaces() {
    interfaces_.clear();
    
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        return;
    }
    
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }
        
        // Skip loopback
        if (strcmp(ifa->ifa_name, "lo") == 0) {
            continue;
        }
        
        // Only process IPv4 addresses
        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        
        // Check if interface already exists
        auto it = std::find_if(interfaces_.begin(), interfaces_.end(),
            [&](const NetworkInterface& iface) {
                return iface.name == ifa->ifa_name;
            });
        
        if (it != interfaces_.end()) {
            continue;
        }
        
        NetworkInterface iface;
        iface.name = ifa->ifa_name;
        iface.is_wireless = (strncmp(ifa->ifa_name, "wl", 2) == 0);
        
        // Get IP address
        struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr->sin_addr, ip, INET_ADDRSTRLEN);
        iface.ip_address = ip;
        
        // Check if interface is up
        iface.is_up = (ifa->ifa_flags & IFF_UP) && (ifa->ifa_flags & IFF_RUNNING);
        
        // Get WiFi signal strength if wireless
        if (iface.is_wireless) {
            // Read signal strength from /proc/net/wireless
            std::ifstream wireless_file("/proc/net/wireless");
            std::string line;
            iface.signal_strength = -1;
            iface.ssid = "";
            
            while (std::getline(wireless_file, line)) {
                if (line.find(iface.name) != std::string::npos) {
                    std::istringstream iss(line);
                    std::string dummy;
                    int status, quality, signal;
                    iss >> dummy >> status >> quality >> signal;
                    
                    // Convert signal from dBm to percentage (rough approximation)
                    // Typical range: -100 dBm (0%) to -50 dBm (100%)
                    if (signal < 0) {
                        iface.signal_strength = std::max(0, std::min(100, (signal + 100) * 2));
                    }
                    break;
                }
            }
            
            // Try to get SSID from /sys/class/net/<iface>/wireless/
            std::string ssid_path = "/sys/class/net/" + iface.name + "/wireless/ssid";
            std::ifstream ssid_file(ssid_path);
            if (ssid_file.good()) {
                std::getline(ssid_file, iface.ssid);
            }
            
            // Fallback: try iwgetid command
            if (iface.ssid.empty()) {
                std::string cmd = "iwgetid -r " + iface.name + " 2>/dev/null";
                FILE* pipe = popen(cmd.c_str(), "r");
                if (pipe) {
                    char buffer[128];
                    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                        iface.ssid = buffer;
                        // Remove trailing newline
                        if (!iface.ssid.empty() && iface.ssid.back() == '\n') {
                            iface.ssid.pop_back();
                        }
                    }
                    pclose(pipe);
                }
            }
        } else {
            iface.signal_strength = -1;
            iface.ssid = "";
        }
        
        interfaces_.push_back(iface);
    }
    
    freeifaddrs(ifaddr);
}

void NetworkWidget::UpdatePrimaryInterface() {
    // Find primary interface (first one that's up)
    // Prefer wired over wireless
    
    NetworkInterface* wired_up = nullptr;
    NetworkInterface* wireless_up = nullptr;
    
    for (auto& iface : interfaces_) {
        if (iface.is_up) {
            if (iface.is_wireless && !wireless_up) {
                wireless_up = &iface;
            } else if (!iface.is_wireless && !wired_up) {
                wired_up = &iface;
            }
        }
    }
    
    if (wired_up) {
        primary_interface_ = *wired_up;
    } else if (wireless_up) {
        primary_interface_ = *wireless_up;
    } else if (!interfaces_.empty()) {
        primary_interface_ = interfaces_[0];
    } else {
        primary_interface_ = NetworkInterface{
            .name = "none",
            .ip_address = "No network",
            .is_up = false,
            .is_wireless = false,
            .signal_strength = -1,
            .ssid = ""
        };
    }
}

std::string NetworkWidget::GetSignalIcon(int signal_strength) const {
    if (signal_strength < 0) {
        return "";
    } else if (signal_strength >= 75) {
        return "▂▄▆█";  // Excellent
    } else if (signal_strength >= 50) {
        return "▂▄▆";   // Good
    } else if (signal_strength >= 25) {
        return "▂▄";    // Fair
    } else {
        return "▂";     // Poor
    }
}

std::string NetworkWidget::GetInterfaceIcon(const NetworkInterface& iface) const {
    if (!iface.is_up) {
        return "✗";
    } else if (iface.is_wireless) {
        return "⚡";  // WiFi icon
    } else {
        return "⚈";  // Ethernet icon
    }
}

void NetworkWidget::CalculateSize(int available_width, int available_height) {
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* temp_cr = cairo_create(temp_surface);
    
    cairo_select_font_face(temp_cr, "sans-serif", 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(temp_cr, font_size_);
    
    std::string display_text;
    if (show_icon_) {
        display_text += GetInterfaceIcon(primary_interface_);
        display_text += " ";
    }
    
    if (show_ip_) {
        if (primary_interface_.is_wireless && !primary_interface_.ssid.empty()) {
            display_text += primary_interface_.ssid;
        } else {
            display_text += primary_interface_.ip_address;
        }
    } else {
        display_text += primary_interface_.name;
    }
    
    if (primary_interface_.is_wireless && primary_interface_.signal_strength >= 0) {
        display_text += " ";
        display_text += GetSignalIcon(primary_interface_.signal_strength);
    }
    
    cairo_text_extents_t extents;
    cairo_text_extents(temp_cr, display_text.c_str(), &extents);
    
    cairo_destroy(temp_cr);
    cairo_surface_destroy(temp_surface);
    
    int padding = 8;
    width_ = static_cast<int>(extents.width) + padding * 2;
    height_ = font_size_ + padding;
}

bool NetworkWidget::ParseColor(const std::string& hex, double& r, double& g, double& b, double& a) const {
    a = 1.0;
    if (hex.empty() || hex[0] != '#') return false;
    
    std::string color = hex.substr(1);
    if (color.length() == 6) {
        r = std::stoi(color.substr(0, 2), nullptr, 16) / 255.0;
        g = std::stoi(color.substr(2, 2), nullptr, 16) / 255.0;
        b = std::stoi(color.substr(4, 2), nullptr, 16) / 255.0;
    } else if (color.length() == 8) {
        r = std::stoi(color.substr(0, 2), nullptr, 16) / 255.0;
        g = std::stoi(color.substr(2, 2), nullptr, 16) / 255.0;
        b = std::stoi(color.substr(4, 2), nullptr, 16) / 255.0;
        a = std::stoi(color.substr(6, 2), nullptr, 16) / 255.0;
    } else {
        return false;
    }
    return true;
}

void NetworkWidget::Render(cairo_t* cr) {
    // Draw background
    double bg_r, bg_g, bg_b, bg_a;
    if (ParseColor(bg_color_, bg_r, bg_g, bg_b, bg_a)) {
        cairo_set_source_rgba(cr, bg_r, bg_g, bg_b, bg_a);
    } else {
        cairo_set_source_rgba(cr, 0.23, 0.25, 0.32, 1.0); // fallback
    }
    cairo_rectangle(cr, 0, 0, width_, height_);
    cairo_fill(cr);
    
    // Draw text
    double fg_r, fg_g, fg_b, fg_a;
    if (ParseColor(fg_color_, fg_r, fg_g, fg_b, fg_a)) {
        cairo_set_source_rgba(cr, fg_r, fg_g, fg_b, fg_a);
    } else {
        cairo_set_source_rgba(cr, 0.93, 0.94, 0.96, 1.0); // fallback
    }
    cairo_select_font_face(cr, "sans-serif", 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size_);
    
    std::string display_text;
    if (show_icon_) {
        display_text += GetInterfaceIcon(primary_interface_);
        display_text += " ";
    }
    
    if (show_ip_) {
        if (primary_interface_.is_wireless && !primary_interface_.ssid.empty()) {
            display_text += primary_interface_.ssid;
        } else {
            display_text += primary_interface_.ip_address;
        }
    } else {
        display_text += primary_interface_.name;
    }
    
    if (primary_interface_.is_wireless && primary_interface_.signal_strength >= 0) {
        display_text += " ";
        display_text += GetSignalIcon(primary_interface_.signal_strength);
    }
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, display_text.c_str(), &extents);
    
    int padding = 8;
    int text_x = padding;
    int text_y = (height_ + extents.height) / 2;
    
    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, display_text.c_str());
    
    // Render popover if visible
    if (popover_visible_) {
        RenderPopoverContent(cr);
    }
}

void NetworkWidget::RenderPopoverContent(cairo_t* cr) {
    int popover_width = 350;
    int popover_height = 40 + interfaces_.size() * 30 + 10;
    int popover_x = 0;
    int popover_y = height_ + 5;
    
    // Draw popover background with shadow
    cairo_save(cr);
    
    // Shadow
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.3);
    cairo_rectangle(cr, popover_x + 2, popover_y + 2, popover_width, popover_height);
    cairo_fill(cr);
    
    // Draw popover background
    double bg_r, bg_g, bg_b, bg_a;
    if (ParseColor(popover_bg_color_, bg_r, bg_g, bg_b, bg_a)) {
        cairo_set_source_rgba(cr, bg_r, bg_g, bg_b, 0.98);
    } else {
        cairo_set_source_rgba(cr, 0.18, 0.20, 0.25, 0.98);
    }
    cairo_rectangle(cr, popover_x, popover_y, popover_width, popover_height);
    cairo_fill(cr);
    
    // Draw border
    cairo_set_source_rgba(cr, 0.35, 0.35, 0.35, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, popover_x, popover_y, popover_width, popover_height);
    cairo_stroke(cr);
    
    // Draw header
    double fg_r, fg_g, fg_b, fg_a;
    if (ParseColor(popover_fg_color_, fg_r, fg_g, fg_b, fg_a)) {
        cairo_set_source_rgba(cr, fg_r, fg_g, fg_b, fg_a);
    } else {
        cairo_set_source_rgba(cr, 0.93, 0.94, 0.96, 1.0);
    }
    cairo_select_font_face(cr, "sans-serif", 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, font_size_ + 1);
    
    cairo_move_to(cr, popover_x + 12, popover_y + 22);
    cairo_show_text(cr, "Network Interfaces");
    
    // Draw separator line
    cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.5);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, popover_x + 12, popover_y + 30);
    cairo_line_to(cr, popover_x + popover_width - 12, popover_y + 30);
    cairo_stroke(cr);
    
    // Draw interfaces
    if (ParseColor(popover_fg_color_, fg_r, fg_g, fg_b, fg_a)) {
        cairo_set_source_rgba(cr, fg_r, fg_g, fg_b, fg_a * 0.9);
    } else {
        cairo_set_source_rgba(cr, 0.85, 0.86, 0.88, 1.0);
    }
    
    cairo_select_font_face(cr, "monospace", 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size_ - 1);
    
    int y_offset = popover_y + 52;
    
    if (interfaces_.empty()) {
        cairo_move_to(cr, popover_x + 12, y_offset);
        cairo_show_text(cr, "No network interfaces found");
    } else {
        for (const auto& iface : interfaces_) {
            // Interface name and icon
            std::string line = GetInterfaceIcon(iface) + " " + iface.name;
            cairo_move_to(cr, popover_x + 12, y_offset);
            cairo_show_text(cr, line.c_str());
            
            // IP address
            cairo_move_to(cr, popover_x + 100, y_offset);
            cairo_show_text(cr, iface.ip_address.c_str());
            
            // WiFi details (SSID and signal)
            if (iface.is_wireless) {
                std::string wifi_info;
                if (!iface.ssid.empty()) {
                    wifi_info = iface.ssid;
                }
                if (iface.signal_strength >= 0) {
                    wifi_info += " " + GetSignalIcon(iface.signal_strength);
                    wifi_info += " " + std::to_string(iface.signal_strength) + "%";
                }
                if (!wifi_info.empty()) {
                    cairo_move_to(cr, popover_x + 220, y_offset);
                    cairo_show_text(cr, wifi_info.c_str());
                }
            }
            
            y_offset += 30;
        }
    }
    
    cairo_restore(cr);
}

bool NetworkWidget::HandleClick(int click_x, int click_y) {
    // Toggle popover visibility
    if (popover_visible_) {
        HidePopover();
    } else {
        ShowPopover();
    }
    MarkDirty();
    return true;
}

void NetworkWidget::ShowPopover() {
    popover_visible_ = true;
    // Refresh network data
    ScanNetworkInterfaces();
}

void NetworkWidget::HidePopover() {
    popover_visible_ = false;
}

} // namespace UI
} // namespace Leviathan

// Export plugin
extern "C" {
    EXPORT_PLUGIN_CREATE(NetworkWidget)
    EXPORT_PLUGIN_DESTROY(NetworkWidget)
}
